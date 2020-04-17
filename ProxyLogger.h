//
// Created by hexi on 2020/3/16.
//

#ifndef ROCKETMQ_PROXY_ESLOG
#define ROCKETMQ_PROXY_ESLOG

#include "curl/curl.h"
#include <ctime>
#include <memory>
#include "QueueTS.hpp"

#define PROXY_LOGGER_TYPE_PRODUCE 1
#define PROXY_LOGGER_TYPE_CONSUME 2
#define PROXY_LOGGER_TYPE_CONSUME_ACL 3

using namespace std;

class LogUnit {
public:
    int type;//1,produce,consume
    string msgId;
    string topic;
    string group;
    string body;
    int delayLevel;
    int status;//0
public:
    LogUnit() : type(0), msgId(""), topic(""), group(""), body(), delayLevel(0), status(0) {};
};

class ProxyLogger {
    int max;
    QueueTS<shared_ptr<LogUnit>> logQueue;
    string host;
    FILE *logFile;
    int esErrorCount;
    int esErrorMax;
    bool logFileOpened;
    void getTime(string &timeStr);
    bool bulk(const string &url, const string &data);
public:
    ProxyLogger(string esHost, string logFileName, int _max);
    bool writeLog(int type, string msgId, string topic, string group, string body, int delayLevel, int status);
    void loopConsumeLog();
};


#endif