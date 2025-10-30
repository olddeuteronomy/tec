// Time-stamp: <Last changed 2025-10-30 13:40:18 by magnolia>
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
 * @file tec_actor.hpp
 * @brief Defines an interface for actor implementations in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-10-28
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_signal.hpp"
#include "tec/tec_message.hpp"


namespace tec {

/**
 * @struct ActorParams
 * @brief Base configuration parameters for actor instances.
 */
struct ActorParams {
};


/**
 * @class Actor
 * @brief Abstract interface for an actor implementation.
 * @details Defines the minimum set of methods for an actor, including starting and
 * shutting down operations, with associated signals for state transitions. Derived
 * classes must implement these methods to provide specific actor functionality.
 */
class Actor {
public:
    struct SignalOnExit {
        explicit SignalOnExit(Signal* sig): sig_{sig} {}
        ~SignalOnExit() { sig_->set(); }

    private:
        Signal* sig_;
    };

public:
    /**
     * @brief Default constructor.
     * @details Initializes an Actor base class. Derived classes should provide
     * specific initialization logic.
     */
    Actor() = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    Actor(const Actor&) = delete;

    /**
     * @brief Deleted move constructor to prevent moving.
     */
    Actor(Actor&&) = delete;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Ensures proper cleanup of derived classes.
     */
    virtual ~Actor() = default;

    /**
     * @brief Starts the actor.
     * @details Initiates the actor’s operation and signals completion via sig_started.
     * If successful, the actor may run indefinitely (e.g., in gRPC) until shutdown()
     * is called from another thread. In case of timeout or error, the status argument
     * is updated with the appropriate error code (e.g., Error::Kind::Timeout).
     * @param sig_started Signal set when the actor has started (possibly with an error).
     * @param status Updated with the result of the operation (e.g., Error::Kind::Ok or Error::Kind::Timeout).
     * @note In some implementations (e.g., gRPC), this method may not return until shutdown() is called.
     * @see Signal
     * @see Status
     */
    virtual void start(Signal* sig_started, Status* status) = 0;

    /**
     * @brief Shuts down the actor.
     * @details Stops the actor’s operation and signals completion via sig_stopped.
     * @param sig_stopped Signal set when the server has stopped.
     * @see Signal
     */
    virtual void shutdown(Signal* sig_stopped) = 0;

    /**
     * @brief Processes a request and generates a corresponding reply.
     *
     * This pure virtual method must be implemented by derived classes to handle
     * the processing of a given request and populate the provided reply object.
     * It defines the core request-handling logic for the actor.
     *
     * @param request The input request object to be processed.
     * @param reply The output reply object to store the response.
     * @return Status The status of the request processing, indicating success or an error.
     * @see Status
     * @see Request
     * @see Reply
     */
    virtual Status process_request(Request request, Reply reply) = 0;

}; // class Actor


} // namespace tec
