//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_WORKERPOOL_HPP
#define ROCKETMQ_PROXY_WORKERPOOL_HPP

#include "common.hpp"

class WorkerPool {
    class ConsumerConnectionUnit
    {
    public:
        std::mutex mtx;
        std::map<shared_ptr<WsServer::Connection>, int> conn;
    };
    std::map<string, shared_ptr<ProxyPushConsumer> > consumers;
    std::map<string, shared_ptr<DefaultMQProducer>> producers;
    std::map<shared_ptr<DefaultMQProducer>, shared_ptr<std::map<shared_ptr<WsServer::Connection>, int>>> producerConnectionMap;
    MapTS<shared_ptr<ProxyPushConsumer>, shared_ptr<ConsumerConnectionUnit> > consumerConnUnit;
    string nameServerHost;
    string accessKey;
    string secretKey;
    string accessChannel;
public:
    ProxyLogger log;
    MapTS<string, MsgConsumeUnit *> consumerUnitMap;
    map<shared_ptr<WsServer::Connection>, shared_ptr<ConnectionUnit> > connectionUnit;
    WorkerPool(string nameServer, string accessKey, string secretKey, string  esServer)
            : nameServerHost(nameServer),accessKey(accessKey),secretKey(secretKey),accessChannel("local"),log(esServer) {
        startEsLog();
    };

    void startEsLog() {
        boost::thread(boost::bind(&ProxyLogger::loopConsumeLog, &log));
    }
    void deleteProducerConn(shared_ptr<WsServer::Connection> &con) {
        auto iter = producers.begin();
        while(iter != producers.end()) {
            auto producer = iter->second;
            string key = iter->first;
            auto connMap = producerConnectionMap[producer];
            auto iter1 = connMap->find(con);
            if (iter1 != connMap->end()) {
                connMap->erase(con);
            }
            if (!connMap->size()) {
                producer->shutdown();
                producers.erase(key);
                producerConnectionMap.erase(producer);
            }
            iter++;
        }
    }

    void deleteConsumerQueue(shared_ptr<WsServer::Connection> &con) {
        auto iter = consumers.begin();
        while (iter != consumers.end()) {
            auto consumer = iter->second;
            shared_ptr<WsServer::Connection> value;
            QueueTS<shared_ptr<WsServer::Connection>> tempQueue;
            //1.过滤掉消息
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

    //连接断掉后，以前队列要把相关连接清空！
    void deleteConsumerConnection(shared_ptr<WsServer::Connection> &con) {
        auto iter = consumers.begin();
        QueueTS<string> consumerEraseQueue;
        while (iter != consumers.end()) {
            auto consumer = iter->second;
            //删掉consumer
            shared_ptr<ConsumerConnectionUnit> unit(new ConsumerConnectionUnit);
            if (consumerConnUnit.try_get(consumer, unit)) {
                {
                    std::unique_lock<std::mutex> lck(unit->mtx);
                    auto iterConn = unit->conn.find(con);
                    if (iterConn != unit->conn.end()) {
                        unit->conn.erase(con);
                    }
                }
                if (!unit->conn.size()) {
                    consumer->toDelete = true;
                    consumer->shutdown();
                    consumerEraseQueue.push(consumer->uniqKey);
                    consumerConnUnit.erase(consumer);
                }
            }
            iter++;
        }
        string key;
        //map遍历的时候不能修改
        while (consumerEraseQueue.try_pop(key)) {
            consumers.erase(key);
        }
    }

    shared_ptr<DefaultMQProducer> getProducer(const string &topic, const string &group, shared_ptr<WsServer::Connection> & conn) {
        auto key = topic + group;
        auto iter = producers.find(key);
        if (iter != producers.end()) {
            auto connMap = producerConnectionMap[iter->second];
            auto iter1 = connMap->find(conn);
            if (iter1 == connMap->end()) {
                connMap->insert(make_pair(conn, 1));
            }
            return iter->second;
        } else {
            shared_ptr<DefaultMQProducer> producer(new DefaultMQProducer(topic));
            producer->setNamesrvAddr(nameServerHost);
            producer->setGroupName(group);
            producer->setInstanceName(topic);
            producer->setSendMsgTimeout(500);
            producer->setTcpTransportTryLockTimeout(1000);
            producer->setTcpTransportConnectTimeout(400);
            producer->setSessionCredentials(accessKey, secretKey, accessChannel);
            try {
                producer->start();
                producers.insert(pair<string, shared_ptr<DefaultMQProducer>>(key, producer));
                shared_ptr<std::map<shared_ptr<WsServer::Connection>, int>> connMap(new std::map<shared_ptr<WsServer::Connection>, int>);
                connMap->insert(make_pair(conn, 1));
                producerConnectionMap.insert(make_pair(producer, connMap));
                return producer;
            } catch (exception &e) {
                cout << e.what() << endl;
                return NULL;
            }
        }
    }

    shared_ptr<ProxyPushConsumer> getConsumer(const string &topic, const string &group, shared_ptr<WsServer::Connection> &conn) {
        auto key = topic + group;
        auto iter = consumers.find(key);
        if (iter != consumers.end()) {
            shared_ptr<ConsumerConnectionUnit> unit(new ConsumerConnectionUnit);
            if (consumerConnUnit.try_get(iter->second, unit)) {
                {
                    std::unique_lock<std::mutex> lck(unit->mtx);
                    auto iterConn = unit->conn.find(conn);
                    if (iterConn == unit->conn.end()) {
                        unit->conn.insert(make_pair(conn, 1));
                    }
                }
            }
            return iter->second;
        } else {
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
            consumer->setSessionCredentials(accessKey, secretKey, accessChannel);
            consumer->log = &log;
            ConsumerMsgListener *listener = new ConsumerMsgListener();
            listener->setConsumer(consumer);
            consumer->registerMessageListener(listener);
            consumer->uniqKey = key;
            try {
                consumer->start();
                cout << "connected to "<< nameServerHost<< " topic is " << topic << endl;
                consumers.insert(pair<string, shared_ptr<ProxyPushConsumer>>(key, consumer));
                shared_ptr<ConsumerConnectionUnit> unit(new ConsumerConnectionUnit);
                unit->conn.insert(make_pair(conn, 1));
                consumerConnUnit.insert(consumer, unit);
                return consumer;
            } catch (MQClientException &e) {
                cout << e << endl;
                return NULL;
            }
        }
    }
};

#endif //ROCKETMQ_PROXY_WORKERPOOL_HPP
