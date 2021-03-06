// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: Proxy.proto
#ifndef GRPC_Proxy_2eproto__INCLUDED
#define GRPC_Proxy_2eproto__INCLUDED

#include "Proxy.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace grpc_impl {
class CompletionQueue;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc_impl

namespace grpc {
namespace experimental {
template <typename RequestT, typename ResponseT>
class MessageAllocator;
}  // namespace experimental
}  // namespace grpc

namespace Proxy {

class RMQProxy final {
 public:
  static constexpr char const* service_full_name() {
    return "Proxy.RMQProxy";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::Proxy::ProduceReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>> AsyncProduce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>>(AsyncProduceRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>> PrepareAsyncProduce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>>(PrepareAsyncProduceRaw(context, request, cq));
    }
    virtual ::grpc::Status Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::Proxy::ConsumeReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>> AsyncConsume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>>(AsyncConsumeRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>> PrepareAsyncConsume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>>(PrepareAsyncConsumeRaw(context, request, cq));
    }
    virtual ::grpc::Status ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::Proxy::ConsumeAckReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>> AsyncConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>>(AsyncConsumeAckRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>> PrepareAsyncConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>>(PrepareAsyncConsumeAckRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      virtual void Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest* request, ::Proxy::ProduceReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Produce(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ProduceReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest* request, ::Proxy::ProduceReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      virtual void Produce(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ProduceReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      virtual void Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest* request, ::Proxy::ConsumeReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Consume(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest* request, ::Proxy::ConsumeReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      virtual void Consume(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      virtual void ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest* request, ::Proxy::ConsumeAckReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void ConsumeAck(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeAckReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest* request, ::Proxy::ConsumeAckReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
      virtual void ConsumeAck(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeAckReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) = 0;
    };
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>* AsyncProduceRaw(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ProduceReply>* PrepareAsyncProduceRaw(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>* AsyncConsumeRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeReply>* PrepareAsyncConsumeRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>* AsyncConsumeAckRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::Proxy::ConsumeAckReply>* PrepareAsyncConsumeAckRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::Proxy::ProduceReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>> AsyncProduce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>>(AsyncProduceRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>> PrepareAsyncProduce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>>(PrepareAsyncProduceRaw(context, request, cq));
    }
    ::grpc::Status Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::Proxy::ConsumeReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>> AsyncConsume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>>(AsyncConsumeRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>> PrepareAsyncConsume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>>(PrepareAsyncConsumeRaw(context, request, cq));
    }
    ::grpc::Status ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::Proxy::ConsumeAckReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>> AsyncConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>>(AsyncConsumeAckRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>> PrepareAsyncConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>>(PrepareAsyncConsumeAckRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest* request, ::Proxy::ProduceReply* response, std::function<void(::grpc::Status)>) override;
      void Produce(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ProduceReply* response, std::function<void(::grpc::Status)>) override;
      void Produce(::grpc::ClientContext* context, const ::Proxy::ProduceRequest* request, ::Proxy::ProduceReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      void Produce(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ProduceReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      void Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest* request, ::Proxy::ConsumeReply* response, std::function<void(::grpc::Status)>) override;
      void Consume(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeReply* response, std::function<void(::grpc::Status)>) override;
      void Consume(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest* request, ::Proxy::ConsumeReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      void Consume(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      void ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest* request, ::Proxy::ConsumeAckReply* response, std::function<void(::grpc::Status)>) override;
      void ConsumeAck(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeAckReply* response, std::function<void(::grpc::Status)>) override;
      void ConsumeAck(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest* request, ::Proxy::ConsumeAckReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
      void ConsumeAck(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::Proxy::ConsumeAckReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) override;
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>* AsyncProduceRaw(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::Proxy::ProduceReply>* PrepareAsyncProduceRaw(::grpc::ClientContext* context, const ::Proxy::ProduceRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>* AsyncConsumeRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeReply>* PrepareAsyncConsumeRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>* AsyncConsumeAckRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::Proxy::ConsumeAckReply>* PrepareAsyncConsumeAckRaw(::grpc::ClientContext* context, const ::Proxy::ConsumeAckRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_Produce_;
    const ::grpc::internal::RpcMethod rpcmethod_Consume_;
    const ::grpc::internal::RpcMethod rpcmethod_ConsumeAck_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status Produce(::grpc::ServerContext* context, const ::Proxy::ProduceRequest* request, ::Proxy::ProduceReply* response);
    virtual ::grpc::Status Consume(::grpc::ServerContext* context, const ::Proxy::ConsumeRequest* request, ::Proxy::ConsumeReply* response);
    virtual ::grpc::Status ConsumeAck(::grpc::ServerContext* context, const ::Proxy::ConsumeAckRequest* request, ::Proxy::ConsumeAckReply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Produce() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestProduce(::grpc::ServerContext* context, ::Proxy::ProduceRequest* request, ::grpc::ServerAsyncResponseWriter< ::Proxy::ProduceReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Consume() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestConsume(::grpc::ServerContext* context, ::Proxy::ConsumeRequest* request, ::grpc::ServerAsyncResponseWriter< ::Proxy::ConsumeReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_ConsumeAck() {
      ::grpc::Service::MarkMethodAsync(2);
    }
    ~WithAsyncMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestConsumeAck(::grpc::ServerContext* context, ::Proxy::ConsumeAckRequest* request, ::grpc::ServerAsyncResponseWriter< ::Proxy::ConsumeAckReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(2, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_Produce<WithAsyncMethod_Consume<WithAsyncMethod_ConsumeAck<Service > > > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_Produce() {
      ::grpc::Service::experimental().MarkMethodCallback(0,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ProduceRequest, ::Proxy::ProduceReply>(
          [this](::grpc::ServerContext* context,
                 const ::Proxy::ProduceRequest* request,
                 ::Proxy::ProduceReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   return this->Produce(context, request, response, controller);
                 }));
    }
    void SetMessageAllocatorFor_Produce(
        ::grpc::experimental::MessageAllocator< ::Proxy::ProduceRequest, ::Proxy::ProduceReply>* allocator) {
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ProduceRequest, ::Proxy::ProduceReply>*>(
          ::grpc::Service::experimental().GetHandler(0))
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_Consume() {
      ::grpc::Service::experimental().MarkMethodCallback(1,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ConsumeRequest, ::Proxy::ConsumeReply>(
          [this](::grpc::ServerContext* context,
                 const ::Proxy::ConsumeRequest* request,
                 ::Proxy::ConsumeReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   return this->Consume(context, request, response, controller);
                 }));
    }
    void SetMessageAllocatorFor_Consume(
        ::grpc::experimental::MessageAllocator< ::Proxy::ConsumeRequest, ::Proxy::ConsumeReply>* allocator) {
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ConsumeRequest, ::Proxy::ConsumeReply>*>(
          ::grpc::Service::experimental().GetHandler(1))
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithCallbackMethod_ConsumeAck() {
      ::grpc::Service::experimental().MarkMethodCallback(2,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ConsumeAckRequest, ::Proxy::ConsumeAckReply>(
          [this](::grpc::ServerContext* context,
                 const ::Proxy::ConsumeAckRequest* request,
                 ::Proxy::ConsumeAckReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   return this->ConsumeAck(context, request, response, controller);
                 }));
    }
    void SetMessageAllocatorFor_ConsumeAck(
        ::grpc::experimental::MessageAllocator< ::Proxy::ConsumeAckRequest, ::Proxy::ConsumeAckReply>* allocator) {
      static_cast<::grpc_impl::internal::CallbackUnaryHandler< ::Proxy::ConsumeAckRequest, ::Proxy::ConsumeAckReply>*>(
          ::grpc::Service::experimental().GetHandler(2))
              ->SetMessageAllocator(allocator);
    }
    ~ExperimentalWithCallbackMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  typedef ExperimentalWithCallbackMethod_Produce<ExperimentalWithCallbackMethod_Consume<ExperimentalWithCallbackMethod_ConsumeAck<Service > > > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Produce() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Consume() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_ConsumeAck() {
      ::grpc::Service::MarkMethodGeneric(2);
    }
    ~WithGenericMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Produce() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestProduce(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Consume() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestConsume(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_ConsumeAck() {
      ::grpc::Service::MarkMethodRaw(2);
    }
    ~WithRawMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestConsumeAck(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(2, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_Produce() {
      ::grpc::Service::experimental().MarkMethodRawCallback(0,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->Produce(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithRawCallbackMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void Produce(::grpc::ServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_Consume() {
      ::grpc::Service::experimental().MarkMethodRawCallback(1,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->Consume(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithRawCallbackMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void Consume(::grpc::ServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    ExperimentalWithRawCallbackMethod_ConsumeAck() {
      ::grpc::Service::experimental().MarkMethodRawCallback(2,
        new ::grpc_impl::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->ConsumeAck(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithRawCallbackMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void ConsumeAck(::grpc::ServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_Produce : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_Produce() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::Proxy::ProduceRequest, ::Proxy::ProduceReply>(std::bind(&WithStreamedUnaryMethod_Produce<BaseClass>::StreamedProduce, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_Produce() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status Produce(::grpc::ServerContext* /*context*/, const ::Proxy::ProduceRequest* /*request*/, ::Proxy::ProduceReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedProduce(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::Proxy::ProduceRequest,::Proxy::ProduceReply>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_Consume : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_Consume() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::StreamedUnaryHandler< ::Proxy::ConsumeRequest, ::Proxy::ConsumeReply>(std::bind(&WithStreamedUnaryMethod_Consume<BaseClass>::StreamedConsume, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_Consume() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status Consume(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeRequest* /*request*/, ::Proxy::ConsumeReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedConsume(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::Proxy::ConsumeRequest,::Proxy::ConsumeReply>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_ConsumeAck : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_ConsumeAck() {
      ::grpc::Service::MarkMethodStreamed(2,
        new ::grpc::internal::StreamedUnaryHandler< ::Proxy::ConsumeAckRequest, ::Proxy::ConsumeAckReply>(std::bind(&WithStreamedUnaryMethod_ConsumeAck<BaseClass>::StreamedConsumeAck, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_ConsumeAck() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status ConsumeAck(::grpc::ServerContext* /*context*/, const ::Proxy::ConsumeAckRequest* /*request*/, ::Proxy::ConsumeAckReply* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedConsumeAck(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::Proxy::ConsumeAckRequest,::Proxy::ConsumeAckReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_Produce<WithStreamedUnaryMethod_Consume<WithStreamedUnaryMethod_ConsumeAck<Service > > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_Produce<WithStreamedUnaryMethod_Consume<WithStreamedUnaryMethod_ConsumeAck<Service > > > StreamedService;
};

}  // namespace Proxy


#endif  // GRPC_Proxy_2eproto__INCLUDED
