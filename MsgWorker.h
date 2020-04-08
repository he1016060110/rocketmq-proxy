//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_MSG_WORKER_H
#define ROCKETMQ_PROXY_MSG_WORKER_H

#include "ProducerCallback.h"
#include "ConsumeCallData.h"
#include "ConsumeAckCallData.h"
#include "DefaultMQProducer.h"
#include "QueueTS.hpp"
#include "MapTS.hpp"
#include "DefaultMQPushConsumer.h"
#include <iostream>
#include <string>
#include "boost/thread.hpp"
#include <memory>
#include <thread>

using namespace std;
using namespace rocketmq;

#define MAX_MSG_WAIT_CONSUME_TIME 10
#define MAX_MSG_WAIT_CONSUME_ACK_TIME 10

enum MsgWorkerConsumeStatus {
    PROXY_CONSUME_INIT,
    CLIENT_RECEIVE,
    CONSUME_ACK
};

enum MsgConsumeStatus {
    MSG_FETCH_FROM_BROKER,
    MSG_DISTRIBUTED,
    MSG_CONSUME_ACK
};


class ConsumerUnit {
public:
    ConsumerUnit(string topic) : consumer(DefaultMQPushConsumer(topic)), lastActiveAt(time(0)) {};
    DefaultMQPushConsumer consumer;
    time_t lastActiveAt;
};

class MsgMatchUnit {
public:
    MsgMatchUnit() : status(MSG_FETCH_FROM_BROKER) {};
    MsgConsumeStatus status;
    //rocketmq status
    ConsumeStatus consumeStatus;
    std::mutex mtx;
    std::condition_variable cv;
};

class ConsumeMsgUnit {
public:
    ConsumeMsgUnit(ConsumeCallData *paramCallData, string paramTopic, string paramGroup) :
        callData(paramCallData), topic(paramTopic), group(paramGroup), status(PROXY_CONSUME_INIT),
        lastActiveAt(time(0)) {
    };
    ConsumeCallData *callData;
    string topic;
    string group;
    string msgId;
    time_t lastActiveAt;

    bool getIsFetchMsgTimeout() {
      return status == PROXY_CONSUME_INIT && time(0) - lastActiveAt >= MAX_MSG_WAIT_CONSUME_TIME;
    }

    bool getIsAckTimeout() {
      return status == CLIENT_RECEIVE && time(0) - lastActiveAt >= MAX_MSG_WAIT_CONSUME_ACK_TIME;
    }

    bool getIsAck() {
      return status == CONSUME_ACK;
    }

    MsgWorkerConsumeStatus status;
};


class ConsumerMsgListener : public MessageListenerConcurrently {
public:
    ConsumerMsgListener() {}

    virtual ~ConsumerMsgListener() {}

    ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs) {
      return callback(msgs);
    }

    void setMsgCallback(std::function<ConsumeStatus(const std::vector<MQMessageExt> &msgs)> paramCallback) {
      callback = paramCallback;
    }

private:
    std::function<ConsumeStatus(const std::vector<MQMessageExt> &msgs)> callback;
};

class MsgWorker {
    string nameServerHost_;
    string accessKey_;
    string secretKey_;
    string accessChannel_;

    class ProducerUnit {
    public:
        ProducerUnit(string topic) : producer(DefaultMQProducer(topic)), lastActiveAt(time(0)) {};
        DefaultMQProducer producer;
        time_t lastActiveAt;
    };

    map<string, shared_ptr<ProducerUnit>> producers;
    MapTS<string, shared_ptr<ConsumerUnit>> consumers;


    shared_ptr<ProducerUnit> getProducer(const string &topic, const string &group) {
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
          cout << e.what() << endl;
          return nullptr;
        }
      }
    }

    void initMsgQueue(const string &key) {
      shared_ptr<QueueTS<MQMessageExt>> msgP(new QueueTS<MQMessageExt>);
      msgPool.insert(key, msgP);
    }

    bool getConsumerExist(const string &topic, const string &group) {
      auto key = topic + group;
      shared_ptr<ConsumerUnit> unit;
      return consumers.try_get(key, unit);
    }

    shared_ptr<ConsumerUnit> getConsumer(const string &topic, const string &group) {
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
        auto callback = [this, topic, group, key](const std::vector<MQMessageExt> &msgs) {
            if (msgs.size() != 1) {
              cout << "msg batch size is not eq 1" << endl;
              exit(1);
            }
            cout << "thread id[" << std::this_thread::get_id() << "]" << endl;
            auto msg = msgs[0];
            shared_ptr<QueueTS<MQMessageExt>> pool;
            if (this->msgPool.try_get(key, pool)) {
              pool->push(msg);
            } else {
              //不应该出现这种情况
            }
            shared_ptr<MsgMatchUnit> unit(new MsgMatchUnit);
            {
              std::unique_lock<std::mutex> lk(unit->mtx);
              MsgMatchUnits.insert(msg.getMsgId(), unit);
              this->notifyCV.notify_all();
              unit->cv.wait(lk, [&] { return unit->status == MSG_CONSUME_ACK; });
            }
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

    MapTS<string, shared_ptr<QueueTS<MQMessageExt>>> msgPool;

    void notifyTimeout() {
      for (;;) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        std::unique_lock<std::mutex> lk(notifyMtx);
        notifyCV.notify_all();
      }
    }

    void loopMatch() {
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
            shared_ptr<QueueTS<MQMessageExt>> pool;
            if (msgPool.try_get(key, pool)) {
              MQMessageExt msg;
              if (pool->try_pop(msg)) {
                unit->msgId = msg.getMsgId();
                unit->status = CLIENT_RECEIVE;
                idUnitMap.insert_or_update(msg.getMsgId(), unit);
                unit->callData->responseMsg(0, "", msg.getMsgId(), msg.getBody());
                resetConsumerActive(unit->topic, unit->group);
              }
            }
          }
          if (!unit->getIsAck()) {
            if (unit->getIsFetchMsgTimeout()) {
              resetConsumerActive(unit->topic, unit->group);
              unit->callData->responseTimeOut();
            } else if (unit->getIsAckTimeout()) {
              shared_ptr<MsgMatchUnit> matchUnit;
              //如果超时，设置为reconsume later
              if (MsgMatchUnits.try_get(unit->msgId, matchUnit)) {
                std::unique_lock<std::mutex> lk(matchUnit->mtx);
                matchUnit->status = MSG_CONSUME_ACK;
                matchUnit->consumeStatus = RECONSUME_LATER;
                matchUnit->cv.notify_all();
              }
            }
            tmp.push(unit);
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

public:
    void startMatcher() {
      boost::thread(boost::bind(&MsgWorker::loopMatch, this));
    }

    void startNotifyTimeout() {
      boost::thread(boost::bind(&MsgWorker::notifyTimeout, this));
    }

    MapTS<string, shared_ptr<MsgMatchUnit>> MsgMatchUnits;
    QueueTS<shared_ptr<ConsumeMsgUnit>> consumeMsgPool;

    void resetConsumerActive(const string &topic, const string &group) {
      auto key = topic + group;
      shared_ptr<ConsumerUnit> unit;
      if (consumers.try_get(key, unit)) {
        unit->lastActiveAt = time(0);
      }
    }

    //msgId 与消息消费对应关系
    MapTS<string, shared_ptr<ConsumeMsgUnit>> idUnitMap;
    std::mutex notifyMtx;
    std::condition_variable notifyCV;

    void produce(ProducerCallback *callback, const string &topic, const string &group,
                 const string &tag, const string &body, const int delayLevel = 0) {
      rocketmq::MQMessage msg(topic, tag, body);
      msg.setDelayTimeLevel(delayLevel);
      auto producerUnit = getProducer(topic, group);
      producerUnit->producer.send(msg, callback);
    }

    void setConfig(string &nameServer, string &accessKey, string &secretKey, string channel) {
      nameServerHost_ = nameServer;
      accessKey_ = accessKey;
      secretKey_ = secretKey;
      accessChannel_ = channel;
    }
};


#endif //ROCKETMQ_PROXY_MSG_WORKER_H
