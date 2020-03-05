//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_PRODUCERCALLBACK_H
#define ROCKETMQ_PROXY_PRODUCERCALLBACK_H

#include "common.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

class ProducerCallback : public AutoDeleteSendCallBack {
    shared_ptr<WsServer::Connection> conn;
    virtual ~ProducerCallback() {}
    virtual void onSuccess(SendResult &sendResult) {
        ptree data;
        data.put("msgId", sendResult.getMsgId());
        RESPONSE_SUCCESS(this->conn, data);
    }
    virtual void onException(MQException &e) {
        RESPONSE_ERROR(this->conn, 1, e.what());
    }
public:
    void setConn(shared_ptr<WsServer::Connection> &con) {
        this->conn = con;
    }
};

#endif //ROCKETMQ_PROXY_PRODUCERCALLBACK_H
