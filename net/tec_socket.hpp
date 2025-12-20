// Time-stamp: <Last changed 2025-12-20 11:44:18 by magnolia>
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

#include <cstring>
#include <sys/types.h>
#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <array>
#include <string>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_memfile.hpp"


namespace tec {


struct SocketParams {
    static constexpr char kAnyAddr[]{"0.0.0.0"};          ///< IPv4 accept from any address.
    static constexpr char kLocalAddr[]{"127.0.0.1"};      ///< IPv4 localhost.

    static constexpr char kLocalURI[]{"localhost"};       ///< IPv4 and IPv6 localhost.

    static constexpr char kAnyAddrIP6[]{"::"};            ///< IPv6 accept from any address.
    static constexpr char kLocalAddrIP6[]{"::1"};         ///< IPv6 localhost.

    static constexpr int kDefaultPort{4321};              ///< Test port.
    static constexpr int kDefaultFamily{AF_UNSPEC};       ///< IPv4 or IPv6.
    static constexpr int kDefaultSockType{SOCK_STREAM};   ///< TCP.
    static constexpr int kDefaultProtocol{0};             ///< Any protocol.
    static constexpr int kDefaultServerFlags{AI_PASSIVE}; ///< Server: use local IP.
    static constexpr int kDefaultClientFlags{0};          ///< Client: not set.

    static constexpr char kNullString{0};

    std::string addr;
    int port;
    int family;
    int socktype;
    int protocol;
    int flags;

    SocketParams()
        : addr{kLocalURI}
        , port{kDefaultPort}
        , family{kDefaultFamily}
        , socktype{kDefaultSockType}
        , protocol{kDefaultProtocol}
        , flags{0}
    {}
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Client parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


struct SocketClientParams: public SocketParams {

    SocketClientParams() {
        flags = kDefaultClientFlags;
    }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Server parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


struct SocketServerParams: public SocketParams  {
    static constexpr int kOptReuseAddress{0};
    static constexpr int kOptReusePort{1};

    static constexpr int kModeCharStream{0};
    static constexpr int kModeNetData{1};
    static constexpr int kDefaultMode{kModeCharStream};

    /** The maximum length to which the queue of pending connections for socket fd may
       grow. If a connection request arrives when the queue is full, the client may receive an error with an
       indication of ECONNREFUSED or, if the underlying protocol supports retransmission, the request may be
       ignored so that a later reattempt at connection succeeds.
     */
    static constexpr int kDefaultConnQueueSize{SOMAXCONN};

    int mode;
    int queue_size;
    int opt_reuse_addr;
    int opt_reuse_port;

    SocketServerParams()
        : mode{kDefaultMode}
        , queue_size{kDefaultConnQueueSize}
        , opt_reuse_addr{kOptReuseAddress}
        , opt_reuse_port{kOptReusePort}
    {
        flags = kDefaultServerFlags;
        addr = kAnyAddr; // IPv4, use kAnyAddrIP6 to accept from both IPv4 and IPv6.
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Character stream request
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct SocketCharStreamIn {
    const std::string* str;
};

struct SocketCharStreamOut {
    std::string* str;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Socket helpers
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Socket {

    int fd;
    char addr[INET6_ADDRSTRLEN];
    int port;

    Socket(int _fd, const char* _addr, int _port)
        : fd{_fd}
        , port{_port}
    {
        std::strncpy(addr, _addr, INET6_ADDRSTRLEN);
        addr[INET6_ADDRSTRLEN-1] = '\0';
    }

    static Status recv(MemFile& data, Socket* sock, size_t length) {
        TEC_ENTER("Socket::recv");
        // Default buffer size as defined in stdio.h (8192).
        std::array<char, BUFSIZ> buffer;
        ssize_t received{0};
        size_t total_received{0};
        bool eof{false};

        // Read data from the client.
        while ((received = read(sock->fd, buffer.data(), BUFSIZ)) > 0) {
            if (length == 0) {
                // Length is unknown -- check for null-terminated char stream.
                if (buffer[received-1] == '\0') {
                    TEC_TRACE("{}:{} EOF received.", sock->addr, sock->port);
                    eof = true;
                 }
            }
            if (received > 0) {
                data.write(buffer.data(), received);
                TEC_TRACE("{}:{} --> RECV {} bytes.", sock->addr, sock->port, received);
                total_received += received;
                if (length > 0 && length == total_received) {
                    break;
                }
            }
            if (eof || received < BUFSIZ) {
                break;
            }
        }

        if (received == 0) {
            auto errmsg = format("{}:{} Peer closed the connection.", sock->addr, sock->port);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }
        else if (received == -1) {
            auto errmsg = format("{}:{} socket read error {}.", sock->addr, sock->port, errno);
            TEC_TRACE(errmsg);
            return {errno, errmsg, Error::Kind::NetErr};
        }
        else if (length > 0  &&  total_received != length) {
            auto errmsg = format("{}:{} socket partial read: {} bytes of {}.",
                                 sock->addr, sock->port, total_received, length);
            TEC_TRACE(errmsg);
            return {EIO, errmsg, Error::Kind::NetErr};
        }

        return {};
    }


    static Status send(const MemFile& bytes, Socket* sock) {
        TEC_ENTER("Socket::send");
        ssize_t sent{0};

        // Send data to the client
        if (bytes.size() > 0) {

            // NOTE: valgrind gcc/clang -O0 -g: "Syscall param write(buf) points to uninitialised byte(s)"
            sent = write(sock->fd, bytes.ptr(0), bytes.size());

            // Check error.
            if (sent < 0) {
                auto errmsg = format("{}:{} socket write error {}.", sock->addr, sock->port, errno);
                TEC_TRACE(errmsg.c_str());
                return {errno, errmsg, Error::Kind::NetErr};
            }
            else if (bytes.size() != static_cast<size_t>(sent)) {
                auto errmsg = format("{}:{} socket partial write: {} bytes of {}.",
                                     sock->addr, sock->port, sent, bytes.size());
                return {EIO, errmsg, Error::Kind::NetErr};
            }

        }
        TEC_TRACE("{}:{} <-- SEND {} bytes.", sock->addr, sock->port, sent);
        return {};
    }

}; // struct Socket

} // namespace tec
