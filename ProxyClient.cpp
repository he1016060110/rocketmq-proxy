#include "client_ws.hpp"
#include "server_ws.hpp"
#include <future>
#include <boost/timer.hpp>

using namespace std;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    WsClient client("localhost:8080/producerEndpoint");
    int count = 0;
    client.on_message = [&count](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        count++;
        cout << in_message->string() << "\n";
        if (count >= 10) {
            connection->send_close(1000);
            cout << "Client: Sending close connection" << endl;
        }
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;
        string out_message("Hello");
        boost::timer t;
        for (int i =0 ; i< 10; i++) {
            connection->send(out_message);
        }
        std::cout << "time cost: " << t.elapsed() << "\n" << std::endl;
        cout << "Client: Sending message: \"" << out_message << "\"" << endl;
    };

    client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
}
