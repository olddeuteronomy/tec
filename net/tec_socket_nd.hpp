// Time-stamp: <Last changed 2025-12-21 11:40:21 by magnolia>
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
 * @brief NetData BSD socket operations.
 * @note For BSD, macOS, Linux.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <cerrno>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket.hpp"


namespace tec {


struct SocketNd: public Socket {

    SocketNd(int _fd, const char* _addr, int _port)
        : Socket(_fd, _addr, _port)
    {}


    static Status send_nd(const NetData* nd, SocketNd* sock) {
        TEC_ENTER("SocketNd::send_nd");
        //
        // Write the NetData header to the stream.
        //
        const NetData::Header* hdr = nd->header();
        ssize_t sent = write(sock->fd, hdr, sizeof(NetData::Header));
        if (sent == 0) {
            auto errmsg = format("{}:{} Peer closed the connection.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }
        else if (sent != sizeof(NetData::Header)) {
            auto errmsg = format("{}:{} NetData::Header write error.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }
        //
        // Write data to the stream.
        //
        if (hdr->size > 0) {
            return Socket::send(nd->bytes(), sock);
        }
        return {};
    }


    static Status recv_nd(NetData* nd, SocketNd* sock) {
        TEC_ENTER("SocketNd::recv_nd");
        Status status;
        NetData::Header* hdr = nd->header();
        //
        // Read the header.
        //
        // The MSG_PEEK flag causes the receive operation to return data
        // from the beginning of the receive queue without removing that data from the queue.
        // Thus, a subsequent receive call will return the same data.
        //
        ssize_t rd = ::recv(sock->fd, hdr, sizeof(NetData::Header),
                            MSG_PEEK);
        if (rd == 0) {
            auto errmsg = format("{}:{} Peer closed the connection.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }
        else if (rd != sizeof(NetData::Header)) {
            auto errmsg = format("{}:{} NetData::Header read error.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EBADMSG, errmsg, Error::Kind::Invalid};
        }
        // Validate the header.
        else if (!hdr->is_valid()) {
            auto errmsg = format("{}:{} NetData::Header is invalid.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EBADMSG, errmsg, Error::Kind::Invalid};

        }
        // Re-read the header from the message queue.
        read(sock->fd, hdr, sizeof(NetData::Header));
        //
        // Read data.
        //
        if (hdr->size > 0) {
            status = Socket::recv(nd->bytes(), sock, hdr->size);
        }
        nd->rewind();
        return status;
    }
};

} // namespace tec
