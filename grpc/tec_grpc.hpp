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
 *   \file tec_grpc.hpp
 *   \brief Base gRPC server/client definitions.
 *
 *  TODO
 *
*/

#pragma once

#include "tec/tec_rpc.hpp"


namespace tec {

namespace rpc {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   gRPC Server and Worker parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Default server URI.
static const char GRPC_SERVER_ADDR_URI[] = "0.0.0.0:50051";

//! Declare a health check service.
struct HealthCheckBuilder {
    void (*fptr)(bool);
};

//! Declare a reflection service.
struct ReflectionBuilder {
    void (*fptr)(void);
};


//! gRPC Server.
struct GrpcServerParams: public ServerParams {
    std::string addr_uri;
    HealthCheckBuilder health_check_builder;  //!< e.g. {&grpc::EnableDefaultHealthCheckService}
    ReflectionBuilder reflection_builder;     //!< e.g. {&grpc::reflection::InitProtoReflectionServerBuilderPlugin}

    GrpcServerParams()
        : addr_uri(GRPC_SERVER_ADDR_URI)
        , health_check_builder({nullptr})
        , reflection_builder({nullptr})
        {}
};


//! gRPC Worker.
struct GrpcWorkerParams: public ServerWorkerParams {
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   gRPC Client parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Default server URI.
static const char GRPC_CLIENT_URI_ADDR[] = "127.0.0.1:50051";


//! gRPC client parameters.
struct GrpcClientParams: public ClientParams {
    std::string addr_uri;

    GrpcClientParams()
        : addr_uri(GRPC_CLIENT_URI_ADDR)
    {}
};


} // ::rpc

} // ::tec

