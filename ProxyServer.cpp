#include "client_ws.hpp"
#include "server_ws.hpp"
#include <future>

using namespace std;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    // WebSocket (WS)-server at port 8080 using 1 thread
    WsServer server;
    server.config.port = 8080;

    // Example 1: echo WebSocket endpoint
    // Added debug messages for example use of the callbacks
    // Test with the following JavaScript:
    //   var ws=new WebSocket("ws://localhost:8080/echo");
    //   ws.onmessage=function(evt){console.log(evt.data);};
    //   ws.send("test");
    auto &echo = server.endpoint["^/echo/?$"];

    echo.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = in_message->string();
        connection->send(out_message, [](const SimpleWeb::error_code &ec) {
            if(ec) {
                cout << "Server: Error sending message. " <<
                     // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                     "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    };

    echo.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    // See RFC 6455 7.4.1. for status codes
    echo.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    // Can modify handshake response headers here if needed
    echo.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap & /*response_header*/) {
        return SimpleWeb::StatusCode::information_switching_protocols; // Upgrade to websocket
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    echo.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

    // Example 2: Echo thrice
    // Demonstrating queuing of messages by sending a received message three times back to the client.
    // Concurrent send operations are automatically queued by the library.
    // Test with the following JavaScript:
    //   var ws=new WebSocket("ws://localhost:8080/echo_thrice");
    //   ws.onmessage=function(evt){console.log(evt.data);};
    //   ws.send("test");
    auto &echo_thrice = server.endpoint["^/echo_thrice/?$"];
    echo_thrice.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = make_shared<string>(in_message->string());

        connection->send(*out_message, [connection, out_message](const SimpleWeb::error_code &ec) {
            if(!ec)
                connection->send(*out_message); // Sent after the first send operation is finished
        });
        connection->send(*out_message); // Most likely queued. Sent after the first send operation is finished.
    };

    // Example 3: Echo to all WebSocket endpoints
    // Sending received messages to all connected clients
    // Test with the following JavaScript on more than one browser windows:
    //   var ws=new WebSocket("ws://localhost:8080/echo_all");
    //   ws.onmessage=function(evt){console.log(evt.data);};
    //   ws.send("test");
    auto &echo_all = server.endpoint["^/echo_all/?$"];
    echo_all.on_message = [&server](shared_ptr<WsServer::Connection> /*connection*/, shared_ptr<WsServer::InMessage> in_message) {
        auto out_message = in_message->string();

        // echo_all.get_connections() can also be used to solely receive connections on this endpoint
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
