
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
    if( !result ) {
        tec::println("Abnormally exited with {}.", result);
        return result.code.value_or(tec::Error::Code<>::Unspecified);
    }

    // Make a request the the server.
    while( !quit.load() ) {
        // Make a call and print a result.
        TestHelloRequest req{"world"};
        TestHelloReply rep;
        tec::println("{} ->", req.name);
        auto status = client->request<>(&req, &rep);
        if( !status ) {
            tec::println("Error: {}", status.as_string());
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
    return result.code.value_or(tec::Error::Code<>::Unspecified);
}
