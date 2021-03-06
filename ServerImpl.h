//
// Created by hexi on 2020/4/4.
//

#ifndef ROCKETMQ_PROXY_SERVERIMPL_H
#define ROCKETMQ_PROXY_SERVERIMPL_H

#include <string>
#include "CallData.h"
#include "ProduceCallData.h"
#include "ConsumeCallData.h"
#include "ConsumeAckCallData.h"
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
using Proxy::RMQProxy;

class ServerImpl final {
public:
    ~ServerImpl() {
      server_->Shutdown();
      cq_->Shutdown();
    }

    ServerImpl(string host, int port, string nameServer, string accessKey, string secretKey, string accessChannel) :
        host_(host), port_(port), nameServerHost_(nameServer), accessKey_(accessKey), secretKey_(secretKey),
        accessChannel_(accessChannel) {

    };
    string host_;
    int port_;
    string nameServerHost_;
    string accessKey_;
    string secretKey_;
    string accessChannel_;

    void Run() {
      string address = host_ + ":" + to_string(port_);
      std::string server_address(address);
      ServerBuilder builder;
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&service_);
      cq_ = builder.AddCompletionQueue();
      server_ = builder.BuildAndStart();
      std::cout << "Server listening on " << server_address << std::endl;

      HandleRpcs();
    }

private:
    void HandleRpcs() {
      new ProduceCallData(&service_, cq_.get());
      new ConsumeCallData(&service_, cq_.get());
      new ConsumeAckCallData(&service_, cq_.get());
      void *tag;
      bool ok;
      while (true) {
        if(!cq_->Next(&tag, &ok)) {
          cout << "Next failed!" <<endl;
          std::this_thread::sleep_for(std::chrono::seconds(1));
          continue;
        }
        //如果不ok，是客户端取消了，或者网络不通这些原因，应该取消
        switch (static_cast<CallDataBase *>(tag)->getType()) {
          case REQUEST_PRODUCE:
            if (ok) {
              static_cast<ProduceCallData *>(tag)->Proceed();
            } else {
              static_cast<ProduceCallData *>(tag)->cancel();
            }
            break;
          case REQUEST_CONSUME:
            if (ok) {
              static_cast<ConsumeCallData *>(tag)->Proceed();
            } else {
              static_cast<ConsumeCallData *>(tag)->cancel();
            }
            break;
          case REQUEST_CONSUME_ACK:
            if (ok) {
              static_cast<ConsumeAckCallData *>(tag)->Proceed();
            } else {
              static_cast<ConsumeAckCallData *>(tag)->cancel();
            }
            break;
        }
      }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    RMQProxy::AsyncService service_;
    std::unique_ptr<Server> server_;
};

#endif //ROCKETMQ_PROXY_SERVERIMPL_H
