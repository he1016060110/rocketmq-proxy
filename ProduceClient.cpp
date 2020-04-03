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

#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "Proxy.pb.h"
#include "Proxy.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using Proxy::ProduceRequest;
using Proxy::ProduceReply;
using Proxy::ProxyServer;

using namespace std;

class ProduceClient {
 public:
  ProduceClient(std::shared_ptr<Channel> channel)
      : stub_(ProxyServer::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string Produce(const std::string& topic, string &group , string &tag, string &body) {
    // Data we are sending to the server.
    ProduceRequest request;
    request.set_topic(topic);
    request.set_group(group);
    request.set_tag(tag);
    request.set_body(body);
    // Container for the data we expect from the server.
    ProduceReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Produce(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.msg_id();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<ProxyServer::Stub> stub_;
};

int main(int argc, char** argv) {
  ProduceClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string topic("test-topic");
  std::string group("test-topic");
  std::string tag("*");
  std::string body("this is test!");
  std::string reply = client.Produce(topic, group, tag, body);
  std::cout << "received: " << reply << std::endl;

  return 0;
}
