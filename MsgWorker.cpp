//
// Created by hexi on 2020/4/4.
//

#include "MsgWorker.h"

void MsgWorker::loopMatch() {
  shared_ptr<ConsumeMsgUnit> unit;
  std::function<void(shared_ptr<MsgUnit>)> func = [&](shared_ptr<MsgUnit> msg) {
      unit->msgId = msg->msgId;
#ifdef DEBUG
      cout << msg->msgId << " consumed!" << endl;
#endif
      unit->status = CLIENT_RECEIVE;
      idUnitMap.insert_or_update(msg->msgId, unit);
      unit->callData->responseMsg(0, "", msg->msgId, msg->body);
      resetConsumerActive(unit->topic, unit->group);
      writeLog(PROXY_LOGGER_TYPE_CONSUME, msg->msgId, msg->topic, msg->group,
               msg->body, msg->delayLevel, 0);
  };
  shared_ptr<ConsumerUnit> consumer;
  while (true) {
    QueueTS<shared_ptr<ConsumeMsgUnit>> tmp;
    while (consumeMsgPool.try_pop(unit)) {
      consumer = getConsumer(unit->topic, unit->group);
      if (unit->status == PROXY_CONSUME_INIT) {
        consumer->fetchAndConsume(func);
        if (unit->getIsFetchMsgTimeout()) {
          resetConsumerActive(unit->topic, unit->group);
          unit->callData->responseTimeOut();
          continue;
        }
        tmp.push(unit);
      } else if (unit->status == CLIENT_RECEIVE) {
        if (unit->getIsAckTimeout()) {
          consumer->setMsgReconsume(unit->msgId);
          continue;
        }
        tmp.push(unit);
      } else {
        //CONSUME_ACK
        continue;
      }
      //没有处理掉的重新推进去
      while (tmp.try_pop(unit)) {
        consumeMsgPool.push(unit);
      }
    }

    std::unique_lock<std::mutex> lk(notifyMtx);
    notifyCV.wait(lk);
  }
}

shared_ptr<ProducerUnit> MsgWorker::getProducer(const string &topic, const string &group) {
  auto key = topic + group;
  auto iter = producers.find(key);
  if (iter != producers.end()) {
    return iter->second;
  } else {
    shared_ptr<ProducerUnit> producerUnit(new ProducerUnit(topic));
    producerUnit->producer.setNamesrvAddr(nameServerHost_);
    producerUnit->producer.setGroupName(group);
    producerUnit->producer.setInstanceName(topic);
    producerUnit->producer.setTcpTransportPullThreadNum(1);
    producerUnit->producer.setSendMsgTimeout(500);
    producerUnit->producer.setTcpTransportTryLockTimeout(1000);
    producerUnit->producer.setTcpTransportConnectTimeout(400);
    producerUnit->producer.setSessionCredentials(accessKey_, secretKey_, accessChannel_);
    try {
      producerUnit->producer.start();
      producers.insert(pair<string, shared_ptr<ProducerUnit>>(key, producerUnit));
      return producerUnit;
    } catch (exception &e) {
      PRINT_ERROR(e);
      return nullptr;
    }
  }
}

shared_ptr<ConsumerUnit> MsgWorker::getConsumer(const string &topic, const string &group) {
  auto key = topic + group;
  shared_ptr<ConsumerUnit> unit;
  if (consumers.try_get(key, unit)) {
    return unit;
  } else {
    shared_ptr<ConsumerUnit> consumerUnit(new ConsumerUnit(group));
    consumerUnit->consumer.setNamesrvAddr(nameServerHost_);
    consumerUnit->consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
    consumerUnit->consumer.setInstanceName(group);
    consumerUnit->consumer.subscribe(topic, "*");
    consumerUnit->consumer.setConsumeThreadCount(2);
    consumerUnit->consumer.setTcpTransportPullThreadNum(1);
    consumerUnit->consumer.setPullMsgThreadPoolCount(1);
    consumerUnit->consumer.setMaxCacheMsgSizePerQueue(128);
    consumerUnit->consumer.setTcpTransportTryLockTimeout(1000);
    consumerUnit->consumer.setTcpTransportConnectTimeout(400);
    consumerUnit->consumer.setSessionCredentials(accessKey_, secretKey_, accessChannel_);
    auto listener = new ConsumerMsgListener();
    auto callback = [group, consumerUnit](const std::vector<MQMessageExt> &msgs,
                                          std::vector<ConsumeStatus> &statusVector) {
        shared_ptr<ConsumerUnitLocker> locker(new ConsumerUnitLocker(msgs, group));
        consumerUnit->waitLock(locker);
        consumerUnit->eraseLock(locker);
        if (locker->status == RECONSUME_LATER) {
          for (int i = 0; i < msgs.size(); i++) {
            if (locker->reconsumeMap.find(msgs[i].getMsgId()) == locker->reconsumeMap.end()) {
              statusVector.push_back(CONSUME_SUCCESS);
            } else {
              statusVector.push_back(RECONSUME_LATER);
            }
          }
        }
        return locker->status;
    };
    listener->setMsgCallback(callback);
    consumerUnit->consumer.registerMessageListener(listener);
    initMsgQueue(key);
    try {
      consumerUnit->consumer.start();
      consumers.insert(key, consumerUnit);
      return consumerUnit;
    } catch (exception &e) {
      PRINT_ERROR(e);
      return nullptr;
    }
  }
}

void MsgWorker::shutdownConsumer() {
  shared_ptr<ConsumerUnit> unit;
  std::vector<string> keys;
  while (true) {
    consumers.getAllKeys(keys);
    for (size_t i = 0; i < keys.size(); i++) {
      if (consumers.try_get(keys[i], unit) && unit->getIsTooInactive()) {
        cout << keys[i] << " is going to shutdown!" << endl;
        clearConsumerKey = keys[i];
        unit->consumer.shutdown();
        cout << keys[i] << " shutdown success!" << endl;
        consumers.erase(keys[i]);
      }
    }
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
}

void MsgWorker::clearMsgForConsumer() {
  shared_ptr<ConsumerUnit> consumer;
  while (true) {
    //等待shutdown 处理程序通知
    //获取到锁之后立即通知
    //可以过滤消息了
    if (clearConsumerKey != "" && consumers.try_get(clearConsumerKey, consumer)) {
      //sleep一下，保证肯定执行到shutdown了
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      consumer->unlockAll();
      clearConsumerKey = "";
    }
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
}

void ConsumerUnit::unlockAll() {
  boost::unique_lock<boost::shared_mutex> lk(lockersMtx);
  auto iter = lockers.begin();
  while (iter != lockers.end()) {
    auto iter1 = iter->first->clientStatusMap.begin();
    while (iter1 != iter->first->clientStatusMap.end()) {
      if (iter1->second != MSG_CONSUME_ACK) {
        iter1->second = MSG_CONSUME_ACK;
        iter->first->statusMap[iter1->first] = RECONSUME_LATER;
      }
    }
    iter->first->triggerCheck();
    iter++;
  }
}

bool ConsumerUnit::setMsgReconsume(const string &msgId) {
  return setMsgAck(msgId, RECONSUME_LATER);
}

bool ConsumerUnit::pushMsgBack(const string &msgId) {
  boost::unique_lock<boost::shared_mutex> lk(lockersMtx);
  auto iter = lockers.begin();
  while (iter != lockers.end()) {
    auto iter1 = iter->first->idMsgMap.find(msgId);
    if (iter1 != iter->first->idMsgMap.end()) {
      iter->first->fetchedArr.push(iter1->second.begin()->first);
    }
    iter++;
  }
}

bool ConsumerUnit::fetchAndConsume(std::function<void(shared_ptr<MsgUnit>)> &callback) {
  shared_ptr<MsgUnit> msg;
  bool found = false;
  {
    boost::shared_lock<boost::shared_mutex> lk(lockersMtx);
    auto iter = lockers.begin();
    while (iter != lockers.end()) {
      if (iter->first->getMsg(msg)) {
        found = true;
        break;
      }
      iter++;
    }
  }

  if (found) {
    callback(msg);

  }
}

bool ConsumerUnit::setMsgAck(const string &msgId, ConsumeStatus s) {
  bool ret = false;
  {
    boost::shared_lock<boost::shared_mutex> lk(lockersMtx);
    auto iter = lockers.begin();
    while (iter != lockers.end()) {
      if (iter->first->setMsgStatus(msgId, s, MSG_CONSUME_ACK)) {
        ret = true;
      }
      iter->first->triggerCheck();
      iter++;
    }
  }
  return ret;
}

ConsumerUnitLocker::ConsumerUnitLocker(const std::vector<MQMessageExt> &msgs, const string &group) : status(
    RECONSUME_LATER), clientStatus(MSG_FETCH_FROM_BROKER) {
  shared_ptr<MsgUnit> msgUnit;
  for (int i = 0; i < msgs.size(); i++) {
    auto msg = msgs[i];
    msgUnit = shared_ptr<MsgUnit>(new MsgUnit);
    msgUnit->msgId = msg.getMsgId();
    msgUnit->type = 1;
    msgUnit->delayLevel = msg.getDelayTimeLevel();
    msgUnit->body = msg.getBody();
    msgUnit->topic = msg.getTopic();
    msgUnit->group = group;
    clientStatusMap.insert(pair<shared_ptr<MsgUnit>, ClientMsgConsumeStatus>(msgUnit, MSG_FETCH_FROM_BROKER));
    statusMap.insert(pair<shared_ptr<MsgUnit>, ConsumeStatus>(msgUnit, RECONSUME_LATER));
    if (idMsgMap.find(msg.getMsgId()) == idMsgMap.end()) {
      map<shared_ptr<MsgUnit>, int> msgQueueTmp;
      msgQueueTmp.insert(pair<shared_ptr<MsgUnit>, int>(msgUnit, 1));
      idMsgMap.insert(pair<string, map<shared_ptr<MsgUnit>, int>>(msg.getMsgId(), msgQueueTmp));
    } else {
      map<shared_ptr<MsgUnit>, int> &msgQueueTmp = idMsgMap[msg.getMsgId()];
      msgQueueTmp.insert(pair<shared_ptr<MsgUnit>, int>(msgUnit, 1));
    }
    fetchedArr.push(msgUnit);
  }
}

bool ConsumerUnitLocker::getMsg(shared_ptr<MsgUnit> &unit) {
  //这个锁是来让消息要wait之后才能消费
  std::unique_lock<std::mutex> lk(mtx);
  if (fetchedArr.empty())
    return false;
  unit = std::move(fetchedArr.front());
  fetchedArr.pop();
  return true;
}

bool ConsumerUnitLocker::setMsgStatus(const string msgId, ConsumeStatus s, ClientMsgConsumeStatus cs) {
  bool ret = false;
  if (idMsgMap.find(msgId) != idMsgMap.end()) {
    auto &idMap = idMsgMap[msgId];
    auto iter = idMap.begin();
    while (iter != idMap.end()) {
      statusMap[iter->first] = s;
      clientStatusMap[iter->first] = cs;
      ret = true;
      if (s == RECONSUME_LATER) {
        reconsumeMap[msgId] = 1;
      }
      iter++;
    }
  }
  return ret;
}

void ConsumerUnitLocker::waitForLock(std::function<void(std::unique_lock<std::mutex> &)> &func) {
  std::unique_lock<std::mutex> lk(mtx);
  func(lk);
  cv.wait(lk, [this] { return clientStatus == MSG_CONSUME_ACK; });
}

void ConsumerUnitLocker::triggerCheck() {
  if (fetchedArr.size()) {
    return;
  }
  auto iter = clientStatusMap.begin();
  ConsumeStatus s(CONSUME_SUCCESS);
  ClientMsgConsumeStatus cs(MSG_CONSUME_ACK);
  while (iter != clientStatusMap.end()) {
    cs = iter->second;
    if (cs != MSG_CONSUME_ACK) {
      break;
    }

    if (statusMap[iter->first] == RECONSUME_LATER) {
      s = RECONSUME_LATER;
    }
    iter++;
  }
  if (cs == MSG_CONSUME_ACK) {
    status = s;
    clientStatus = cs;
    cv.notify_all();
  }
}

void ConsumerUnit::waitLock(shared_ptr<ConsumerUnitLocker> &locker) {
  //锁的顺序很重要，先锁大锁
  boost::unique_lock<boost::shared_mutex> lk(lockersMtx);
  lockers.insert(pair<shared_ptr<ConsumerUnitLocker>, int>(locker, 1));
  std::unique_lock<std::mutex> lk1(locker->mtx);
  lk.unlock();
  locker->cv.wait(lk1, [&] { return locker->clientStatus == MSG_CONSUME_ACK; });
}

void ConsumerUnit::eraseLock(const shared_ptr<ConsumerUnitLocker> &lock) {
  boost::unique_lock<boost::shared_mutex> lk(lockersMtx);
  lockers.erase(lock);
}

MsgWorker *CallDataBase::msgWorker = new MsgWorker();
