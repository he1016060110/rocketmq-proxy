#include "client_ws.hpp"
#include <future>
#include <boost/timer.hpp>
#include <boost/thread.hpp>

using namespace std;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    WsClient client("localhost:8080/producerEndpoint");
    int count = 0;
    client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        count++;
        cout << in_message->string();
        string out_message("Hello");
        string json= "{ \
            \"topic\": \"TestTopicProxy\", \
            \"tag\": \"*\", \
            \"body\": \"this this the TestTopicProxy!\" \
        }";
        connection->send(json);
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        string out_message("Hello");
        string json= "{ \
            \"topic\": \"TestTopicProxy\", \
            \"tag\": \"*\", \
            \"body\": \"this this the TestTopicProxy!\" \
        }";
        connection->send(json);
        cout << "Client: Opened connection" << endl;
    };

    client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
}
