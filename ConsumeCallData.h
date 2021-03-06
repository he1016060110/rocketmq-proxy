//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_CALL_DATA_H

#include "CallData.h"

class ConsumeCallData : public CallDataBase {
public:
    ConsumeCallData(RMQProxy::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME), responder_(&ctx_), msgId(""), topic(""), group("") {
      Proceed();
    }

    void cancel() override;

    void responseMsg(int code, string errMsg, string msgId, string body);

    void responseTimeOut() {
      reply_.set_code(RESPONSE_TIMEOUT);
      string errMsg = "get msg timeout";
      reply_.set_error_msg(errMsg);
      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    }

private:
    void del() override;

    void create() override {
      status_ = PROCESS;
      service_->RequestConsume(&ctx_, &request_, &responder_, cq_, cq_,
                               this);
    }

    void process() override;

    ConsumeRequest request_;
    ConsumeReply reply_;
    ServerAsyncResponseWriter<ConsumeReply> responder_;
    string msgId;
    string topic;
    string group;
};


#endif //ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
