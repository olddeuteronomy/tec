// Time-stamp: <Last changed 2025-09-17 13:58:25 by magnolia>
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
 * @file tec_client.hpp
 * @brief Defines client parameters and an interface for client implementations in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_utils.hpp"


namespace tec {

/// @name Client Parameters
/// @brief Configuration parameters for client instances.
/// @details Defines default settings for client connection timeouts.
/// @{

/**
 * @struct ClientParams
 * @brief Configuration parameters for client instances.
 * @details Specifies the connection timeout for client operations, with a default value.
 */
struct ClientParams {
    /**
     * @brief Default timeout for client connection.
     * @details Set to 5 seconds.
     */
    static constexpr const MilliSec kConnectTimeout{Seconds{5}};

    MilliSec connect_timeout; ///< Timeout for client connection in milliseconds.

    /**
     * @brief Constructs client parameters with default timeout.
     * @details Initializes connect_timeout to kConnectTimeout (5 seconds).
     */
    ClientParams()
        : connect_timeout{kConnectTimeout}
    {}
};

/// @}

/**
 * @class Client
 * @brief Abstract interface for a client implementation.
 * @details Defines the minimum set of methods for a client, including connecting to a server
 * and closing the connection. Derived classes must implement these methods to provide
 * specific client functionality.
 */
class Client {
public:
    /**
     * @brief Default constructor.
     * @details Initializes a Client base class. Derived classes should provide
     * specific initialization logic.
     */
    Client() = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    Client(const Client&) = delete;

    /**
     * @brief Deleted move constructor to prevent moving.
     */
    Client(Client&&) = delete;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Ensures proper cleanup of derived classes.
     */
    virtual ~Client() = default;

    /**
     * @brief Connects to the server.
     * @details Initiates a connection to the server, respecting the timeout specified
     * in the client's parameters. Must be implemented by derived classes.
     * @return Status The result of the connection attempt (e.g., Error::Kind::Ok or an error).
     * @see Status
     */
    virtual Status connect() = 0;

    /**
     * @brief Closes the connection to the server.
     * @details Terminates the client's connection to the server. Must be implemented
     * by derived classes.
     */
    virtual void close() = 0;
}; // class Client

} // namespace tec
