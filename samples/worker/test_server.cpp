// Time-stamp: <Last changed 2025-04-09 17:08:08 by magnolia>
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

#include <iostream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_server.hpp"
#include "tec/tec_server_worker.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Test Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Test ServerMessage.
struct TestMessage: public tec::WorkerMessage {};

// Test ServerParams.
struct TestParams: public tec::ServerParams {};

// Declare ServerWorker traits.
using TestServerTraits = tec::worker_traits<
    TestParams,
    TestMessage
    >;

// Implement the test Server.
class TestServer: public tec::Server {
public:
    TestServer(const TestParams& params)
        : tec::Server()
    {}

    void start(tec::Signal& sig_started, tec::Status& status) override {
        // Emulate error:
        // status = {"cannot start the server"};
        tec::println("Server started with {} ...", status);
        sig_started.set();
    }

    void shutdown(tec::Signal& sig_stopped) override {
        tec::println("Server stopped.");
        sig_stopped.set();
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                    Test Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Define the Test Server Worker.
using TestServerWorker = tec::ServerWorker<TestServerTraits, TestServer>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Test proc
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// #define DAEMON

tec::Status test_server() {
    TestParams params;

#if defined(DAEMON)
    // Build a daemon.
    auto svr{TestServerWorker::DaemonBuilder<TestServerWorker, TestServer>{}(params)};
    tec::println("*** Using Daemon.");
#else
    // Build a worker.
    auto svr{TestServerWorker::WorkerBuilder<TestServerWorker, TestServer>{}(params)};
    tec::println("*** Using Worker.");
#endif

    // Run it and check for initialization result.
    auto status = svr->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    std::cout << "Press <Return> to shutdown the server" << std::endl;
    std::getchar();

    // If we want to get the server shutdown status.
    return svr->terminate();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    tec::println("*** Running {} built at {}, {} with {} ***", __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    auto result = test_server();

    tec::println("\nExited with {}", result);
    tec::print("Press <Enter> to quit ...");
    std::getchar();

    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
