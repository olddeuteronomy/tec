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
 *   \file tec_grpc_server.hpp
 *   \brief Base gRPC server.
 *
 *  Declares a base gRPC server.
 *
*/

#pragma once

#include "tec/grpc/tec_grpc.hpp"


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
    typename TGrpcServerCredentials
    >
struct grpc_server_traits {
    typedef TUserService TService;
    typedef TGrpcServer TServer;
    typedef TGrpcServerBuilder TBuilder;
    typedef TGrpcServerCredentials TCredentials;
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

protected:

    //! User-defined parameters - must be inherited from ServerParams.
    TParams params_;

    //! gRPC specific attributes.
    std::unique_ptr<TServer> server_;
    std::shared_ptr<TCredentials> credentials_;

public:

    GrpcServer(const TParams& params, const std::shared_ptr<TCredentials>& credentials)
        : params_(params)
        , credentials_(credentials)
    {}


    virtual ~GrpcServer() = default;


    /**
     *  \brief Start the RPC server.
     *
     *  Registers the service, builds and runs the server.
     *  This procedure doesn't quit until Server::shutdown() is called
     *  from *another* thread.
     *
     *  tec::Worker provides a suitable mechanism to manage
     *  tec::Server as a daemon (or as MS Windows service ).
     *
     *  \param none
     *  \return tec::Result
     *  \sa tec::rpc::ServerWorker
     */
    Result start() override {
        TEC_ENTER("Server::start");

        // Build the server and the service.
        TService service;

        // Set builder parameters.
        if( params_.health_check_builder.fptr ) {
            params_.health_check_builder.fptr(true);
            TEC_TRACE("Health checking enabled.\n");
        }
        if( params_.reflection_builder.fptr ) {
            params_.reflection_builder.fptr();
            TEC_TRACE("Reflection enabled.\n");
        }
        TEC_TRACE("starting gRPC server on % ...\n", params_.addr_uri);

        TBuilder builder;

        // Listen on the given address with given authentication mechanism.
        builder.AddListeningPort(params_.addr_uri, credentials_);

        // Register a "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to a *synchronous* service.
        builder.RegisterService(&service);

        // Finally assemble the server.
        server_ = builder.BuildAndStart();
        if( !server_ ) {
            auto errmsg = format("gRPC Server cannot start on %", params_.addr_uri);
            TEC_TRACE("error: %.\n", errmsg);
            return {Status::ERROR_SERVER_START, errmsg};
        }

        TEC_TRACE("server listening on %.\n", params_.addr_uri);
        server_->Wait();
        return {};
    }

    /**
     *  \brief Shutdown the server.
     *
     *  Closes all connections, shuts the server down.
     *
     *  \param none
     *  \return none
     */
    void shutdown() override {
        TEC_ENTER("Server::shutdown");
        if( server_ ) {
            TEC_TRACE("terminating gRPC server ...\n");
            server_->Shutdown();
        }
    }

}; // Server


} // ::tec
