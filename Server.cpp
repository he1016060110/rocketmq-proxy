#include "Arg_helper.h"
#include "common.hpp"
#include "ProducerCallback.h"
#include "ConsumerMsgListener.hpp"
#include "ProxyPushConsumer.hpp"
#include "WorkerPool.hpp"

void startProducer(WsServer &server, WorkerPool &wp)
{
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];
    producerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection,
                                        shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        string name = jsonItem.get<string>("topic");
        string tag = jsonItem.get<string>("tag");
        string body = jsonItem.get<string>("body");
        rocketmq::MQMessage msg(name, tag, body);
        ProducerCallback *calllback = new ProducerCallback();
        calllback->setConn(connection);
        auto producer = wp.getProducer(name);
        if (producer == NULL) {
            RESPONSE_ERROR(connection, 1, "system error!");
        } else {
            producer->send(msg, calllback);
        };
    };

    producerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    producerEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status,
                                   const string & /*reason*/) {
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    producerEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };
}

void startConsumer(WsServer &server, WorkerPool &wp)
{
    auto &consumerEndpoint = server.endpoint["^/consumerEndpoint/?$"];
    consumerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection,
                                        shared_ptr<WsServer::InMessage> in_message) {
        string json = in_message->string();
        std::istringstream jsonStream;
        jsonStream.str(json);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        try {
            string topic = jsonItem.get<string>("topic");
            int type = jsonItem.get<int>("type");
            if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME ||
                type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_ACK) {
                //消费消息
                auto consumer = wp.getConsumer(topic, topic);
                if (consumer == NULL) {
                    RESPONSE_ERROR(connection, 1, "system error!");
                    return;
                }
                if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME) {
                    consumer->queue.push(connection);
                } else {
                    //ack消息
                    string msgId = jsonItem.get<string>("msgId");
                    int status = jsonItem.get<int>("status");
                    MsgConsumeUnit * unit;
                    consumer->consumerUnitMap->try_get(msgId, unit);
                    unit->status = (ConsumeStatus)status;
                    {
                        while(unit->syncStatus != ROCKETMQ_PROXY_MSG_STATUS_SYNC_SENT) {
                            boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds (1));
                        }
                        unit->cv.notify_one();
                    }
                    ptree data;
                    data.put("msgId", msgId);
                    data.put("type", ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_ACK);
                    RESPONSE_SUCCESS(connection, data);
                }
            } else {
                RESPONSE_ERROR(connection, 1, "params error!");
            }
        } catch (exception &e) {
            RESPONSE_ERROR(connection, 1, e.what());
        }
    };

    consumerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    auto clearMsgPool = [](shared_ptr<WsServer::Connection> &connection, WorkerPool &wp) {
        //删掉每个consumer里面连接队列的值
        wp.deleteConnection(connection);
        auto msgPool = wp.pool;
        auto iter = msgPool.find(connection);
        if (iter != msgPool.end()) {
            auto msgMap = iter->second;
            auto iter1 = msgMap.begin();
            while (iter1 != msgMap.end()) {
                MsgConsumeUnit * unit;
                if (wp.consumerUnitMap.try_get(iter1->first, unit)) {
                    {
                        unit->cv.notify_all();
                        cout << "notify_all:" << iter1->first << "\n";
                    }
                }
                iter1++;
            }
        }
    };

    consumerEndpoint.on_close = [&wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection, int status,
                                                               const string & /*reason*/) {
        clearMsgPool(connection, wp);
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    consumerEndpoint.on_error = [&wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection,
                                                               const SimpleWeb::error_code &ec) {
        clearMsgPool(connection, wp);
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };
}

int main(int argc, char* argv[]) {
    rocketmq::Arg_helper arg_help(argc, argv);
    string nameServer = arg_help.get_option_value("-n");
    string host = arg_help.get_option_value("-h");
    string port = arg_help.get_option_value("-p");
    if (!nameServer.size() || !host.size() || !port.size()) {
        cout << "-n nameServer -h host -p port" <<endl;
        return 0;
    }
    WsServer server;
    server.config.port = stoi(port);
    WorkerPool wp(nameServer);
    startProducer(server, wp);
    startConsumer(server, wp);
    promise<unsigned short> server_port;
    thread server_thread([&server, &server_port]() {
        server.start([&server_port](unsigned short port) {
            server_port.set_value(port);
        });
    });

    cout << "Server listening on port " << server_port.get_future().get() << endl << endl;
    server_thread.join();
}
