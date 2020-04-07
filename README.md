#### XHJ ROCKETMQ PROXY SERVER  
rocketmq在性能和功能上非常优秀，我们选择了rocketmq作为享换机消息队列。  
我们技术栈是PHP，尴尬的是rocketmq对PHP没有很好的支持。我们面临三种选择：
- 根据c++sdk封装php扩展
- 根据php实现rocketmq通信协议
- 封装rocketmq代理

第一种方法经过验证不可行，因为c++sdk采用多线程的方式编写，php无法接入，会出现致命bug。  
第二种方法代价较为大。  
第三种方法代价比较小，只需要在c++ sdk上封装一个server即可。  
经过权衡，我们采用了第三种方法。

##### server协议

使用grpc作为底层rpc框架，因为它成熟且稳定。

#### 使用软件

- [rocketmq-client-cpp](https://github.com/apache/rocketmq-client-cpp)
- [grpc](https://github.com/grpc/grpc)
- [protobuf](https://github.com/protocolbuffers/protobuf)
- [boost-1.58.0](https://www.boost.org/)

#### 安装

推荐使用docker安装，安装命令如下

```
git clone git@github.com:he1016060110/rocketmq-proxy.git
cd rocketmq-proxy
docker build -t rocketmq-proxy:2.0.1 .
```

#### 使用方法

把新创建的image run一个container出来
```
docker run --rm -d --name rocketmq-proxy -it rocketmq-proxy:2.0.1
docker exec -it rocketmq-proxy bash
```

##### Server
制作一个json配置文件，命名为server.json
```
{
  "host":"127.0.0.1",
  "port": 8080,
  "accessKey": "XXXXXXX",//broker配置的acl
  "secretKey": "XXXXX",//broker配置的acl
  "nameServer":"XXXXXXX",//nameserver地址
  "esServer": "http://es.AAAA.com:9200",//发送消息日志地址
  "logFileName": "/root/es.log"//如果没有传es地址，消息日志将保存到本地
}
```

```
Server -f server.json
```

##### Client
Producer
```text
ProducerClient -t topic -h host -g group -m msg (optional) -n mum (optional)
```

Consumer
```text
ConsumerClient -g group -t topic -h host -n mum (optional)
```

#### PHP Usage

##### docker-compose

项目中docker-compose.yaml包含以下组件以及配置

- rocketmq
- rocketmq-console-ng
- rocketmq-proxy
- elasticsearch

```text
docker-compose -f docker-compose.yaml up
```
##### php

```text
cd php
composer install
php Producer.php
php Consumer.php
```

