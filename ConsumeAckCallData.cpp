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
    {
      std::unique_lock<std::mutex> lk(matchUnit->mtx);
      matchUnit->status = MSG_CONSUME_ACK;
      matchUnit->consumeStatus = (ConsumeStatus) request_.status();
    }
    matchUnit->cv.notify_all();
    shared_ptr<ConsumeMsgUnit> unit;
    if (msgWorker->idUnitMap.try_get(msgId, unit)) {
      unit->status = CONSUME_ACK;
    }
    reply_.set_code(0);
#ifdef DEBUG
    cout <<"ConsumeAckCallData [" << msgId << "] unit address[" << matchUnit.get() << "]" << endl;
#endif
    reply_.set_error_msg("msg ack succ!");
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    status_ = FINISH;
    reply_.set_code(1);
    string errMsg = "msg[" + msgId + "] cannot be found!";
    reply_.set_error_msg(errMsg);
    responder_.Finish(reply_, Status::OK, this);
  }
}

void ConsumeAckCallData::del() {
  msgWorker->idUnitMap.erase(request_.msg_id());
  msgWorker->MsgMatchUnits.erase(request_.msg_id());
  GPR_ASSERT(status_ == FINISH);
  delete this;
}

void ConsumeAckCallData::cancel()
{
  string key = request_.topic() + request_.consumer_group();
  msgWorker->pushConsumerShutdown(key);
  status_ = FINISH;
  del();
}