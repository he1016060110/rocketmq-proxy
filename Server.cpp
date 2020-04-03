/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "Proxy.pb.h"
#include "Proxy.grpc.pb.h"
#include "Const.hpp"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using Proxy::ProduceRequest;
using Proxy::ProduceReply;
using Proxy::ProxyServer;


class ServerImpl final {
public:
    ~ServerImpl() {
      server_->Shutdown();
      cq_->Shutdown();
    }

    void Run() {
      std::string server_address("0.0.0.0:50051");

      ServerBuilder builder;
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&service_);
      cq_ = builder.AddCompletionQueue();
      server_ = builder.BuildAndStart();
      std::cout << "Server listening on " << server_address << std::endl;

      HandleRpcs();
    }

private:
    enum RequestType {
        PRODUCE,
        CONSUME,
        CONSUME_ACK
    };

    class CallDataBase {
    public:
        CallDataBase(ProxyServer::AsyncService *service, ServerCompletionQueue *cq, RequestType type)
            : service_(service), cq_(cq), status_(CREATE), type_(type) {
          Proceed();
        }
        virtual void create();
        virtual void process();
        virtual void del();
        void Proceed() {
          if (status_ == CREATE) {
            status_ = PROCESS;
            create();
          } else if (status_ == PROCESS) {
            process();
          } else {
            del();
          }
        }

    protected:
        ProxyServer::AsyncService *service_;
        ServerCompletionQueue *cq_;
        ServerContext ctx_;
        enum CallStatus {
            CREATE, PROCESS, FINISH
        };
        CallStatus status_;
        RequestType type_;
    };

    class ProduceCallData : CallDataBase {
    public:
        ProduceCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq, RequestType type) : CallDataBase(
            service, cq, type), responder_(&ctx_) {
          Proceed();
          CallDataBase(service, cq, type);
        }

    private:
        void del()
        {
          GPR_ASSERT(status_ == FINISH);
          delete this;
        }
        void create() {
          service_->RequestProduce(&ctx_, &request_, &responder_, cq_, cq_,
                                   this);
        }

        void process() {
          new ProduceCallData(service_, cq_, type_);
          std::string prefix("Hello ");
          reply_.set_msg_id(prefix + request_.topic());

          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, this);
        }

        ProduceRequest request_;
        ProduceReply reply_;
        ServerAsyncResponseWriter<ProduceReply> responder_;
    };

    void HandleRpcs() {
      new ProduceCallData(&service_, cq_.get(), PRODUCE);
      //new CallData(&service_, cq_.get(), CONSUME);
      //new CallData(&service_, cq_.get(), CONSUME_ACK);
      void *tag;  // uniquely identifies a request.
      bool ok;
      while (true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<CallDataBase *>(tag)->Proceed();
      }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    ProxyServer::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main(int argc, char **argv) {
  ServerImpl server;
  server.Run();

  return 0;
}
