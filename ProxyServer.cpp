#include "server_ws.hpp"
#include <future>
#include "common.h"

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

class WorkerPool
{
    std::map<string, shared_ptr<DefaultMQProducer> > producers;
public:
    shared_ptr<DefaultMQProducer> getProducer(const string &name)
    {
        auto iter = producers.find(name);
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
            producers.insert(pair<string, shared_ptr<DefaultMQProducer>> (name, producer));

            return producer;
        }
    }
};

int main() {
    WsServer server;
    server.config.port = 8080;
    WorkerPool wp;
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];

    producerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto name = in_message->string();
        rocketmq::MQMessage msg(name,  // topic
                                "*",          // tag
                                "test message!");  // body
        ProducerCallback * calllback = new ProducerCallback();
        calllback->setConn(connection);
        auto producer = wp.getProducer(name);
        producer->send(msg, calllback);
    };

    producerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    // See RFC 6455 7.4.1. for status codes
    producerEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    // Can modify handshake response headers here if needed
    producerEndpoint.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    producerEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

    auto &producerEndpoint_thrice = server.endpoint["^/producerEndpoint_thrice/?$"];
    producerEndpoint_thrice.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = make_shared<string>(in_message->string());

        connection->send(*out_message, [connection, out_message](const SimpleWeb::error_code &ec) {
            if(!ec)
                connection->send(*out_message); // Sent after the first send operation is finished
        });
        connection->send(*out_message); // Most likely queued. Sent after the first send operation is finished.
    };

    auto &producerEndpoint_all = server.endpoint["^/producerEndpoint_all/?$"];
    producerEndpoint_all.on_message = [&server](shared_ptr<WsServer::Connection> /*connection*/, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = in_message->string();

        // producerEndpoint_all.get_connections() can also be used to solely receive connections on this endpoint
        for(auto &a_connection : server.get_connections())
            a_connection->send(out_message);
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
