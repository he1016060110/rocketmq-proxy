#### XHJ ROCKETMQ PROXY SERVER  
由于rocketmq在性能和功能上比其他消息队列都优秀，我们选择了rocketmq作为享换机消息队列。  
由于我们技术栈是PHP，rocketmq对PHP没有很好的支持。我们面临三种选择：
- 根据c++sdk封装php扩展
- 根据php实现rocketmq通信协议
- 封装rocketmq代理

第一种方法经过验证不可行，因为c++sdk采用多线程的方式编写，php无法接入，会出现致命bug。  
第二种方法代价较为大。  
第三种方法代价比较小，只需要在c++ sdk上封装一个server即可。  
经过权衡，我们采用了第三种方法。

##### server协议
由于http协议会有大量的包传header信息，没有长连接，用http做高并发的应用并不合适。  
于是我们面临选择，是选用一个公用协议，还是自己封装一层协议。  
我们选择了websocket协议作为server和服务端的通信协议，主要由于它足够common并且简单。  
我们选择的开源 [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server) 作为server框架。
没有使用grpc、thrift或brpc等是因为它足够简单，遇到问题好修改，而且也足够稳定（这也很重要）。

#### 使用软件

- [rocketmq-client-cpp](https://github.com/apache/rocketmq-client-cpp)
- [Simple-WebSocket-Server](https://gitlab.com/eidheim/Simple-WebSocket-Server)
- [boost-1.58.0](https://www.boost.org/)

#### 安装

