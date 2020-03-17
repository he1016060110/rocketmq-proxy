//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP
#define ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP

#include "common.hpp"

class ProxyPushConsumer : public DefaultMQPushConsumer {
public:
    string uniqKey;
    bool toDelete;
    ProxyLogger * log;
    QueueTS<shared_ptr<WsServer::Connection>> queue;
    map<shared_ptr<WsServer::Connection>, shared_ptr<ConnectionUnit> > *connectionUnit;
    MapTS<string, MsgConsumeUnit *> *consumerUnitMap;
    void initResource(map<shared_ptr<WsServer::Connection>, shared_ptr<ConnectionUnit> > *unit_,
                      MapTS<string, MsgConsumeUnit *> *consumerUnitMap_) {
        connectionUnit = unit_;
        consumerUnitMap = consumerUnitMap_;
    }

    ProxyPushConsumer(const std::string &groupname) : DefaultMQPushConsumer(groupname) {
        toDelete = false;
    }
};

#endif //ROCKETMQ_PROXY_PROXYPUSHCONSUMER_HPP
