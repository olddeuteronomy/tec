// Time-stamp: <Last changed 2026-02-08 01:48:59 by magnolia>
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
#pragma once

#include "tec/tec_daemon.hpp"
#include "tec/grpc/tec_grpc.hpp"


// Instantiate gRPC Client parameters.
struct ClientParams: public tec::GrpcClientParams {
    // You can add custom parameters here.
};


// Test request/reply.
struct TestHelloRequest {
    std::string name;
};

struct TestHelloReply {
    std::string message;
};


// Client builder -- implemented in client.cpp
std::unique_ptr<tec::Daemon> build_client(const ClientParams&);
