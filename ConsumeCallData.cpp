//
// Created by hexi on 2020/4/4.
//

#include "ConsumeCallData.h"
#include "MsgWorker.h"

void ConsumeCallData::process() {
    new ConsumeCallData(service_, cq_);
    shared_ptr<ConsumeMsgUnit> unit(new ConsumeMsgUnit(this, request_.topic(), request_.consumer_group()));
    {
      std::unique_lock<std::mutex> lk(msgWorker->notifyMtx);
      msgWorker->consumeMsgPool.push(unit);
      //通知有消息
      msgWorker->notifyCV.notify_all();
    }
}

bool ConsumeCallData::responseMsg(int code, string errMsg, string msgId, string body){
  reply_.set_msg_id(msgId);
  reply_.set_body(body);
  reply_.set_code(code);
  reply_.set_error_msg(errMsg);
  status_ = FINISH;
  responder_.Finish(reply_, Status::OK, this);
}

void ConsumeCallData::del() {
  msgWorker->idUnitMap.erase(reply_.msg_id());
  GPR_ASSERT(status_ == FINISH);
  delete this;
}