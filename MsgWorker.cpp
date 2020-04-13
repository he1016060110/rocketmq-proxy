//
// Created by hexi on 2020/4/4.
//

#include "MsgWorker.h"

void MsgWorker::loopMatch() {
  shared_ptr<ConsumeMsgUnit> unit;
  while (true) {
    QueueTS<shared_ptr<ConsumeMsgUnit>> tmp;
    while (consumeMsgPool.try_pop(unit)) {
      if (unit->status == PROXY_CONSUME_INIT) {
        //consumer不存在的时候创建consumer
        if (!getConsumerExist(unit->topic, unit->group)) {
          getConsumer(unit->topic, unit->group);
        }
        auto key = unit->topic + unit->group;
        //检查消息队列pool里面有没有消息
        {
          shared_ptr<QueueTS<MsgUnit>> pool;
          if (msgPool.try_get(key, pool)) {
            MsgUnit msg;
            if (pool->try_pop(msg)) {
              unit->msgId = msg.msgId;
#ifdef DEBUG
              cout << msg.msgId << " consumed!" << endl;
#endif
              unit->status = CLIENT_RECEIVE;
              idUnitMap.insert_or_update(msg.msgId, unit);
              unit->callData->responseMsg(0, "", msg.msgId, msg.body);
              resetConsumerActive(unit->topic, unit->group);
            }
          }
        }

        if (unit->getIsFetchMsgTimeout()) {
          resetConsumerActive(unit->topic, unit->group);
          unit->callData->responseTimeOut();
          continue;
        }
        tmp.push(unit);
      } else if (unit->status == CLIENT_RECEIVE) {
        shared_ptr<MsgMatchUnit> matchUnit;
        if (MsgMatchUnits.try_get(unit->msgId, matchUnit)) {
          if (unit->getIsAckTimeout()) {
            {
              std::unique_lock<std::mutex> lk(matchUnit->mtx);
              matchUnit->status = MSG_CONSUME_ACK;
              matchUnit->consumeStatus = RECONSUME_LATER;
            }
            matchUnit->cv.notify_all();
            continue;
          }
          tmp.push(unit);
        } else {
          //如果找不到了，说明已经删除msg和请求绑定关系，需要删除掉
          continue;
        }
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
    auto key = topic + group;
    auto callback = [this, topic, group, key, consumerUnit](const std::vector<MQMessageExt> &msgs) {
        if (msgs.size() != 1) {
          cout << "msg batch size is not eq 1" << endl;
          exit(1);
        }
        auto msg = msgs[0];
        shared_ptr<MsgMatchUnit> unit;

        auto pushMsg = [msg, group, key, this] {
            shared_ptr<QueueTS<MsgUnit>> pool;
            MsgUnit msgUnit;
            msgUnit.msgId = msg.getMsgId();
            msgUnit.type = 1;
            msgUnit.delayLevel = msg.getDelayTimeLevel();
            msgUnit.body = msg.getBody();
            msgUnit.topic = msg.getTopic();
            msgUnit.group = group;
            if (this->msgPool.try_get(key, pool)) {
              pool->push(msgUnit);
            } else {
              //不应该出现这种情况
            }
        };

        //找到了，就直接wait
        if (MsgMatchUnits.try_get(msg.getMsgId(), unit)) {
          std::unique_lock<std::mutex> lk(unit->mtx);
          pushMsg();
          this->notifyCV.notify_all();
          unit->cv.wait(lk, [&] { return unit->status == MSG_CONSUME_ACK; });
        } else {
          unit = shared_ptr<MsgMatchUnit>(new MsgMatchUnit);
          consumerUnit->insertLock(unit);
          std::unique_lock<std::mutex> lk(unit->mtx);
          //要先在MsgMatchUnits 插入消息，然后才能发送消息，不然会找不到消息消息中断
          MsgMatchUnits.insert(msg.getMsgId(), unit);
#ifdef DEBUG
          cout << "thread id[" << std::this_thread::get_id() << "] msg id[" <<
               msg.getMsgId() << "] unit address[" << unit.get() << "]" << endl;
#endif
          pushMsg();
          this->notifyCV.notify_all();
          unit->cv.wait(lk, [&] { return unit->status == MSG_CONSUME_ACK; });
        }
#ifdef DEBUG
        cout << "thread id[" << std::this_thread::get_id() << "] msg id[" << msg.getMsgId() << "] unlock!" << endl;
#endif
        consumerUnit->eraseLock(unit);
        return unit->consumeStatus;
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
    if (msgPool.try_get(clearConsumerKey, pool)) {
      MsgUnit msg;
      while (pool->try_pop(msg)) {
        shared_ptr<MsgMatchUnit> matchUnit;
        if (MsgMatchUnits.try_get(msg.msgId, matchUnit)) {
          {
            std::unique_lock<std::mutex> lk(matchUnit->mtx);
            matchUnit->status = MSG_CONSUME_ACK;
            matchUnit->consumeStatus = RECONSUME_LATER;
          }
          matchUnit->cv.notify_all();
        }
      }
      shared_ptr<ConsumerUnit> unit;
      //双重保险
      if (consumers.try_get(clearConsumerKey, unit)) {
        unit->unlockAll();
      }
    }
  }
}

void ConsumerUnit::unlockAll()
{
  std::unique_lock<std::mutex> lk(matchUnitsMtx);
  auto iter = matchUnits.begin();
  while (iter != matchUnits.end()) {
    auto matchUnit = iter->first;
    {
      std::unique_lock<std::mutex> lk(matchUnit->mtx);
      matchUnit->status = MSG_CONSUME_ACK;
      matchUnit->consumeStatus = RECONSUME_LATER;
    }
    matchUnit->cv.notify_all();
    iter++;
  }
  matchUnits.empty();
}

void ConsumerUnit::insertLock(shared_ptr<MsgMatchUnit> lock)
{
  std::unique_lock<std::mutex> lk(matchUnitsMtx);
  matchUnits.insert(pair<shared_ptr<MsgMatchUnit>, int> (lock, 1));
}

void ConsumerUnit::eraseLock(shared_ptr<MsgMatchUnit> lock)
{
  std::unique_lock<std::mutex> lk(matchUnitsMtx);
  matchUnits.erase(lock);
}

MsgWorker *CallDataBase::msgWorker = new MsgWorker();
