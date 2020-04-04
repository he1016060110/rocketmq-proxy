//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_CALL_DATA_H
#define ROCKETMQ_PROXY_CALL_DATA_H

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "Proxy.pb.h"
#include "Proxy.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using Proxy::ProduceRequest;
using Proxy::ProduceReply;
using Proxy::ConsumeRequest;
using Proxy::ConsumeReply;
using Proxy::ConsumeAckRequest;
using Proxy::ConsumeAckReply;
using Proxy::ProxyServer;

enum RequestType {
    REQUEST_PRODUCE,
    REQUEST_CONSUME,
    REQUEST_CONSUME_ACK,
};

enum CallStatus {
    CREATE, PROCESS, FINISH
};
class MsgWorker;
class CallDataBase {
public:
    CallDataBase(ProxyServer::AsyncService *service, ServerCompletionQueue *cq, RequestType type)
        : service_(service), cq_(cq), status_(CREATE), type_(type) {
      Proceed();
    }

    static MsgWorker *msgWorker;

    //virtual todo 为啥需要实现？？？
    virtual void create() {};

    virtual void process() {};

    virtual void del() {};

    void Proceed() {
      if (status_ == CREATE) {
        create();
      } else if (status_ == PROCESS) {
        process();
      } else {
        del();
      }
    }

    RequestType getType() {
      return type_;
    }

protected:
    ProxyServer::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    CallStatus status_;
    RequestType type_;
};

#endif //ROCKETMQ_PROXY_CALL_DATA_H