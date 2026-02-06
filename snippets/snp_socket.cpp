
//![socketserver]
#include <csignal>
#include <sys/socket.h>

#include "tec/tec_print.hpp"
#include "tec/tec_actor_worker.hpp"
#include "tec/net/tec_socket_server.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*                     Simplest BSD socket server
*   in the kModeCharStream mode (default, receives and sends strings)
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Instantiate core blocks.
using TCPParams = tec::SocketServerParams;
using TCPServer = tec::SocketServer<TCPParams>;
using TCPServerWorker = tec::ActorWorker<TCPParams, TCPServer>;

tec::Signal sig_quit;

tec::Status tcp_server() {
    tec::SocketServerParams params;
    // Accepts both IPv4 and IPv6.
    params.addr = tec::SocketParams::kAnyAddrIP6;
    params.family = AF_INET6;
    // To accept IPv4 only:
    // params.addr = tec::SocketParams::kAnyAddr;
    // params.family = AF_INET;
    //
    // Use internal thread pool to process incoming connections.
    // `false` by default.
    params.use_thread_pool = true;

    // Create TCP server as Daemon.
    auto srv{TCPServerWorker::Builder<TCPServerWorker, TCPServer>{}(params)};

    // Run it and check for the result.
    auto status = srv->run();
    if( !status ) {
        tec::println("run(): {}", status);
        return status;
    }

    // Wait for <Ctrl-C> pressed to terminate the server.
    tec::println("\nPRESS <Ctrl-C> TO QUIT THE SERVER");
    sig_quit.wait();

    // Terminate the server
    status = srv->terminate();
    return status;
}

int main() {
    // Install Ctrl-C handler.
    std::signal(SIGINT, [](int){ sig_quit.set(); });
    // Run the server.
    auto result = tcp_server();
    tec::println("\nExited with {}", result);
    return result.code.value_or(0);
}
//![socketserver]
