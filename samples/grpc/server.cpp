// Time-stamp: <Last changed 2026-02-03 14:18:17 by magnolia>
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

#include <memory>
#include <csignal>

#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <string>

#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

#include "tec/tec_print.hpp"
#include "tec/tec_daemon.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/grpc/tec_grpc.hpp"
#include "tec/grpc/tec_grpc_server.hpp"

#include "server.hpp"

using grpc::Status;
using grpc::ServerContext;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             gRPC Service
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static int count{0};

// Implement gRPC service - logic and data behind the server's behavior.
class TestService final : public Greeter::Service {

    Status SayHello(ServerContext *context, const HelloRequest *request, HelloReply *reply) override {
        count += 1;
        tec::println("\nREQUEST #{}: <- \"{}\"", count, request->name());
        // Set reply.
        reply->set_message("Hello " + request->name() + "!");
        // Set metadata.
        tec::add_server_medadata(context, "req_num", std::to_string(count));
        tec::println("REPLY #{}:   -> \"{}\"", count, reply->message());
        return Status::OK;
    }

}; // ::MyService


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Instantiate Server traits.
using ServerTraits = tec::grpc_server_traits<
    TestService
    , grpc::Server
    , grpc::ServerBuilder
    , grpc::ServerCredentials
    , grpc_compression_algorithm
    , grpc_compression_level
    >;


// Instantiate gRPC Server.
class Server: public tec::GrpcServer<ServerParams, ServerTraits> {
public:
    Server(const ServerParams& params)
        : tec::GrpcServer<ServerParams, ServerTraits>{params, grpc::InsecureServerCredentials()}
    {}
};

// Instantiate gRPC Server Worker.
using ServerWorker = tec::ActorWorker<ServerParams, Server>;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              Server builder
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::unique_ptr<tec::Daemon> build_server(const ServerParams& _params) {
    ServerParams params{_params};

    // Set additional gRPC server parameters.
    params.health_check_builder = {&grpc::EnableDefaultHealthCheckService};
    params.reflection_builder = {&grpc::reflection::InitProtoReflectionServerBuilderPlugin};

    return ServerWorker::Builder<ServerWorker, Server>{}(params);
}
