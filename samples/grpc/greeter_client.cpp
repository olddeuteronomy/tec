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
 *   \file greeter_client.cpp
 *   \brief A simple gRPC client.
 *
 *  Detailed description
 *
*/


#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>

#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"

#include "tec/grpc/tec_grpc_client.hpp"


using helloworld::HelloReply;
using helloworld::HelloRequest;
using helloworld::Greeter;


// Instantiate gRPC Client parameters.
struct MyParams: public tec::GrpcClientParams {
    // Add custom parameters here.
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


class MyClient: public BaseClient {
public:
    MyClient(const MyParams& params, const grpc::ChannelArguments& arguments)
        : BaseClient(params, {&grpc::CreateCustomChannel}, grpc::InsecureChannelCredentials(), arguments)
    {}

    std::string SayHello(const std::string& user) {
        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloReply reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        grpc::ClientContext context;

        // The actual RPC.
        grpc::Status status = stub_->SayHello(&context, request, &reply);

        return reply.message();
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEC_DECLARE_TRACER()

int main()
{
    grpc::ChannelArguments arguments;
    MyParams params;
    MyClient client(params, arguments);

    auto result = client.connect();
    if( !result ) {
        tec_print("Error: %.\n", result.str());
        return result.code();
    }

    auto val = client.SayHello("world!");
    tec_print("<- %\n", val);

    client.close();
    return result.code();
}
