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
    enum MsgWorkerConsumeStatus {
        PROXY_CONSUME,
        CLIENT_RECEIVE,
        CONSUME_ACK
    };

    class CallDataBase;

    class MsgWorker {
        string nameServerHost;
        string accessKey;
        string secretKey;
        string accessChannel;

        class ProducerUnit {
        public:
            ProducerUnit(string topic) : producer(DefaultMQProducer(topic)), lastActiveAt(time(0)) {};
            DefaultMQProducer producer;
            time_t lastActiveAt;
        };

        class ConsumerUnit {
        public:
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
            producerUnit->producer.setNamesrvAddr(nameServerHost);
            producerUnit->producer.setGroupName(group);
            producerUnit->producer.setInstanceName(topic);
            producerUnit->producer.setSendMsgTimeout(500);
            producerUnit->producer.setTcpTransportTryLockTimeout(1000);
            producerUnit->producer.setTcpTransportConnectTimeout(400);
            producerUnit->producer.setSessionCredentials(accessKey, secretKey, accessChannel);
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

    public:
        void produce(string &topic, string &group, string &tag, string &body, int delayLevel = 0) {
          rocketmq::MQMessage msg(topic, tag, body);
          msg.setDelayTimeLevel(delayLevel);
          auto producerUnit = getProducer(topic, group);
          auto callback = new ProducerCallback();
          //不能传引用，因为都是临时变量
          callback->successFunc = [=](const string &msgId) {
              //wp.log.writeLog(ROCKETMQ_PROXY_LOG_TYPE_PRODUCER, msgId, topic, group, body, delayLevel);
          };
          callback->failureFunc = [=](const string &msgResp) {

          };
          producerUnit->producer.send(msg, callback);
        }
    };


    class CallDataBase {
    public:
        CallDataBase(ProxyServer::AsyncService *service, ServerCompletionQueue *cq)
            : service_(service), cq_(cq), status_(CREATE) {
          Proceed();
        }

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

    protected:
        ProxyServer::AsyncService *service_;
        ServerCompletionQueue *cq_;
        ServerContext ctx_;
        enum CallStatus {
            CREATE, PROCESS, FINISH
        };
        CallStatus status_;
    };

    class ProduceCallData : CallDataBase {
    public:
        ProduceCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
            service, cq), responder_(&ctx_) {
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
          std::string prefix("Produce ");
          reply_.set_msg_id(prefix + request_.topic());

          status_ = FINISH;
          responder_.Finish(reply_, Status::OK, this);
        }

        ProduceRequest request_;
        ProduceReply reply_;
        ServerAsyncResponseWriter<ProduceReply> responder_;
    };

    class ConsumeCallData : CallDataBase {
    public:
        ConsumeCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
            service, cq), responder_(&ctx_) {
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

    class ConsumeAckCallData : CallDataBase {
    public:
        ConsumeAckCallData(ProxyServer::AsyncService *service, ServerCompletionQueue *cq) : CallDataBase(
            service, cq), responder_(&ctx_) {
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

    void HandleRpcs() {
      new ProduceCallData(&service_, cq_.get());
      new ConsumeCallData(&service_, cq_.get());
      new ConsumeAckCallData(&service_, cq_.get());
      void *tag;
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
