//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H

#include "CallData.h"
#include "MsgWorker.h"

class ConsumeMsgUnit;
class MsgMatchUnit;

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
      auto msg_id = request_.msg_id();
      shared_ptr<MsgMatchUnit> matchUnit;
      if (msgWorker->MsgMatchUnits.try_get(matchUnit)) {
        matchUnit->status = MSG_CONSUME_ACK;
        matchUnit->consumeStatus = (ConsumeStatus) request_.status();
        matchUnit->ackCallData = this;
        std::unique_lock<std::mutex> lk(matchUnit->mtx);
        matchUnit->cv.notify_one();
        reply_.set_code(0);
        reply_.set_error_msg("msg ack succ!");
        responder_.Finish(reply_, Status::OK, this);
      } else {
        status_ = FINISH;
        reply_.set_code(1);
        reply_.set_error_msg("msg cannot be found!");
        responder_.Finish(reply_, Status::OK, this);
      }
    }

    ConsumeAckRequest request_;
    ConsumeAckReply reply_;
    ServerAsyncResponseWriter<ConsumeAckReply> responder_;
};

#endif //ROCKETMQ_PROXY_CONSUME_ACK_CALL_DATA_H
