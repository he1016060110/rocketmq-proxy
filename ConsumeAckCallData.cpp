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
    reply_.set_code(RESPONSE_SUCCESS);
#ifdef DEBUG
    cout <<"ConsumeAckCallData [" << msgId << "]" << endl;
#endif
    msgWorker->writeLog(PROXY_LOGGER_TYPE_CONSUME_ACL, msgId, topic, group,
             "", 0,request_.status());
    reply_.set_error_msg("msg ack succ!");
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    status_ = FINISH;
    reply_.set_code(RESPONSE_ERROR);
    string errMsg = "msg[" + msgId + "] cannot be found!";
    reply_.set_error_msg(errMsg);
    responder_.Finish(reply_, Status::OK, this);
  }
}

void ConsumeAckCallData::del() {
  msgWorker->idUnitMap.erase(msgId);
  GPR_ASSERT(status_ == FINISH);
  delete this;
}

void ConsumeAckCallData::cancel()
{
  status_ = FINISH;
  del();
}