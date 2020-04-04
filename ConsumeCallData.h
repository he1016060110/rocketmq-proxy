//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
#define ROCKETMQ_PROXY_CONSUME_CALL_DATA_H


class ConsumeCallData : public CallDataBase {
public:
    ConsumeCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME), responder_(&ctx_) {
      Proceed();
    }

    bool responseMsg(int code, string errMsg, string &msgId, string &body)
    {
      reply_.set_msg_id(msgId);
      reply_.set_body(body);
      reply_.set_code(code);
      reply_.set_error_msg(errMsg);
      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestConsume(&ctx_, &request_, &responder_, cq_, cq_,
                               this);
    }

    void process() override {
      new ConsumeCallData(service_, cq_);
      shared_ptr<ConsumeMsgUnit> unit(new ConsumeMsgUnit(this, request_.topic(), request_.consumer_group()));
      msgWorker->consumeMsgPool.push_back(unit);
    }

    ConsumeRequest request_;
    ConsumeReply reply_;
    ServerAsyncResponseWriter<ConsumeReply> responder_;
};


#endif //ROCKETMQ_PROXY_CONSUME_CALL_DATA_H
