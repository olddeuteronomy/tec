// Time-stamp: <Last changed 2025-12-16 02:17:30 by magnolia>
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
 * @file tec_socket.hpp
 * @brief Generic BSD socket parameters and helpers.
 * @note For BSD, macOS, Linux.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#include <cerrno>
#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <string>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket.hpp"


namespace tec {


struct SocketNd: public Socket {

    SocketNd(int _fd, const std::string& _addr, int _port)
        : Socket(_fd, _addr, _port)
    {}


    static Status send_nd(const NetData* nd, SocketNd* sock) {
        TEC_ENTER("SocketNd::send_nd");
        // Write header to the stream.
        const NetData::Header* hdr = nd->header();
        ssize_t sent = write(sock->fd, hdr, sizeof(NetData::Header));
        if (sent != sizeof(NetData::Header)) {
            int ecode{EIO};
            auto errmsg = format("{}:{} NetData::Header write error.",
                                 sock->addr, sock->port, ecode);
            TEC_TRACE(errmsg.c_str());
            return {ecode, errmsg, Error::Kind::NetErr};
        }
        // Write data to the stream.
        if (hdr->size > 0) {
            return Socket::send(nd->bytes(), sock);
        }
        return {};
    }


    static Status recv_nd(NetData* nd, SocketNd* sock) {
        TEC_ENTER("SocketNd::recv_nd");
        // Read the header.
        ssize_t rd = read(sock->fd, nd->header(), sizeof(NetData::Header));
        if (rd != sizeof(NetData::Header)) {
            int ecode{EIO};
            auto errmsg = format("{}:{} NetData::Header read error.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg.c_str());
            return {ecode, errmsg, Error::Kind::NetErr};
        }
        // Validate data.
        if (!nd->header()->is_valid()) {
            int ecode{EBADMSG};
            auto errmsg = format("{}:{} NetData::Header is invalid.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg.c_str());
            return {ecode, errmsg, Error::Kind::Invalid};

        }
        // Read data.
        if (nd->size() > 0) {
            return Socket::recv(nd->bytes(), sock, nd->header()->size);
        }
        return {};
    }
};

} // namespace tec
