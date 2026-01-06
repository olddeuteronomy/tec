// Time-stamp: <Last changed 2026-01-06 14:15:54 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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

#ifndef _POSIX_C_SOURCE
// This line fixes the "storage size of 'hints' isn't known" issue.
#define _POSIX_C_SOURCE 200809L
#endif

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <array>
#include <cstring>
#include <string>
#include <thread>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_memfile.hpp"
#include "tec/net/tec_compression.hpp"


namespace tec {

/**
 * @struct SocketParams
 * @brief Common parameters used for both client and server socket configuration.
 *
 * This structure holds all configurable options for creating and binding sockets,
 * including address, port, address family, socket type, protocol, and compression settings.
 */
struct SocketParams {
    /// IPv4 address to bind/accept connections from any interface.
    static constexpr char kAnyAddr[]{"0.0.0.0"};

    /// IPv4 loopback address (localhost).
    static constexpr char kLocalAddr[]{"127.0.0.1"};

    /// Hostname that resolves to localhost for both IPv4 and IPv6.
    static constexpr char kLocalURI[]{"localhost"};

    /// IPv6 address to bind/accept connections from any interface.
    static constexpr char kAnyAddrIP6[]{"::"};

    /// IPv6 loopback address (localhost).
    static constexpr char kLocalAddrIP6[]{"::1"};

    /// Default port number used for testing and examples.
    static constexpr int kDefaultPort{4321};

    /// Default address family: AF_UNSPEC allows both IPv4 and IPv6.
    static constexpr int kDefaultFamily{AF_UNSPEC};

    /// Default socket type: TCP stream socket.
    static constexpr int kDefaultSockType{SOCK_STREAM};

    /// Default protocol: 0 means "any appropriate protocol".
    static constexpr int kDefaultProtocol{0};

    /// Default addrinfo flags for server sockets (bind to local address).
    static constexpr int kDefaultServerFlags{AI_PASSIVE};

    /// Default addrinfo flags for client sockets (no special behaviour).
    static constexpr int kDefaultClientFlags{0};

    /// Null character constant (for internal use).
    static constexpr char kNullString{0};

    /// Target address or hostname.
    std::string addr;

    /// Port number to connect to or bind.
    int port;

    /// Address family (AF_INET, AF_INET6, AF_UNSPEC, ...).
    int family;

    /// Socket type (SOCK_STREAM, SOCK_DGRAM, ...).
    int socktype;

    /// Protocol (usually 0).
    int protocol;

    /// Flags passed to getaddrinfo().
    int flags;

    /// Compression algorithm to use (see CompressionParams).
    int compression;

    /// Compression level (higher = better compression, slower).
    int compression_level;

    /// Minimum size of data to apply compression (bytes).
    size_t compression_min_size;

    /**
     * @brief Default constructor.
     *
     * Initializes with sensible defaults suitable for a client connecting to localhost.
     */
    SocketParams()
        : addr{kLocalURI}
        , port{kDefaultPort}
        , family{kDefaultFamily}
        , socktype{kDefaultSockType}
        , protocol{kDefaultProtocol}
        , flags{0}
        , compression{CompressionParams::kDefaultCompression}
        , compression_level{CompressionParams::kDefaultCompressionLevel}
        , compression_min_size{CompressionParams::kMinSize}
    {}
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Client parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct SocketClientParams
 * @brief Parameters specific to client-side socket connections.
 *
 * Inherits all common parameters from SocketParams and sets client-specific defaults.
 */
struct SocketClientParams : public SocketParams {

    /**
     * @brief Default constructor.
     *
     * Sets addrinfo flags appropriate for client use (no AI_PASSIVE).
     */
    SocketClientParams() {
        flags = kDefaultClientFlags;
    }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Server parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct SocketServerParams
 * @brief Parameters specific to server-side socket configuration.
 *
 * Extends SocketParams with server-specific options such as listen queue size,
 * socket reuse options, threading model, and data handling mode.
 */
struct SocketServerParams : public SocketParams  {

    /// Disable SO_REUSEADDR option.
    static constexpr int kOptReuseAddress{0};

    /// Enable SO_REUSEADDR and SO_REUSEPORT (if supported).
    static constexpr int kOptReusePort{1};

    /// Treat incoming data as null-terminated character streams.
    static constexpr int kModeCharStream{0};

    /// Treat incoming data as length-prefixed binary network messages.
    static constexpr int kModeNetData{1};

    /// Default data handling mode.
    static constexpr int kDefaultMode{kModeCharStream};

    /// Default maximum number of threads in a custom thread pool.
    static constexpr int kDefaultMaxThreads{16};

    /**
     * @brief Maximum length of the pending connections queue for listen().
     *
     * If a connection request arrives when the queue is full, the client may receive
     * ECONNREFUSED or the request may be ignored (depending on protocol).
     */
    static constexpr int kDefaultConnQueueSize{SOMAXCONN};

    /// Whether to use a fixed-size thread pool instead of one-thread-per-connection.
    static constexpr bool kUseThreadPool{false};

    /// Data handling mode (character stream or binary network data).
    int mode;

    /// Maximum backlog for listen().
    int queue_size;

    /// Whether to set SO_REUSEADDR (0 = no, 1 = yes).
    int opt_reuse_addr;

    /// Whether to set SO_REUSEPORT (if available).
    int opt_reuse_port;

    /// Use a thread pool for handling accepted connections.
    bool use_thread_pool;

    /// Number of threads in the thread pool (if enabled).
    size_t thread_pool_size;

    /**
     * @brief Default constructor.
     *
     * Initializes with server-appropriate defaults:
     * - Binds to any address (0.0.0.0)
     * - Uses AI_PASSIVE flag
     * - Sets thread pool size to hardware concurrency
     */
    SocketServerParams()
        : mode{kDefaultMode}
        , queue_size{kDefaultConnQueueSize}
        , opt_reuse_addr{kOptReuseAddress}
        , opt_reuse_port{kOptReusePort}
        , use_thread_pool{kUseThreadPool}
    {
        addr = kAnyAddr; // IPv4, use kAnyAddrIP6 to accept from both IPv4 and IPv6.
        flags = kDefaultServerFlags;
        thread_pool_size = std::thread::hardware_concurrency();
    }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Character stream request
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct SocketCharStreamIn
 * @brief Input descriptor for character stream mode.
 *
 * Used when the server is configured in kModeCharStream to pass a pointer to
 * a received null-terminated string.
 */
struct SocketCharStreamIn {
    /// Pointer to the received null-terminated string.
    const std::string* str;
};

/**
 * @struct SocketCharStreamOut
 * @brief Output descriptor for character stream mode.
 *
 * Used when the server is configured in kModeCharStream to provide a pointer
 * to a string where the response should be stored.
 */
struct SocketCharStreamOut {
    /// Pointer to the string that will receive the response.
    std::string* str;
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Socket helpers
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @struct Socket
 * @brief Lightweight wrapper around a connected socket file descriptor.
 *
 * Stores the file descriptor along with peer address and port for logging
 * and diagnostic purposes.
 */
struct Socket {

    /// Underlying socket file descriptor.
    int fd;

    /// Peer address as a null-terminated string (IPv4 or IPv6).
    char addr[INET6_ADDRSTRLEN];

    /// Peer port number.
    int port;

    /**
     * @brief Construct a Socket wrapper from an accepted or connected fd.
     *
     * @param _fd    Socket file descriptor.
     * @param _addr  Peer address string (must fit in INET6_ADDRSTRLEN).
     * @param _port  Peer port number.
     */
    Socket(int _fd, const char* _addr, int _port)
        : fd{_fd}
        , port{_port}
    {
        std::strncpy(addr, _addr, INET6_ADDRSTRLEN);
        addr[INET6_ADDRSTRLEN-1] = '\0';
    }

    /**
     * @brief Receive data from a socket into a MemFile.
     *
     * Reads until the requested length is received, or until a null terminator
     * is detected when length == 0 (character stream mode).
     *
     * @param data    MemFile to append received data to.
     * @param sock    Pointer to the Socket instance.
     * @param length  Expected number of bytes (0 = read until null terminator).
     *
     * @return Status::OK on success, or an error status with details.
     */
    static Status recv(MemFile& data, const Socket* sock, size_t length) {
        TEC_ENTER("Socket::recv");
        // Default buffer size as defined in stdio.h (8192).
        std::array<char, BUFSIZ> buffer;
        ssize_t received{0};
        size_t total_received{0};
        bool eof{false};
        //
        // Read data from the socket.
        //
        while ((received = read(sock->fd, buffer.data(), BUFSIZ)) > 0) {
            if (length == 0 && received > 0) {
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
        //
        // Check for errors.
        //
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

    /**
     * @brief Send the entire contents of a MemFile through a socket.
     *
     * Blocks until all data is sent or an error occurs.
     *
     * @param data  MemFile containing data to send.
     * @param sock  Pointer to the Socket instance.
     *
     * @return Status::OK on success, or an error status with details.
     */
    static Status send(const MemFile& data, const Socket* sock) {
        TEC_ENTER("Socket::send");
        ssize_t sent{0};
        //
        // Write data to the socket.
        //
        if (data.size() > 0) {
            sent = write(sock->fd, data.ptr(0), data.size());
            //
            // Check for errors.
            //
            if (sent < 0) {
                auto errmsg = format("{}:{} socket write error {}.", sock->addr, sock->port, errno);
                TEC_TRACE(errmsg.c_str());
                return {errno, errmsg, Error::Kind::NetErr};
            }
            else if (data.size() != static_cast<size_t>(sent)) {
                auto errmsg = format("{}:{} socket partial write: {} bytes of {}.",
                                     sock->addr, sock->port, sent, data.size());
                return {EIO, errmsg, Error::Kind::NetErr};
            }

        }
        TEC_TRACE("{}:{} <-- SEND {} bytes.", sock->addr, sock->port, sent);
        return {};
    }

}; // struct Socket

} // namespace tec
