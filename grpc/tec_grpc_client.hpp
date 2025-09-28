// Time-stamp: <Last changed 2025-09-28 16:12:21 by magnolia>
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
 *   @file tec_grpc_client.hpp
 *   @brief gRPC client implementation.
 *
 *  Declares a gRPC client.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_client.hpp"
#include "tec/grpc/tec_grpc.hpp" // IWYU pragma: keep


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                      gRPC Client traits
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! gRPC client definition traits.
template <
    typename TService,
    typename TGrpcChannel,
    typename TGrpcClientCredentials,
    typename TGrpcChannelArguments,
    typename TGrpcCompressionAlgorithm
    >
struct grpc_client_traits {
    typedef TService Service;
    typedef TGrpcChannel Channel;
    typedef TGrpcClientCredentials Credentials;
    typedef TGrpcChannelArguments Arguments;
    typedef TGrpcCompressionAlgorithm CompressionAlgorithm;

};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          gRPC client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TParams, typename Traits>
class GrpcClient: public Client {
public:

    typedef Traits traits;
    typedef TParams Params;
    typedef typename traits::Service Service;
    typedef typename traits::Channel Channel;
    typedef typename traits::Credentials Credentials;
    typedef typename traits::Arguments Arguments;
    typedef typename traits::CompressionAlgorithm CompressionAlgorithm;

    //! Declares a pointer to CreateChannel() gRPC function.
    struct ChannelBuilder {
        std::shared_ptr<Channel> (*fptr)(const std::string&, const std::shared_ptr<Credentials>&, const Arguments&);
    };

protected:

    //! Custom parameters - must be inherited from ClientParams.
    Params params_;

    // gRPC-specific stuff.
    std::shared_ptr<Credentials> credentials_;
    std::unique_ptr<typename Service::Stub> stub_;
    ChannelBuilder channel_builder_;
    std::shared_ptr<Channel> channel_;
    Arguments arguments_;

protected:

    //! Set grpc::ChannelArgiments *before* creating a channel. Can be overwritten.
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
            arguments_.SetCompressionAlgorithm(static_cast<CompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);
    }

public:
    //! Constructs a GrpcClient.
    GrpcClient(const Params& params,
               const ChannelBuilder& channel_builder,
               const std::shared_ptr<Credentials>& credentials
        )
        : params_{params}
        , channel_builder_{channel_builder}
        , credentials_{credentials}
    {
        static_assert(
            std::is_base_of<ClientParams, Params>::value,
            "not derived from tec::ClientParams class");
    }

    virtual ~GrpcClient() = default;


    /**
     *  @brief Connect to the gRPC server.
     *
     *  1) Sets gRPC channel arguments as specified in *params*.
     *
     *  2) Creates the gRPC channel.
     *
     *  3) Connects to the server using *addr_uri* and *connect_timeout*
     *  provided in *params*.
     *
     *  @return Status
     */
    Status connect() override {
        TEC_ENTER("GrpcClient::connect");

        // Set channel arguments.
        set_channel_arguments();

        // Create a channel.
        // If failed, a lame channel (one on which all operations fail) is created.
        channel_ = channel_builder_.fptr(params_.addr_uri, credentials_, arguments_);

        TEC_TRACE("Connecting to {} ...", params_.addr_uri);

        // Connect to the server with timeout.
        auto deadline = std::chrono::system_clock::now() + params_.connect_timeout;
        if (!channel_->WaitForConnected(deadline)) {
            std::string msg{format(
                    "It took too long (> {} ms) to reach out the server on \"{}\"",
                    MilliSec{params_.connect_timeout}.count(), params_.addr_uri)};
            TEC_TRACE("!!! Error: {}.", msg);
            return {msg, Error::Kind::RpcErr};
        }

        // Create a stub.
        stub_ = Service::NewStub(channel_);
        TEC_TRACE("connected to {} OK.", params_.addr_uri);
        return {};
    }


    void close() override {
        TEC_ENTER("GrpcClient::close");
        TEC_TRACE("closed OK.");
    }


    Status process_request(Request& request, Reply& reply) {
        return {Error::Kind::NotImplemented};
    }
}; // class GrpcClient

} // ::tec
