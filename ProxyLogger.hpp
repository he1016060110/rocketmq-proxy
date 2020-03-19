//
// Created by hexi on 2020/3/16.
//

#ifndef ROCKETMQ_PROXY_ESLOG
#define ROCKETMQ_PROXY_ESLOG

#include "common.hpp"
#include "Ip.hpp"
#include <ctime>

using namespace std;
using namespace boost::property_tree;

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
    int esErrorCount;
    int esErrorMax;
    bool logFileOpened;
public:
    ProxyLogger(string esHost, string logFileName, int _max = 100) : max(_max), host(esHost), esErrorCount(0),
                                                                     esErrorMax(10), logFileOpened(false) {
        size_t size = 16;
        char ip[size];
        if (getLocalIp("eth0", ip, size) != 0) {
            cout << "cannot find ip!" << endl;
        }
        logFileName.append("_").append(ip);
        logFile = fopen(logFileName.c_str(), "a+");
        if (logFile == NULL) {
            cout << "log file cannot open!" << endl;
        } else {
            logFileOpened = true;
        }
    };

    void getTime(string &timeStr) {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        char t[25];
        sprintf(t, "%d-%02d-%02dT%02d:%02d:%02d.000+0800", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
                ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        timeStr.append(t);
    }

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
            string timeStr;
            getTime(timeStr);
            json.put("type", unit->type);;
            json.put("msgId", unit->msgId);;
            json.put("topic", unit->topic);;
            json.put("group", unit->group);;
            json.put("delayLevel", unit->delayLevel);;
            json.put("status", unit->status);
            json.put("body", unit->body);
            json.put("created_at", timeStr);
            write_json(json_str, json, false);
            data += json_str.str() + "\n";
            if (count >= max) {
                if (esErrorCount >= esErrorMax) {
                    logFileOpened && fwrite(data.c_str(), data.size(), 1, logFile);
                } else {
                    if (!bulk(url, data)) {
                        //如果发送不了es，就发写入到日志
                        logFileOpened && fwrite(data.c_str(), data.size(), 1, logFile);
                        esErrorCount++;
                    }
                }
                data = "";
                count = 0;
            }
        }
    }

    bool bulk(const string &url, const string &data) {
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
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            if (res == CURLE_OK) {
                std::istringstream jsonStream;
                jsonStream.str(ret);
                boost::property_tree::ptree jsonItem;
                boost::property_tree::json_parser::read_json(jsonStream, jsonItem);
                if (jsonItem.get_child_optional("errors")) {
                    bool hasError = jsonItem.get<bool>("errors");
                    if (!hasError) {
                        return true;
                    }
                }
                cout << ret << endl;
            }
        }

        return false;
    }
};


#endif