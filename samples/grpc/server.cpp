// Time-stamp: <Last changed 2026-02-08 01:25:17 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

// stdlib++
#include <memory>
#include <csignal>
#include <string>

// gRPC headers.

// TEC library optionally uses the gRPC framework (https://grpc.io/).
// If used, gRPC is
// Copyright 2015-2024 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

// Protobuf generated.
#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

// TEC
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

// Instantiate gRPC Server ActorWorker.
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
