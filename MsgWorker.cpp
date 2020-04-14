//
// Created by hexi on 2020/4/4.
//

#include "MsgWorker.h"

void MsgWorker::loopMatch() {
  shared_ptr<ConsumeMsgUnit> unit;
  while (true) {
    QueueTS<shared_ptr<ConsumeMsgUnit>> tmp;
    while (consumeMsgPool.try_pop(unit)) {
      auto consumer = getConsumer(unit->topic, unit->group);
      if (unit->status == PROXY_CONSUME_INIT) {
        auto iter = consumer->lockers.begin();
        while(iter != consumer->lockers.end()) {
          auto locker = iter->first;
          shared_ptr<MsgUnit> msg;
          if (locker->getMsg(msg)) {
            unit->msgId = msg->msgId;
#ifdef DEBUG
            cout << msg->msgId << " consumed!" << endl;
#endif
            unit->status = CLIENT_RECEIVE;
            idUnitMap.insert_or_update(msg->msgId, unit);
            unit->callData->responseMsg(0, "", msg->msgId, msg->body);
            resetConsumerActive(unit->topic, unit->group);
            break;
          }
          iter++;
        }
        if (unit->getIsFetchMsgTimeout()) {
          resetConsumerActive(unit->topic, unit->group);
          unit->callData->responseTimeOut();
          continue;
        }
        tmp.push(unit);
      } else if (unit->status == CLIENT_RECEIVE) {
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
    consumerUnit->consumer.setConsumeThreadCount(3);
    consumerUnit->consumer.setTcpTransportTryLockTimeout(1000);
    consumerUnit->consumer.setTcpTransportConnectTimeout(400);
    consumerUnit->consumer.setSessionCredentials(accessKey_, secretKey_, accessChannel_);
    auto listener = new ConsumerMsgListener();
    auto callback = [group, consumerUnit](const std::vector<MQMessageExt> &msgs) {
        shared_ptr<ConsumerUnitLocker> locker(new ConsumerUnitLocker(msgs, group));
        consumerUnit->insertLock(locker);
        locker->waitForLock();
        consumerUnit->eraseLock(locker);
        return locker->status;
    };
    listener->setMsgCallback(callback);
    consumerUnit->consumer.registerMessageListener(listener);
    initMsgQueue(key);
    try {
      consumerUnit->consumer.start();
      consumers.insert(key, consumerUnit);
      return consumerUnit;
    } catch (MQClientException &e) {
      cout << e << endl;
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
#ifdef DEBUG
        cout << keys[i] << " is going to shutdown!" << endl;
#endif
        clearCV.notify_one();
        std::unique_lock<std::mutex> lk(processMsgMtx);
        clearConsumerKey = keys[i];
        processMsgCV.wait(lk);
        unit->consumer.shutdown();
#ifdef DEBUG
        cout << keys[i] << " shutdown success!" << endl;
#endif
        consumers.erase(keys[i]);
      }
    }
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
}

void MsgWorker::clearMsgForConsumer() {
  while (true) {
    //等待shutdown 处理程序通知
    std::unique_lock<std::mutex> lk1(clearMtx);
    clearCV.wait(lk1);
    //获取到锁之后立即通知
    std::unique_lock<std::mutex> lk2(processMsgMtx);
    processMsgCV.notify_one();
    //可以过滤消息了

    shared_ptr<QueueTS<MsgUnit>> pool;
  }
}

void ConsumerUnit::unlockAll() {

}

bool ConsumerUnit::setMsgReconsume(const string &msgId) {
  auto iter= lockers.begin();
  while(iter != lockers.end()) {
    auto locker = iter->first;
    locker->setMsgStatus(msgId, RECONSUME_LATER, MSG_CONSUME_ACK);
    iter++;
  }
}

bool ConsumerUnit::setMsgAck(const string & msgId, ConsumeStatus s) {
  auto iter= lockers.begin();
  bool ret = false;
  while(iter != lockers.end()) {
    auto locker = iter->first;
    ret = locker->setMsgStatus(msgId, s, MSG_CONSUME_ACK);
    iter++;
  }
  return ret;
}

ConsumerUnitLocker::ConsumerUnitLocker(const std::vector<MQMessageExt> &msgs, const string &group) : status(
    RECONSUME_LATER), clientStatus(MSG_FETCH_FROM_BROKER) {
  for (int i = 0; i < msgs.size(); i++) {
    auto msg = msgs[i];
    shared_ptr<MsgUnit> msgUnit(new MsgUnit);
    msgUnit->msgId = msg.getMsgId();
    msgUnit->type = 1;
    msgUnit->delayLevel = msg.getDelayTimeLevel();
    msgUnit->body = msg.getBody();
    msgUnit->topic = msg.getTopic();
    msgUnit->group = group;
    clientStatusMap.insert(pair<shared_ptr<MsgUnit>, ClientMsgConsumeStatus>(msgUnit, MSG_FETCH_FROM_BROKER));
    statusMap.insert(pair<shared_ptr<MsgUnit>, ConsumeStatus>(msgUnit, RECONSUME_LATER));
    idMsgMap.insert(pair<string, shared_ptr<MsgUnit>>(msg.getMsgId(), msgUnit));
    fetchedArr.push(msgUnit);
  }
}

bool ConsumerUnitLocker::getMsg(shared_ptr<MsgUnit> &unit) {
  std::unique_lock<std::mutex> lk(mtx);
  if (fetchedArr.empty())
    return false;
  unit = std::move(fetchedArr.front());
  fetchedArr.pop();
  return true;
}

bool ConsumerUnitLocker::setMsgStatus(const string msgId, ConsumeStatus s, ClientMsgConsumeStatus cs) {
  std::unique_lock<std::mutex> lk(mtx);
  bool ret = false;
  if (idMsgMap.find(msgId) != idMsgMap.end()) {
    auto unit = idMsgMap[msgId];
    statusMap[unit] = s;
    clientStatusMap[unit] = cs;
    ret = true;
  }
  return ret;
}

void ConsumerUnitLocker::waitForLock()
{
  std::unique_lock<std::mutex> lk(mtx);
  cv.wait(lk, [this] {return clientStatus == MSG_CONSUME_ACK;});
}

void ConsumerUnit::insertLock(shared_ptr<ConsumerUnitLocker> lock) {
  std::unique_lock<std::mutex> lk(lockersMtx);
  lockers.insert(pair<shared_ptr<ConsumerUnitLocker>, int>(lock, 1));
}

void ConsumerUnit::eraseLock(const shared_ptr<ConsumerUnitLocker> &lock) {
  std::unique_lock<std::mutex> lk(lockersMtx);
  lockers.erase(lock);
}

MsgWorker *CallDataBase::msgWorker = new MsgWorker();
