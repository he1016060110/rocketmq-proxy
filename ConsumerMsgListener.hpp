//
// Created by hexi on 2020/3/5.
//

#include "common.hpp"
#include "ProxyPushConsumer.hpp"
#include "stdio.h"

#ifndef ROCKETMQ_PROXY_CONSUMERMSGLISTENER_HPP
#define ROCKETMQ_PROXY_CONSUMERMSGLISTENER_HPP

using namespace std;
using namespace rocketmq;
using namespace boost::property_tree;

class ConsumerMsgListener : public MessageListenerConcurrently {
    shared_ptr<ProxyPushConsumer> consumer;
public:
    ConsumerMsgListener() {}

    virtual ~ConsumerMsgListener() {}

    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs) {
        if (msgs.size() != 1 ) {
            cout << "consumeMessage msg max size is "<< msgs.size() << "\n";
            return RECONSUME_LATER;
        }
        //如果是要删除consumer了，则不再堵塞消费
        shared_ptr<WsServer::Connection> conn;
        while(!consumer->queue.try_pop(conn)) {
            if (consumer->toDelete) {
                return RECONSUME_LATER;
            }
            //
            boost::this_thread::sleep(boost::posix_time::milliseconds (1));
        }
        //设置锁信息，客户端发送ack后解开锁
        string msgId = msgs[0].getMsgId();
        auto unit = new MsgConsumeUnit();
        consumer->consumerUnitMap->insert_or_update(msgId, unit);
        auto connectionUnit = (*(consumer->connectionUnit))[conn];
        {
            std::unique_lock<std::mutex> lck(connectionUnit->mtx);
            connectionUnit->msgPool->insert(make_pair(msgId, ROCKETMQ_PROXY_MSG_STATUS_SENT));
        }
        //必须大括号括起来，不然删掉了两个变量，但是lck却最后才释放
        {
            //先设置lock，然后再发送消息，顺序很重要
            std::unique_lock<std::mutex> lck(unit->mtx);
            ptree data;
            data.put("msgId", msgId);
            data.put("type", ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME);
            RESPONSE_SUCCESS(conn, data);
            consumer->log->writeLog(ROCKETMQ_PROXY_LOG_TYPE_CONSUMER, msgId, msgs[0].getTopic(),
                    consumer->getGroupName() , msgs[0].getBody(), msgs[0].getDelayTimeLevel(),
                    ROCKETMQ_PROXY_LOG_STATUS_CONSUMER_SEND);
            unit->syncStatus = ROCKETMQ_PROXY_MSG_STATUS_SYNC_SENT;
            //printf("%s lock before!\n", msgId.c_str());
            unit->cv.wait(lck);
            //printf("%s lock after!\n", msgId.c_str());
        }
        //唤醒后删除lock
        //lock被唤醒，删除lock，避免内存泄漏
        {
            std::unique_lock<std::mutex> lck(connectionUnit->mtx);
            connectionUnit->msgPool->erase(msgId);
        }
        ConsumeStatus status = unit->status;
        consumer->log->writeLog(ROCKETMQ_PROXY_LOG_TYPE_CONSUMER, msgId, msgs[0].getTopic(),
                                consumer->getGroupName() , msgs[0].getBody(), msgs[0].getDelayTimeLevel(),
                                unit->status == CONSUME_SUCCESS ? ROCKETMQ_PROXY_LOG_STATUS_CONSUMER_ACK_SUCCESS : ROCKETMQ_PROXY_LOG_STATUS_CONSUMER_ACK_LATER);
        //阻塞住，等待客户端消费掉消息，或者断掉连接
        consumer->consumerUnitMap->erase(msgId);
        delete unit;
        return status;
    }

    void setConsumer(shared_ptr<ProxyPushConsumer> con) {
        this->consumer = con;
    }
};

#endif //ROCKETMQ_PROXY_CONSUMERMSGLISTENER_HPP
