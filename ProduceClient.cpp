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
using Proxy::RMQProxy;

using namespace std;

class ProduceClient {
public:
    ProduceClient(std::shared_ptr<Channel> channel)
        : stub_(RMQProxy::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::string Produce(const std::string &topic, string &group, string &tag, string &body) {
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
        if (reply.code()) {
          cout << reply.err_msg() << endl;
        } else {
          return reply.msg_id();
        }
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        exit(1);
      }
    }

private:
    std::unique_ptr<RMQProxy::Stub> stub_;
};

int main(int argc, char **argv) {
  if (argc < 4) {
    cout << "输入host topic body num！" << endl;
    exit(1);
  }
  string host(*(argv + 1));
  string topic(*(argv + 2));
  string body(*(argv + 3));
  int num = 1;
  if (argc > 4) {
    std::string::size_type sz;
    string num_str(*(argv + 4));
    num = std::stoi(num_str, &sz);
  }
  ProduceClient client(grpc::CreateChannel(
      host, grpc::InsecureChannelCredentials()));
  std::string tag("*");
  for (int i = 0; i < num; i++) {
    std::string reply = client.Produce(topic, topic, tag, body);
    std::cout << "received: " << reply << std::endl;
  }

  return 0;
}
