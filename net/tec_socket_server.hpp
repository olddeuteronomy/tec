// Time-stamp: <Last changed 2025-11-12 02:16:21 by magnolia>
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

#include "tec/tec_signal.hpp"
#include <atomic>
#include <cerrno>
#include <csignal>
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <memory.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>
#include <thread>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
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

public:
    explicit SocketServer(const Params& params)
        : Actor()
        , params_{params}
        , listenfd_{-1}
        , stop_polling_{false}
        , polling_stopped_{}
    {
        static_assert(
            std::is_base_of<SocketServerParams, Params>::value,
            "not derived from tec::SocketClientParams class");
    }


    virtual Status set_socket_options(int sockid) {
        // Avoid "Address already in use" error
        int yes{1};
        if( ::setsockopt(sockid, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 ) {
            return {"`setsockopt' SO_REUSEADDR failed"};
        }
        return {};
    }


    virtual void poll(Signal* sig_started, Status* status) {
        TEC_ENTER("SocketServer::poll");
        sig_started->set();
        while (!stop_polling_) {
            sockaddr_storage client_addr;
            socklen_t sin_size = sizeof client_addr;
            int clientfd = ::accept(listenfd_,
                                    (sockaddr *)&client_addr,
                                    &sin_size);

            if (clientfd == -1) {
                if( errno == EINVAL || errno == EINTR || errno == EBADF ) {
                    TEC_TRACE("Polling interrupted by signal {}", errno);
                    continue;
                }
                TEC_TRACE("`accept' failed with errno={}", errno);
                continue;
            }
        }
        polling_stopped_.set();
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
            *status = {emsg, Error::Kind::NetErr};
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
            *status = {emsg, Error::Kind::NetErr};
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
            close(listenfd_);
        });
        polling_thread.join();

        TEC_TRACE("Closing server socket...");
        polling_stopped_.wait();

        TEC_TRACE("Server stopped");
        sig_stopped->set();
    }


    Status process_request(Request request, Reply reply) override {
        return {Error::Kind::NotImplemented};
    }

}; // class SocketServer

} // namespace tec
