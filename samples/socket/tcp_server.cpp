// Time-stamp: <Last changed 2026-02-06 16:48:35 by magnolia>
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

#include <csignal>
#include <sys/socket.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include "tec/net/tec_socket.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/net/tec_socket_server.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~
*
*                     Simplest BSD socket server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using TCPParams = tec::SocketServerParams;
using TCPServer = tec::SocketServer<TCPParams>;
using TCPServerWorker = tec::ActorWorker<TCPParams, TCPServer>;

tec::Signal sig_quit;


tec::Status tcp_server() {
    tec::SocketServerParams params;
    // Accepts both IPv4 and IPv6.
    params.addr = tec::SocketParams::kAnyAddrIP6;
    params.family = AF_INET6;
    // To accept IPv4 only:
    params.addr = tec::SocketParams::kAnyAddr;
    params.family = AF_INET;
    // Use internal thread pool to process incoming connections.
    // `false` by default.
    params.use_thread_pool = true;

    // Create TCP server as Daemon.
    auto srv{TCPServerWorker::Builder<TCPServerWorker, TCPServer>{}(params)};

    // Run it and check for the result.
    auto status = srv->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    // Wait for <Ctrl-C> pressed to terminate the server.
    tec::println("\nPRESS <Ctrl-C> TO QUIT THE SERVER");
    sig_quit.wait();

    // Terminate the server
    status = srv->terminate();
    return status;
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){ sig_quit.set(); });

    tec::println("*** Running {} built at {}, {} with {} ***",
                 __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run the test.
    auto result = tcp_server();

    tec::println("\nExited with {}", result);
    return result.code.value_or(0);
}
