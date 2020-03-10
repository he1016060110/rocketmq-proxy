#include "client_ws.hpp"
#include "Arg_helper.h"
#include <chrono>

using namespace std;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using namespace std;
using namespace chrono;

int main(int argc, char* argv[]) {
    rocketmq::Arg_helper arg_help(argc, argv);
    string host = arg_help.get_option_value("-h");
    string port = arg_help.get_option_value("-p");
    if (!host.size() || !port.size()) {
        cout << "-h host -p port" <<endl;
        return 0;
    }
    string serverPath = host + ":" + port + "/producerEndpoint";
    WsClient client(serverPath);
    int count = 0;
    auto start = system_clock::now();
    client.on_message = [&count, &start](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        count++;
        cout << "Received msg: "<< in_message->string();
        if (count % 1000 == 0) {
            auto end   = system_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            cout <<  count << "条花费了"
                 << double(duration.count()) * microseconds::period::num / microseconds::period::den
                 << "秒" << endl;
        }
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
