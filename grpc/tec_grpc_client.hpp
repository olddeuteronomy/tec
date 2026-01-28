// Time-stamp: <Last changed 2026-01-28 16:24:28 by magnolia>
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
 * @file tec_grpc_client.hpp
 * @brief gRPC client template.
 * @author The Emacs Cat
 * @date 2026-01-28
 *
 * Declares a flexible, actor-based gRPC client template that can be specialized
 * for different gRPC services via template parameters.
 * Provides channel creation, connection management, compression and message size
 * configuration, and serves as a base class for concrete service clients.
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_actor.hpp"
#include "tec/grpc/tec_grpc.hpp" // IWYU pragma: keep


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                      gRPC Client traits
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct grpc_client_traits
 * @brief Type traits / type aliases collection for configuring a gRPC client.
 *
 * This structure is used to group and alias all the gRPC-related types that
 * a particular GrpcClient instantiation will use.
 * It allows clean decoupling between the client implementation and the
 * concrete gRPC service/channel/credentials types.
 *
 * @tparam TService                The generated gRPC service class (e.g. `MyService::Stub`)
 * @tparam TGrpcChannel            Channel type (usually `grpc::Channel`)
 * @tparam TGrpcClientCredentials  Credentials type (usually `grpc::ChannelCredentials`)
 * @tparam TGrpcChannelArguments   Channel creation arguments (usually `grpc::ChannelArguments`)
 * @tparam TGrpcCompressionAlgorithm Compression algorithm enum (usually `grpc::CompressionAlgorithm`)
 */
template <
    typename TService,
    typename TGrpcChannel,
    typename TGrpcClientCredentials,
    typename TGrpcChannelArguments,
    typename TGrpcCompressionAlgorithm
    >
struct grpc_client_traits {
    typedef TService                Service;               ///< gRPC service stub type
    typedef TGrpcChannel            Channel;               ///< gRPC channel type
    typedef TGrpcClientCredentials  Credentials;           ///< credentials factory type
    typedef TGrpcChannelArguments   Arguments;             ///< channel creation arguments
    typedef TGrpcCompressionAlgorithm CompressionAlgorithm; ///< supported compression algorithms enum
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          gRPC client
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @class GrpcClient
 * @brief Base class for actor-style asynchronous gRPC clients.
 *
 * Provides common infrastructure for creating and managing a gRPC channel,
 * configuring message size limits and compression, connecting with timeout,
 * and creating a service stub.
 *
 * Concrete clients should inherit from this class and implement
 * `process_request()` to perform actual RPC calls.
 *
 * @tparam TParams  User-defined parameter structure (must derive from `GrpcClientParams`)
 * @tparam Traits   Instance of `grpc_client_traits<...>` defining all gRPC-related types
 */
template <typename TParams, typename Traits>
class GrpcClient : public Actor {
public:
    typedef Traits   traits;               ///< Traits providing all gRPC-related type aliases
    typedef TParams  Params;               ///< Configuration parameters type
    typedef typename traits::Service                Service;               ///< gRPC service stub type
    typedef typename traits::Channel                Channel;               ///< gRPC channel type
    typedef typename traits::Credentials            Credentials;           ///< credentials type
    typedef typename traits::Arguments              Arguments;             ///< channel arguments type
    typedef typename traits::CompressionAlgorithm   CompressionAlgorithm;  ///< compression enum type

    /**
     * @struct ChannelBuilder
     * @brief Holder for a function pointer that creates a gRPC channel.
     *
     * Abstracts away the exact channel creation function (`grpc::CreateChannel`, `CreateChannelWithInterceptor`, etc.)
     * so different channel factories can be injected at construction time.
     */
    struct ChannelBuilder {
        /// Function pointer matching signature of grpc::CreateChannel / CreateCustomChannel / etc.
        std::shared_ptr<Channel> (*fptr)(const std::string&,
                                        const std::shared_ptr<Credentials>&,
                                        const Arguments&);
    };

protected:
    // ── Configuration & state ───────────────────────────────────────────────

    /// Runtime configuration parameters (address, timeouts, message size, compression, etc.)
    Params params_;

    /// Credentials object used to create secure/insecure channels
    std::shared_ptr<Credentials> credentials_;

    /// Owned stub instance — created after successful channel connection
    std::unique_ptr<typename Service::Stub> stub_;

    /// Channel creation functor (injected at construction)
    ChannelBuilder channel_builder_;

    /// Shared pointer to the active gRPC channel
    std::shared_ptr<Channel> channel_;

    /// Channel creation arguments (message size limits, compression, keepalive, etc.)
    Arguments arguments_;

protected:
    /**
     * @brief Configures channel arguments before channel creation.
     *
     * Called automatically during `start()`.
     * Sets maximum send/receive message sizes and compression algorithm
     * according to values in `params_`.
     *
     * Can be overridden in derived classes to set additional channel arguments
     * (e.g. keep-alive, HTTP/2 settings, custom interceptors, etc.).
     */
    virtual void set_channel_arguments() {
        TEC_ENTER("GrpcClient::set_channel_arguments");

        // Maximum message size, see tec::kGrpcMaxMessageSize
        if (params_.max_message_size > 0) {
            // Convert MB → bytes
            const int max_size = params_.max_message_size * 1024 * 1024;
            arguments_.SetMaxSendMessageSize(max_size);
            arguments_.SetMaxReceiveMessageSize(max_size);
        }
        TEC_TRACE("MaxMessageSize is set to {} Mb.", params_.max_message_size);

        // Compression algorithm
        // GRPC_COMPRESS_NONE = 0, GRPC_COMPRESS_DEFLATE, GRPC_COMPRESS_GZIP, ...
        if (params_.compression_algorithm > 0) {
            arguments_.SetCompressionAlgorithm(static_cast<CompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);
    }

public:
    /**
     * @brief Constructs a GrpcClient instance.
     *
     * @param params          Configuration parameters (address, timeouts, limits…)
     * @param channel_builder Functor responsible for creating the gRPC channel
     * @param credentials     Credentials to use (insecure / SSL / custom)
     */
    GrpcClient(const Params& params,
               const ChannelBuilder& channel_builder,
               const std::shared_ptr<Credentials>& credentials)
        : Actor()
        , params_{params}
        , channel_builder_{channel_builder}
        , credentials_{credentials}
    {
        static_assert(
            std::is_base_of<GrpcClientParams, Params>::value,
            "Must be derived from tec::GrpcClientParams class");
    }

    /// Virtual destructor (defaulted)
    virtual ~GrpcClient() = default;

    /**
     * @brief Establishes connection to the gRPC server.
     *
     * 1. Configures channel arguments
     * 2. Creates the channel using the injected ChannelBuilder
     * 3. Waits for connection with timeout
     * 4. Creates service stub on success
     *
     * @param sig_started Signal to notify when initialization completes
     * @param status      [out] Connection result status
     */
    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("GrpcClient::start");
        Signal::OnExit on_exit(sig_started);

        // Prepare channel creation arguments
        set_channel_arguments();

        // Create channel (may return "lame" channel on error)
        channel_ = channel_builder_.fptr(params_.addr_uri, credentials_, arguments_);
        TEC_TRACE("Connecting to {} ...", params_.addr_uri);

        // Wait for connection with timeout
        auto deadline = std::chrono::system_clock::now() + params_.connect_timeout;
        if (!channel_->WaitForConnected(deadline)) {
            std::string msg{format(
                    "It took too long (> {} ms) to reach out the server on \"{}\"",
                    MilliSec{params_.connect_timeout}.count(), params_.addr_uri)};
            TEC_TRACE("!!! Error: {}.", msg);
            *status = {msg, Error::Kind::NetErr};
            return;
        }

        // Create service stub
        stub_ = Service::NewStub(channel_);
        TEC_TRACE("connected to {} OK.", params_.addr_uri);
    }

    /**
     * @brief Graceful shutdown hook.
     *
     * Currently only logs; derived classes may override to drain queues,
     * cancel pending RPCs, etc.
     *
     * @param sig_stopped Signal to notify when shutdown is complete
     */
    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("GrpcClient::shutdown");
        Signal::OnExit on_exit{sig_stopped};
        TEC_TRACE("closed OK.");
    }

    /**
     * @brief Processes a single request → reply pair.
     *
     * Pure virtual in the base class — **must** be implemented by derived classes.
     * This is where actual RPC calls (unary, client-streaming, server-streaming, bidi)
     * should be performed.
     *
     * @param request  Input request message
     * @param reply    Output reply message (to be filled)
     * @return Status  Success or detailed error information
     */
    virtual Status process_request(Request request, Reply reply) override {
        // Must be implemented in derived class.
        return {Error::Kind::NotImplemented};
    }
}; // class GrpcClient

} // ::tec
