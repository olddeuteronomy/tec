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
 *   \file tec_grpc_client.hpp
 *   \brief A Documented file.
 *
 *  Detailed description
 *
*/

#pragma once

#include "tec/tec_trace.hpp"
#include "tec/grpc/tec_grpc.hpp" // IWYU pragma: keep
#include "tec/tec_utils.hpp"

namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                      gRPC Client traits
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <
    typename TUserService,
    typename TGrpcChannel,
    typename TGrpcClientCredentials,
    typename TGrpcChannelArguments,
    typename TGrpc_compression_algorithm
    >
struct grpc_client_traits {
    typedef TUserService TService;
    typedef TGrpcChannel TChannel;
    typedef TGrpcClientCredentials TCredentials;
    typedef TGrpcChannelArguments TArguments;
    typedef TGrpc_compression_algorithm TCompressionAlgorithm;

};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          gRPC client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TParams, typename Traits>
class GrpcClient: public Client {
public:
    typedef typename Traits::TService TService;
    typedef typename Traits::TChannel TChannel;
    typedef typename Traits::TCredentials TCredentials;
    typedef typename Traits::TArguments TArguments;
    typedef typename Traits::TCompressionAlgorithm TCompressionAlgorithm;

    // Declare a pointer to CreateChannel function.
    struct ChannelBuilder {
        std::shared_ptr<TChannel> (*fptr)(const std::string&, const std::shared_ptr<TCredentials>&, const TArguments&);
    };

protected:
    // Custom parameters - must be inherited from ClientParams.
    TParams params_;

    // gRPC-specific implementation.
    std::shared_ptr<TCredentials> credentials_;
    std::unique_ptr<typename TService::Stub> stub_;
    ChannelBuilder channel_builder_;
    std::shared_ptr<TChannel> channel_;
    TArguments arguments_;

protected:

    // Sets grpc::ChannelArgiments before creating a channel. Can be overwritten.
    virtual void set_channel_arguments() {
        TEC_ENTER("GrpcClient::set_channel_arguments");

        // Maximum message size, see tec::kGrpcMaxMessageSize
        if (params_.max_message_size > 0) {
            // Set max message size in Mb
            const int max_size = params_.max_message_size * 1024 * 1024;
            arguments_.SetMaxSendMessageSize(max_size);
            arguments_.SetMaxReceiveMessageSize(max_size);
        }
        TEC_TRACE("MaxMessageSize is set to {} Mb.", params_.max_message_size);

        // Compression algorithm
        // GRPC_COMPRESS_NONE = 0, GRPC_COMPRESS_DEFLATE, GRPC_COMPRESS_GZIP, GRPC_COMPRESS_ALGORITHMS_COUNT
        if (params_.compression_algorithm > 0) {
            arguments_.SetCompressionAlgorithm(static_cast<TCompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);
    }

public:
    GrpcClient(const TParams& params,
               const ChannelBuilder& channel_builder,
               const std::shared_ptr<TCredentials>& credentials
        )
        : params_{params}
        , channel_builder_{channel_builder}
        , credentials_{credentials}
    {}

    virtual ~GrpcClient() = default;


    /**
     *  @brief Connect to a gRPC server.
     *
     *  1) Sets gRPC channel arguments as specified in params_.
     *
     *  2) Creates the gRPC channel.
     *
     *  3) Connects to a server using `addr_uri' and `connect_timeout'
     *  provided in `params_'.
     *
     *  @return tec::Result
     */
    Result connect() override {
        TEC_ENTER("GrpcClient::connect");

        // Set channel arguments. Can be overwritten.
        set_channel_arguments();

        // Create a channel.
        // If failed, a lame channel (one on which all operations fail) is created.
        channel_ = channel_builder_.fptr(params_.addr_uri, credentials_, arguments_);

        // Connect to the server with timeout.
        auto deadline = std::chrono::system_clock::now() + params_.connect_timeout;
        if (!channel_->WaitForConnected(deadline)) {
            std::string msg{format(
                    "It took too long (> % ms) to reach out the server on \"{}\"",
                    MilliSec{params_.connect_timeout}.count(), params_.addr_uri)};
            TEC_TRACE("!!! Error: {}.", msg);
            return {msg, Result::Kind::GrpcErr};
        }

        // Create a stub.
        stub_ = TService::NewStub(channel_);
        TEC_TRACE("connected to {} OK.", params_.addr_uri);
        return {};
    }


    void close() override {
        TEC_ENTER("GrpcClient::close");
        TEC_TRACE("closed OK.");
    }

};


} // ::tec
