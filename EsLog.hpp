//
// Created by hexi on 2020/3/16.
//

#include <curl/curl.h>
#include <string>

class EsLog {
    class EasyCurl {
        bool get(const string &url) {
            CURL *curl;
            CURLcode res;
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "user-agent: XHJ-MSG-ES-LOGGER");
            curl = curl_easy_init();    // 初始化
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);// 改协议头
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                res = curl_easy_perform(curl);   // 执行
                if (res != 0) {

                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                }
                return true;
            }
        }

        bool post(const string &url, const string & data) {
            CURL *curl;
            CURLcode res;
            FILE *fp;
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

};