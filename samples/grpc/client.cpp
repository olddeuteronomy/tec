// Time-stamp: <Last changed 2026-02-08 02:12:47 by magnolia>
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
#include <cstdio>
#include <csignal>
#include <memory>
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

// Protobuf generated
#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

// TEC
#include "tec/tec_daemon.hpp"
#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_message.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/grpc/tec_grpc_client.hpp"

#include "client.hpp"


// Instantiate Client traits.
using ClientTraits = tec::grpc_client_traits<
    helloworld::Greeter
    , grpc::Channel
    , grpc::ChannelCredentials
    , grpc::ChannelArguments
    , grpc_compression_algorithm
    >;

// Instantiate a Client.
using BaseClient = tec::GrpcClient<ClientParams, ClientTraits>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Client implementation
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class Client final: public BaseClient {
public:
    Client(const ClientParams& params,
           const std::shared_ptr<grpc::ChannelCredentials>& credentials
        )
        : BaseClient(params, {&grpc::CreateCustomChannel}, credentials)
    {}

    tec::Status process_request(tec::Request _request, tec::Reply _reply) override {
        // Check type compatibility.
        // NOTE: *Request* is const!
        // NOTE: Both _request and _reply contain **pointers** to actual data.

        if( _request.type() == typeid(const TestHelloRequest*) &&
            _reply.type() == typeid(TestHelloReply*) ) {
            // Get pointers to actual data.
            auto req = std::any_cast<const TestHelloRequest*>(_request);
            auto rep = std::any_cast<TestHelloReply*>(_reply);
            return call(req, rep);
        }

        return {tec::Error::Kind::Unsupported};
    }

private:
    tec::Status call(const TestHelloRequest* _request, TestHelloReply* _reply) {
        // Data we are sending to the server.
        helloworld::HelloRequest request;
        request.set_name(_request->name);

        // Container for the data we expect from the server.
        helloworld::HelloReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain gRPC behavior.
        grpc::ClientContext context;

        // The actual RPC.
        auto status = stub_->SayHello(&context, request, &reply);
        if( !status.ok() ) {
            return {status.error_code(), status.error_message(), tec::Error::Kind::RpcErr};
        }
        // Get server metadata.
        std::string postfix = tec::format(" #{}", tec::get_server_metadata(context, "req_num"));

        // Fill the reply.
        _reply->message = reply.message() + postfix;

        return {};
    }

}; // class Client

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Client Builder
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using ClientWorker = tec::ActorWorker<ClientParams, Client>;

std::unique_ptr<tec::Daemon> build_client(const ClientParams& params) {
    auto actor{std::make_unique<Client>(params, grpc::InsecureChannelCredentials())};
    return std::make_unique<ClientWorker>(params, std::move(actor));
}
