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
#include "DefaultMQProducer.h"
#include "DefaultMQPushConsumer.h"
#include "ProducerCallback.h"
#include "Arg_helper.h"
#include <unistd.h>
#include <fstream>

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

#define MAX_PRODUCE_INACTIVE_TIME 100

enum MsgWorkerConsumeStatus {
    PROXY_CONSUME,
    CLIENT_RECEIVE,
    CONSUME_ACK
};

enum RequestType{
    REQUEST_PRODUCE,
    REQUEST_CONSUME,
    REQUEST_CONSUME_ACK,
};
enum CallStatus {
    CREATE, PROCESS, FINISH
};

class CallDataBase;

class ConsumerMsgListener : public MessageListenerConcurrently {
public:
    ConsumerMsgListener() {}
    virtual ~ConsumerMsgListener() {}
    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs) {
      return RECONSUME_LATER;
    }
};

class MsgWorker {
    string nameServerHost_;
    string accessKey_;
    string secretKey_;
    string accessChannel_;

    class ProducerUnit {
    public:
        ProducerUnit(string topic) : producer(DefaultMQProducer(topic)), lastActiveAt(time(0)) {};
        DefaultMQProducer producer;
        time_t lastActiveAt;
    };

    class ConsumerUnit {
    public:
        ConsumerUnit(string topic) : consumer(DefaultMQPushConsumer(topic)), lastActiveAt(time(0)) {};
        DefaultMQPushConsumer consumer;
        time_t lastActiveAt;
    };

    class ConsumeMsgUnit {
    public:
        CallDataBase *call_data;
        time_t consumeByProxyAt;
        time_t sendClientAt;
        time_t ackAt;
        MsgWorkerConsumeStatus status;
    };

    std::map<string, shared_ptr<ProducerUnit>> producers;
    std::map<string, shared_ptr<ConsumerUnit>> consumers;
    std::map<string, shared_ptr<ConsumeMsgUnit>> msgs;

    shared_ptr<ProducerUnit> getProducer(const string &topic, const string &group) {
      auto key = topic + group;
      auto iter = producers.find(key);
      if (iter != producers.end()) {
        return iter->second;
      } else {
        shared_ptr<ProducerUnit> producerUnit(new ProducerUnit(topic));
        producerUnit->producer.setNamesrvAddr(nameServerHost_);
        producerUnit->producer.setGroupName(group);
        producerUnit->producer.setInstanceName(topic);
        producerUnit->producer.setSendMsgTimeout(500);
        producerUnit->producer.setTcpTransportTryLockTimeout(1000);
        producerUnit->producer.setTcpTransportConnectTimeout(400);
        producerUnit->producer.setSessionCredentials(accessKey_, secretKey_, accessChannel_);
        try {
          producerUnit->producer.start();
          producers.insert(pair<string, shared_ptr<ProducerUnit>>(key, producerUnit));
          return producerUnit;
        } catch (exception &e) {
          cout << e.what() << endl;
          return nullptr;
        }
      }
    }
    shared_ptr<ConsumerUnit> getConsumer(const string &topic, const string &group) {
      auto key = topic + group;
      auto iter = consumers.find(key);
      if (iter != consumers.end()) {
        return iter->second;
      } else {
        shared_ptr<ConsumerUnit> consumerUnit(new ConsumerUnit(group));
        consumerUnit->consumer.setNamesrvAddr(nameServerHost_);
        consumerUnit->consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
        consumerUnit->consumer.setInstanceName(group);
        consumerUnit->consumer.subscribe(topic, "*");
        consumerUnit->consumer.setConsumeThreadCount(3);
        consumerUnit->consumer.setTcpTransportTryLockTimeout(1000);
        consumerUnit->consumer.setTcpTransportConnectTimeout(400);
        consumerUnit->consumer.setSessionCredentials(accessKey_, secretKey_, accessChannel_);
        ConsumerMsgListener *listener = new ConsumerMsgListener();
        consumerUnit->consumer.registerMessageListener(listener);
        try {
          consumerUnit->consumer.start();
          cout << "connected to "<< nameServerHost_<< " topic is " << topic << endl;
          consumers.insert(pair<string, shared_ptr<ConsumerUnit>>(key, consumerUnit));
          return consumerUnit;
        } catch (MQClientException &e) {
          cout << e << endl;
          return nullptr;
        }
      }
    }
public:
    void produce(ProducerCallback *callback, const string &topic, const string &group,
                 const string &tag, const string &body, const int delayLevel = 0) {
      rocketmq::MQMessage msg(topic, tag, body);
      msg.setDelayTimeLevel(delayLevel);
      auto producerUnit = getProducer(topic, group);
      producerUnit->producer.send(msg, callback);
    }
    void setConfig(string &nameServer, string &accessKey, string & secretKey, string channel)
    {
      nameServerHost_ = nameServer;
      accessKey_ = accessKey;
      secretKey_ = secretKey;
      accessChannel_ = channel;
    }
};


class CallDataBase {
public:
    CallDataBase(ProxyServer::AsyncService *service, ServerCompletionQueue *cq, RequestType type)
        : service_(service), cq_(cq), status_(CREATE) {
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

    RequestType getType()
    {
      return type_;
    }

protected:
    ProxyServer::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    CallStatus status_;
    RequestType type_;
};


class ProduceCallData : public CallDataBase {
public:
    ProduceCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_PRODUCE), responder_(&ctx_) {
      Proceed();
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestProduce(&ctx_, &request_, &responder_, cq_, cq_,
                               this);
    }

    void process() override {
      new ProduceCallData(service_, cq_);
      auto that = this;
      auto callback = new ProducerCallback();

      if(!request_.topic().size() || !request_.group().size()  || !request_.tag().size()  || !request_.body().size() ) {
        reply_.set_code(1);
        reply_.set_err_msg("params error!");
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, that);
        return;
      }

      callback->successFunc = [&](const string &msgId) {
          reply_.set_code(0);
          reply_.set_msg_id(msgId);
          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, that);
      };
      callback->failureFunc = [&](const string &msgResp) {
          reply_.set_code(1);
          reply_.set_err_msg(msgResp);
          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, that);
      };
      msgWorker->produce(callback, request_.topic(), request_.group(), request_.tag(), request_.body());
    }

    ProduceRequest request_;
    ProduceReply reply_;
    ServerAsyncResponseWriter<ProduceReply> responder_;
};

class ConsumeCallData : public CallDataBase {
public:
    ConsumeCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME), responder_(&ctx_) {
      Proceed();
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestConsume(&ctx_, &request_, &responder_, cq_, cq_,
                               this);
    }

    void process() override {
      new ConsumeCallData(service_, cq_);
      std::string prefix("Consume ");
      reply_.set_msg_id(prefix + request_.topic());

      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    }

    ConsumeRequest request_;
    ConsumeReply reply_;
    ServerAsyncResponseWriter<ConsumeReply> responder_;
};

class ConsumeAckCallData : public CallDataBase {
public:
    ConsumeAckCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
        service, cq, REQUEST_CONSUME_ACK), responder_(&ctx_) {
      Proceed();
    }

private:
    void del() override {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }

    void create() override {
      status_ = PROCESS;
      service_->RequestConsumeAck(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
    }

    void process() override {
      new ConsumeAckCallData(service_, cq_);
      std::string prefix("ConsumeAck ");
      reply_.set_msg_id(prefix + request_.topic());

      status_ = FINISH;
      responder_.Finish(reply_, Status::OK, this);
    }

    ConsumeAckRequest request_;
    ConsumeAckReply reply_;
    ServerAsyncResponseWriter<ConsumeAckReply> responder_;
};

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
      string address = host_ + ":" +to_string(port_);
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
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        switch (static_cast<CallDataBase *>(tag)->getType()) {
          case REQUEST_PRODUCE:
            static_cast<ProduceCallData *>(tag)->Proceed();
            break;
          case REQUEST_CONSUME:
            static_cast<ConsumeCallData *>(tag)->Proceed();
            break;
          case REQUEST_CONSUME_ACK:
            static_cast<ConsumeAckCallData *>(tag)->Proceed();
            break;
        }
      }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    ProxyServer::AsyncService service_;
    std::unique_ptr<Server> server_;
};

MsgWorker *CallDataBase::msgWorker = new MsgWorker();

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
