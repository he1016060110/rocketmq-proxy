//
// Created by hexi on 2020/4/4.
//

#include "ConsumeAckCallData.h"

void ConsumeAckCallData::process() {
  new ConsumeAckCallData(service_, cq_);
  msgId = request_.msg_id();
  group = request_.consumer_group();
  topic = request_.topic();
  msgWorker->resetConsumerActive(request_.topic(), request_.consumer_group());
  auto consumer = msgWorker->getConsumer(topic, group);
  if (consumer->setMsgAck(msgId, (ConsumeStatus) request_.status())) {
    shared_ptr<ConsumeMsgUnit> unit;
    if (msgWorker->idUnitMap.try_get(msgId, unit)) {
      unit->status = CONSUME_ACK;
    }
    reply_.set_code(0);
#ifdef DEBUG
    cout <<"ConsumeAckCallData [" << msgId << "]" << endl;
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
  //todo
  GPR_ASSERT(status_ == FINISH);
  delete this;
}

void ConsumeAckCallData::cancel()
{
  status_ = FINISH;
  del();
}