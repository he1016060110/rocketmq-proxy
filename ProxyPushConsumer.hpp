//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP
#define ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP

#include "common.hpp"

class ProxyPushConsumer : public DefaultMQPushConsumer {
public:
    QueueTS<shared_ptr<WsServer::Connection>> queue;
    //被锁住的消息列表
    map<shared_ptr<WsServer::Connection>, shared_ptr<map<string, int>> > *pool;
    MapTS<string, MsgConsumeUnit *> *consumerUnitMap;
    void initResource(map<shared_ptr<WsServer::Connection>, shared_ptr<map<string, int>> > *pool_,
                      MapTS<string, MsgConsumeUnit *> *consumerUnitMap_) {
        pool = pool_;
        consumerUnitMap = consumerUnitMap_;
    }

    ProxyPushConsumer(const std::string &groupname) : DefaultMQPushConsumer(groupname) {
    }
};

#endif //ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP
