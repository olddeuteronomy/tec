// Time-stamp: <Last changed 2026-01-23 14:55:21 by magnolia>
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

/**
 * @brief Specialized socket wrapper optimized for sending/receiving NetData protocol messages
 *
 * SocketNd extends the basic Socket class with protocol-aware send/receive operations
 * specifically tailored for the NetData message format (header + payload).
 *
 * Main differences from base Socket class:
 * - Understands NetData::Header structure
 * - Performs header validation
 * - Uses MSG_PEEK to safely inspect message size before committing to read
 * - Combines header + payload operations into atomic-like protocol messages
 */
struct SocketNd : public Socket
{
    /**
     * @brief Constructs SocketNd by copying from an existing Socket object
     * @param sock Source Socket object to copy file descriptor and address information from
     */
    explicit SocketNd(const Socket& sock)
        : Socket(sock.fd, sock.addr, sock.port, sock.buffer, sock.buffer_size)
    {}

    /**
     * @brief Constructs SocketNd with explicit parameters
     * @param _fd File descriptor of the socket
     * @param _addr Remote peer address (usually string representation of IP)
     * @param _port Remote peer port number
     * @param _buffer Pointer to external buffer used for receive operations
     * @param _buffer_size Size of the provided external buffer (in bytes)
     */
    SocketNd(int _fd, const char* _addr, int _port, char* _buffer, size_t _buffer_size)
        : Socket(_fd, _addr, _port, _buffer, _buffer_size)
    {}

    /**
     * @brief Sends a complete NetData message (header + payload) over the socket
     *
     * The function performs two write operations:
     * 1. Writes the fixed-size NetData header
     * 2. Writes the variable-length payload (if present)
     *
     * @param nd Pointer to the NetData message to send
     * @param sock SocketNd instance representing the connection
     * @return Status success if entire message was sent,
     *         appropriate error status otherwise (EIO, connection closed, partial write, etc.)
     */
    static Status send_nd(const NetData* nd, const SocketNd* sock)
    {
        TEC_ENTER("SocketNd::send_nd");
        //
        // Write the NetData header to the stream.
        //
        ssize_t sent = write(sock->fd, &nd->header, sizeof(NetData::Header));
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
        if (nd->size() > 0) {
            return Socket::send(nd->bytes(), sock);
        }
        return {};
    }

    /**
     * @brief Receives one complete NetData message (header + payload)
     *
     * Protocol flow:
     * 1. Peeks at the header using MSG_PEEK (non-destructive read)
     * 2. Validates header sanity (magic, version, size field, etc.)
     * 3. Removes header from receive queue with normal read()
     * 4. Reads payload according to size specified in header
     * 5. Rewinds the NetData internal cursor
     *
     * Important: Uses MSG_PEEK to avoid reading partial/invalid messages.
     *            This helps implement reliable framed message reading over TCP.
     *
     * @param[out] nd Pointer to NetData object that will receive the message
     *                Must be properly constructed (buffer must be valid)
     * @param sock SocketNd instance representing the connection
     * @return Status::success() when complete message was received,
     *         various error codes when:
     *          - connection closed (EIO)
     *          - read error (errno value)
     *          - malformed header (EBADMSG)
     *          - invalid protocol fields (EBADMSG)
     *          - payload receive failure (from Socket::recv)
     */
    static Status recv_nd(NetData* nd, const SocketNd* sock)
    {
        TEC_ENTER("SocketNd::recv_nd");
        Status status;
        NetData::Header hdr;
        //
        // Read the header.
        //
        // The MSG_PEEK flag causes the receive operation to return data
        // from the beginning of the receive queue without removing that data from the queue.
        // Thus, a subsequent receive call will return the same data.
        //
        ssize_t rd = ::recv(sock->fd, &hdr, sizeof(NetData::Header),
                            MSG_PEEK);
        if (rd == 0) {
            auto errmsg = format("{}:{} Peer closed the connection.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }
        else if (rd < 0) {
            auto errmsg = format("{}:{} Socket read error.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {errno, errmsg, Error::Kind::NetErr};
        }
        else if (rd != sizeof(NetData::Header)) {
            auto errmsg = format("{}:{} NetData::Header read error.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EBADMSG, errmsg, Error::Kind::Invalid};
        }
        // Validate the header.
        else if (!hdr.is_valid()) {
            auto errmsg = format("{}:{} NetData::Header is invalid.",
                                 sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EBADMSG, errmsg, Error::Kind::Invalid};
        }
        // Re-read the header from the message queue (destructive this time).
        auto rd2 = read(sock->fd, &hdr, sizeof(NetData::Header));
        if (rd2 != sizeof(NetData::Header)) {
            return {EIO, Error::Kind::System};
        }
        //
        // Read data.
        //
        nd->header = hdr;
        if (nd->size() > 0) {
            status = Socket::recv(nd->bytes(), sock, nd->size());
        }
        nd->rewind();
        return status;
    }
};

} // namespace tec
