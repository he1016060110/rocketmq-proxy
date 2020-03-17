//
// Created by hexi on 2020/3/16.
//

#ifndef ROCKETMQ_PROXY_ESLOG
#define ROCKETMQ_PROXY_ESLOG

#include <curl/curl.h>
#include <string>
#include "QueueTS.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::property_tree;
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

size_t writeData(void *ptr, size_t size, size_t nmemb, string *str) {
    int numbytes = size * nmemb;
    str->append(static_cast<char *>(ptr), numbytes);
    return numbytes;
}

class ProxyLogger {
    int max;
    QueueTS<shared_ptr<LogUnit>> logQueue;
    string host;
    FILE *logFile;
public:
    ProxyLogger(string esHost, int _max = 100) : max(_max), host(esHost) {
        logFile = fopen("/root/es.log", "a+");
    };

    bool writeLog(int type, string msgId, string topic, string group, string body, int delayLevel = 0, int status = 0) {
        shared_ptr<LogUnit> unit(new LogUnit);
        unit->type = type;
        unit->topic = topic;
        unit->group = group;
        unit->body = body;
        unit->msgId = msgId;
        unit->delayLevel = delayLevel;
        unit->status = status;
        logQueue.push(unit);
    }

    void loopConsumeLog() {
        int count = 0;
        string data;
        string init("{ \"index\": { \"_index\": \"msg\", \"_type\": \"msg\"}}\n");
        shared_ptr<LogUnit> unit;
        string url = host + "/_bulk";
        while (unit = logQueue.wait_and_pop()) {
            data += init;
            count++;
            stringstream json_str;
            ptree json;
            json.put("type", unit->type);;
            json.put("topic", unit->topic);;
            json.put("group", unit->group);;
            json.put("delayLevel", unit->delayLevel);;
            json.put("status", unit->type);
            json.put("body", unit->body);
            write_json(json_str, json, false);
            data += json_str.str() + "\n";
            if (count >= max) {
                post(url, data);
                data = "";
                count = 0;
            }
        }
    }

    bool post(const string &url, const string &data) {
        CURL *curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            struct curl_slist *chunk = NULL;
            string ret;
            chunk = curl_slist_append(chunk, "Content-type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());    // 指定post内容
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());   // 指定url
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            if (res == CURLE_OK) {
                cout << ret <<endl;
                return true;
            }
        }
        return false;
    }
};


#endif