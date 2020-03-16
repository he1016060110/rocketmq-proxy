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
    string topic;
    string group;
    string body;
    int delayLevel;
    int status;//0
public:
    LogUnit() : type(0), topic(""), group(""), body(), delayLevel(0) ,status(0){};
};

class EsLog {
    int max;
    QueueTS<shared_ptr<LogUnit>> logQueue;
public:
    EsLog(int _max = 100): max(_max) {};
    bool writeLog(int type, string topic, string group, string body, int delayLevel = 0, int status = 0)
    {
        shared_ptr<LogUnit> unit(new LogUnit);
        unit->type = type;
        unit->topic = topic;
        unit->group = group;
        unit->body = body;
        unit->delayLevel = delayLevel;
        unit->status = status;
        logQueue.push(unit);
    }

    void loopConsumeLog()
    {
        int count = 0;
        string data;
        string init("{ \"index\": { \"_index\": \"msg\", \"_type\": \"msg\"}}\n");
        shared_ptr<LogUnit> unit;
        while(unit= logQueue.wait_and_pop())
        {
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
                post("http://es.t.xianghuanji.com:9200/_bulk", data);
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
            chunk = curl_slist_append(chunk, "Content-type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());    // 指定post内容
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());   // 指定url
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
        }
        return true;
    }
};


#endif