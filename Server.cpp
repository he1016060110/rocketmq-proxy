#include "server_ws.hpp"
#include <future>
#include "DefaultMQProducer.h"
#include "DefaultMQPushConsumer.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "QueueTS.hpp"

using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using namespace rocketmq;

class ProducerCallback : public AutoDeleteSendCallBack {
    shared_ptr<WsServer::Connection> conn;
    virtual ~ProducerCallback() {}
    virtual void onSuccess(SendResult& sendResult) {
        this->conn->send(sendResult.getMsgId(), [](const SimpleWeb::error_code &ec) {
            if(ec) {
                cout << "Server: Error sending message. " <<
                     "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    }
    virtual void onException(MQException& e) {
        this->conn->send(e.what());
    }

public:
    void setConn(shared_ptr<WsServer::Connection> &con)
    {
        this->conn = con;
    }
};

class ProxyPushConsumer : public DefaultMQPushConsumer {
public:
    QueueTS<shared_ptr<WsServer::Connection>> queue;
    ProxyPushConsumer(const std::string& groupname) : DefaultMQPushConsumer(groupname) {
    }
};

class ConsumerMsgListener : public MessageListenerConcurrently {
    shared_ptr<ProxyPushConsumer> consumer;
public:
    ConsumerMsgListener() {}
    virtual ~ConsumerMsgListener() {}

    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) {
        for (size_t i = 0; i < msgs.size(); ++i) {
            auto conn = consumer->queue.wait_and_pop();
            conn->send(msgs[i].getMsgId());
        }
        return CONSUME_SUCCESS;
    }
    void setConsumer(shared_ptr<ProxyPushConsumer>  con)
    {
        this->consumer = con;
    }
};

class WorkerPool
{
    std::map<string, shared_ptr<DefaultMQProducer> > producers;
    std::map<string, shared_ptr<ProxyPushConsumer> > consumers;
public:
    shared_ptr<DefaultMQProducer> getProducer(const string &topic)
    {
        auto iter = producers.find(topic);
        if(iter != producers.end())
            return iter->second;
        else {
            shared_ptr<DefaultMQProducer> producer (new DefaultMQProducer(topic));
            producer->setNamesrvAddr("namesrv:9876");
            producer->setInstanceName(topic);
            producer->setSendMsgTimeout(500);
            producer->setTcpTransportTryLockTimeout(1000);
            producer->setTcpTransportConnectTimeout(400);
            producer->start();
            producers.insert(pair<string, shared_ptr<DefaultMQProducer>> (topic, producer));

            return producer;
        }
    }
    shared_ptr<ProxyPushConsumer>  getConsumer(const string &topic, const string &group,
            shared_ptr<WsServer::Connection> &con)
    {
        auto iter = consumers.find(topic);
        if(iter != consumers.end())
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
            ConsumerMsgListener * listener = new ConsumerMsgListener();
            listener->setConsumer(consumer);
            consumer->registerMessageListener(listener);
            try {
                consumer->start();
            } catch (MQClientException &e) {
                cout << e << endl;
            }
            consumers.insert(pair<string, shared_ptr<ProxyPushConsumer>>(topic, consumer));

            return consumer;
        }
    }
};

int main() {
    WsServer server;
    server.config.port = 8080;
    WorkerPool wp;
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];
    auto &consumerEndpoint = server.endpoint["^/consumerEndpoint/?$"];

    //producer proxy
    producerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        string name = jsonItem.get<string>("topic");
        string tag = jsonItem.get<string>("tag");
        string body = jsonItem.get<string>("body");
        rocketmq::MQMessage msg(name, tag, body);
        ProducerCallback * calllback = new ProducerCallback();
        calllback->setConn(connection);
        auto producer = wp.getProducer(name);
        producer->send(msg, calllback);
    };

    producerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    producerEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    // Can modify handshake response headers here if needed
    producerEndpoint.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    producerEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

    //consumer proxy
    consumerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        string topic = jsonItem.get<string>("topic");
        auto consumer = wp.getConsumer(topic, topic, connection);
        consumer->queue.push(connection);
    };

    consumerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    consumerEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    consumerEndpoint.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    consumerEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
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
