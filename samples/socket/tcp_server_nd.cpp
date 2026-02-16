// Time-stamp: <Last changed 2026-02-17 01:48:45 by magnolia>
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

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_compression.hpp"
#include "tec/net/tec_socket_server_nd.hpp"

#include "../test_data.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     BSD socket NetData server
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using TCPParams = tec::SocketServerParams;
using TCPServer = tec::SocketServerNd<TCPParams>;

using DataInOut = TCPServer::DataInOut;

class Server final: public TCPServer {
public:
    Server(const TCPParams& params)
        : TCPServer(params)
    {
        // Register NetData RPC GetPersons handler. See ../test_data.hpp.
        register_handler(this, test::RPCID::GetPersons, &Server::on_persons);
    }

    virtual void on_persons(DataInOut dio) {
        TEC_ENTER("Server::on_persons");
        //
        // Get request. (Not used in this sample.)
        //
        test::GetPersonsIn request;
        *dio.nd_in >> request;
        //
        // Generate a reply...
        //
        test::GetPersonsOut reply;
        reply.persons.push_back({67, "John", "Dow"});
        reply.persons.push_back({52, "Boris", "Applegate"});
        reply.persons.push_back({29, "Lucy", "Skywalker"});
        reply.persons.push_back({20, "Harry", "Long"});
        //
        // ... and send it back to a client.
        //
        *dio.nd_out << reply;
    }

};

// Instantiate an ActorWorker.
using TCPServerWorker = tec::ActorWorker<TCPParams, Server>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                            TEST
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

tec::Signal sig_quit;

tec::Status tcp_server() {
    tec::SocketServerParams params;
    // To accept IPv6:
    // params.addr = tec::SocketParams::kAnyAddrIP6;
    // params.family = AF_INET6;
    params.mode = tec::SocketServerParams::kModeNetData;
    params.compression = tec::CompressionParams::kCompressionZlib;
    params.use_thread_pool = true;
    auto srv{TCPServerWorker::Builder<TCPServerWorker, Server>{}(params)};
    //
    // Run it and check for the result.
    //
    auto status = srv->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }
    //
    // Wait for <Ctrl-C> pressed to terminate the server.
    //
    tec::println("\nPRESS <Ctrl-C> TO QUIT THE SERVER");
    sig_quit.wait();
    //
    // Terminate the server.
    //
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
