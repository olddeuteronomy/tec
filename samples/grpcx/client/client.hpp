
#include <memory>

#include "tec/tec_client.hpp"
#include "tec/grpc/tec_grpc.hpp"


// Instantiate gRPC Client parameters.
struct MyParams: public tec::GrpcClientParams {
    // You can add custom parameters here.
};


// Test request/reply.
struct TestHelloRequest {
    std::string name;
};

struct TestHelloReply {
    std::string message;
};

// Client builder.
std::unique_ptr<tec::Client> build_client(const MyParams&);
