//
// Created by hexi on 2020/3/16.
//

#ifndef ROCKETMQ_PROXY_ESLOG
#define ROCKETMQ_PROXY_ESLOG

#include <curl/curl.h>
#include <string>
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
    LogUnit() : type(0), topic(""), group(""), delayLevel(0), status(0), body() {};
};

class EsLog {
    int max = 100;
    QueueTS<shared_ptr<LogUnit>> logQueue;
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
        string data = "";
        shared_ptr<LogUnit> unit;
        while(unit= logQueue.wait_and_pop())
        {
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
            if (count <= max) {
                post("/_bulk", data);
                data.empty();
                count = 0;
            }
        }
    }


    bool post(const string &url, const string &data) {
        CURL *curl;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());    // 指定post内容
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());   // 指定url
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return true;
    }
};


#endif