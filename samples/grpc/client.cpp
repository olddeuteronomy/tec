// Time-stamp: <Last changed 2026-02-03 14:39:51 by magnolia>
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
 * @file client.cpp
 * @brief A simplest gRPC client.
 *
 *  Creates a gRPC client and makes some RPC calls to the server.
 *
 */

#include <cstdio>
#include <csignal>
#include <memory>
#include <string>

// gRPC
#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>

// Protobuf
#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

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

}; // class MyClient

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
