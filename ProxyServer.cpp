#include "client_ws.hpp"
#include "server_ws.hpp"
#include <future>
#include "common.h"

using namespace std;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

using namespace rocketmq;

SendCallback* g_callback = NULL;

class ProducerCallback : public SendCallback {
    shared_ptr<WsServer::Connection> conn;
    virtual void onSuccess(SendResult& sendResult) {
        this->conn->send(sendResult.getMsgId(), [](const SimpleWeb::error_code &ec) {
            if(ec) {
                cout << "Server: Error sending message. " <<
                     // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
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


int main() {
    // WebSocket (WS)-server at port 8080 using 1 thread
    WsServer server;
    server.config.port = 8080;

    DefaultMQProducer * producer = new DefaultMQProducer("AsyncProducer");

    producer->setNamesrvAddr("namesrv:9876");
    producer->setInstanceName("AsyncProducer");
    producer->setSendMsgTimeout(500);
    producer->setTcpTransportTryLockTimeout(1000);
    producer->setTcpTransportConnectTimeout(400);
    producer->start();
    
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];

    producerEndpoint.on_message = [&producer](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = in_message->string();
        rocketmq::MQMessage msg("TestTopicAsync",  // topic
                                "*",          // tag
                                "test message!");  // body
        ProducerCallback * calllback = new ProducerCallback();
        calllback->setConn(connection);
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

    // Example 2: producerEndpoint thrice
    // Demonstrating queuing of messages by sending a received message three times back to the client.
    // Concurrent send operations are automatically queued by the library.
    // Test with the following JavaScript:
    //   var ws=new WebSocket("ws://localhost:8080/producerEndpoint_thrice");
    //   ws.onmessage=function(evt){console.log(evt.data);};
    //   ws.send("test");
    auto &producerEndpoint_thrice = server.endpoint["^/producerEndpoint_thrice/?$"];
    producerEndpoint_thrice.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = make_shared<string>(in_message->string());

        connection->send(*out_message, [connection, out_message](const SimpleWeb::error_code &ec) {
            if(!ec)
                connection->send(*out_message); // Sent after the first send operation is finished
        });
        connection->send(*out_message); // Most likely queued. Sent after the first send operation is finished.
    };

    // Example 3: producerEndpoint to all WebSocket endpoints
    // Sending received messages to all connected clients
    // Test with the following JavaScript on more than one browser windows:
    //   var ws=new WebSocket("ws://localhost:8080/producerEndpoint_all");
    //   ws.onmessage=function(evt){console.log(evt.data);};
    //   ws.send("test");
    auto &producerEndpoint_all = server.endpoint["^/producerEndpoint_all/?$"];
    producerEndpoint_all.on_message = [&server](shared_ptr<WsServer::Connection> /*connection*/, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = in_message->string();

        // producerEndpoint_all.get_connections() can also be used to solely receive connections on this endpoint
        for(auto &a_connection : server.get_connections())
            a_connection->send(out_message);
    };

    // Start server and receive assigned port when server is listening for requests
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
