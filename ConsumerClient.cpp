#include "client_ws.hpp"
#include <future>
#include <boost/timer.hpp>

using namespace std;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    WsClient client("localhost:8080/consumerEndpoint");
    client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        cout << in_message->string() << "\n";
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;
        string json= "{ \
            \"topic\": \"TestTopicProxy\" \
        }";
        int count = 0;
        while(count ++ < 10000) {
            connection->send(json);
        }
    };

    client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
}
