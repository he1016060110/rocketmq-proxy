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
#include "common.h"

using namespace std;
using namespace rocketmq;

#define MAX_MSG_WAIT_CONSUME_TIME 10
#define MAX_MSG_WAIT_CONSUME_ACK_TIME 10
#define MAX_MSG_CONSUME_MAX_INACTIVE_TIME 20

enum MsgWorkerConsumeStatus {
    PROXY_CONSUME_INIT,
    CLIENT_RECEIVE,
    CONSUME_ACK
};

enum ClientMsgConsumeStatus {
    MSG_FETCH_FROM_BROKER,
    CLIENT_RECEIVE,
    MSG_CONSUME_ACK
};

class MsgUnit {
public:
    int type;//1,produce,consume
    string msgId;
    string topic;
    string group;
    string body;
    int delayLevel;
    int status;//0
    time_t fetchTime;
public:
    MsgUnit() : type(0), msgId(""), topic(""), group(""), body(), delayLevel(0), status(0), fetchTime(time(0)){};
};

class MsgMatchUnit;

class ConsumerUnitLocker {
public:
    ConsumerUnitLocker(const std::vector<MQMessageExt> &msgs, const string & group);
    std::mutex mtx;
    std::condition_variable cv;
    std::map<shared_ptr<MsgUnit>, ClientMsgConsumeStatus> clientStatusMap;
    std::map<shared_ptr<MsgUnit>, ConsumeStatus> statusMap;
    std::map<string, shared_ptr<MsgUnit>> idMsgMap;
    std::queue<shared_ptr<MsgUnit>> fetchedArr;
    std::queue<shared_ptr<MsgUnit>> matchedArr;
    std::queue<shared_ptr<MsgUnit>> ackArr;
    //全部的status
    ConsumeStatus status;
    ClientMsgConsumeStatus clientStatus;
    bool getMsg(shared_ptr<MsgUnit> unit);
    bool setMsgStatus(const string msgId, ConsumeStatus s, ClientMsgConsumeStatus cs);
    void waitForLock();
};

class ConsumerUnit {
public:
    std::map<shared_ptr<ConsumerUnitLocker>, int> lockers;
    std::mutex lockersMtx;
    ConsumerUnit(string topic) : consumer(DefaultMQPushConsumer(topic)), lastActiveAt(time(0)) {};
    DefaultMQPushConsumer consumer;
    time_t lastActiveAt;

    void unlockAll();

    void insertLock(shared_ptr<ConsumerUnitLocker> lock);

    void eraseLock(shared_ptr<ConsumerUnitLocker> lock);

    bool getIsTooInactive() {
      return time(0) - lastActiveAt >= MAX_MSG_CONSUME_MAX_INACTIVE_TIME;
    }

};

class MsgMatchUnit {
public:
    MsgMatchUnit() : status(MSG_FETCH_FROM_BROKER), counter(1) {};
    MsgConsumeStatus status;
    //rocketmq status
    ConsumeStatus consumeStatus;
    std::mutex mtx;
    std::condition_variable cv;
    int counter;
};

class ConsumeMsgUnit {
public:
    ConsumeMsgUnit(ConsumeCallData *paramCallData, string paramTopic, string paramGroup) :
        callData(paramCallData), topic(paramTopic), group(paramGroup), lastActiveAt(time(0)),
        status(PROXY_CONSUME_INIT) {
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

class ProducerUnit {
public:
    ProducerUnit(string topic) : producer(DefaultMQProducer(topic)), lastActiveAt(time(0)) {};
    DefaultMQProducer producer;
    time_t lastActiveAt;
};

class MsgWorker {
    string nameServerHost_;
    string accessKey_;
    string secretKey_;
    string accessChannel_;
    map<string, shared_ptr<ProducerUnit>> producers;
    MapTS<string, shared_ptr<ConsumerUnit>> consumers;

    shared_ptr<ProducerUnit> getProducer(const string &topic, const string &group);

    void initMsgQueue(const string &key) {
      shared_ptr<QueueTS<MsgUnit>> msgP(new QueueTS<MsgUnit>);
      msgPool.insert(key, msgP);
    }

    bool getConsumerExist(const string &topic, const string &group) {
      auto key = topic + group;
      shared_ptr<ConsumerUnit> unit;
      return consumers.try_get(key, unit);
    }

    shared_ptr<ConsumerUnit> getConsumer(const string &topic, const string &group);

    MapTS<string, shared_ptr<QueueTS<MsgUnit>>> msgPool;

    std::mutex clearMtx;
    std::mutex processMsgMtx;
    std::condition_variable clearCV;
    std::condition_variable processMsgCV;
    string clearConsumerKey;

    void shutdownConsumer();

    void clearMsgForConsumer();

    void notifyTimeout() {
      for (;;) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        notifyCV.notify_all();
      }
    }

    void loopMatch();

public:
    void startMatcher() {
      boost::thread(boost::bind(&MsgWorker::loopMatch, this));
    }

    void startNotifyTimeout() {
      boost::thread(boost::bind(&MsgWorker::notifyTimeout, this));
    }

    void startShutdownConsumer() {
      boost::thread(boost::bind(&MsgWorker::shutdownConsumer, this));
    }

    void startClearMsgForConsumer() {
      boost::thread(boost::bind(&MsgWorker::clearMsgForConsumer, this));
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
