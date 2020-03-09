#include "client_ws.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Const.hpp"
#include "Arg_helper.h"
#include <boost/timer.hpp>
#include <chrono>

using namespace std;
using namespace boost::property_tree;
using namespace std;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using boost::timer;
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
    string serverPath = host + ":" + port + "/consumerEndpoint";
    WsClient client(serverPath);
    int count = 0;
    auto start = system_clock::now();
    client.on_message = [&count, &start](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        string json = in_message->string();
        //cout << "Received msg: "<< json;
        count++;
        if (count % 1000 == 0) {
            auto end   = system_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            cout <<  count << "条花费了"
                 << double(duration.count()) * microseconds::period::num / microseconds::period::den
                 << "秒" << endl;
        }
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        int code = jsonItem.get<int>("code");
        if (code == 0) {
            auto data = jsonItem.get_child("data");
            int type = data.get<int>("type");
            string msgId = data.get<string>("msgId");
            if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME) {
                ptree requestItem;
                requestItem.put("topic", "TestTopicProxy");
                requestItem.put("msgId", msgId);
                requestItem.put("status", 0);
                requestItem.put("type", ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_ACK);
                stringstream request_str;
                write_json(request_str, requestItem, false);
                connection->send(request_str.str());
            } else {
                ptree requestItem;
                requestItem.put("topic", "TestTopicProxy");
                requestItem.put("type", ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME);
                stringstream request_str;
                write_json(request_str, requestItem, false);
                connection->send(request_str.str());
            }
        }
    };

    client.on_open = [](shared_ptr<WsClient::Connection> connection) {
        cout << "Client: Opened connection" << endl;
        string json= "{ \
            \"topic\": \"TestTopicProxy\", \
            \"type\": 1 \
        }";
        connection->send(json);
    };

    client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };

    client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };

    client.start();
}
