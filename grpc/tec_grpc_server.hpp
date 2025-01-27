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
 *   \file tec_grpc_server.hpp
 *   \brief Base gRPC server.
 *
 *  Declares a base gRPC server.
 *
*/

#pragma once

#include "tec/tec_trace.hpp"
#include "tec/tec_server.hpp" // IWYU pragma: keep
#include "tec/grpc/tec_grpc.hpp" // IWYU pragma: keep
#include "tec/tec_utils.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     gRPC Server traits
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <
    typename TUserService,
    typename TGrpcServer,
    typename TGrpcServerBuilder,
    typename TGrpcServerCredentials,
    typename TGrpc_compression_algorithm,
    typename TGrpc_compression_level
    >
struct grpc_server_traits {
    typedef TUserService TService;
    typedef TGrpcServer TServer;
    typedef TGrpcServerBuilder TBuilder;
    typedef TGrpcServerCredentials TCredentials;
    typedef TGrpc_compression_algorithm TCompressionAlgorithm;
    typedef TGrpc_compression_level TCompressionLevel;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Generic gRPC Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TParams, typename Traits>
class GrpcServer: public Server {
public:

    typedef typename Traits::TService TService;
    typedef typename Traits::TServer TServer;
    typedef typename Traits::TBuilder TBuilder;
    typedef typename Traits::TCredentials TCredentials;
    typedef typename Traits::TCompressionAlgorithm TCompressionAlgorithm;
    typedef typename Traits::TCompressionLevel TCompressionLevel;

protected:

    //! User-defined parameters - must be inherited from ServerParams.
    TParams params_;

    //! gRPC specific attributes.
    std::unique_ptr<TServer> server_;
    std::shared_ptr<TCredentials> credentials_;

protected:

    /**
     * @brief      Sets builder plugins such as HealthCheck and Reflection.
     *
     * @details    Called *before* the builder is created. Can be overwritten.
     *
     * @param      None.
     * @return     None.
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
     *
     * @details    Called *after* the builder is created. Can be overwritten.
     *
     * @return     void.
     */
    virtual void set_builder_options(TBuilder& builder) {
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
            builder.SetDefaultCompressionAlgorithm(static_cast<TCompressionAlgorithm>(params_.compression_algorithm));
        }
        TEC_TRACE("CompressionAlgorithm is set to {}.", params_.compression_algorithm);

        // Set compression level
        if (params_.compression_level > 0) {
            builder.SetDefaultCompressionLevel(static_cast<TCompressionLevel>(params_.compression_level));
        }
        TEC_TRACE("CompressionLevel is set to {}.", params_.compression_level);
    }

public:

    GrpcServer(const TParams& params, const std::shared_ptr<TCredentials>& credentials)
        : params_(params)
        , credentials_(credentials)
    {}


    virtual ~GrpcServer() = default;


    /**
     *  @brief Start the RPC server.
     *
     *  Registers the service, builds and runs the server.
     *  This procedure doesn't quit until Server::shutdown() is called
     *  from *another* thread.
     *
     *  Worker provides a suitable mechanism to manage
     *  Server as a daemon (or as MS Windows service).
     *
     *  @param sig_started Signal Signals GrpcSever is started, possible with error.
     *  @param result tec::Result
     *  @sa tec::Server
     */
    void start(Signal& sig_started, Result& result) override {
        TEC_ENTER("GrpcServer::start");

        // Build the server and the service.
        TService service;

        // Set builder plugins.
        set_plugins();

        TBuilder builder;

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
            TEC_TRACE("!!! Error: %.", errmsg);
            result = {errmsg, Result::Kind::GrpcErr};
            // Signal that the server started, set error result.
            sig_started.set();
            return;
        }

        TEC_TRACE("server listening on \"{}\".", params_.addr_uri);

        // Signal that the gRPC server is started OK.
        sig_started.set();
        // Wait until this->shutdown() is called from another thread.
        server_->Wait();
    }

    /**
     *  @brief Shutdown the gRPC server.
     *
     *  Closes all connections, shuts the server down.
     *
     *  @param sig_stopped Signalled on gRPC server is stopped.
     *  @return none
     */
    void shutdown(Signal& sig_stopped) override {
        TEC_ENTER("GrpcServer::shutdown");
        if( server_ ) {
            TEC_TRACE("terminating gRPC server ...");
            server_->Shutdown();
        }
        sig_stopped.set();
    }

}; // GrpcServer


} // ::tec
