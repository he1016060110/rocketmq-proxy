//
// Created by hexi on 2020/4/10.
//

#ifndef ROCKETMQ_PROXY_COMMON_H
#define ROCKETMQ_PROXY_COMMON_H

enum reponseCode {
    RESPONSE_SUCCESS,
    RESPONSE_ERROR,
    RESPONSE_TIMEOUT = 1000
};

#define PRINT_ERROR(e) do { \
  cout << "file: " << __FILE__ << " line: " << __LINE__ << " msg: " << e.what() << endl; \
} while(0);

#endif //ROCKETMQ_PROXY_COMMON_H
