// Time-stamp: <Last changed 2025-09-28 02:49:27 by magnolia>
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
#include "tec/tec_trace.hpp"


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

    tec::Status process_request(tec::Request& _request, tec::Reply& _reply) override {
        TEC_ENTER("TestServer::process_request");
        // Check type compatibility.
        if( _request.type() != typeid(const ChrRequest*) ||
            _reply.type() != typeid(ChrReply*) ) {
            TEC_TRACE("Type mismatch!");
            return {tec::Error::Kind::Unsupported};
        }

        TEC_TRACE("_request: {}", _request.type().name());
        TEC_TRACE("_reply:   {}", _reply.type().name());

        auto request = std::any_cast<const ChrRequest*>(_request);
        auto reply = std::any_cast<ChrReply*>(_reply);
        reply->ch = request->ch;
        return {};
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                    Test Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class TestWorker final: public tec::ServerWorkerEx<TestServerParams, TestServer> {
public:
    TestWorker(const TestServerParams& params)
        : tec::ServerWorkerEx<TestServerParams, TestServer>(params, std::make_unique<TestServer>(params))
    {}
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Test proc
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Status test_server() {
    TEC_ENTER("test_server");
    TestServerParams params;

    // Build a server worker.
    auto svr = std::make_unique<TestWorker>(params);

    // Run it and check for initialization result.
    auto status = svr->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    std::cout << "Press <ESC> to shutdown the server" << std::endl;
    for( int ch; (ch = std::getchar()) != EOF ; )
    {
        if( ch == 27 ) { // 'ESC' (escape) in ASCII
            break;
        }
        if( ch == 10 ) { // newline
            continue;
        }

        ChrRequest req{ch};
        ChrReply rep{0};

        TEC_TRACE("req: {}", typeid(&req).name());
        TEC_TRACE("rep: {}", typeid(&rep).name());

        auto status = svr->request<>(&req, &rep);
        if( status ) {
            tec::println("'{}' -> '{}'", static_cast<char>(req.ch), static_cast<char>(rep.ch));
        }
        else {
            tec::println("Error: {}", status);
        }
    }

    // std::getchar();

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
