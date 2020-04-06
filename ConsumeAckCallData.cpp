//
// Created by hexi on 2020/4/4.
//

#include "ConsumeAckCallData.h"

void ConsumeAckCallData::process() {
  new ConsumeAckCallData(service_, cq_);
  auto msg_id = request_.msg_id();
  shared_ptr<MsgMatchUnit> matchUnit;
  if (msgWorker->MsgMatchUnits.try_get(msg_id, matchUnit)) {
    matchUnit->status = MSG_CONSUME_ACK;
    matchUnit->consumeStatus = (ConsumeStatus) request_.status();
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