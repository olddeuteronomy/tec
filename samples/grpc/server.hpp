
#include <memory>

#include "tec/tec_daemon.hpp"
#include "tec/grpc/tec_grpc.hpp"


// Instantiate gRPC server parameters.
struct ServerParams: public tec::GrpcServerParams {
    // You can add custom parameters here.
};


std::unique_ptr<tec::Daemon> build_server(const ServerParams& _params);
