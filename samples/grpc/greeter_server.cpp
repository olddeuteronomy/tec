/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2024 The Emacs Cat (https://github.com/olddeuteronomy/tec).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

/**
 *   \file greeter_server.cpp
 *   \brief A Documented file.
 *
 *  Detailed description
 *
*/

#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>
#include <memory>

#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

#include "tec/grpc/tec_grpc_server.hpp"


using grpc::Status;
using grpc::ServerContext;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;


// Instantiate gRPC service - logic and data behind the server's behavior.
class MyService final : public Greeter::Service
{
    Status SayHello(ServerContext* context, const HelloRequest* request, HelloReply* reply) override
    {
        TEC_ENTER("Greeter::SayHello");
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        TEC_TRACE("request.name=\"%\".\n", request->name());
        return Status::OK;
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Instatiate Server traits.
using MyServerTraits = tec::grpc_server_traits<
    MyService
    , grpc::Server
    , grpc::ServerBuilder
    , grpc::ServerCredentials
    , grpc_compression_algorithm
    , grpc_compression_level
    >;

// Instantiate gRPC server parameters.
struct MyServerParams: public tec::GrpcServerParams {
    // Add custom parameters here.
};

// Instantiate gRPC Server.
using MyServer = tec::GrpcServer<MyServerParams, MyServerTraits>;

// Instantiate gRPC Worker.
using MyServerWorker = tec::ServerWorker<MyServerParams, MyServer>;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEC_DECLARE_TRACER()


std::unique_ptr<tec::Daemon> build_daemon(const MyServerParams& params) {
    MyServerParams params_ = params;
    params_.health_check_builder = {&grpc::EnableDefaultHealthCheckService};
    params_.reflection_builder = {&grpc::reflection::InitProtoReflectionServerBuilderPlugin};

    std::unique_ptr<MyServer> server(new MyServer(
        params_,
        grpc::InsecureServerCredentials()));

    // Return the worker.
    return std::unique_ptr<tec::Daemon>(new MyServerWorker(params_, std::move(server)));
}


int main() {
    // Build the daemon.
    MyServerParams params;
    tec::DaemonBuilder<MyServerParams> builder({build_daemon});
    std::unique_ptr<tec::Daemon> daemon(builder(params));

    // Run the daemon
    daemon->create();
    auto result = daemon->run();
    if( !result ) {
        std::cout << "ErrCode=" << result.code.value() << " (" << result.desc.value_or("Unknown") << ")." << std::endl;
    }

    // Wait for <Enter> key pressed to terminate the server.
    getchar();

    return daemon->terminate().code.value_or(-1);
}
