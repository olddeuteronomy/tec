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


// Instantiate gRPC server parameters.
struct ServerParams: public tec::GrpcServerParams {
    // You can add custom parameters here.
};

// Build GrpcServer as a Daemon.
std::unique_ptr<tec::Daemon> build_server(const ServerParams& _params);
