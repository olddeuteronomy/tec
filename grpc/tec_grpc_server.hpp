// Time-stamp: <Last changed 2026-02-20 16:19:27 by magnolia>
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
/**
 * @file tec_grpc_server.hpp
 * @brief gRPC server template.
 *
 * Declares a generic, actor-based gRPC server template that can be specialized
 * for different gRPC services via template parameters.
 * Handles server startup, listening port configuration, compression settings,
 * optional health checking and server reflection, and graceful shutdown.
 *
 * @author The Emacs Cat
 * @date 2026-01-28
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_signal.hpp"
#include "tec/tec_utils.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_actor.hpp" // IWYU pragma: keep
#include "tec/grpc/tec_grpc.hpp" // IWYU pragma: keep


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     gRPC Server traits
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct grpc_server_traits
 * @brief Type traits / type aliases collection for configuring a gRPC server.
 *
 * Groups and aliases all gRPC-related types used by a particular `GrpcServer`
 * instantiation, allowing clean decoupling between the server implementation
 * and the concrete gRPC service/server/builder/credentials types.
 *
 * @tparam TService                  The generated gRPC service class (e.g. `MyService`)
 * @tparam TGrpcServer               Server type (usually `grpc::Server`)
 * @tparam TGrpcServerBuilder        Server builder type (usually `grpc::ServerBuilder`)
 * @tparam TGrpcServerCredentials    Server credentials type (usually `grpc::ServerCredentials`)
 * @tparam TGrpcCompressionAlgorithm Compression algorithm enum (usually `grpc::CompressionAlgorithm`)
 * @tparam TGrpcCompressionLevel     Compression level enum (usually `grpc::CompressionLevel`)
 */
template <
    typename TService,
    typename TGrpcServer,
    typename TGrpcServerBuilder,
    typename TGrpcServerCredentials,
    typename TGrpcCompressionAlgorithm,
    typename TGrpcCompressionLevel
    >
struct grpc_server_traits {
    typedef TService                  Service;               ///< gRPC service implementation type
    typedef TGrpcServer               RpcServer;             ///< running gRPC server instance type
    typedef TGrpcServerBuilder        Builder;               ///< server builder type
    typedef TGrpcServerCredentials    Credentials;           ///< server-side credentials type
    typedef TGrpcCompressionAlgorithm CompressionAlgorithm;  ///< supported compression algorithms enum
    typedef TGrpcCompressionLevel     CompressionLevel;      ///< compression level enum
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Generic gRPC Server
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @class GrpcServer
 * @brief Base class for actor-style gRPC servers.
 *
 * Provides common infrastructure for:
 * - configuring server builder (message size limits, compression)
 * - enabling optional features (health checking, reflection)
 * - starting a listening server with timeout-protected startup
 * - graceful shutdown
 *
 * Concrete servers should inherit from this class and implement
 * `process_request()` if synchronous request handling is needed,
 * or override `start()` / use async service registration patterns.
 *
 * @tparam TParams  User-defined parameter structure (must derive from `GrpcServerParams`)
 * @tparam Traits   Instance of `grpc_server_traits<...>` defining all gRPC-related types
 */
template <typename TParams, typename Traits>
class GrpcServer : public Actor {
public:
    typedef TParams  Params;               ///< Configuration parameters type
    typedef typename Traits::Service                Service;               ///< gRPC service type
    typedef typename Traits::RpcServer              RpcServer;             ///< running server instance type
    typedef typename Traits::Builder                Builder;               ///< server builder type
    typedef typename Traits::Credentials            Credentials;           ///< server credentials type
    typedef typename Traits::CompressionAlgorithm   CompressionAlgorithm;  ///< compression algorithm enum
    typedef typename Traits::CompressionLevel       CompressionLevel;      ///< compression level enum

protected:
    // ── Configuration & state ───────────────────────────────────────────────

    /// Runtime configuration parameters (listen address, message size, compression, plugins…)
    Params params_;

    /// Owned server instance — created and started during `start()`
    std::unique_ptr<RpcServer> server_;

    /// Server-side credentials (insecure / SSL / custom)
    std::shared_ptr<Credentials> credentials_;

protected:
    /**
     * @brief Configures optional gRPC server plugins (reflection, health checking).
     *
     * Called automatically during `start()`.
     * Enables server reflection and/or gRPC health checking if the corresponding
     * builder functions were provided in `params_`.
     *
     * Can be overridden in derived classes to register additional interceptors,
     * authentication/authorization plugins, etc.
     */
    virtual void set_plugins() {
        TEC_ENTER("GrpcServer::set_plugins");

        if (params_.health_check_builder.fptr) {
            params_.health_check_builder.fptr(true);
            TEC_TRACE("Health checking enabled.");
        }
        if (params_.reflection_builder.fptr) {
            params_.reflection_builder.fptr();
            TEC_TRACE("Reflection enabled.");
        }
    }

    /**
     * @brief Configures server-wide options on the `ServerBuilder`.
     *
     * Called automatically during `start()`.
     * Sets maximum message sizes and default compression settings
     * according to values in `params_`.
     *
     * Can be overridden in derived classes to set additional builder options
     * (e.g. keep-alive, resource quotas, custom interceptors, etc.).
     *
     * @param builder The `ServerBuilder` instance being configured
     */
    virtual void set_builder_options(Builder& builder) {
        TEC_ENTER("GrpcServer::set_builder_options");

        // Set max message size (both send and receive)
        if (params_.max_message_size > 0) {
            const int max_size = params_.max_message_size * 1024 * 1024;
            builder.SetMaxReceiveMessageSize(max_size);
            builder.SetMaxSendMessageSize(max_size);
        }
        TEC_TRACE("MaxMessageSize is set to {} Mb.", params_.max_message_size);

        // Set default compression algorithm
        // Note: this overrides any compression level set by SetDefaultCompressionLevel
        if (params_.compression_algorithm > 0) {
            builder.SetDefaultCompressionAlgorithm(static_cast<CompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);

        // Set default compression level
        if (params_.compression_level > 0) {
            builder.SetDefaultCompressionLevel(static_cast<CompressionLevel>(params_.compression_level));
        }
        TEC_TRACE("CompressionLevel is set to {}.", params_.compression_level);
    }

public:
    /**
     * @brief Constructs a GrpcServer instance.
     *
     * @param params      Server configuration (listen address, limits, compression…)
     * @param credentials Server-side credentials (for TLS or other authentication)
     */
    GrpcServer(const Params& params, const std::shared_ptr<Credentials>& credentials)
        : Actor()
        , params_{params}
        , credentials_{credentials}
    {
        static_assert(
            std::is_base_of<GrpcServerParams, Params>::value,
            "Must derive from tec::GrpcServerParams class");
    }

    /// Virtual destructor (defaulted)
    virtual ~GrpcServer() = default;

    /**
     * @brief Synchronous request processor (optional override).
     *
     * Default implementation returns `NotImplemented`.
     * May be overridden in derived classes that implement a synchronous service pattern.
     *
     * @param request Input request
     * @param reply   Output reply (to be filled)
     * @return Status Success or error information
     */
    Status process_request(Request, Reply) override {
        return {tec::Error::Kind::NotImplemented};
    }

    /**
     * @brief Starts the gRPC server and begins listening.
     *
     * Sequence:
     * 1. Configures optional plugins (health, reflection)
     * 2. Creates and configures `ServerBuilder`
     * 3. Adds listening port with credentials
     * 4. Registers the service instance
     * 5. Builds and starts the server
     * 6. Waits for shutdown signal (blocking call)
     *
     * @param sig_started Signal to set when server is successfully listening
     * @param status      [out] Startup result status
     */
    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("GrpcServer::start");

        // Create service instance (usually synchronous)
        Service service;

        // Enable optional plugins
        set_plugins();

        Builder builder;

        // Bind to address with authentication
        builder.AddListeningPort(params_.addr_uri, credentials_);

        // Apply message size / compression settings
        set_builder_options(builder);

        // Register service implementation
        builder.RegisterService(&service);

        // Start the server
        TEC_TRACE("starting gRPC server on {} ...", params_.addr_uri);
        server_ = builder.BuildAndStart();
        if (!server_) {
            auto errmsg = format("gRPC Server cannot start on \"{}\"", params_.addr_uri);
            TEC_TRACE("!!! Error: {}.", errmsg);
            *status = {errmsg, Error::Kind::NetErr};
            sig_started->set();
            return;
        }
        TEC_TRACE("server listening on \"{}\".", params_.addr_uri);

        // Notify caller that server is up
        sig_started->set();

        // Block until shutdown is requested from another thread
        server_->Wait();
    }

    /**
     * @brief Initiates graceful shutdown of the server.
     *
     * Triggers server shutdown (stops accepting new connections,
     * allows existing RPCs to finish within deadline).
     *
     * @param sig_stopped Signal to set when shutdown procedure has been completed
     */
    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("GrpcServer::shutdown");
        Signal::OnExit on_exit(sig_stopped);
        if (server_) {
            TEC_TRACE("terminating gRPC server ...");
            server_->Shutdown();
        }
    }

}; // GrpcServer

} // ::tec
