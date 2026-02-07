// Time-stamp: <Last changed 2026-02-08 02:05:13 by magnolia>
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

#include <atomic>
#include <csignal>

#include "tec/tec_print.hpp"

#include "client.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         MAIN
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

std::atomic_bool quit{false};


int main() {
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){quit.store(true);});

    // Create a client.
    ClientParams params;
    auto client{build_client(params)};

    // Connect to the server.
    auto result = client->run();
    if (!result) {
        tec::println("Abnormally exited with {}.", result);
        return result.code.value_or(tec::Error::Code<>::Unspecified);
    }

    // Make requests to the server.
    while (!quit) {
        // Make a call and print a result.
        TestHelloRequest req{"world"};
        TestHelloReply rep;
        tec::println("{} ->", req.name);
        auto status = client->request<>(&req, &rep);
        if (!status) {
            tec::println("Error: {}", status);
        }
        else {
            tec::println("<- {}", rep.message);
        }

        tec::println("\nPRESS <Return> TO REPEAT THE REQUEST...");
        tec::println("PRESS <Ctrl-C> THEN <Return> TO QUIT");
        getchar();
    }

    // Clean up.
    result = client->terminate();

    tec::println("Exited with {}.", result);
    return result.code.value_or(0);
}
