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
#include "Arg_helper.h"
#include <unistd.h>
#include <fstream>
#include "CallData.h"
#include "ServerImpl.h"

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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

using namespace std;
using namespace rocketmq;



int main(int argc, char *argv[]) {
  rocketmq::Arg_helper arg_help(argc, argv);
  string file = arg_help.get_option_value("-f");
  if (file.size() == 0 || access(file.c_str(), F_OK) == -1) {
    cout << "Server -f [file_name]" << endl;
    return 0;
  }
  string nameServer;
  string host;
  string accessKey;
  string secretKey;
  string esServer;
  string logFileName;
  int port;
  try {
    std::ifstream t(file);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string contents(buffer.str());
    std::istringstream jsonStream;
    jsonStream.str(contents);
    boost::property_tree::ptree jsonItem;
    boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
    nameServer = jsonItem.get<string>("nameServer");
    accessKey = jsonItem.get<string>("accessKey");
    secretKey = jsonItem.get<string>("secretKey");
    esServer = jsonItem.get<string>("esServer");
    host = jsonItem.get<string>("host");
    logFileName = jsonItem.get<string>("logFileName");
    port = jsonItem.get<int>("port");
  } catch (exception &e) {
    cout << e.what() << endl;
    return 0;
  }

  ServerImpl server(host, port, nameServer, accessKey, secretKey, "channel");
  CallDataBase::msgWorker->setConfig(nameServer, accessKey, secretKey, "channel");

  server.Run();

  return 0;
}
