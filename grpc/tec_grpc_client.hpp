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
 *   \file tec_grpc_client.hpp
 *   \brief A Documented file.
 *
 *  Detailed description
 *
*/

#pragma once

#include "tec/grpc/tec_grpc.hpp"


namespace tec {

namespace rpc {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                      Generic gRPC client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// gRPC client traits.
template <
    typename TUserService,
    typename TGrpcChannel,
    typename TGrpcClientCredentials
    >
struct grpc_client_traits {
    typedef TUserService TService;
    typedef TGrpcChannel TChannel;
    typedef TGrpcClientCredentials TCredentials;
};

template <typename TParams, typename Traits>
class GrpcClient {
public:
    typedef typename Traits::TService TService;
    typedef typename Traits::TChannel TChannel;
    typedef typename Traits::TCredentials TCredentials;

    // Declare a pointer to CreateChannel function.
    struct ChannelBuilder {
        std::shared_ptr<TChannel> (*fptr)(const std::string&, const std::shared_ptr<TCredentials>&);
    };

protected:
    //! Custom parameters - must be inherited from ClientParams.
    TParams params_;

    //! gRPC-specific implementation.
    std::shared_ptr<TCredentials> credentials_;
    std::unique_ptr<typename TService::Stub> stub_;
    ChannelBuilder channel_builder_;
    std::shared_ptr<TChannel> channel_;

public:
    GrpcClient(const TParams& params,
               const ChannelBuilder& channel_builder,
               const std::shared_ptr<TCredentials>& credentials)
        : params_(params)
        , channel_builder_(channel_builder)
        , credentials_(credentials)
    {}

    virtual ~GrpcClient()
    {}


    virtual tec::Result connect(const ChannelBuilder& builder) {
        TEC_ENTER("Client::connect");

        // Create a channel.
        // If failed, a lame channel (one on which all operations fail) is created.
        channel_ = channel_builder_.fptr(params_.addr_uri, credentials_);
        auto deadline = std::chrono::system_clock::now() + params_.connect_timeout;

        // Connect to the server with timeout.
        if (!channel_->WaitForConnected(deadline))
        {
            std::string msg{tec::format(
                    "It took too long (> % ms) to reach out the server on %",
                    params_.connect_timeout.count(), params_.addr_uri)};
            TEC_TRACE("error: %.\n", msg);
            return {Status::RPC_ERROR_CLIENT_CONNECT, msg};
        }

        // Create a stub.
        stub_ = TService::NewStub(channel_);

        TEC_TRACE("connected to % OK.\n", params_.addr_uri);
        return {};
    }

};


} // ::rpc

} // ::tec
