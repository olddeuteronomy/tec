// Time-stamp: <Last changed 2025-09-30 17:59:01 by magnolia>
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

#include <any>
#include <cstdio>
#include <iostream>
#include <memory>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_message.hpp"
#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_server.hpp"
#include "tec/tec_server_worker_ex.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Test Server Request/Reply
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct ChrRequest {
    int ch;
};

struct ChrReply {
    int ch;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Test Server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Server parameters.
struct TestServerParams: public tec::ServerParams {};


// Implement the Server.
class TestServer final: public tec::Server {
private:
    TestServerParams params_;

public:
    TestServer(const TestServerParams& params)
        : tec::Server()
        , params_{params}
    {}

    void start(tec::Signal* sig_started, tec::Status* status) override {
        // Emulate error:
        // status = {"cannot start the server"};
        tec::println("Server started with {} ...", status);
        sig_started->set();
    }

    void shutdown(tec::Signal* sig_stopped) override {
        tec::println("Server stopped.");
        sig_stopped->set();
    }

    // Request handler.
    tec::Status process_request(tec::Request& _request, tec::Reply& _reply) override {
        // Check type compatibility.
        // NOTE: *Request* is const!
        // NOTE: Both _request and _reply contain *pointers* to actual data.
        if( _request.type() != typeid(const ChrRequest*) ||
            _reply.type() != typeid(ChrReply*) ) {
            return {tec::Error::Kind::Unsupported};
        }

        // Get pointers to actual data.
        auto request = std::any_cast<const ChrRequest*>(_request);
        auto reply = std::any_cast<ChrReply*>(_reply);

        // Just duplicate the request.
        reply->ch = request->ch;

        // OK
        return {};
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                    Test Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using TestWorker = tec::ServerWorkerEx<TestServerParams, TestServer>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Status test_server() {
    TestServerParams params;

    // A traditional way to build a server worker.
    // auto server = std::make_unique<TestServer>(params);
    // auto svr = std::make_unique<TestWorker>(params, std::move(server));

    // Build a server worker as a daemon.
    auto svr{TestWorker::DaemonBuilder<TestWorker, TestServer>{}(params)};

    // Run it and check for the result.
    auto status = svr->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    std::cout << "\nPress <ESC> to shutdown the server" << std::endl;
    for( int ch; (ch = std::getchar()) != EOF ; )
    {
        if( ch == 27 ) { // 'ESC' (escape) in ASCII
            break;
        }
        if( ch == 10 ) { // newline
            continue;
        }

        // Data.
        ChrRequest req{ch};
        ChrReply rep{0};

        // Make a request.
        auto status = svr->request<>(&req, &rep);
        if( status ) {
            tec::println("'{}' -> '{}'", static_cast<char>(req.ch), static_cast<char>(rep.ch));
        }
        else {
            tec::println("Error: {}", status);
        }
    }

    // We want to get the server shutdown status.
    return svr->terminate();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    tec::println("*** Running {} built at {}, {} with {} ***", __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run a test.
    auto result = test_server();

    tec::println("\nExited with {}", result);
    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
