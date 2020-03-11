//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_WORKERPOOL_HPP
#define ROCKETMQ_PROXY_WORKERPOOL_HPP

#include "common.hpp"

class WorkerPool {
    std::map<string, shared_ptr<DefaultMQProducer> > producers;
    std::map<string, shared_ptr<ProxyPushConsumer> > consumers;
    string nameServerHost;
public:
    MapTS<string, MsgConsumeUnit *> consumerUnitMap;
    map<shared_ptr<WsServer::Connection>, shared_ptr<ConnectionUnit> > connectionUnit;
    WorkerPool(string nameServer)
            : nameServerHost(nameServer) {};
    //连接断掉后，以前队列要把相关连接清空！
    void deleteConnection(shared_ptr<WsServer::Connection> &con) {
        auto iter = consumers.begin();
        while (iter != consumers.end()) {
            auto consumer = iter->second;
            shared_ptr<WsServer::Connection> value;
            QueueTS<shared_ptr<WsServer::Connection>> tempQueue;
            while (consumer->queue.try_pop(value)) {
                if (value == con) {
                    continue;
                }
                tempQueue.push(value);
            }
            while (tempQueue.try_pop(value)) {
                consumer->queue.push(value);
            }
            iter++;
        }
    }

    shared_ptr<DefaultMQProducer> getProducer(const string &topic, const string &group) {
        auto key = topic + group;
        auto iter = producers.find(key);
        if (iter != producers.end())
            return iter->second;
        else {
            shared_ptr<DefaultMQProducer> producer(new DefaultMQProducer(topic));
            producer->setNamesrvAddr(nameServerHost);
            producer->setGroupName(group);
            producer->setInstanceName(topic);
            producer->setSendMsgTimeout(500);
            producer->setTcpTransportTryLockTimeout(1000);
            producer->setTcpTransportConnectTimeout(400);
            try {
                producer->start();
                producers.insert(pair<string, shared_ptr<DefaultMQProducer>>(key, producer));
                return producer;
            } catch (exception &e) {
                cout << e.what() << endl;
                return NULL;
            }
        }
    }

    shared_ptr<ProxyPushConsumer> getConsumer(const string &topic, const string &group) {
        auto key = topic + group;
        auto iter = consumers.find(key);
        if (iter != consumers.end())
            return iter->second;
        else {
            shared_ptr<ProxyPushConsumer> consumer(new ProxyPushConsumer(group));
            consumer->setNamesrvAddr(nameServerHost);
            consumer->setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
            consumer->setInstanceName(group);
            consumer->subscribe(topic, "*");
            //改为一个线程看是否有问题
            consumer->setConsumeThreadCount(5);
            consumer->setTcpTransportTryLockTimeout(1000);
            consumer->setTcpTransportConnectTimeout(400);
            consumer->initResource(&connectionUnit, &consumerUnitMap);
            ConsumerMsgListener *listener = new ConsumerMsgListener();
            listener->setConsumer(consumer);
            consumer->registerMessageListener(listener);
            try {
                consumer->start();
                cout << "connected to "<< nameServerHost<< " topic is " << topic << endl;
                consumers.insert(pair<string, shared_ptr<ProxyPushConsumer>>(key, consumer));
                return consumer;
            } catch (MQClientException &e) {
                cout << e << endl;
                return NULL;
            }
        }
    }
};

#endif //ROCKETMQ_PROXY_WORKERPOOL_HPP
