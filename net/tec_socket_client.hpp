// Time-stamp: <Last changed 2026-02-07 14:13:52 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/
/**
 * @file tec_socket_client.hpp
 * @brief Definition of the SocketClient class template.
 *
 * This file contains the implementation of the SocketClient class,
 * which is a templated actor-based client for socket communications.
 * It handles connection establishment, data transmission (especially
 * string-based), and lifecycle management. The class uses a parameter
 * template to allow customization while ensuring derivation from
 * SocketClientParams via static assertion.
 *
 * The SocketClient supports raw character stream processing and is
 * extensible for other data types (e.g., NetData in separate
 * headers). It leverages system calls for socket operations and
 * provides virtual hooks for customization, such as setting socket
 * options or overriding send/receive behavior.
 *
 * @note This class is part of the TEC library
 *       namespace and inherits from Actor for asynchronous or
 *       threaded operation integration.
 *
 * @author The Emacs Cat
 * @date 2025-12-02
 */

#pragma once

#include <cstddef>
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <cerrno>
#include <cstdlib>
#include <vector>

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

/**
 * @class SocketClient
 * @brief Templated client socket actor for establishing and managing connections.
 *
 * The SocketClient class provides a mechanism to connect to a server using socket APIs, send and receive
 * data (primarily strings via SocketCharStreamIn/Out), and handle actor lifecycle events like start and
 * shutdown. It uses a template parameter for configuration, ensuring type safety through static assertion.
 *
 * Key features:
 * - Resolves server addresses using getaddrinfo.
 * - Establishes connections with fallback on multiple address candidates.
 * - Supports sending and receiving null-terminated strings.
 * - Provides virtual methods for customization (e.g., socket options, send/receive logic).
 * - Integrates with Actor for signal-based start/stop and request processing.
 *
 * @tparam TParams The parameter type, which must derive from SocketClientParams for configuration.
 *
 * @see Actor
 * @see SocketClientParams
 * @see Socket
 */
template <typename TParams>
class SocketClient: public Actor {
private:
    /// @brief Internal buffer used for send and receive operations.
    /// This vector holds raw character data and is resized based on params_.buffer_size.
    std::vector<char> buffer_;

public:
    /// @brief Type alias for the template parameter TParams.
    /// This allows easy reference to the params type within the class.
    using Params = TParams;

protected:
    /// @brief Instance of the parameters used for configuration.
    /// This holds settings like address, port, family, socktype, protocol, and buffer_size.
    Params params_;

    /// @brief Socket file descriptor for the established connection.
    /// Initialized to EOF (-1) and set upon successful connection.
    int sockfd_;

    /**
     * @brief Returns a pointer to the internal buffer.
     * @return char* Pointer to the buffer's data.
     */
    constexpr char* get_buffer() { return buffer_.data(); }

    /**
     * @brief Returns the size of the internal buffer.
     * @return size_t The buffer size as specified in params_.
     */
    constexpr size_t get_buffer_size() { return params_.buffer_size; }

public:
    /**
     * @brief Constructs a SocketClient with the given parameters.
     *
     * Initializes the actor base, copies the parameters, and sets the socket descriptor to EOF.
     * Performs a static assertion to ensure TParams derives from SocketClientParams.
     *
     * @param params The configuration parameters for the client.
     */
    explicit SocketClient(const Params& params)
        : Actor()
        , params_{params}
        , sockfd_{EOF}
    {
        static_assert(
            std::is_base_of<SocketClientParams, Params>::value,
            "Not derived from tec::SocketClientParams class");
    }

    /**
     * @brief Destructor that ensures the socket is terminated if still open.
     *
     * Calls terminate() if sockfd_ is not EOF to cleanly close the connection.
     */
    virtual ~SocketClient() {
        if (sockfd_ != EOF) {
            terminate();
        }
    }

    /**
     * @brief Starts the client by resolving the address and establishing a connection.
     *
     * This override of Actor::start resolves the server address using getaddrinfo, attempts to create
     * a socket and connect to each resolved address until successful. Allocates the buffer upon success.
     *
     * @param sig_started Signal to trigger on exit (success or failure).
     * @param status Pointer to Status object to report errors.
     */
    void start(Signal* sig_started, Status* status) override {
        TEC_ENTER("SocketClient::start");
        Signal::OnExit on_exit(sig_started);

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
        //
        // Success. Allocate buffer for send/recv operations.
        //
        sockfd_ = fd;
        TEC_TRACE("Connected OK.");
        buffer_.resize(get_buffer_size());
        TEC_TRACE("Buffer size is {} bytes.", get_buffer_size());
    }

    /**
     * @brief Shuts down the client connection.
     *
     * This override of Actor::shutdown calls ::shutdown to stop RD/WR operations and closes the socket.
     * Resets sockfd_ to EOF.
     *
     * @param sig_stopped Signal to trigger on exit.
     */
    void shutdown(Signal* sig_stopped) override {
        TEC_ENTER("SocketClient::shutdown");
        Signal::OnExit on_exit(sig_stopped);
        ::shutdown(sockfd_, SHUT_RDWR);
        close(sockfd_);
        sockfd_ = EOF;
    }

    /**
     * @brief Processes incoming requests, handling SocketCharStreamIn types.
     *
     * Checks the request type and delegates to send_recv_string for
     * string streams. Returns NotImplemented for other types (e.g.,
     * future NetData support).
     *
     * @param request The incoming request (std::any).
     * @param reply Optional reply object (std::any).
     * @return Status indicating success or error.
     */
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

        // Process future NetData request. See `tec_socket_client_nd.hpp`
        // that implements NetData processing.
        return {Error::Kind::NotImplemented};
    }

    /**
     * @brief Convenience method to send a string request and receive a response.
     *
     * Wraps the input/output strings in SocketCharStreamIn/Out and
     * calls send_recv_string. Copies the response back to str_out if
     * provided and successful.
     *
     * @param str_in Pointer to the input string to send.
     * @param str_out Pointer to the output string to receive (optional).
     * @return Status indicating success or error.
     */
    Status request_str(const std::string* str_in, std::string* str_out) {
        TEC_ENTER("SocketClient::request_str");
        SocketCharStreamIn request{str_in};
        SocketCharStreamOut reply{str_out};
        auto status = send_recv_string(&request, &reply);
        if (status && (str_out != nullptr)) {
            *str_out = *reply.str;
        }
        return status;
    }

protected:

    /**
     * @brief Virtual hook to set custom socket options after connection.
     *
     * This method is called after successful connection but before
     * buffer allocation (in derived classes). Default implementation
     * does nothing.
     *
     * @param sockfd The socket file descriptor.
     * @return Status indicating success (empty) or error.
     */
    virtual Status set_socket_options(int sockfd) {
        return {};
    }

    /**
     * @brief Sends a string over the socket.
     *
     * Converts the input string to Bytes (including null terminator)
     * and uses Socket::send. Validates the request before sending.
     *
     * @param request Pointer to SocketCharStreamIn containing the string.
     * @return Status indicating success or error (e.g., Invalid if request is null).
     */
    virtual Status send_string(const SocketCharStreamIn* request) {
        TEC_ENTER("SocketClient::send_string");
        if (request && request->str) {
            // `request` must be valid.
            Bytes data(request->str->data(), request->str->size() + 1);
            Socket sock{sockfd_, params_.addr.c_str(), params_.port,
                        get_buffer(), get_buffer_size()};
            auto status = Socket::send(data, &sock);
            return status;
        }
        return {EFAULT, Error::Kind::Invalid};
    }

    /**
     * @brief Receives a string from the socket.
     *
     * Uses Socket::recv to receive into Bytes, then copies to the
     * reply string (assuming null-terminated). Skips if reply is null
     * (notification mode).
     *
     * @param reply Pointer to SocketCharStreamOut to store the received string.
     * @return Status indicating success or error (e.g., Invalid if reply->str is null).
     */
    virtual Status recv_string(SocketCharStreamOut* reply) {
        TEC_ENTER("SocketClient::recv_string");
        if (reply == nullptr) {
            // Notification message -- no reply required.
            return {};
        }
        if (reply->str == nullptr) {
            return {EFAULT, Error::Kind::Invalid};
        }
        Bytes data;
        Socket sock{sockfd_, params_.addr.c_str(), params_.port,
                    get_buffer(), get_buffer_size()};
        auto status = Socket::recv(data, &sock, 0);
        if (status) {
            *reply->str = static_cast<const char*>(data.data());
        }
        return status;
    }

    /**
     * @brief Sends a request and receives a reply in one operation.
     *
     * Calls send_string followed by recv_string. Terminates the
     * connection if send fails.
     *
     * @param request Pointer to SocketCharStreamIn for sending.
     * @param reply Pointer to SocketCharStreamOut for receiving (optional).
     * @return Status indicating overall success or error.
     */
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

}; // class SocketClient

} // namespace tec
