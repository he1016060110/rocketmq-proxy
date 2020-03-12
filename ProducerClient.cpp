#include "client_ws.hpp"
#include "Arg_helper.h"
#include <chrono>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::property_tree;
using namespace std;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using namespace std;
using namespace chrono;

int main(int argc, char *argv[]) {
    rocketmq::Arg_helper arg_help(argc, argv);
    string host = arg_help.get_option_value("-h");
    string group = arg_help.get_option_value("-g");
    string topic = arg_help.get_option_value("-t");
    string concurrency = arg_help.get_option_value("-c");
    string num = arg_help.get_option_value("-n");
    if (!host.size() || !topic.size()) {
        cout << "-t topic -h host -g group (optional) -c concurrency (optional) " << endl;
        return 0;
    }
    int concurrencyNum = 1;
    if (concurrency.size()) {
        concurrency = atoi(concurrency.c_str());
    }
    int max = 1000;
    if (num.size()) {
        max = atoi(num.c_str());
    }
    if (!group.size()) {
        group = topic;
    }
    string serverPath = host + "/producerEndpoint";
    WsClient client(serverPath);
    int count = 0;
    auto start = system_clock::now();
    auto sendConsumeRequest = [](shared_ptr<WsClient::Connection> &connection, string &topic, string &group) {
        ptree requestItem;
        requestItem.put("topic", topic);
        requestItem.put("group", group);
        requestItem.put("tag", "*");
        requestItem.put("body", "this is test!");
        stringstream request_str;
        write_json(request_str, requestItem, false);
        connection->send(request_str.str());
    };

    client.on_message = [&count, &start, &max, &sendConsumeRequest, &topic, &group](
            shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        count++;
        //cout << "Received msg: "<< in_message->string();
        if (count >= max) {
            auto end = system_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            cout << count << "条花费了"
                 << double(duration.count()) * microseconds::period::num / microseconds::period::den
                 << "秒" << endl;
            return;
        }
        sendConsumeRequest(connection, topic, group);
    };

    client.on_open = [&sendConsumeRequest, &topic, &group, &concurrencyNum](
            shared_ptr<WsClient::Connection> connection) {
        for (int i = 0; i < concurrencyNum; i++) {
            sendConsumeRequest(connection, topic, group);
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
