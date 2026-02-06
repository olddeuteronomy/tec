// Time-stamp: <Last changed 2026-02-06 16:18:24 by magnolia>
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
/**
 * @file tec_socket_server.hpp
 * @brief Generic BSD socket server template using actor pattern with configurable parameters.
 * @note For BSD, macOS, Linux. Windows version is not tested yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <cstdio>
#include <unistd.h>
#include <memory.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <string>
#include <thread>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_signal.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_memfile.hpp"
#include "tec/tec_actor.hpp"
#include "tec/net/tec_socket.hpp"
#include "tec/net/tec_socket_thread_pool.hpp"


namespace tec {

/**
 * @brief Generic BSD socket server template using actor pattern with configurable parameters.
 *
 * Supports both single-threaded (synchronous) and multi-threaded (thread-pool) modes.
 * Implements the *kModeCharStream* mode -- text oriented (default echo server behavior).
 *
 * Main responsibilities:
 *   - address resolution & binding
 *   - listening
 *   - accepting connections in a loop
 *   - dispatching client sockets either synchronously or via thread pool
 *   - graceful shutdown with proper socket closure
 *
 * @tparam TParams Must derive from `SocketServerParams`
 *
 * @par Example
 * @snippet snp_socket.cpp socketserver
 */
template <typename TParams>
class SocketServer : public Actor {

public:
    using Params = TParams; ///< Parameter type — must inherit from `tec::SocketServerParams`

protected:

    Params params_; ///< Server configuration parameters (address, port, buffer size, threading mode, etc.)
    int listenfd_; ///< Listening socket file descriptor (-1 when not bound/listening)
    std::atomic_bool stop_polling_; ///< Atomic flag used to signal the acceptor/polling loop to exit
    Signal polling_stopped_; ///< Signal object set when polling loop has fully exited

private:

    std::unique_ptr<SocketThreadPool> pool_; ///< Owned thread pool instance (only used when `params_.use_thread_pool == true`)
    std::vector<char> buffer_; ///< Single buffer used in synchronous (single-threaded) mode

    /// Task structure passed to thread pool workers
    template <typename Params>
    struct Task {
        SocketServer<Params>* server;  ///< Back-pointer to owning server
        Socket sock;                   ///< Client connection socket info
    };

    /// Internal helper grouping static functions used by thread pool tasks
    struct details {
        /**
         * @brief Static entry point called by thread pool workers
         * @param task Task containing server pointer and client socket
         */
        static void socket_proc(Task<Params> task) {
            task.server->dispatch_socket(task.sock);
        }
    };

public:

    /**
     * @brief Constructs server instance with given parameters
     * @param params Configuration (address, port, buffer size, threading mode, socket options…)
     *
     * Does **not** start listening — call `start()` to begin serving.
     */
    explicit SocketServer(const Params& params)
        : Actor()
        , params_{params}
        , listenfd_{-1}
        , stop_polling_{false}
        , polling_stopped_{}
    {
        static_assert(
            std::is_base_of_v<SocketServerParams, Params>,
            "Not derived from tec::SocketServerParams class");
    }

    /// Virtual destructor (default implementation)
    virtual ~SocketServer() = default;

    /**
     * @brief Starts the server: bind → listen → accept loop
     * @param sig_started Signal to set once server is ready (bound & listening)
     * @param status [out] Result of initialization (binding/listening)
     *
     * Called from actor framework. Blocking until server is stopped.
     */
    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketServer::start");
        //
        // Resolve the server address and bind the host.
        //
        *status = resolve_and_bind_host();
        if (!status->ok()) {
            sig_started->set();
            return;
        }
        //
        // Start listening on the host.
        //
        *status = start_listening();
        if (!status->ok()) {
            sig_started->set();
            return;
        }
        //
        // Prepare a single-thread buffer or a multi-thread pool.
        //
        if (params_.use_thread_pool) {
            // Multiple-threaded server.
            pool_ = std::make_unique<SocketThreadPool>(get_buffer_size(), params_.thread_pool_size);
        } else {
            // Single-threaded server.
            buffer_.resize(get_buffer_size());
        }
        TEC_TRACE("Buffer size is {} bytes.", get_buffer_size());
        //
        // Start polling.
        //
        TEC_TRACE("Thread pool is {}.", (params_.use_thread_pool ? "ON" : "OFF"));
        poll(sig_started);
    }

    /**
     * @brief Gracefully shuts down the server
     * @param sig_stopped Signal to set when shutdown is complete
     *
     * Closes listening socket and waits until acceptor loop exits.
     * Running client handlers (in thread pool) are allowed to finish.
     */
    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("SocketServer::shutdown");
        //
        // Stop polling in a separate thread
        //
        TEC_TRACE("Stopping server polling...");
        std::thread polling_thread([this] {
            stop_polling_ = true;
            ::shutdown(listenfd_, SHUT_RDWR);
            ::close(listenfd_);
        });
        polling_thread.join();
        TEC_TRACE("Closing server socket...");
        polling_stopped_.wait();
        TEC_TRACE("Server stopped.");
        sig_stopped->set();
    }

    /**
     * @brief Default request processor (not used in socket server)
     * @return Always returns NotImplemented
     */
    Status process_request(Request request, Reply reply) override {
        return {Error::Kind::NotImplemented};
    }

protected:
    /// @return Pointer to the per-connection buffer (single-threaded mode only)
    constexpr char* get_buffer() {
        return buffer_.data();
    }

    /// @return Configured buffer size for client connections
    constexpr size_t get_buffer_size() const {
        return params_.buffer_size;
    }

    /**
     * @brief Sets common socket options (SO_REUSEADDR, SO_REUSEPORT)
     * @param fd Socket file descriptor to configure
     * @return Status indicating success or specific socket error
     */
    virtual Status set_socket_options(int fd) {
        TEC_ENTER("SocketServer::set_socket_options");
        // Avoid "Address already in use" error
        if( ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                         &params_.opt_reuse_addr, sizeof(int)) < 0 ) {
            return {errno, "setsockopt SO_REUSEADDR failed", Error::Kind::NetErr};
        }
        TEC_TRACE("SO_REUSEADDR is {}.", params_.opt_reuse_addr);
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                       &params_.opt_reuse_port, sizeof(int)) < 0) {
            ::close(fd);
            return {errno, "setsockopt SO_REUSEPORT", Error::Kind::NetErr};
        }
        TEC_TRACE("SO_REUSEPORT is {}.", params_.opt_reuse_port);
        return {};
    }

    /**
     * @brief Creates Socket object from raw file descriptor and peer address
     * @param client_fd Accepted client socket fd
     * @param client_addr Peer address structure (IPv4 or IPv6)
     * @return Filled Socket structure with IP, port and buffer info
     */
    virtual Socket get_socket_info(int client_fd, sockaddr_storage* client_addr) {
        // Get socket address and port for IP protocol.
        char client_ip[INET6_ADDRSTRLEN];
        client_ip[0] = '\0';
        int client_port{0};
        if (client_addr->ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in*)&client_addr;
            ::inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof(client_ip));
            client_port = ::ntohs(s->sin_port);
        }
        else if (client_addr->ss_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6*)&client_addr;
            ::inet_ntop(AF_INET6, &s->sin6_addr, client_ip, sizeof(client_ip));
            client_port = ::ntohs(s->sin6_port);
        }

        return {client_fd, client_ip, client_port, get_buffer(), get_buffer_size()};
    }

    /**
     * @brief Resolves address/port and binds listening socket
     * @return Status — success or detailed bind failure reason
     */
    virtual Status resolve_and_bind_host() {
        TEC_ENTER("SocketServer::resolve_and_bind_host");

        // Resolve the server address.
        TEC_TRACE("Resolving address {}:{}...", params_.addr, params_.port);
        addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = params_.family;
        hints.ai_socktype = params_.socktype;
        hints.ai_protocol = params_.protocol;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully bind().
        addrinfo* servinfo{NULL};
        char port_str[16];
        ::snprintf(port_str, 15, "%d", params_.port);
        int ecode = ::getaddrinfo(params_.addr.c_str(), port_str,
                                  &hints, &servinfo);
        if( ecode != 0 ) {
            std::string emsg{::gai_strerror(ecode)};
            TEC_TRACE("Address resolving error: {}", emsg);
            return {ecode, emsg, Error::Kind::NetErr};
        }
        TEC_TRACE("Address resolved OK.");

        // If socket() or bind() fails, we close the socket
        // and try the next address.
        addrinfo* p{NULL};
        int fd{-1};
        TEC_TRACE("Binding...");
        for (p = servinfo ; p != NULL ; p = p->ai_next) {
            fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if( fd == -1 ) {
                // Try next socket.
                continue;
            }

            // Set options.
            auto option_status = set_socket_options(fd);
            if (!option_status) {
                ::close(fd);
                // TODO: should we continue with the next socket here?
                ::freeaddrinfo(servinfo);
                return option_status;
            }

            if (::bind(fd, p->ai_addr, p->ai_addrlen) != -1) {
                // Success.
                break;
            }

            // Try next socket.
            ::close(fd);
        }

        // No longer needed.
        ::freeaddrinfo(servinfo);

        if (p == NULL) {
            auto emsg = format("Failed to bind to {}:{}", params_.addr, params_.port);
            TEC_TRACE(emsg);
            return {EAFNOSUPPORT, emsg, Error::Kind::NetErr};
        }

        // Success.
        listenfd_ = fd;
        return {};
    }

    /**
     * @brief Starts listening on the bound socket
     * @return Status — success or listen() failure
     */
    virtual Status start_listening() {
        TEC_ENTER("SocketServer::start_listening");
        if (::listen(listenfd_, params_.queue_size) == -1) {
            auto emsg = format("Failed to listen on {}:{}.", params_.addr, params_.port);
            ::close(listenfd_);
            listenfd_ = EOF;
            return {errno, emsg, Error::Kind::NetErr};
        }
        TEC_TRACE("Server listening on {}:{}.", params_.addr, params_.port);
        return {};
    }

    /**
     * @brief Accepts one incoming connection (blocking)
     * @param[out] clientfd Accepted client file descriptor
     * @param[out] client_addr Filled peer address structure
     * @return Status — success or accept() failure/interruption
     */
    virtual Status accept_connection(int* clientfd, sockaddr_storage* client_addr) {
        TEC_ENTER("SocketServer::accept_connection");
        // Wait for incoming connection.
        socklen_t sin_size = sizeof(sockaddr_storage);
        auto fd = ::accept(listenfd_,
                             (sockaddr *)client_addr,
                             &sin_size);
        // Check result.
        if (fd == EOF) {
            std::string err_msg;
            if( errno == EINVAL || errno == EINTR || errno == EBADF ) {
                err_msg = format("Polling interrupted by signal {}.", errno);
            }
            else {
                err_msg = format("accept() failed with errno={}.", errno);
            }
            TEC_TRACE(err_msg);
            return {errno, err_msg, Error::Kind::NetErr};
        }
        *clientfd = fd;
        return {};
    }

    /**
     * @brief Main acceptor loop — runs until stop_polling_ is set
     * @param sig_started Signal to set once loop has started
     */
    virtual void poll(Signal* sig_started) {
        TEC_ENTER("SocketServer::poll");
        sig_started->set();

        while (!stop_polling_) {
            int clientfd{-1};

            sockaddr_storage client_addr;
            TEC_TRACE("Waiting for incoming connection...");
            if (!accept_connection(&clientfd, &client_addr)) {
                continue;
            }

            Socket sock = get_socket_info(clientfd, &client_addr);
            TEC_TRACE("Accepted connection from {}:{}.", sock.addr, sock.port);

            process_socket(sock);
        }
        polling_stopped_.set();
    }

    /**
     * @brief Decides how to handle newly accepted client socket
     * @param sock Filled client socket information
     *
     * Routes to thread pool (async) or direct dispatch (sync) based on configuration.
     */
    virtual void process_socket(Socket sock) {
        TEC_ENTER("process_socket");
        if (pool_) {
            // Round-robin.
            size_t idx = pool_->get_next_worker_index();
            TEC_TRACE("Pool IDX={}", idx);
            sock.buffer = pool_->get_buffer(idx);
            sock.buffer_size = pool_->get_buffer_size();
            // Async processing -- enqueue a client handling task to the thread pool.
            Task<Params> task{this, sock};
            pool_->enqueue([task] {
                details::socket_proc(task);
            });
        }
        else {
            // Synchronous processing.
            dispatch_socket(sock);
        }
    }

    /**
     * @brief Executes client connection handling logic
     * @param _sock Client socket (moved or copied)
     *
     * Dispatches to mode-specific handler (`on_string` or `on_net_data`).
     * Always closes connection when handler returns.
     */
    virtual void dispatch_socket(Socket _sock) {
        TEC_ENTER("SocketServer::dispatch_socket");
        Socket sock{_sock};
        if(params_.mode == SocketServerParams::kModeCharStream) {
            on_string(&sock);
        }
        else if(params_.mode == SocketServerParams::kModeNetData) {
            on_net_data(&sock);
        }
        close_client_connection(&sock);
    }

    /**
     * @brief Closes client connection cleanly
     * @param sock Socket to close (fd set to -1 after close)
     */
    virtual void close_client_connection(Socket* sock) {
        TEC_ENTER("SocketServer::close_client_connection");
        TEC_TRACE("Closing connection with {}:{}...", sock->addr, sock->port);
        if (sock->fd != -1) {
            ::shutdown(sock->fd, SHUT_RDWR);
            ::close(sock->fd);
            sock->fd = -1;
        }
    }

    /**
     * @brief Default handler for character-stream / line-based protocols
     * @param sock Client connection
     *
     * Default implementation: simple echo server.
     * Override in derived classes for real protocol handling.
     */
    virtual void on_string(const Socket* sock) {
        TEC_ENTER("SocketServer::on_char_stream");
        // Default implementation just echoes received data.
        Bytes data;
        auto status = Socket::recv(data, sock, 0);
        if (status) {
            Socket::send(data, sock);
        }
    }

    /**
     * @brief Default handler for binary / structured network protocols
     * @param sock Client connection
     *
     * Empty by default — override in derived classes.
     */
    virtual void on_net_data(const Socket* sock) {
    }

}; // class SocketServer

} // namespace tec
