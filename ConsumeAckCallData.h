//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
#include "CallData.h"

class ConsumeAckCallData : public CallDataBase {
public:
    ConsumeAckCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME_ACK), responder_(&ctx_) {
      Proceed();
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestConsumeAck(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
    }

    void process() override {
      new ConsumeAckCallData(service_, cq_);
      std::string prefix("ConsumeAck ");
      reply_.set_msg_id(prefix + request_.topic());

      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    }

    ConsumeAckRequest request_;
    ConsumeAckReply reply_;
    ServerAsyncResponseWriter<ConsumeAckReply> responder_;
};

#endif //ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
