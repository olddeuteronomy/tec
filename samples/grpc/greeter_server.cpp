/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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

#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

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
using MyServerTraits = tec::rpc::grpc_server_traits<
    MyService
    , grpc::Server
    , grpc::ServerBuilder
    , grpc::ServerCredentials>;

// Instantiate gRPC server parameters.
struct MyServerParams: public tec::rpc::GrpcServerParams
{
    // Add custom parameters here.
};

// Instantiate gRPC Server.
using MyServer = tec::rpc::GrpcServer<MyServerParams, MyServerTraits>;

// Instantiate gRPC Worker parameters.
struct MyWorkerParams: public tec::rpc::GrpcWorkerParams
{};

// Instantiate gRPC Worker.
using MyServerWorker = tec::rpc::ServerWorker<MyWorkerParams, MyServer>;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEC_DECLARE_TRACER

int main() {
    // Build the server.
    MyServerParams server_params;
    server_params.health_check_builder = {&grpc::EnableDefaultHealthCheckService};
    server_params.reflection_builder = {&grpc::reflection::InitProtoReflectionServerBuilderPlugin};

    MyServer* server = new MyServer(
        server_params,
        grpc::InsecureServerCredentials());

    // Build the worker.
    MyWorkerParams worker_params;
    MyServerWorker worker(worker_params, server);

    worker.create().run();
    if( !worker.get_result().ok() )
    {
        tec_print("Error code=% (%).\n", worker.get_result().code(), worker.get_result().str());
    }

    getchar();

    worker.terminate();
    return worker.get_exit_code();
}
