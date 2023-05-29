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
 *   \brief General gRPC parameters.
 *
 *  gRPC server/client parameter declarations.
 *
*/

#pragma once

#include "tec/tec_server.hpp"


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     gRPC Server parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Default server URI.
static const char GRPC_SERVER_ADDR_URI[] = "0.0.0.0:50051";

//! Declare a gRPC health check service builder.
struct GrpcHealthCheckBuilder {
    void (*fptr)(bool);
};

//! Declare a gRPC reflection service builder.
struct GrpcReflectionBuilder {
    void (*fptr)(void);
};


//! gRPC Server parameters.
struct GrpcServerParams: public ServerParams {
    std::string addr_uri;
    GrpcHealthCheckBuilder health_check_builder;  //!< e.g. {&grpc::EnableDefaultHealthCheckService}
    GrpcReflectionBuilder reflection_builder;     //!< e.g. {&grpc::reflection::InitProtoReflectionServerBuilderPlugin}

    GrpcServerParams()
        : addr_uri(GRPC_SERVER_ADDR_URI)
        , health_check_builder({nullptr})
        , reflection_builder({nullptr})
        {}
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   gRPC Client parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Default server URI.
static const char GRPC_CLIENT_URI_ADDR[] = "127.0.0.1:50051";


struct GrpcClientParams: public ClientParams {
    std::string addr_uri;

    GrpcClientParams()
        : addr_uri(GRPC_CLIENT_URI_ADDR)
    {}
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*                  gRPC metadata at client side
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TClientContext>
std::string get_server_metadata(const TClientContext& ctx, const std::string& key) {
    auto meta = ctx.GetServerInitialMetadata();
    auto data = meta.find(key);
    if( data != meta.end() ) {
        auto ref = data->second;  // grpc::string_ref
        auto len = ref.length();
        // NOTE: grpc::string_ref is NOT null-terminated string!
        if( len > 0 ) {
            return{ref.data(), 0, len};
        }
    }
    return{};

}

template <typename TClientContext>
void add_client_metadata(TClientContext& ctx, const std::string& key, const std::string& data) {
    ctx.AddMetadata(key, data);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*                  gRPC metadata at server side
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TServerContext>
std::string get_client_metadata(const TServerContext* pctx, const std::string& key) {
    auto meta = pctx->client_metadata();
    auto data = meta.find(key);
    if( data != meta.end() ) {
        auto ref = data->second;
        auto len = ref.length();
        // NOTE: grpc::string_ref is NOT null-terminated string!
        if( len > 0 ) {
            return{ref.data(), 0, len};
        }
    }
    return{};
}

template <typename TServerContext>
void add_server_medadata(TServerContext* pctx, const std::string& key, const std::string& data) {
    pctx->AddInitialMetadata(key, data);
}


} // ::tec
