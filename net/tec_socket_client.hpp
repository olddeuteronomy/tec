// Time-stamp: <Last changed 2025-12-24 15:15:37 by magnolia>
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

#include <cerrno>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_message.hpp"
#include "tec/tec_actor.hpp"
#include "tec/tec_memfile.hpp"
#include "tec/net/tec_socket.hpp"


namespace tec {

template <typename TParams>
class SocketClient: public Actor {

public:
    using Params = TParams;

protected:
    Params params_;
    int sockfd_;

public:
    explicit SocketClient(const Params& params)
        : Actor()
        , params_{params}
        , sockfd_{EOF}
    {
        static_assert(
            std::is_base_of<SocketClientParams, Params>::value,
            "Not derived from tec::SocketClientParams class");
    }

    virtual ~SocketClient() {
        if (sockfd_ != EOF) {
            terminate();
        }
    }


    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketClient::start");
        Actor::SignalOnExit on_exit{sig_started};

        // Resolve the server address.
        TEC_TRACE("Resolving address {}:{}...", params_.addr, params_.port);
        addrinfo hints;
        ::memset(&hints, 0, sizeof(hints));
        hints.ai_family = params_.family;
        hints.ai_socktype = params_.socktype;
        hints.ai_protocol = params_.protocol;

        // getaddrinfo() returns a list of address structures.
        // Try each address until we successfully connect().
        addrinfo* servinfo{NULL};
        char port_str[16];
        ::snprintf(port_str, 15, "%d", params_.port);
        int ecode = ::getaddrinfo(params_.addr.c_str(), port_str,
                                  &hints, &servinfo);
        if( ecode != 0 ) {
            std::string emsg = ::gai_strerror(ecode);
            TEC_TRACE("Error resolving address: {}", emsg);
            *status = {ecode, emsg, Error::Kind::NetErr};
            return;
        }
        TEC_TRACE("Address resolved OK.");

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
            auto emsg = format("Failed to connect to {}:{}", params_.addr, params_.port);
            *status = {ECONNREFUSED, emsg, Error::Kind::NetErr};
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
        sockfd_ = EOF;
    }


    Status process_request(Request request, Reply reply) override {
        TEC_ENTER("SocketClient::process_request");
        if( request.type() == typeid(const SocketCharStreamIn*) ) {
            // Process raw character stream.
            const SocketCharStreamIn* req = std::any_cast<const SocketCharStreamIn*>(request);
            SocketCharStreamOut* rep = nullptr;
            if( reply.has_value() ) {
                rep = std::any_cast<SocketCharStreamOut*>(reply);
            }
            return send_recv_string(req, rep);
        }

        return {Error::Kind::NotImplemented};
    }


    Status request_str(const std::string* str_in, std::string* str_out) {
        TEC_ENTER("SocketClient::request");
        SocketCharStreamIn request{str_in};
        SocketCharStreamOut reply{str_out};
        auto status = send_recv_string(&request, &reply);
        if (status && (str_out != nullptr)) {
            *str_out = *reply.str;
        }
        return status;
    }

protected:

    virtual Status set_socket_options(int sockfd) {
        return {};
    }


    virtual Status send_string(const SocketCharStreamIn* request) {
        TEC_ENTER("SocketClient::send_string");
        if (request && request->str) {
            // `request` must be valid.
            MemFile data(*request->str);
            Socket sock{sockfd_, params_.addr.c_str(), params_.port};
            auto status = Socket::send(data, &sock);
            return status;
        }
        return {EFAULT, Error::Kind::Invalid};
    }

    virtual Status recv_string(SocketCharStreamOut* reply) {
        TEC_ENTER("SocketClient::recv_string");
        if (reply == nullptr) {
            // Notification message -- no reply required.
            return {};
        }
        if (reply->str == nullptr) {
            return {EFAULT, Error::Kind::Invalid};
        }
        MemFile data;
        Socket sock{sockfd_, params_.addr.c_str(), params_.port};
        auto status = Socket::recv(data, &sock, 0);
        if (status) {
            *reply->str = static_cast<const char*>(data.data());
        }
        return status;
    }

    virtual Status send_recv_string(const SocketCharStreamIn* request, SocketCharStreamOut* reply) {
        TEC_ENTER("SocketClient::send_recv_string");
        auto status = send_string(request);
        if (status) {
            status = recv_string(reply);
        }
        else {
            terminate();
        }
        return status;
    }

};

}
