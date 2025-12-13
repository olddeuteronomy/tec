// Time-stamp: <Last changed 2025-12-14 02:31:02 by magnolia>
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

/**
 * @file tec_socket_server.hpp
 * @brief Generic BSD socket server implementation.
 * @note For BSD, macOS, Linux. Windows version is not implemented yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

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
#include "tec/tec_bytes.hpp"
#include "tec/tec_actor.hpp"
#include "tec/net/tec_socket.hpp"


namespace tec {

template <typename TParams>
class SocketServer: public Actor {

public:
    using Params = TParams;

protected:

    Params params_;
    int listenfd_;
    std::atomic_bool stop_polling_;
    Signal polling_stopped_;

public:

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

    virtual ~SocketServer() = default;


    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketServer::start");

        // Resolve the server address and bind the host.
        *status = resolve_and_bind_host();
        if (!status->ok()) {
            sig_started->set();
            return;
        }

        // Start listening on the host.
        *status = start_listening();
        if (!status->ok()) {
            sig_started->set();
            return;
        }

        // Start polling.
        poll(sig_started);
    }


    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("SocketServer::shutdown");

        // Stop polling in a separate thread
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


    Status process_request(Request request, Reply reply) override {
        return {Error::Kind::NotImplemented};
    }

protected:

    virtual Status set_socket_options(int sockfd) {
        // Avoid "Address already in use" error
        if( ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                         &params_.opt_reuse_addr, sizeof(int)) < 0 ) {
            return {errno, "setsockopt SO_REUSEADDR failed", Error::Kind::NetErr};
        }
        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                       &params_.opt_reuse_port, sizeof(int)) < 0) {
            ::close(sockfd);
            return {errno, "setsockopt SO_REUSEPORT", Error::Kind::NetErr};
        }
        return {};
    }


    virtual Socket get_socket_info(int client_fd, sockaddr_storage* client_addr) {
        // Get client IP and port
        char client_ip[INET6_ADDRSTRLEN];
        int client_port{0};
        if (client_addr->ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in*)&client_addr;
            ::inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof client_ip);
            client_port = ::ntohs(s->sin_port);
        } else {
            struct sockaddr_in6 *s = (struct sockaddr_in6*)&client_addr;
            ::inet_ntop(AF_INET6, &s->sin6_addr, client_ip, sizeof client_ip);
            client_port = ::ntohs(s->sin6_port);
        }

        return {client_fd, client_ip, client_port};
    }


    virtual Status resolve_and_bind_host() {
        TEC_ENTER("SocketServer::resolve_and_bind_host");

        // Resolve the server address.
        TEC_TRACE("Resolving address {}:{}...", params_.addr, params_.port);
        addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family = params_.family;
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


    virtual Status start_listening() {
        TEC_ENTER("SocketServer::start_listening");

        if (::listen(listenfd_, params_.queue_size) == -1) {
            auto emsg = format("Failed to listen to {}:{}.", params_.addr, params_.port);
            ::close(listenfd_);
            listenfd_ = EOF;
            return {errno, emsg, Error::Kind::NetErr};
        }
        TEC_TRACE("Server listening on {}:{}.", params_.addr, params_.port);
        return {};
    }


    virtual Status accept_connection(int* clientfd, sockaddr_storage* client_addr) {
        TEC_ENTER("SocketServer::accept_connection");

        // Wait for incoming connection.
        socklen_t sin_size = sizeof(sockaddr_storage);
        *clientfd = ::accept(listenfd_,
                             (sockaddr *)client_addr,
                             &sin_size);
        // Check result.
        if (*clientfd == EOF) {
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

        return {};
    }


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

            // Pass the client socket for further processing.
            process_socket(sock);
        }
        polling_stopped_.set();
    }


    virtual void process_socket(Socket sock) {
        TEC_ENTER("SocketServer::process_socket");
        if(params_.mode == SocketServerParams::kModeCharStream) {
            on_string(&sock);
        }
        else if(params_.mode == SocketServerParams::kModeNetData) {
            on_net_data(&sock);
        }

        TEC_TRACE("Closing connection with {}:{}...", sock.addr, sock.port);
        ::shutdown(sock.fd, SHUT_RDWR);
        ::close(sock.fd);
    }


    virtual void on_string(Socket* sock) {
        TEC_ENTER("SocketServer::on_char_stream");
        // Default implementation just echoes received data.
        Bytes data;
        Socket::recv(data, sock, 0);
        Socket::send(data, sock);
    }


    virtual void on_net_data(Socket* sock) {
    }

}; // class SocketServer

} // namespace tec
