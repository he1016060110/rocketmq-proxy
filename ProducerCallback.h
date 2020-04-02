//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_PRODUCERCALLBACK_H
#define ROCKETMQ_PROXY_PRODUCERCALLBACK_H

#include "common.hpp"

class ProducerCallback : public AutoDeleteSendCallBack {
    virtual ~ProducerCallback() {}
    virtual void onSuccess(SendResult &sendResult) {
        successFunc(sendResult.getMsgId());
    }
    virtual void onException(MQException &e) {
        failureFunc(e.what());
    }
public:
    std::function<void(string)> successFunc;
    std::function<void(string)> failureFunc;
};

#endif //ROCKETMQ_PROXY_PRODUCERCALLBACK_H
