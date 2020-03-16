//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_PRODUCERCALLBACK_H
#define ROCKETMQ_PROXY_PRODUCERCALLBACK_H

#include "common.hpp"

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

class ProducerCallback : public AutoDeleteSendCallBack {
    std::function<void(string)> successFunc;
    std::function<void(string)> failureFunc;
    virtual ~ProducerCallback() {}
    virtual void onSuccess(SendResult &sendResult) {
        successFunc(sendResult.getMsgId());
    }
    virtual void onException(MQException &e) {
        failureFunc(e.what());
    }
public:
    void setCallbackFunc(std::function<void(string)> &succ, std::function<void(string)> &failure)
    {
        successFunc = succ;
        failureFunc = failure;
    }
};

#endif //ROCKETMQ_PROXY_PRODUCERCALLBACK_H
