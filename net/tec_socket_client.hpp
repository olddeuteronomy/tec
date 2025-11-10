// Time-stamp: <Last changed 2025-11-11 01:11:55 by magnolia>
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
 * @file tec_socket_client.hpp
 * @brief Generic BSD socket client implemented as Actor.
 * @note For BSD, macOS, Linux. Windows version is not implemented yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <memory.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_actor.hpp"
#include "tec/net/tec_socket.hpp"


namespace tec {

template <typename TParams>
class SocketClient: public Actor {

public:
    typedef TParams Params;

protected:
    Params params_;
    int sockfd_;

public:
    explicit SocketClient(const Params& params)
        : Actor()
        , params_{params}
        , sockfd_{-1}
    {
        static_assert(
            std::is_base_of<SocketClientParams, Params>::value,
            "not derived from tec::SocketClientParams class");
    }


    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketClient::start");
        Actor::SignalOnExit on_exit{sig_started};

        // Resolve the server address.
        TEC_TRACE("Resolving server [{}]:{}...", params_.addr, params_.port);
        addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family = params_.family;
        hints.ai_socktype = params_.socktype;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect().
        addrinfo* servinfo{NULL};
        int ecode = ::getaddrinfo(params_.addr.c_str(), params_.port.c_str(),
                                  &hints, &servinfo);
        if( ecode != 0 ) {
            std::string emsg{::gai_strerror(ecode)};
            TEC_TRACE("Server resolving error: {}", emsg);
            *status = {ecode, emsg, Error::Kind::NetErr};
            return;
        }
        TEC_TRACE("Server resolved OK.");

        // If socket() (or connect()) fails, we close the socket
        // and try the next address.
        addrinfo* p{NULL};
        int fd{-1};
        TEC_TRACE("Connecting...");
        for( p = servinfo; p != NULL; p = p->ai_next ) {
            fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if( fd == -1 ) {
                // Try next socket.
                continue;
            }

            if( ::connect(fd, p->ai_addr, p->ai_addrlen) != -1 ) {
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
                    "Failed to connect to [{}]:{}", params_.addr, params_.port
                    )};
            *status = {emsg, Error::Kind::NetErr};
            TEC_TRACE(emsg);
            return;
        }

        // Success.
        sockfd_ = fd;
        TEC_TRACE("Connected OK.");
    }


    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("SocketClient::shutdown");
        Actor::SignalOnExit on_exit(sig_stopped);
        ::shutdown(sockfd_, SHUT_RDWR);
        close(sockfd_);
    }


    Status process_request(Request request, Reply reply) override {
        return {Error::Kind::NotImplemented};
    }

};

}
