// Time-stamp: <Last changed 2025-02-14 16:54:55 by magnolia>
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
#include "tec/tec_result.hpp"
#include "tec/tec_semaphore.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_server.hpp"
#include "tec/tec_server_worker.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Test Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Define the test ServerMessage.
struct TestMessage: public tec::WorkerMessage {};

// Define the test ServerParams.
struct TestParams: public tec::ServerParams {};

// Define Server and Worker traits
using TestServerTraits = tec::worker_traits<
    TestParams,
    TestMessage
    >;

// Define the test Server.
class TestServer: public tec::Server<TestParams> {
public:
    TestServer(const TestParams& params)
        : tec::Server<TestParams>{params}
    {}

    void start(tec::Signal& sig_started, tec::Result& result) override {
        // Emulate error:
        // result = {"cannot start the server"};
        tec::println("Server started with {} ...", result);
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

tec::Result test_server() {
    TestParams params;

    // Build the daemon.
    // auto daemon{TestServerWorker::DaemonBuilder<TestServerWorker, TestServer>{}(params)};
    // Build the worker.
    auto daemon{TestServerWorker::Builder<TestServerWorker, TestServer>{}(params)};

    // Run it and check for initialization result.
    auto result = daemon->run();
    if( !result ) {
        tec::println("run(): {}", result);
        return result;
    }

    std::cout << "Press <Return> to shutdown the server" << std::endl;
    std::getchar();

    // If we want to get the server shutdown result.
    // return daemon->terminate();
    return {};
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
