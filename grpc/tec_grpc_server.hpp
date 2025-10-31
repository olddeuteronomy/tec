// Time-stamp: <Last changed 2025-10-31 16:40:24 by magnolia>
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
 *   @file tec_grpc_server.hpp
 *   @brief Generic gRPC server.
 *
 *  Declares a generic gRPC server.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
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

template <
    typename TService,
    typename TGrpcServer,
    typename TGrpcServerBuilder,
    typename TGrpcServerCredentials,
    typename TGrpcCompressionAlgorithm,
    typename TGrpcCompressionLevel
    >
struct grpc_server_traits {
    typedef TService Service;
    typedef TGrpcServer RpcServer;
    typedef TGrpcServerBuilder Builder;
    typedef TGrpcServerCredentials Credentials;
    typedef TGrpcCompressionAlgorithm CompressionAlgorithm;
    typedef TGrpcCompressionLevel CompressionLevel;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Generic gRPC Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TParams, typename Traits>
class GrpcServer: public Actor {
public:

    typedef TParams Params;
    typedef typename Traits::Service Service;
    typedef typename Traits::RpcServer RpcServer;
    typedef typename Traits::Builder Builder;
    typedef typename Traits::Credentials Credentials;
    typedef typename Traits::CompressionAlgorithm CompressionAlgorithm;
    typedef typename Traits::CompressionLevel CompressionLevel;

protected:

    //! User-defined parameters - must be inherited from ServerParams.
    Params params_;

    //! gRPC specific attributes.
    std::unique_ptr<RpcServer> server_;
    std::shared_ptr<Credentials> credentials_;

protected:

    /**
     * @brief      Sets builder plugins such as HealthCheck and Reflection.
     * @details    Called *before* the builder is created. Can be overwritten.
     */
    virtual void set_plugins() {
        TEC_ENTER("GrpcServer::set_plugins");

        if( params_.health_check_builder.fptr ) {
            params_.health_check_builder.fptr(true);
            TEC_TRACE("Health checking enabled.");
        }
        if( params_.reflection_builder.fptr ) {
            params_.reflection_builder.fptr();
            TEC_TRACE("Reflection enabled.");
        }
    }


    /**
     * @brief      Sets various builder options such as MaxMessageSize etc.
     * @details    Called *after* the builder is created. Can be overwritten.
     */
    virtual void set_builder_options(Builder& builder) {
        TEC_ENTER("GrpcServer::set_builder_options");

        // Set max message size.
        if(params_.max_message_size > 0) {
            const int max_size = params_.max_message_size * 1024 * 1024;
            builder.SetMaxReceiveMessageSize(max_size);
            builder.SetMaxSendMessageSize(max_size);
        }
        TEC_TRACE("MaxMessageSize is set to {} Mb.", params_.max_message_size);

        // Set compression algorithm.
        // Note that it overrides any compression level set by SetDefaultCompressionLevel.
        if (params_.compression_algorithm > 0) {
            builder.SetDefaultCompressionAlgorithm(static_cast<CompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);

        // Set compression level
        if (params_.compression_level > 0) {
            builder.SetDefaultCompressionLevel(static_cast<CompressionLevel>(params_.compression_level));
        }
        TEC_TRACE("CompressionLevel is set to {}.", params_.compression_level);
    }

public:

    GrpcServer(const Params& params, const std::shared_ptr<Credentials>& credentials)
        : Actor()
        , params_{params}
        , credentials_{credentials}
    {
        static_assert(
            std::is_base_of<GrpcServerParams, Params>::value,
            "not derived from tec::GrpcServerParams class");
    }

    virtual ~GrpcServer() = default;

    Status process_request(Request, Reply) override {
        return {tec::Error::Kind::NotImplemented};
    }


    /**
     *  @brief Starts the RPC server.
     *
     *  Registers the service, builds and runs the server.
     *  This procedure doesn't quit until `Server::shutdown()` is invoked
     *  from *another* thread.
     *
     *  Worker provides a suitable mechanism to manage
     *  Server as a daemon (or as MS Windows service).
     *
     *  @param sig_started Signal signals on GrpcSever has been started, possible with error.
     *  @param status Status
     *  @sa Actor
     */
    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("GrpcServer::start");

        // Build the server and the service.
        Service service;

        // Set builder plugins.
        set_plugins();

        Builder builder;

        // Listen on the given address with given authentication mechanism.
        builder.AddListeningPort(params_.addr_uri, credentials_);

        // Set builder options.
        set_builder_options(builder);

        // Register a "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to a *synchronous* service.
        builder.RegisterService(&service);

        // Finally assemble the server.
        TEC_TRACE("starting gRPC server on {} ...", params_.addr_uri);
        server_ = builder.BuildAndStart();
        if( !server_ ) {
            auto errmsg = format("gRPC Server cannot start on \"{}\"", params_.addr_uri);
            TEC_TRACE("!!! Error: {}.", errmsg);
            *status = {errmsg, Error::Kind::NetErr};
            return;
        }

        TEC_TRACE("server listening on \"{}\".", params_.addr_uri);

        // Signals the server is started.
        sig_started->set();
        // Waits until this->shutdown() is called from another thread.
        server_->Wait();
    }

    /**
     *  @brief Shutdowns the gRPC server.
     *
     *  Closes all connections, shuts the server down.
     *
     *  @param sig_stopped Signal signals on gRPC server is stopped.
     */
    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("GrpcServer::shutdown");
        Actor::SignalOnExit on_exit(sig_stopped);
        if( server_ ) {
            TEC_TRACE("terminating gRPC server ...");
            server_->Shutdown();
        }
    }

}; // GrpcServer


} // ::tec
