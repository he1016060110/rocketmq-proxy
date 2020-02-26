#include "server_ws.hpp"
#include <future>
#include "common.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using namespace rocketmq;

class ProducerCallback : public SendCallback {
    shared_ptr<WsServer::Connection> conn;
    virtual void onSuccess(SendResult& sendResult) {
        this->conn->send(sendResult.getMsgId(), [](const SimpleWeb::error_code &ec) {
            if(ec) {
                cout << "Server: Error sending message. " <<
                     "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    }
    virtual void onException(MQException& e) {
        this->conn->send("error!");
    }

public:
    void setConn(shared_ptr<WsServer::Connection> &con)
    {
        this->conn = con;
    }
};

class ConsumerMsgListener : public MessageListenerConcurrently {
    shared_ptr<WsServer::Connection> conn;
public:
    ConsumerMsgListener() {}
    virtual ~ConsumerMsgListener() {}

    virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) {
        for (size_t i = 0; i < msgs.size(); ++i) {
            conn->send(msgs[i].getMsgId());
        }
        return CONSUME_SUCCESS;
    }
    void setConn(shared_ptr<WsServer::Connection> con)
    {
        this->conn = con;
    }
};

class WorkerPool
{
    std::map<string, shared_ptr<DefaultMQProducer> > producers;
    std::map<string, shared_ptr<DefaultMQPushConsumer> > consumers;
public:
    shared_ptr<DefaultMQProducer> getProducer(const string &topic)
    {
        auto iter = producers.find(topic);
        if(iter != producers.end())
            return iter->second;
        else {
            shared_ptr<DefaultMQProducer> producer (new DefaultMQProducer("AsyncProducer"));
            producer->setNamesrvAddr("namesrv:9876");
            producer->setInstanceName("AsyncProducer");
            producer->setSendMsgTimeout(500);
            producer->setTcpTransportTryLockTimeout(1000);
            producer->setTcpTransportConnectTimeout(400);
            producer->start();
            producers.insert(pair<string, shared_ptr<DefaultMQProducer>> (topic, producer));

            return producer;
        }
    }
    shared_ptr<DefaultMQPushConsumer>  getConsumer(const string &topic, shared_ptr<WsServer::Connection> &con)
    {
        auto iter = consumers.find(topic);
        if(iter != consumers.end())
            return iter->second;
        else {
            shared_ptr<DefaultMQPushConsumer> consumer(new DefaultMQPushConsumer("AsyncConsumer"));
            consumer->setNamesrvAddr("namesrv:9876");
            consumer->setGroupName("AsyncConsumer");
            consumer->setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
            consumer->setInstanceName("AsyncConsumer");
            consumer->subscribe(topic, "*");
            consumer->setConsumeThreadCount(15);
            consumer->setTcpTransportTryLockTimeout(1000);
            consumer->setTcpTransportConnectTimeout(400);
            ConsumerMsgListener * listener = new ConsumerMsgListener();
            listener->setConn(con);
            consumer->registerMessageListener(listener);
            try {
                consumer->start();
            } catch (MQClientException &e) {
                cout << e << endl;
            }
            consumers.insert(pair<string, shared_ptr<DefaultMQPushConsumer>>(topic, consumer));

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
        //todo 需要区分connection，这里会有多进程消费的情况
        auto consumer = wp.getConsumer(topic, connection);
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
