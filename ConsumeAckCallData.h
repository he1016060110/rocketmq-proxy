//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H

#include "CallData.h"
#include "MsgWorker.h"

class ConsumeAckCallData : public CallDataBase {
public:
    ConsumeAckCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME_ACK), responder_(&ctx_) {
      Proceed();
    }

    void cancel() override;

private:
    void del() override;

    void create() override {
      status_ = PROCESS;
      service_->RequestConsumeAck(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
    }

    void process() override;


    ConsumeAckRequest request_;
    ConsumeAckReply reply_;
    ServerAsyncResponseWriter<ConsumeAckReply> responder_;
};

#endif //ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
