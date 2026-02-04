// Time-stamp: <Last changed 2026-02-04 12:59:07 by magnolia>
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

#include <any>
#include <cstdio>
#include <iostream>
#include <memory>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_message.hpp"
#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_actor.hpp"
#include "tec/tec_actor_worker.hpp"

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
struct ServerParams {
    int inc;
};


// Implement the Server as an Actor.
class Server final: public tec::Actor {
private:
    ServerParams params_;

public:
    Server(const ServerParams& params)
        : tec::Actor()
        , params_{params}
    {}

    void start(tec::Signal* sig_started, tec::Status* status) override {
        // Does nothing, just set `sig_started` on exit.
        tec::Signal::OnExit on_exit{sig_started};
        tec::println("Server started with {} ...", *status);
    }

    void shutdown(tec::Signal* sig_stopped) override {
        // Does nothing, just set `sig_stopped` on exit.
        tec::Signal::OnExit on_exit{sig_stopped};
        tec::println("Server stopped.");
    }

    // Request handler. Increments input character by ServerParams::inc
    tec::Status process_request(tec::Request _request, tec::Reply _reply) override {
        // Check type match.
        // NOTE: Both `_request` and `_reply` contain *pointers* to actual data.
        // NOTE: `_request` content is const!
        if( _request.type() != typeid(const ChrRequest*) ||
            _reply.type() != typeid(ChrReply*) ) {
            return {tec::Error::Kind::Unsupported};
        }

        // Get pointers to actual data.
        auto request = std::any_cast<const ChrRequest*>(_request);
        auto reply = std::any_cast<ChrReply*>(_reply);

        // Increment a character.
        reply->ch = request->ch + params_.inc;

        // OK
        return {};
    }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Instantiate ServerWorker.
using ServerWorker = tec::ActorWorker<ServerParams, Server>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Status test_server() {
    ServerParams params{1}; // Increment input

    // An alternative way to build a server worker as a daemon.
    // auto server = std::make_unique<Server>(params);
    // auto daemon  = std::make_unique<ServerWorker>(params, std::move(server));

    // Build a server worker as a daemon.
    auto svr{ServerWorker::Builder<ServerWorker, Server>{}(params)};

    // Run it and check for the result.
    auto status = svr->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    // Read a character and send it to the server.
    std::cout << "\nPress <ESC><Enter> to shutdown the server" << std::endl;
    for( int ch; (ch = std::getchar()) != EOF ; )
    {
        if( ch == 27 ) { // 'ESC' in ASCII
            break;
        }
        if( ch == 10 ) { // newline
            continue;
        }

        // Input/output data.
        ChrRequest req{ch};
        ChrReply rep{0};

        // Make a request.
        auto status = svr->request(&req, &rep);
        if( status ) {
            tec::println("'{}' -> '{}'", static_cast<char>(req.ch), static_cast<char>(rep.ch));
        }
        else {
            tec::println("Error: {}", status);
        }
    }

    // If we want to get the server shutdown status.
    return svr->terminate();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
    tec::println("*** Running {} built at {}, {} with {} ***",
                 __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run the test.
    auto result = test_server();

    tec::println("\nExited with {}", result);
    return result.code.value_or(0);
}
