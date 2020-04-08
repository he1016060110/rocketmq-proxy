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

void ConsumeCallData::responseMsg(int code, string errMsg, string msgId, string body){
  reply_.set_msg_id(msgId);
  reply_.set_body(body);
  reply_.set_code(code);
  reply_.set_error_msg(errMsg);
  status_ = FINISH;
  this->msgId = msgId;
  responder_.Finish(reply_, Status::OK, this);
}

void ConsumeCallData::del() {
  GPR_ASSERT(status_ == FINISH);
  delete this;
}

void ConsumeCallData::cancel() {
  shared_ptr<MsgMatchUnit> matchUnit;
  if (msgId.size() && msgWorker->MsgMatchUnits.try_get(msgId, matchUnit)) {
    std::unique_lock<std::mutex> lk(matchUnit->mtx);
    matchUnit->status = MSG_CONSUME_ACK;
    matchUnit->consumeStatus = RECONSUME_LATER;
    matchUnit->cv.notify_one();
  }
  if (msgId.size()) {
    msgWorker->idUnitMap.erase(msgId);
    msgWorker->MsgMatchUnits.erase(msgId);
  }
  status_ = FINISH;
  Proceed();
}