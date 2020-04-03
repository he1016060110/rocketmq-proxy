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
    class CallData {
    public:
        CallData(ProxyServer::AsyncService* service, ServerCompletionQueue* cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
          Proceed();
        }

        void Proceed() {
          if (status_ == CREATE) {
            status_ = PROCESS;
            service_->RequestProduce(&ctx_, &request_, &responder_, cq_, cq_,
                                      this);
          } else if (status_ == PROCESS) {
            new CallData(service_, cq_);
            std::string prefix("Hello ");
            reply_.set_msg_id(prefix + request_.topic());

            status_ = FINISH;
            responder_.Finish(reply_, Status::OK, this);
          } else {
            GPR_ASSERT(status_ == FINISH);
            delete this;
          }
        }

    private:
        ProxyServer::AsyncService* service_;
        ServerCompletionQueue* cq_;
        ServerContext ctx_;
        ProduceRequest request_;
        ProduceReply reply_;
        ServerAsyncResponseWriter<ProduceReply> responder_;
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_;  // The current serving state.
    };

    void HandleRpcs() {
      new CallData(&service_, cq_.get());
      void* tag;  // uniquely identifies a request.
      bool ok;
      while (true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        static_cast<CallData*>(tag)->Proceed();
      }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    ProxyServer::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
  ServerImpl server;
  server.Run();

  return 0;
}
