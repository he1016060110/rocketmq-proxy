#include "client_ws.hpp"
#include <future>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "Const.hpp"
using namespace std;
using namespace boost::property_tree;

using namespace std;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
    WsClient client("localhost:8080/consumerEndpoint");
    client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
        string json = in_message->string();
        cout << "Received msg: "<< json;
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
