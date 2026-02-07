// Time-stamp: <Last changed 2026-02-08 00:46:30 by magnolia>
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

#include "tec/tec_print.hpp"
#include "tec/tec_daemon.hpp"

#include "server.hpp"


tec::Signal sig_quit;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          Run the server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Status run() {
    // The gRPC daemon.
    ServerParams params;
    auto daemon = build_server(params);

    // Start the daemon
    tec::println("Starting ...");
    auto status = daemon->run();
    if( !status ) {
        tec::println("Exited abnormally with {}.", status);
        return status;
    }
    tec::println("Server listening on \"{}\"", params.addr_uri);

    // Wait for <Ctrl-C> pressed to terminate the server.
    tec::println("\nPRESS <Ctrl-C> TO QUIT THE SERVER");
    sig_quit.wait();

    // Terminate the server.
    status = daemon->terminate();
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

    tec::println("*** Running gRPC server built at {}, {} with {} {}.{} ***",
                 __DATE__, __TIME__,
                 __TEC_COMPILER_NAME__,
                 __TEC_COMPILER_VER_MAJOR__, __TEC_COMPILER_VER_MINOR__);
    auto status = run();

    tec::println("\nExited with {}", status);
    return status.code.value_or(tec::Error::Code<>::Unspecified);
}
