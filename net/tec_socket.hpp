// Time-stamp: <Last changed 2025-11-11 00:28:35 by magnolia>
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
 * @brief Generic BSD socket parameters for both client and server.
 * @note For BSD, macOS, Linux. Windows version is not implemented yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>

#include "tec/tec_def.hpp"  // IWYU pragma: keep


namespace tec {


struct SocketClientParams {

    static constexpr const int kDefaultFamily{AF_UNSPEC};     ///!< IPv4 or IPv6.
    static constexpr const int kDefaultSockType{SOCK_STREAM}; ///!< TCP.
    static constexpr const char kDefaultAddr[] = "127.0.0.1"; ///!< localhost.
    static constexpr const char kDefaultPort[] = "8080";

    std::string addr;
    std::string port;
    int family;
    int socktype;

    SocketClientParams()
        : addr{kDefaultAddr}
        , port{kDefaultPort}
        , family{kDefaultFamily}
        , socktype{kDefaultSockType}
    {}
};


struct SocketServerParams {

    static constexpr const int kDefaultFlags{AI_PASSIVE}; ///!< Use local IP.

    std::string addr;
    std::string port;
    int family;
    int socktype;
    int flags;

    SocketServerParams()
        : addr{SocketClientParams::kDefaultAddr}
        , port{SocketClientParams::kDefaultPort}
        , family{SocketClientParams::kDefaultFamily}
        , socktype{SocketClientParams::kDefaultSockType}
        , flags{kDefaultFlags}
    {}
};

}
