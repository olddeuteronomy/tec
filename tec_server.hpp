// Time-stamp: <Last changed 2025-09-28 14:17:11 by magnolia>
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
 * @file tec_server.hpp
 * @brief Defines server parameters and an interface for server implementations in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_signal.hpp"
#include "tec/tec_message.hpp"


namespace tec {

/// @name Server Parameters
/// @brief Configuration parameters for server instances.
/// @details Defines default timeouts and configuration options for server operations.
/// @{

/**
 * @struct ServerParams
 * @brief Configuration parameters for server instances.
 * @details Specifies timeouts for server startup and shutdown operations, with default values.
 */
struct ServerParams {
    /**
     * @brief Default timeout for server startup.
     * @details Set to 5 seconds.
     */
    static constexpr const MilliSec kStartTimeout{Seconds{5}};

    /**
     * @brief Default timeout for server shutdown.
     * @details Set to 10 seconds.
     */
    static constexpr const MilliSec kShutdownTimeout{Seconds{10}};

    MilliSec start_timeout;    ///< Timeout for server startup in milliseconds.
    MilliSec shutdown_timeout; ///< Timeout for server shutdown in milliseconds.

    /**
     * @brief Constructs server parameters with default timeouts.
     * @details Initializes start_timeout to kStartTimeout (5 seconds) and
     * shutdown_timeout to kShutdownTimeout (10 seconds).
     */
    ServerParams()
        : start_timeout{kStartTimeout}
        , shutdown_timeout{kShutdownTimeout}
    {}
};

/// @}

/**
 * @class Server
 * @brief Abstract interface for a server implementation.
 * @details Defines the minimum set of methods for a server, including starting and
 * shutting down operations, with associated signals for state transitions. Derived
 * classes must implement these methods to provide specific server functionality.
 */
class Server {
public:
    /**
     * @brief Default constructor.
     * @details Initializes a Server base class. Derived classes should provide
     * specific initialization logic.
     */
    Server() = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    Server(const Server&) = delete;

    /**
     * @brief Deleted move constructor to prevent moving.
     */
    Server(Server&&) = delete;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Ensures proper cleanup of derived classes.
     */
    virtual ~Server() = default;

    /**
     * @brief Starts the server.
     * @details Initiates the server’s operation and signals completion via sig_started.
     * If successful, the server may run indefinitely (e.g., in gRPC) until shutdown()
     * is called from another thread. In case of timeout or error, the status argument
     * is updated with the appropriate error code (e.g., Error::Kind::Timeout).
     * @param sig_started Signal set when the server has started (possibly with an error).
     * @param status Updated with the result of the operation (e.g., Error::Kind::Ok or Error::Kind::Timeout).
     * @note In some implementations (e.g., gRPC), this method may not return until shutdown() is called.
     * @see Signal
     * @see Status
     */
    virtual void start(Signal& sig_started, Status& status) = 0;

    /**
     * @brief Shuts down the server.
     * @details Stops the server’s operation and signals completion via sig_stopped.
     * @param sig_stopped Signal set when the server has stopped.
     * @see Signal
     */
    virtual void shutdown(Signal& sig_stopped) = 0;

    /**
     * @brief Processes a request and generates a corresponding reply.
     *
     * This pure virtual method must be implemented by derived classes to handle
     * the processing of a given request and populate the provided reply object.
     * It defines the core request-handling logic for the server.
     *
     * @param request The input request object to be processed.
     * @param reply The output reply object to store the response.
     * @return Status The status of the request processing, indicating success or an error.
     * @see Status
     * @see Request
     * @see Reply
     */
    virtual Status process_request(Request& request, Reply& reply) = 0;

}; // class Server

} // namespace tec
