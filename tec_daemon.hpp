// Time-stamp: <Last changed 2025-11-15 16:18:33 by magnolia>
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
 * @file Daemon.hpp
 * @brief Defines a daemon interface for background processes in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_signal.hpp"
#include "tec/tec_message.hpp"


namespace tec {

/**
 * @class Daemon
 * @brief Abstract interface for a daemon that runs as a background process.
 * @details A daemon is a process or thread that runs continuously in the background
 * and handles periodic service messages and requests. This interface defines the minimum set of
 * methods and signals that a derived class must implement, including starting,
 * terminating, sending messages, requests, and signaling state changes.
 * @note Daemons are **non-copyable** and **non-movable** to ensure unique ownership.
 */
class Daemon {
public:
    struct Payload {
        Signal* ready;
        Status* status;
        Request request;
        Reply   reply;
    };

public:
    /**
     * @brief Default constructor.
     * @details Initializes a Daemon base class. Derived classes should provide
     * specific initialization logic.
     */
    Daemon() = default;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Ensures proper cleanup of derived classes.
     */
    virtual ~Daemon() = default;

    // Deleted to prevent accidental copying/moving.
    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;
    Daemon(Daemon&&) = delete;
    Daemon& operator=(Daemon&&) = delete;

    /**
     * @brief Starts the daemon's operation.
     * @details Initiates the daemon's background process or thread. Must be implemented
     * by derived classes.
     * @return Status The result of the start operation.
     */
    virtual Status run() = 0;

    /**
     * @brief Terminates the daemon's operation.
     * @details Stops the daemon's background process or thread. Must be implemented
     * by derived classes.
     * @return Status The result of the termination operation.
     */
    virtual Status terminate() = 0;

    /**
     * @brief Sends a control message to the daemon.
     * @details Allows external components to send a message to the daemon for processing.
     * Must be implemented by derived classes.
     * @param message The message to send.
     * @return bool True if the message was successfully sent, false otherwise.
     */
    virtual bool send(const Message& message) = 0;

    /**
     * @brief Retrieves the signal indicating the daemon is running.
     * @details Returns a reference to the signal that indicates the daemon has started
     * and is operational. Must be implemented by derived classes.
     * @return const Signal& The running signal.
     */
    virtual const Signal& sig_running() const = 0;

    /**
     * @brief Retrieves the signal indicating the daemon is initialized.
     * @details Returns a reference to the signal that indicates the daemon has completed
     * initialization, possibly with an error. Must be implemented by derived classes.
     * @return const Signal& The initialization signal.
     */
    virtual const Signal& sig_inited() const = 0;

    /**
     * @brief Retrieves the signal indicating the daemon is terminated.
     * @details Returns a reference to the signal that indicates the daemon has stopped.
     * Must be implemented by derived classes.
     * @return const Signal& The termination signal.
     */
    virtual const Signal& sig_terminated() const = 0;

    /**
     * @brief Sends a request and waits for a reply in a daemon process.
     *
     * This method sends a request of type TRequest to the daemon and waits for a corresponding reply of type TReply.
     * It uses a signal to synchronize the operation and returns the status of the request processing.
     * If the request cannot be sent, a runtime error status is returned.
     *
     * @tparam TRequest The type of the request object.
     * @tparam TReply The type of the reply object.
     * @param req Pointer to the request object to be sent.
     * @param rep Pointer to the reply object where the response will be stored.
     * @return Status The status of the request operation, indicating success or an error.
     * @see Daemon::Payload
     */
    template <typename TRequest, typename TReply>
    Status request(const TRequest* req, TReply* rep) {
        Signal ready;
        Status status;
        Payload  payload{&ready, &status, {req}, {rep}};
        if( !send({&payload}) ) {
            status = {Error::Kind::RuntimeErr};
        }
        ready.wait();
        return status;
    }

    template <typename TRequest>
    Status request(const TRequest* req) {
        Signal ready;
        Status status;
        Payload  payload{&ready, &status, {req}, {}};
        if( !send({&payload}) ) {
            status = {Error::Kind::RuntimeErr};
        }
        ready.wait();
        return status;
    }

public:
    /**
     * @struct Builder
     * @brief Factory for creating daemon instances.
     * @details Provides a templated builder to create instances of derived daemon classes,
     * hiding implementation details and ensuring type safety through static assertions.
     * @tparam Derived The derived daemon class type.
     */
    template <typename Derived>
    struct Builder {
        /**
         * @brief Creates a unique pointer to a derived daemon instance.
         * @details Constructs a new instance of the specified Derived class using the
         * provided parameters. Ensures Derived is a subclass of Daemon.
         * @param params The parameters for constructing the Derived daemon.
         * @return std::unique_ptr<Daemon> A unique pointer to the created daemon instance.
         */
        std::unique_ptr<Daemon> operator()(typename Derived::Params const& params) {
            static_assert(std::is_base_of<Daemon, Derived>::value,
                          "Derived type must inherit from tec::Daemon");
            return std::make_unique<Derived>(params);
        }
    };

}; // class Daemon

} // namespace tec
