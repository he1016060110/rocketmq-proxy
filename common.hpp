//
// Created by hexi on 2020/3/5.
//

#ifndef ROCKETMQ_PROXY_COMMON_HPP
#define ROCKETMQ_PROXY_COMMON_HPP

#include "DefaultMQProducer.h"
#include "DefaultMQPushConsumer.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include "QueueTS.hpp"
#include "MapTS.hpp"
#include "Const.hpp"
#include "server_ws.hpp"
#include <future>
#include <boost/thread.hpp>

using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using namespace rocketmq;
using namespace boost::property_tree;

void getResponseJson(stringstream &ret, int code, string &msg, ptree &arr) {
    ptree root;
    root.put("code", code);
    root.put("msg", msg.c_str());
    root.add_child("data", arr);
    write_json(ret, root, false);
};

#define RESPONSE_ERROR(con_, code_, msg_) do { \
    int code = code_; \
    string msg = msg_; \
    ptree data; \
    stringstream ret; \
    getResponseJson(ret, code, msg, data); \
    con_->send(ret.str()); \
} while(0);

#define RESPONSE_ERROR_DATA(con_, code_, msg_, data_) do { \
    int code = code_; \
    string msg = msg_; \
    stringstream ret; \
    getResponseJson(ret, code, msg, data_); \
    con_->send(ret.str()); \
} while(0);


#define RESPONSE_SUCCESS(con_, data_) do { \
    int code = 0; \
    string msg = ""; \
    stringstream ret; \
    getResponseJson(ret, code, msg, data_); \
    con_->send(ret.str()); \
} while (0);

class MsgConsumeUnit
{
public:
    std::mutex mtx;
    std::condition_variable cv;
    ConsumeStatus status;
    int syncStatus;
    string msgId;
    MsgConsumeUnit(): syncStatus(ROCKETMQ_PROXY_MSG_STATUS_SYNC_INIT), status(RECONSUME_LATER) {}
};

class ConnectionUnit
{
public:
    std::mutex mtx;
    std::condition_variable cv;
    shared_ptr<map<string, int>> msgPool;
};

#endif //ROCKETMQ_PROXY_COMMON_HPP
