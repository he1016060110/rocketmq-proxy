//
// Created by hexi on 2020/4/4.
//

#include "ConsumeAckCallData.h"

void ConsumeAckCallData::process() {
  new ConsumeAckCallData(service_, cq_);
  auto msgId = request_.msg_id();
  shared_ptr<MsgMatchUnit> matchUnit;
  msgWorker->resetConsumerActive(request_.topic(), request_.consumer_group());
  if (msgWorker->MsgMatchUnits.try_get(msgId, matchUnit)) {
    matchUnit->status = MSG_CONSUME_ACK;
    matchUnit->consumeStatus = (ConsumeStatus) request_.status();
    shared_ptr<ConsumeMsgUnit> unit;
    if (msgWorker->idUnitMap.try_get(msgId, unit)) {
      unit->status = CONSUME_ACK;
    }
    std::unique_lock<std::mutex> lk(matchUnit->mtx);
    matchUnit->cv.notify_one();
    reply_.set_code(0);
    reply_.set_error_msg("msg ack succ!");
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    status_ = FINISH;
    reply_.set_code(1);
    reply_.set_error_msg("msg cannot be found!");
    responder_.Finish(reply_, Status::OK, this);
  }
}

void ConsumeAckCallData::del()
{
  msgWorker->idUnitMap.erase(request_.msg_id());
  msgWorker->MsgMatchUnits.erase(request_.msg_id());
  GPR_ASSERT(status_ == FINISH);
  delete this;
}