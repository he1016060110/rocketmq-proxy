//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_PRODUCE_CALL_DATA_H
#define ROCKETMQ_PROXY_PRODUCE_CALL_DATA_H

#include "ProducerCallback.h"
#include "CallData.h"
#include "MsgWorker.h"

class ProduceCallData : public CallDataBase {
public:
    ProduceCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_PRODUCE), responder_(&ctx_) {
      Proceed();
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestProduce(&ctx_, &request_, &responder_, cq_, cq_,
                               this);
    }

    void process() override {
      new ProduceCallData(service_, cq_);
      auto callback = new ProducerCallback();

      if (!request_.topic().size() || !request_.group().size() || !request_.tag().size() || !request_.body().size()) {
        reply_.set_code(1);
        reply_.set_err_msg("params error!");
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        return;
      }

      //必须拷贝一份，不用引用
      callback->successFunc = [this](const string &msgId) {
          reply_.set_code(0);
          reply_.set_msg_id(msgId);
          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, this);
      };
      //必须拷贝一份，不用引用
      callback->failureFunc = [this](const string &msgResp) {
          reply_.set_code(1);
          reply_.set_err_msg(msgResp);
          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, this);
      };
      msgWorker->produce(callback, request_.topic(), request_.group(), request_.tag(), request_.body());
    }

    ProduceRequest request_;
    ProduceReply reply_;
    ServerAsyncResponseWriter<ProduceReply> responder_;
};

#endif //ROCKETMQ_PROXY_PRODUCE_CALL_DATA_H
