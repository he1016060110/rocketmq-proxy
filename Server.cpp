#include "server_ws.hpp"
#include <future>
#include "DefaultMQProducer.h"
#include "DefaultMQPushConsumer.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "QueueTS.hpp"
#include "Const.hpp"
#include <stdio.h>

using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using namespace rocketmq;
using namespace boost::property_tree;

void getResponseJson(stringstream &ret, int code, string &msg, ptree &arr) {
    ptree root;
    root.put("code", code);
    root.put("msg", msg.c_str());
    root.add_child("data", arr);
    write_json(ret, root, false);
};

#define RESPONSE_ERROR(con_, code_, msg_) do { \
    int code = code_; \
    string msg = msg_; \
    ptree data; \
    stringstream ret; \
    getResponseJson(ret, code, msg, data); \
    con_->send(ret.str()); \
} while(0);

#define RESPONSE_SUCCESS(con_, data_) do { \
    int code = 0; \
    string msg = ""; \
    stringstream ret; \
    getResponseJson(ret, code, msg, data_); \
    con_->send(ret.str()); \
} while (0);

class ProducerCallback : public AutoDeleteSendCallBack {
    shared_ptr<WsServer::Connection> conn;
    virtual ~ProducerCallback() {}
    virtual void onSuccess(SendResult &sendResult) {
        ptree data;
        data.put("msgId", sendResult.getMsgId());
        RESPONSE_SUCCESS(this->conn, data);
    }
    virtual void onException(MQException &e) {
        RESPONSE_ERROR(this->conn, 1, e.what());
    }
public:
    void setConn(shared_ptr<WsServer::Connection> &con) {
        this->conn = con;
    }
};

class WorkerPool;

class ProxyPushConsumer : public DefaultMQPushConsumer {
public:
    QueueTS<shared_ptr<WsServer::Connection>> queue;
    map<string, std::mutex *> *msgMutexMap;
    map<string, std::condition_variable *> *conditionVariableMap;
    //被锁住的消息列表
    map<shared_ptr<WsServer::Connection>, map<string, int>> *pool;
    WorkerPool *wp;

    void initResource(map<shared_ptr<WsServer::Connection>, map<string, int>> *pool_,
                      map<string, std::mutex *> *msgMutexMap_,
                      map<string, std::condition_variable *> *conditionVariableMap_,
                      WorkerPool *wp_) {
        pool = pool_;
        msgMutexMap = msgMutexMap_;
        conditionVariableMap = conditionVariableMap_;
        wp = wp_;
    }

    ProxyPushConsumer(const std::string &groupname) : DefaultMQPushConsumer(groupname) {
    }
};

class ConsumerMsgListener : public MessageListenerConcurrently {
    shared_ptr<ProxyPushConsumer> consumer;
public:
    ConsumerMsgListener() {}

    virtual ~ConsumerMsgListener() {}

    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt> &msgs) {
        if (msgs.size() != 1 ) {
            cout << "consumeMessage msg max size is "<< msgs.size() << "\n";
            return RECONSUME_LATER;
        }
        auto conn = consumer->queue.wait_and_pop();
        ptree data;
        data.put("msgId", msgs[0].getMsgId());
        RESPONSE_SUCCESS(conn, data);
        auto mtx = new std::mutex;
        auto consumed = new std::condition_variable;
        consumer->msgMutexMap->insert(pair<string, std::mutex *>(msgs[0].getMsgId(), mtx));
        consumer->conditionVariableMap->insert(
                pair<string, std::condition_variable *>(msgs[0].getMsgId(), consumed));
        auto iter = consumer->pool->find(conn);
        if (iter == consumer->pool->end()) {
            map<string, int> temp;
            temp.insert(make_pair(msgs[0].getMsgId(), ROCKETMQ_PROXY_MSG_STATUS_SENT));
            consumer->pool->insert(make_pair(conn, temp));
        } else {
            auto p = &iter->second;
            p->insert(make_pair(msgs[0].getMsgId(), ROCKETMQ_PROXY_MSG_STATUS_SENT));
        }

        //必须大括号括起来，不然删掉了两个变量，但是lck却最后才释放
        {
            std::unique_lock<std::mutex> lck(*mtx);
            consumed->wait(lck);
        }
        //唤醒后删除lock
        delete mtx;
        delete consumed;
        //lock被唤醒，删除lock，避免内存泄漏
        consumer->msgMutexMap->erase(msgs[0].getMsgId());
        consumer->conditionVariableMap->erase(msgs[0].getMsgId());
        iter = consumer->pool->find(conn);
        ConsumeStatus status = RECONSUME_LATER;
        if (iter != consumer->pool->end()) {
            auto p = &iter->second;
            p->erase(msgs[0].getMsgId());
        }

        //阻塞住，等待客户端消费掉消息，或者断掉连接
        return status;
    }

    void setConsumer(shared_ptr<ProxyPushConsumer> con) {
        this->consumer = con;
    }
};

class WorkerPool {
    std::map<string, shared_ptr<DefaultMQProducer> > producers;
    std::map<string, shared_ptr<ProxyPushConsumer> > consumers;
    //
    map<shared_ptr<WsServer::Connection>, map<string, int>> &pool;
public:
    //连接断掉后，以前队列要把相关连接清空！
    void deleteConnection(shared_ptr<WsServer::Connection> con) {
        auto iter = consumers.begin();
        while (iter != consumers.end()) {
            auto consumer = iter->second;
            shared_ptr<WsServer::Connection> value;
            QueueTS<shared_ptr<WsServer::Connection>> tempQueue;
            while (consumer->queue.try_pop(value)) {
                if (value == con) {
                    continue;
                }
                tempQueue.push(value);
            }
            while (tempQueue.try_pop(value)) {
                consumer->queue.push(value);
            }
            iter++;
        }
    }

    map<string, std::mutex *> msgMutexMap;
    map<string, std::condition_variable *> conditionVariableMap;

    WorkerPool(map<shared_ptr<WsServer::Connection>, map<string, int>> &p) : pool(p) {}

    shared_ptr<DefaultMQProducer> getProducer(const string &topic) {
        auto iter = producers.find(topic);
        if (iter != producers.end())
            return iter->second;
        else {
            shared_ptr<DefaultMQProducer> producer(new DefaultMQProducer(topic));
            producer->setNamesrvAddr("namesrv:9876");
            producer->setInstanceName(topic);
            producer->setSendMsgTimeout(500);
            producer->setTcpTransportTryLockTimeout(1000);
            producer->setTcpTransportConnectTimeout(400);
            try {
                producer->start();
                producers.insert(pair<string, shared_ptr<DefaultMQProducer>>(topic, producer));
                return producer;
            } catch (exception &e) {
                cout << e.what() << endl;
                return NULL;
            }
        }
    }

    shared_ptr<ProxyPushConsumer> getConsumer(const string &topic, const string &group) {
        auto iter = consumers.find(topic);
        if (iter != consumers.end())
            return iter->second;
        else {
            shared_ptr<ProxyPushConsumer> consumer(new ProxyPushConsumer(group));
            consumer->setNamesrvAddr("namesrv:9876");
            consumer->setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
            consumer->setInstanceName(group);
            consumer->subscribe(topic, "*");
            consumer->setConsumeThreadCount(2);
            consumer->setTcpTransportTryLockTimeout(1000);
            consumer->setTcpTransportConnectTimeout(400);
            consumer->initResource(&pool, &msgMutexMap, &conditionVariableMap, this);
            ConsumerMsgListener *listener = new ConsumerMsgListener();
            listener->setConsumer(consumer);
            consumer->registerMessageListener(listener);
            try {
                consumer->start();
                consumers.insert(pair<string, shared_ptr<ProxyPushConsumer>>(topic, consumer));
                return consumer;
            } catch (MQClientException &e) {
                cout << e << endl;
                return NULL;
            }

        }
    }
};


int main() {
    WsServer server;
    server.config.port = 8080;
    map<shared_ptr<WsServer::Connection>, map<string, int>> msgPool;
    WorkerPool wp(msgPool);
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];
    auto &consumerEndpoint = server.endpoint["^/consumerEndpoint/?$"];
    //producer proxy
    producerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection,
                                        shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        string name = jsonItem.get<string>("topic");
        string tag = jsonItem.get<string>("tag");
        string body = jsonItem.get<string>("body");
        rocketmq::MQMessage msg(name, tag, body);
        ProducerCallback *calllback = new ProducerCallback();
        calllback->setConn(connection);
        auto producer = wp.getProducer(name);
        if (producer == NULL) {
            RESPONSE_ERROR(connection, 1, "system error!");
        } else {
            producer->send(msg, calllback);
        };
    };

    producerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    producerEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status,
                                   const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    // Can modify handshake response headers here if needed
    producerEndpoint.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/,
                                       SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    producerEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

    //consumer proxy
    consumerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection,
                                        shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        try {
            string topic = jsonItem.get<string>("topic");
            int type = jsonItem.get<int>("type");
            if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME ||
                type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_ACK) {
                //消费消息
                auto consumer = wp.getConsumer(topic, topic);
                if (consumer == NULL) {
                    RESPONSE_ERROR(connection, 1, "system error!");
                    return;
                }
                if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME) {
                    consumer->queue.push(connection);
                } else {
                    //ack消息
                    string msgId = jsonItem.get<string>("msgId");
                    auto iter1 = consumer->msgMutexMap->find(msgId);
                    auto iter2 = consumer->conditionVariableMap->find(msgId);
                    if (iter1 != consumer->msgMutexMap->end() && iter2 != consumer->conditionVariableMap->end()) {
                        auto mtx = iter1->second;
                        auto consumed = iter2->second;
                        std::unique_lock<std::mutex> lck(*mtx);
                        consumed->notify_one();
                    }
                }
            } else {
                RESPONSE_ERROR(connection, 1, "params error!");
            }
        } catch (exception &e) {
            RESPONSE_ERROR(connection, 1, e.what());
        }
    };

    consumerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    auto clearMsgPool = [](shared_ptr<WsServer::Connection> &connection,
                           map<shared_ptr<WsServer::Connection>, map<string, int>> &msgPool, WorkerPool &wp) {
        //删掉每个consumer里面连接队列的值
        wp.deleteConnection(connection);
        auto iter = msgPool.find(connection);
        if (iter != msgPool.end()) {
            auto msgMap = iter->second;
            auto iter1 = msgMap.begin();
            while (iter1 != msgMap.end()) {
                auto iter2 = wp.msgMutexMap.find(iter1->first);
                auto iter3 = wp.conditionVariableMap.find(iter1->first);
                if (iter2 != wp.msgMutexMap.end() && iter3 != wp.conditionVariableMap.end()) {
                    auto mtx = iter2->second;
                    auto consumed = iter3->second;
                    std::unique_lock<std::mutex> lck(*mtx);
                    consumed->notify_one();
                    cout << "notify_all:" << iter1->first << "\n";
                }
                iter1++;
            }
        }
    };

    consumerEndpoint.on_close = [&msgPool, &wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection, int status,
                                                               const string & /*reason*/) {
        clearMsgPool(connection, msgPool, wp);
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    consumerEndpoint.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/,
                                       SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    consumerEndpoint.on_error = [&msgPool, &wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection,
                                                               const SimpleWeb::error_code &ec) {
        clearMsgPool(connection, msgPool, wp);
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };


    promise<unsigned short> server_port;
    thread server_thread([&server, &server_port]() {
        // Start server
        server.start([&server_port](unsigned short port) {
            server_port.set_value(port);
        });
    });

    cout << "Server listening on port " << server_port.get_future().get() << endl << endl;
    server_thread.join();
}
