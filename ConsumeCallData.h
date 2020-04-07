//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_CALL_DATA_H

#include "CallData.h"

class ConsumeCallData : public CallDataBase {
public:
    ConsumeCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME), responder_(&ctx_) {
      Proceed();
    }

    void cancel() override ;

    bool responseMsg(int code, string errMsg, string msgId, string body);

    void responseTimeOut() {
      reply_.set_msg_id("");
      reply_.set_body("");
      reply_.set_code(1);
      reply_.set_error_msg("get msg timeout");
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
};


#endif //ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
