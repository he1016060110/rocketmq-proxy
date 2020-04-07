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
#include <string>
#include <grpcpp/grpcpp.h>
#include "Proxy.pb.h"
#include "Proxy.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using Proxy::ConsumeAckRequest;
using Proxy::ConsumeRequest;
using Proxy::ConsumeAckReply;
using Proxy::ConsumeReply;
using Proxy::ProxyServer;
using namespace std;

class ConsumeClient {
public:
    ConsumeClient(std::shared_ptr<Channel> channel)
        : stub_(ProxyServer::NewStub(channel)), msgId("") {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    string msgId;

    void emptyMsgId() {
      msgId = "";
    }

    void Consume(const std::string &topic, const std::string &group) {
      // Data we are sending to the server.
      ConsumeRequest request;
      request.set_topic(topic);
      request.set_consumer_group(group);

      // Container for the data we expect from the server.
      ConsumeReply reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->Consume(&context, request, &reply);

      // Act upon its status.
      if (status.ok()) {
        if (!reply.code()) {
          msgId = reply.msg_id();
        } else {
          cout << "error!:" << reply.error_msg() << endl;
        }
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        exit(1);
      }
    }

    void ConsumeAck(const std::string &topic, const std::string &group) {
      // Data we are sending to the server.
      ConsumeAckRequest request;
      request.set_topic(topic);
      request.set_consumer_group(group);
      request.set_msg_id(msgId);

      // Container for the data we expect from the server.
      ConsumeAckReply reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->ConsumeAck(&context, request, &reply);

      // Act upon its status.
      if (status.ok()) {
        if (!reply.code()) {
          cout << msgId << " ack success!" << endl;
        } else {
          cout << reply.error_msg() << endl;
        }
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        exit(1);
      }
    }

private:
    std::unique_ptr<ProxyServer::Stub> stub_;
};

int main(int argc, char **argv) {
  ConsumeClient client(grpc::CreateChannel(
      "127.0.0.1:8090", grpc::InsecureChannelCredentials()));
  while (true) {
    client.Consume("test-topic", "test-topic");
    if (client.msgId.size()) {
      client.ConsumeAck("test-topic", "test-topic");
    }
    client.emptyMsgId();
  }

  return 0;
}
