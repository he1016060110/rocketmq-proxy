//
// Created by hexi on 2020/4/4.
//

#include "ConsumeCallData.h"
#include "MsgWorker.h"

void ConsumeCallData::process() {
    new ConsumeCallData(service_, cq_);
    topic = request_.topic();
    group =  request_.consumer_group();
    shared_ptr<ConsumeMsgUnit> unit(new ConsumeMsgUnit(this, topic, group));
    {
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
  if (msgId.size()) {
    auto consumer = msgWorker->getConsumer(topic, group);
    consumer->setMsgReconsume(msgId);
    consumer->pushMsgBack(msgId);
    msgWorker->idUnitMap.erase(msgId);
  }

  status_ = FINISH;
  Proceed();
}