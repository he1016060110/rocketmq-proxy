//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_MSGWORKER_H
#define ROCKETMQ_PROXY_MSGWORKER_H

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

using namespace std;
using namespace rocketmq;

enum MsgWorkerConsumeStatus {
    PROXY_CONSUME_INIT,
    PROXY_CONSUME,
    CLIENT_RECEIVE,
    CONSUME_ACK
};

class ConsumerUnit {
public:
    ConsumerUnit(string topic) : consumer(DefaultMQPushConsumer(topic)), lastActiveAt(time(0)) {};
    DefaultMQPushConsumer consumer;
    time_t lastActiveAt;
};

class ConsumeMsgUnit {
public:
    ConsumeMsgUnit(ConsumeCallData *paramCallData, string paramTopic, string paramGroup) :
        callData(paramCallData), topic(paramTopic), group(paramGroup), status(PROXY_CONSUME_INIT),
        lastActiveAt(time(0)) {
    };
    ConsumeCallData *callData;
    ConsumeAckCallData *ackCallData;
    string topic;
    string group;
    string msgId;
    time_t consumeByProxyAt;
    time_t sendClientAt;
    time_t ackAt;
    time_t lastActiveAt;
    MsgWorkerConsumeStatus status;
};


class ConsumerMsgListener : public MessageListenerConcurrently {
public:
    ConsumerMsgListener() {}

    virtual ~ConsumerMsgListener() {}

    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs) {
      callback(msgs);
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
    map<string, shared_ptr<ConsumeMsgUnit>> msgs;

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
        auto callback = [=](const std::vector<MQMessageExt> &msgs) {
            auto msg = msgs[0];
            auto key = topic + group;
            shared_ptr<QueueTS<MQMessageExt>> pool;
            if (msgPool.try_get(key, pool)) {
              pool->push(msg);
            } else {
              shared_ptr<QueueTS<MQMessageExt>> msgP(new QueueTS<MQMessageExt>);
              msgP->push(msg);
              msgPool.insert(key, msgP);
            }
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            //todo 等待消息被消费
            return CONSUME_SUCCESS;
        };
        listener->setMsgCallback(callback);
        consumerUnit->consumer.registerMessageListener(listener);
        try {
          consumerUnit->consumer.start();
          cout << "connected to " << nameServerHost_ << " topic is " << topic << endl;
          consumers.insert(key, consumerUnit);
          return consumerUnit;
        } catch (MQClientException &e) {
          cout << e << endl;
          return nullptr;
        }
      }
    }

    QueueTS<vector<string>> msgConsumerCreateQueue;
    MapTS<string, shared_ptr<QueueTS<MQMessageExt>>> msgPool;

    void resourceManager() {
      for (;;) {
        vector<string> v = msgConsumerCreateQueue.wait_and_pop();
        string topic = v[0];
        string group = v[1];
        getConsumer(topic, group);
      }
    }

    bool getMsgPoolExist(const string &topic, const string &group) {
      auto key = topic + group;
      shared_ptr<QueueTS<MQMessageExt>> pool;
      return msgPool.try_get(key, pool);
    }

    void loopMatch() {
      while (true) {
        auto iter = consumeMsgPool.begin();
        while (iter != consumeMsgPool.end()) {
          cout << "loopMatch enter!" << endl;
          auto unit = iter->get();
          if (unit->status == PROXY_CONSUME_INIT) {
            //consumer不存在的时候创建consumer
            if (!getConsumerExist(unit->topic, unit->group)) {
              cout << "getConsumerExist enter!" << endl;
              vector<string> v;
              v.push_back(unit->topic);
              v.push_back(unit->group);
              msgConsumerCreateQueue.push(v);
            }
            auto key = unit->topic + unit->group;
            //检查消息队列pool里面有没有消息
            shared_ptr<QueueTS<MQMessageExt>> pool;
            if (msgPool.try_get(key, pool)) {
              MQMessageExt msg;
              if (pool->try_pop(msg)) {
                unit->callData->responseMsg(0, "", msg.getMsgId(), msg.getBody());
              }
            }
          }
        }
      }
    }

public:
    void startMatcher() {
      boost::thread(boost::bind(&MsgWorker::loopMatch, this));
    }

    void startResourceManager() {
      boost::thread(boost::bind(&MsgWorker::resourceManager, this));
    }

    vector<shared_ptr<ConsumeMsgUnit>> consumeMsgPool;

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


#endif //ROCKETMQ_PROXY_MSGWORKER_H
