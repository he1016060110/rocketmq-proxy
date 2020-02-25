#include "client_ws.hpp"
#include "server_ws.hpp"
#include <future>

using namespace std;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    WsClient client("localhost:8080/echo");
    client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        cout << "Client: Message received: \"" << in_message->string() << "\"" << endl;

        cout << "Client: Sending close connection" << endl;
        connection->send_close(1000);
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;

        string out_message("Hello");
        cout << "Client: Sending message: \"" << out_message << "\"" << endl;

        connection->send(out_message);
    };

    client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
}
