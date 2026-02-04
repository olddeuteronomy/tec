
#include "tec/tec_daemon.hpp"
#include "tec/grpc/tec_grpc.hpp"


// Instantiate gRPC Client parameters.
struct ClientParams: public tec::GrpcClientParams {
    // You can add custom parameters here.
};


// Test request/reply.
struct TestHelloRequest {
    std::string name;
};

struct TestHelloReply {
    std::string message;
};


// Client builder -- implemented in client.cpp
std::unique_ptr<tec::Daemon> build_client(const ClientParams&);
