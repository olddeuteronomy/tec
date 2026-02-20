// Time-stamp: <Last changed 2026-02-20 15:54:33 by magnolia>
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
 * @file tec_daemon.hpp
 * @brief Defines the minimal contract for any long-lived service or processing component
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
 * @brief Abstract interface for a daemon that runs in a separate thread.
 * @details A daemon is a process or thread that runs continuously in the background
 * and handles periodic service messages and requests. This interface defines the minimum set of
 * methods and signals that a derived class must implement, including starting,
 * terminating, sending messages, requests, and signaling state changes.
 * @note Daemons are **non-copyable** and **non-movable** to ensure unique ownership.
 *
 * @sa Worker
 */
class Daemon {
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

    /**
     * @brief Daemons are non-copyable to ensure unique ownership.
     */
    Daemon(const Daemon&) = delete;
    /**
     * @brief Daemons are non-movable to ensure unique ownership.
     */
    Daemon(Daemon&&) = delete;

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
     */
    virtual void send(Message&& msg) = 0;

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
     * @brief Sends a request and **waits** for a reply in a daemon process.
     *
     * This method sends a request of type Request to the daemon and **waits** for a corresponding reply of type TReply.
     * It uses a signal to synchronize the operation and returns the status of the request processing.
     *
     * @param req The request object to be sent.
     * @param rep The reply object where the response will be stored.
     * @return Status The status of the request operation, indicating success or an error.
     * @see Worker::make_request()
     */
    virtual Status make_request(Request&&, Reply&&) = 0;

    /**
     * @brief Helper: Sends a request and waits for a reply in a daemon process.
     *
     * This method sends a request of type TRequest to the daemon and **waits** for a corresponding reply of type TReply.
     * It uses a signal to synchronize the operation and returns the status of the request processing.
     * If the request cannot be sent, a runtime error status is returned.
     *
     * @tparam TRequest The type of the request object.
     * @tparam TReply The type of the reply object.
     * @param req Pointer to the request object to be sent.
     * @param rep Pointer to the reply object where the response will be stored.
     * @return Status The status of the request operation, indicating success or an error.
     * @see make_request()
     */
    template <typename TRequest, typename TReply>
    Status request(const TRequest* req, TReply* rep) {
        return make_request({req}, {rep});
    }

    /**
     * @brief Helper: Sends a **notification** request -- no reply required.
     * @see make_request()
     */
    template <typename TRequest>
    Status request(const TRequest* req) {
        return make_request({req}, {});
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
