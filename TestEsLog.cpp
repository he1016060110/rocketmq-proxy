//
// Created by hexi on 2020/3/16.
//

#include "EsLog.hpp"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

void writeLog(EsLog *logger) {
    while (true) {
        for (int i = 0; i < 100; i++) {
            logger->writeLog(0, "ssssssssss", "test", "test", "test", 0, 0);
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}

int main() {
    auto logger = new EsLog("http://es.t.xianghuanji.com:9200",100);
    boost::thread thd(boost::bind(writeLog, logger));
    logger->loopConsumeLog();
}