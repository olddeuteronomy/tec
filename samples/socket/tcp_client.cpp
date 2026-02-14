// Time-stamp: <Last changed 2026-02-14 16:01:21 by magnolia>
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

#include <string>
#include <sys/socket.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/net/tec_socket_client.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Test BSD socket client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using TCPParams = tec::SocketClientParams;
using TCPClient = tec::SocketClient<TCPParams>;
using TCPClientWorker = tec::ActorWorker<TCPParams, TCPClient>;

#define USE_DAEMON 1

tec::Status tcp_client() {
    // By default, it can connect to either IPv4 or IPv6 tec::SocketServer.
    TCPParams params;
    // To use IPv6 only:
    //     params.addr = tec::SocketParams::kLocalAddrIP6;
    //     params.family = AF_INET6;
    // To use IPv4 only:
    //     params.addr = tec::SocketParams::kLocalAddr;
    //     params.family = AF_INET4;

#ifdef USE_DAEMON
    // Use the synchronous Daemon interface.
    auto cli{TCPClientWorker::Builder<TCPClientWorker, TCPClient>{}(params)};
#else
    // An async client.
    auto cli{std::make_unique<TCPClient>(params)};
#endif

    // Run it and check for the result.
    auto status = cli->run();
    if( !status ) {
        tec::println("tcp_client: {}", status);
        return status;
    }

    // Data to process.
    std::string str_send{"Hello world!"};
    // std::string str_send{""};
    std::string str_recv;

    // Send an RPC request.

#ifdef USE_DAEMON
    // Use the Daemon interface to send an RPC request.
    tec::SocketCharStreamIn req{&str_send};
    tec::SocketCharStreamOut rep{&str_recv};
    status = cli->request(&req, &rep);
#else
    // Use async call.
    status = cli->request_str(&str_send, &str_recv);
#endif

    if( !status ) {
        tec::println("tcp_client: {}", status);
        return status;
    }

    tec::println("SEND:\"{}\"", str_send);
    tec::println("RECV:\"{}\"", str_recv);

    // Optionally.
    cli->terminate();
    return status;
}


int main() {
    tec::println("*** Running {} built at {}, {} with {} ***",
                 __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run the test.
    auto result = tcp_client();

    tec::println("\nExited with {}", result);
    return result.code.value_or(0);
}
