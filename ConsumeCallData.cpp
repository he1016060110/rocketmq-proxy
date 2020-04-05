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