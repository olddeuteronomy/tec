// Time-stamp: <Last changed 2025-12-06 15:59:25 by magnolia>
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

#include "tec/tec_print.hpp"
#include <atomic>
#include <cerrno>
#include <csignal>
#include <string>
#include <thread>

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <memory.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_signal.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_actor.hpp"
#include "tec/net/tec_socket.hpp"



namespace tec {

template <typename TParams>
class SocketServer: public Actor {

public:
    typedef TParams Params;

protected:
    Params params_;
    int listenfd_;
    std::atomic_bool stop_polling_;
    Signal polling_stopped_;

protected:
    struct ClientInfo {
        int fd;
        std::string addr;
        int port;
    };

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


    virtual Status set_socket_options(int sockid) {
        // Avoid "Address already in use" error
        int yes{1};
        if( ::setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 ) {
            return {EADDRNOTAVAIL, "`setsockopt' SO_REUSEADDR failed"};
        }
        return {};
    }


    virtual ClientInfo get_client_info(int client_fd, sockaddr_storage* client_addr) {
        // Get client IP and port
        char client_ip[INET6_ADDRSTRLEN];
        int client_port;
        if (client_addr->ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
            ::inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof client_ip);
            client_port = ntohs(s->sin_port);
        } else {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
            ::inet_ntop(AF_INET6, &s->sin6_addr, client_ip, sizeof client_ip);
            client_port = ntohs(s->sin6_port);
        }
        return {client_fd, client_ip, client_port};
    }


    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketServer::start");

        // Resolve the server address.
        TEC_TRACE("Resolving host [{}]:{}...", params_.addr, params_.port);
        addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family = params_.family;
        hints.ai_socktype = params_.socktype;
        hints.ai_protocol = params_.protocol;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully bind().
        addrinfo* servinfo{NULL};
        int ecode = ::getaddrinfo(params_.addr.c_str(), params_.port.c_str(),
                                  &hints, &servinfo);
        if( ecode != 0 ) {
            std::string emsg{::gai_strerror(ecode)};
            TEC_TRACE("Host resolving error: {}", emsg);
            *status = {ecode, emsg, Error::Kind::NetErr};
            sig_started->set();
            return;
        }
        TEC_TRACE("Host resolved OK.");

        // If socket() (or bind()) fails, we close the socket
        // and try the next address.
        addrinfo* p{NULL};
        int fd{-1};
        TEC_TRACE("Binding...");
        for( p = servinfo; p != NULL; p = p->ai_next ) {
            fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if( fd == -1 ) {
                // Try next socket.
                continue;
            }

            // Set options.
            auto option_status = set_socket_options(fd);
            if( !option_status ) {
                close(fd);
                ::freeaddrinfo(servinfo);
                *status = option_status;
                sig_started->set();
                return;
            }

            if( ::bind(fd, p->ai_addr, p->ai_addrlen) != -1 ) {
                // Success.
                break;
            }

            // Try next socket.
            close(fd);
        }

        // No longer needed.
        ::freeaddrinfo(servinfo);

        if( p == NULL ) {
            auto emsg{format(
                    "Failed to bind to [{}]:{}", params_.addr, params_.port
                    )};
            TEC_TRACE(emsg);
            *status = {EAFNOSUPPORT, emsg, Error::Kind::NetErr};
            sig_started->set();
            return;
        }

        // Success.
        listenfd_ = fd;

        // Start listening
        if( listen(listenfd_, 16) == -1 ) {
            auto emsg{format(
                    "Failed to listen to [{}]:{}", params_.addr, params_.port
                    )};
            close(listenfd_);
            *status = {EADDRNOTAVAIL, emsg, Error::Kind::NetErr};
            sig_started->set();
            return;
        }
        TEC_TRACE("Server listening on [{}]:{}", params_.addr, params_.port);

        // Start polling.
        poll(sig_started, status);
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

        TEC_TRACE("Server stopped");
        sig_stopped->set();
    }


    virtual void poll(Signal* sig_started, Status* status) {
        TEC_ENTER("SocketServer::poll");
        sig_started->set();
        while (!stop_polling_) {
            sockaddr_storage client_addr;
            socklen_t sin_size = sizeof(client_addr);
            // Wait for incoming connection.
            int clientfd = ::accept(listenfd_,
                                    (sockaddr *)&client_addr,
                                    &sin_size);
            // Check result.
            if (clientfd == -1) {
                std::string err_msg;
                if( errno == EINVAL || errno == EINTR || errno == EBADF ) {
                    err_msg = format("Polling interrupted by signal {}", errno);
                }
                else {
                    err_msg = format("accept() failed with errno={}", errno);
                }
                TEC_TRACE(err_msg);
                continue;
            }

            ClientInfo ci = get_client_info(clientfd, &client_addr);
            TEC_TRACE("Accepted connection from [{}]:{}", ci.addr, ci.port);

            // Pass the client for further processing.
            on_received(ci);
        }
        polling_stopped_.set();
    }


    virtual void on_received(ClientInfo ci) {
        echo(&ci);
    }


    #define MAX_BUFFER 4096
    virtual void echo(ClientInfo* pci) {
        TEC_ENTER("SocketServer::echo");
        char buffer[MAX_BUFFER];
        ssize_t received{0};

        while ((received = ::read(pci->fd, buffer, MAX_BUFFER - 1)) > 0) {
            buffer[received] = '\0';
            TEC_TRACE("--> [{}]:{} Received {} bytes.", pci->addr, pci->port, received);

            // Echo back
            if (::write(pci->fd, buffer, received) != received) {
                break;
            }
            TEC_TRACE("<-- [{}]:{} Sent {} bytes.", pci->addr, pci->port, received);
        }

        if (received == 0) {
            TEC_TRACE("[{}]:{} Client closed connection.", pci->addr, pci->port);
        }
        else if (received == -1) {
            TEC_TRACE("Error={} [{}]:{} read/write error.", errno, pci->addr, pci->port);
        }

        TEC_TRACE("Closing connection with [{}]:{}...", pci->addr, pci->port);
        ::shutdown(pci->fd, SHUT_RDWR);
        ::close(pci->fd);
        pci->fd = -1;
    }

    Status process_request(Request request, Reply reply) override {
        return {Error::Kind::NotImplemented};
    }

}; // class SocketServer

} // namespace tec
