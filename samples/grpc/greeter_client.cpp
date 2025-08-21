// Time-stamp: <Last changed 2025-08-22 00:05:55 by magnolia>
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
 *   \file greeter_client.cpp
 *   \brief A simplest gRPC client.
 *
 *  Create the gRPC client and make some RPC calls to the server.
 *
*/

#include <atomic>
#include <cstdio>
#include <csignal>

#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>
#include <string>

#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

#include "tec/grpc/tec_grpc.hpp"
#include "tec/tec_print.hpp"
#include "tec/grpc/tec_grpc_client.hpp"


using helloworld::HelloReply;
using helloworld::HelloRequest;
using helloworld::Greeter;


// Instantiate gRPC Client parameters.
struct MyParams: public tec::GrpcClientParams {
    // You can add custom parameters here.
};

// Instantiate Client traits.
using TClientTraits = tec::grpc_client_traits<
    Greeter
    , grpc::Channel
    , grpc::ChannelCredentials
    , grpc::ChannelArguments
    , grpc_compression_algorithm
    >;

// Instantiate Client.
using BaseClient = tec::GrpcClient<MyParams, TClientTraits>;


// Client implementation.
class MyClient: public BaseClient {
public:
    MyClient(const MyParams& params,
             const std::shared_ptr<grpc::ChannelCredentials>& credentials
        )
        : BaseClient(params, {&grpc::CreateCustomChannel}, credentials)
    {}

    // Send a request to the server.
    std::string SayHello(const std::string& user) {
        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain gRPC behavior.
        grpc::ClientContext context;

        // The actual RPC.
        std::string postfix;
        auto status = stub_->SayHello(&context, request, &reply);
        if (status.ok()) {
            postfix = tec::format(" #{}", tec::get_server_metadata(context, "req_num"));
        }

        return reply.message() + postfix;
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::atomic_bool quit{false};

int main()
{
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){quit.store(true);});

    // Create a client.
    MyParams params;
    MyClient client(params, grpc::InsecureChannelCredentials());

    // Connect to the gRPC server.
    auto result = client.connect();
    if( !result ) {
        tec::println("Abnormally exited with {}.", result);
        return result.code.value_or(tec::Error::Code<>::Unspecified);
    }

    // Make a request the the server.
    std::string user{"world"};
    while( !quit.load() ) {
        // Make a call and print a result.
        tec::println("{} ->", user);
        auto response = client.SayHello(user);
        tec::println("<- {}", response);

        tec::println("\nPRESS <Return> TO REPEAT THE REQUEST...");
        tec::println("PRESS <Ctrl-C> THEN <Return> TO QUIT");
        getchar();
    }

    // Clean up.
    client.close();

    tec::println("Exited with {}.", result);
    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
