//
// Created by hexi on 2020/4/4.
//

class ServerImpl final {
public:
    ~ServerImpl() {
      server_->Shutdown();
      cq_->Shutdown();
    }

    ServerImpl(string host, int port, string nameServer, string accessKey, string secretKey, string accessChannel) :
        host_(host), port_(port), nameServerHost_(nameServer), accessKey_(accessKey), secretKey_(secretKey),
        accessChannel_(accessChannel) {

    };
    string host_;
    int port_;
    string nameServerHost_;
    string accessKey_;
    string secretKey_;
    string accessChannel_;

    void Run() {
      string address = host_ + ":" + to_string(port_);
      std::string server_address(address);
      ServerBuilder builder;
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&service_);
      cq_ = builder.AddCompletionQueue();
      server_ = builder.BuildAndStart();
      std::cout << "Server listening on " << server_address << std::endl;

      HandleRpcs();
    }

private:
    void HandleRpcs() {
      new ProduceCallData(&service_, cq_.get());
      new ConsumeCallData(&service_, cq_.get());
      new ConsumeAckCallData(&service_, cq_.get());
      void *tag;
      bool ok;
      while (true) {
        GPR_ASSERT(cq_->Next(&tag, &ok));
        GPR_ASSERT(ok);
        switch (static_cast<CallDataBase *>(tag)->getType()) {
          case REQUEST_PRODUCE:
            static_cast<ProduceCallData *>(tag)->Proceed();
            break;
          case REQUEST_CONSUME:
            static_cast<ConsumeCallData *>(tag)->Proceed();
            break;
          case REQUEST_CONSUME_ACK:
            static_cast<ConsumeAckCallData *>(tag)->Proceed();
            break;
        }
      }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    ProxyServer::AsyncService service_;
    std::unique_ptr<Server> server_;
};
