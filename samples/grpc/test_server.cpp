// Time-stamp: <Last changed 2025-10-31 16:35:54 by magnolia>
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
 *   \file test_server.cpp
 *   \brief A simplest gRPC server.
 *
 *  Runs the gRPC server.
 *
*/

#include <memory>
#include <csignal>

#include "tec/tec_print.hpp"
#include "tec/tec_daemon.hpp"

#include "server.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                             MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Signal sig_quit;


tec::Status test() {
    // The gRPC daemon.
    ServerParams params;
    auto daemon = build_server(params);

    // Run the daemon
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


int main() {
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){ sig_quit.set(); });

    tec::println("*** Running gRPC server built at {}, {} with {} {}.{} ***",
                 __DATE__, __TIME__,
                 __TEC_COMPILER_NAME__,
                 __TEC_COMPILER_VER_MAJOR__, __TEC_COMPILER_VER_MINOR__);
    auto status = test();

    tec::println("\nExited with {}", status);
    return status.code.value_or(tec::Error::Code<>::Unspecified);
}
