#include "Arg_helper.h"
#include "common.hpp"
#include "ProducerCallback.h"
#include "ConsumerMsgListener.hpp"
#include "WorkerPool.hpp"
#include "EsLog.hpp"

void startProducer(WsServer &server, WorkerPool &wp)
{
    auto clearProducers = [](shared_ptr<WsServer::Connection> &connection, WorkerPool &wp) {
        wp.deleteProducerConn(connection);
    };
    auto &producerEndpoint = server.endpoint["^/producerEndpoint/?$"];
    producerEndpoint.on_message = [&wp](shared_ptr<WsServer::Connection> connection,
                                        shared_ptr<WsServer::InMessage> in_message) {
        try {
            string json = in_message->string();
            std::istringstream jsonStream;
            jsonStream.str(json);
            boost::property_tree::ptree jsonItem;
            boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
            string topic = jsonItem.get<string>("topic");
            string group = jsonItem.get<string>("group");
            string tag = jsonItem.get<string>("tag");
            string body = jsonItem.get<string>("body");
            rocketmq::MQMessage msg(topic, tag, body);
            int delayLevel = 0;
            if (jsonItem.get_child_optional("delayLevel")) {
                delayLevel = jsonItem.get<int>("delayLevel");
                msg.setDelayTimeLevel(delayLevel);
            }
            auto producer = wp.getProducer(topic, group, connection);
            auto callback = new ProducerCallback();
            //不能传引用，因为都是临时变量
            callback->successFunc = [=, &wp] (const string &msgId) {
                ptree responseData;
                responseData.put("msgId",msgId);
                wp.log.writeLog(ROCKETMQ_PROXY_LOG_TYPE_PRODUCER, msgId, topic, group, body, delayLevel);
                RESPONSE_SUCCESS(connection, responseData);
            };
            callback->failureFunc = [=](const string &msg)
            {
                RESPONSE_ERROR(connection, 1, msg);
            };

            producer->send(msg, callback);
        } catch (exception &e) {
            auto msg = "send msg error! " + string(e.what());
            RESPONSE_ERROR(connection, 1, msg);
        }
    };

    producerEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    producerEndpoint.on_close = [&wp, &clearProducers](shared_ptr<WsServer::Connection> connection, int status,
                                   const string & /*reason*/) {
        clearProducers(connection, wp);
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
            string group = jsonItem.get<string>("group");
            if (type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_CONSUME ||
                type == ROCKETMQ_PROXY_CONSUMER_REQUEST_TYPE_ACK) {
                //消费消息
                auto consumer = wp.getConsumer(topic, group, connection);
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
                        std::unique_lock<std::mutex> lck(unit->mtx);
                        unit->syncStatus = ROCKETMQ_PROXY_MSG_STATUS_SYNC_ACK;
                        lck.unlock();
                        unit->cv.notify_all();
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

    consumerEndpoint.on_open = [&wp](shared_ptr<WsServer::Connection> connection) {
        shared_ptr<ConnectionUnit> unit(new ConnectionUnit);
        wp.connectionUnit.insert(make_pair(connection, unit));
        cout << "Server: Opened connection " << connection.get() << endl;
    };

    auto clearMsgPool = [](shared_ptr<WsServer::Connection> &connection, WorkerPool &wp) {
        //删掉队列里面的请求
        wp.deleteConsumerQueue(connection);
        //通知锁掉的等待消费结果通知的线程
        {
            auto connectionUnit = wp.connectionUnit[connection];
            std::unique_lock<std::mutex> lck(connectionUnit->mtx);
            auto iter = connectionUnit->msgPool->begin();
            while (iter != connectionUnit->msgPool->end()) {
                MsgConsumeUnit * unit;
                if (wp.consumerUnitMap.try_get(iter->first, unit)) {
                    {
                        cout << "notify_all:" << iter->first << "\n";
                        unit->cv.notify_all();
                    }
                }
                iter++;
            }
        }
        //关闭不必要的consumer
        wp.deleteConsumerConnection(connection);
    };

    consumerEndpoint.on_close = [&wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection, int status,
                                                               const string & /*reason*/) {
        clearMsgPool(connection, wp);
        cout << "Server: Closed connection " << connection.get() << " with status code " << status << endl;
    };

    consumerEndpoint.on_error = [&wp, &clearMsgPool](shared_ptr<WsServer::Connection> connection,
                                                               const SimpleWeb::error_code &ec) {
        cout << "Server: Error in connection " << connection.get() << ". "
             << "Error: " << ec << ", error message: " << ec.message() << endl;
    };
}

int main(int argc, char* argv[]) {
    rocketmq::Arg_helper arg_help(argc, argv);
    string file = arg_help.get_option_value("-f");
    if (file.size() == 0 || access( file.c_str(), F_OK ) == -1) {
        cout << "Server -c [file_name]" <<endl;
        return 0;
    }
    string nameServer;
    string host;
    string accessKey;
    string secretKey;
    string esServer;
    int port;
    try {
        std::ifstream t(file);
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::string contents(buffer.str());
        std::istringstream jsonStream;
        jsonStream.str(contents);
        boost::property_tree::ptree jsonItem;
        boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
        nameServer = jsonItem.get<string>("nameServer");
        accessKey = jsonItem.get<string>("accessKey");
        secretKey = jsonItem.get<string>("secretKey");
        esServer = jsonItem.get<string>("esServer");
        host = jsonItem.get<string>("host");
        port = jsonItem.get<int>("port");
    } catch ( exception &e) {
        cout << e.what() << endl;
        return 0;
    }

    WsServer server;
    server.config.port = port;
    WorkerPool wp(nameServer, accessKey, secretKey, esServer);
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
