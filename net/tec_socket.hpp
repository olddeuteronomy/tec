// Time-stamp: <Last changed 2025-12-09 15:58:04 by magnolia>
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
 * @brief Generic BSD socket parameters.
 * @note For BSD, macOS, Linux. Windows version is not implemented yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#include <cstddef>
#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <unistd.h>
// #include <memory.h>
#include <netdb.h>
// #include <arpa/inet.h>
// #include <sys/types.h>
// #include <sys/socket.h>

// #include <cstdio>
// #include <cerrno>
#include <string>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_bytes.hpp"


namespace tec {


struct SocketParams {
    static constexpr char kDefaultAddr[]{"127.0.0.1"};    ///< localhost.
    static constexpr char kDefaultPort[]{"8080"};         ///< Test port.
    static constexpr int kDefaultFamily{AF_UNSPEC};       ///< IPv4 or IPv6.
    static constexpr int kDefaultSockType{SOCK_STREAM};   ///< TCP.
    static constexpr int kDefaultProtocol{0};             ///< Any protocol.
    static constexpr int kDefaultServerFlags{AI_PASSIVE}; ///< Server: use local IP.
    static constexpr int kDefaultClientFlags{0};          ///< Client: not set.

    std::string addr;
    std::string port;
    int family;
    int socktype;
    int protocol;
    int flags;

    SocketParams()
        : addr{kDefaultAddr}
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
    static constexpr int kModeCharStream{0};
    static constexpr int kModeNetData{1};

    static constexpr int kDefaultMode{kModeCharStream};

    /** The maximum length to which the queue of pending connections for socket fd may
       grow. If a connection request arrives when the queue is full, the client may receive an error with an
       indication of ECONNREFUSED or, if the underlying protocol supports retransmission, the request may be
       ignored so that a later reattempt at connection succeeds.
     */
    static constexpr int kDefaultConnQueueSize{4096};

    int mode;
    int queue_size;

    SocketServerParams()
        : mode{kDefaultMode}
        , queue_size{kDefaultConnQueueSize}
    {
        flags = kDefaultServerFlags;
    }
};


struct SocketCharStream {
    std::string str;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Socket helpers
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct Socket {

    int socketfd;
    std::string addr;
    int port;

    static Status recv(Bytes& data, Socket* pci, size_t length) {
        TEC_ENTER("Socket::recv");
        std::array<char, BUFSIZ> buffer;
        ssize_t received{0};
        size_t total_received{0};
        bool eof{false};

        // Read data from the client.
        while ((received = read(pci->socketfd, buffer.data(), BUFSIZ)) > 0) {
            if (length == 0) {
                // Length is unknown -- check for null-terminated char stream.
                if (buffer[received-1] == '\0') {
                    TEC_TRACE("[{}]:{} EOF received.", pci->addr, pci->port);
                    eof = true;
                 }
            }
            if (received > 0) {
                data.write(buffer.data(), received);
                TEC_TRACE("[{}]:{} --> RECV {} bytes.", pci->addr, pci->port, received);
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
            TEC_TRACE("[{}]:{} Client closed connection.", pci->addr, pci->port);
        }
        else if (received == -1) {
            auto errmsg = format("[{}]:{} socket read error {}.", pci->addr, pci->port, errno);
            TEC_TRACE(errmsg);
            return {errno, errmsg, Error::Kind::NetErr};
        }
        else if (length > 0  &&  total_received != length) {
            auto errmsg = format("[{}]:{} socket partial read: {} bytes of {}.",
                                 pci->addr, pci->port, total_received, length);
            return {EIO, errmsg, Error::Kind::NetErr};
        }

        return {};
    }


    static Status send(Bytes& data, Socket* ps) {
        TEC_ENTER("Socket::send");

        // Send data to the client
        ssize_t sent = write(ps->socketfd, data.data(), data.size());
        TEC_TRACE("[{}]:{} <-- SEND {} bytes.", ps->addr, ps->port, sent);

        // Check error.
        if (sent < 0) {
            auto errmsg = format("[{}]:{} socket write error {}.", ps->addr, ps->port, errno);
            TEC_TRACE(errmsg.c_str());
            return {errno, errmsg, Error::Kind::NetErr};
        }
        else if (data.size() != static_cast<size_t>(sent)) {
            auto errmsg = format("[{}]:{} socket partial write: {} bytes of {}.",
                                 ps->addr, ps->port, sent, data.size());
            return {EIO, errmsg, Error::Kind::NetErr};
        }

        return {};
    }


    static Status send_error(Socket* ps){
        char zero{'\0'};
        Bytes b;
        b.write(&zero, sizeof(char));
        return send(b, ps);
    }


}; // struct Socket

} // namespace tec
