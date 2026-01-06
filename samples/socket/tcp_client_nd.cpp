// Time-stamp: <Last changed 2026-01-06 13:30:15 by magnolia>
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

#include <memory>
#include <string>
#include <sys/socket.h>

#include "tec/net/tec_compression.hpp"
#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_dump.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket_client_nd.hpp"

#include "test_data.hpp"


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Test BSD socket client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

using TCPParams = tec::SocketClientParams;
using TCPClient = tec::SocketClientNd<TCPParams>;
using TCPClientWorker = tec::ActorWorker<TCPParams, TCPClient>;


template <typename T>
void print(const T& p, tec::NetData& nd) {
    std::cout << std::string(40, '=') << "\n";
    std::cout << p << "\n";
    std::cout << std::string(40, '-') << "\n";

    auto hdr = nd.header();
    std::cout
        << "\nMagic:   " << std::hex << hdr->magic
        << "\nVersion: " << hdr->version << std::dec
        << "\nID:      " << hdr->id
        << "\nSize:    " << hdr->size
        << "\nStatus:  " << hdr->status
        << "\n"
        ;

    // If compiled with `g++ -O2` v.13.3, valgrind v.3.22 may report
    // "Use of uninitialised value of size 8" false positive warning.
    nd.rewind();
    std::cout << tec::dump::as_table(nd.bytes().as_hex()) << "\n";
}


// #define USE_DAEMON 1

tec::Status tcp_client() {
    // By default, it can connect to either IPv4 or IPv6 tec::SocketServer.
    TCPParams params;
    params.compression = tec::CompressionParams::kCompressionZlib;
    // To use IPv6 only:
    //     params.addr = tec::SocketParams::kLocalAddrIP6;
    //     params.family = AF_INET6;
    // To use IPv4 only:
    //     params.addr = tec::SocketParams::kLocalAddr;
    //     params.family = AF_INET4;

    // Data to process.
    GetPersonsIn persons_in;
    GetPersonsOut persons_out;

#ifdef USE_DAEMON
    // Use Daemon interface.
    auto cli{TCPClientWorker::Builder<TCPClientWorker, TCPClient>{}(params)};
#else
    // A pure client.
    auto cli{std::make_unique<TCPClient>(params)};
#endif

    // Run it and check for the result.
    auto status = cli->run();
    if( !status ) {
        tec::println("tcp_client: {}", status);
        return status;
    }

     // Request.
    tec::NetData nd_in;
    nd_in << persons_in;
    print(persons_in, nd_in);

    // Reply.
    tec::NetData nd_out;

    // Send a request.
#ifdef USE_DAEMON
    // Use Daemon interface.
    tec::NetData::StreamIn req{&nd_in};
    tec::NetData::StreamOut rep{&nd_out};
    status = cli->request(&req, &rep);
#else
    // Use direct call.
    status = cli->request_nd(&nd_in, &nd_out);
#endif

    if( !status ) {
        tec::println("tcp_client: {}", status);
        return status;
    }

    // Restore reply.
    nd_out >> persons_out;

    // Optionally.
    cli->terminate();

    // Print result.
    print(persons_out, nd_out);

    return status;
}


int main() {
    tec::println("*** Running {} built at {}, {} with {} ***",
                 __FILE__, __DATE__, __TIME__, __TEC_COMPILER_NAME__);

    // Run the test.
    auto result = tcp_client();

    tec::println("\nExited with {}", result);
    return result.code.value_or(0);
}
