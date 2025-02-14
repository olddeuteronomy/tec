// Time-stamp: <Last changed 2025-02-15 01:48:41 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2025 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \brief A simplest gRPC server.
 *
 *  Create the gRPC server and run it.
 *
*/

#include <memory>
#include <csignal>

#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

#include "tec/tec_result.hpp"
#include "tec/tec_semaphore.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_server_worker.hpp"
#include "tec/grpc/tec_grpc_server.hpp"


using grpc::Status;
using grpc::ServerContext;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;


// Implement gRPC service - logic and data behind the server's behavior.
class MyService final : public Greeter::Service
{
    Status SayHello(ServerContext* context, const HelloRequest* request, HelloReply* reply) override
    {
        TEC_ENTER("Greeter::SayHello");

        std::string prefix("Hello ");
        reply->set_message(prefix + request->name() + "!");

        TEC_TRACE("request.name=\"{}\".\n", request->name());
        return Status::OK;
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Instantiate Server traits.
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
    // You can add custom parameters here.
};

// Instantiate gRPC Server.
class MyServer: public tec::GrpcServer<MyServerParams, MyServerTraits> {
public:
    MyServer(const MyServerParams& params)
        : tec::GrpcServer<MyServerParams, MyServerTraits>{params, grpc::InsecureServerCredentials()}
    {}
};

// Instantiate gRPC Server Worker.
using MyMessage = tec::WorkerMessage;
using MyWorkerTraits = tec::worker_traits<
    MyServerParams,
    MyMessage
    >;
using MyServerWorker = tec::ServerWorker<MyWorkerTraits, MyServer>;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Signal sig_quit;


tec::Result test() {
    // The gRPC daemon.
    MyServerParams params;

    // Set additional gRPC server parameters.
    params.health_check_builder = {&grpc::EnableDefaultHealthCheckService};
    params.reflection_builder = {&grpc::reflection::InitProtoReflectionServerBuilderPlugin};

    // [TEST]
    // params.start_timeout = tec::MilliSec{1};
    // [TEST]
    // params.addr_uri = "";
    auto daemon{MyServerWorker::DaemonBuilder<MyServerWorker, MyServer>{}(params)};

    // Run the daemon
    auto result = daemon->run();
    if( !result ) {
        tec::println("Abnormal exited with {}.", result);
        return result;
    }

    // Wait for <Ctrl-C> pressed to terminate the server.
    tec::println("\nPRESS <Ctrl-C> TO QUIT THE SERVER");
    sig_quit.wait();

    // Terminate the server.
    result = daemon->terminate();
    return result;
}


int main() {
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){ sig_quit.set(); });

    auto result = test();

    tec::println("\nExited with {}", result);
    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
