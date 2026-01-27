// Time-stamp: <Last changed 2026-01-27 13:00:51 by magnolia>
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
 * @brief Core interface for TEC actors with lifecycle management and request processing.
 *
 * This header defines the `tec::Actor` abstract base class, which serves as the
 * foundation for all actor implementations in the TEC framework. Actors encapsulate
 * asynchronous services with well-defined **start/shutdown** semantics and
 * **request-reply** message handling.
 *
 * Key features:
 * - **Non-copyable, polymorphic** base class.
 * - **Signal-based** lifecycle notifications (`start`, `shutdown`).
 * - **Synchronous request processing** via `process_request`.
 *
 * @note This file is part of the **TEC** framework.
 * @see tec_signal.hpp, tec_status.hpp, tec_message.hpp
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
 * @class Actor
 * @brief Abstract base class defining the actor lifecycle and request handling interface.
 *
 * All concrete actors must inherit from this class and implement:
 * - `start()` – asynchronous initialization with completion signal.
 * - `shutdown()` – graceful termination with completion signal.
 * - `process_request()` – core message handling logic.
 *
 * Actors are typically long-lived services (e.g., gRPC servers, workers) that
 * run in their own thread or event loop until explicitly shut down.
 *
 * @note Actors are **non-copyable** and **non-movable** to ensure unique ownership.
 */
class Actor {
  public:
      /**
       * @brief Default constructor.
       *
       * Initializes base actor state. Derived classes should perform
       * minimal setup here and defer heavy initialization to `start()`.
       */
      Actor() = default;

      /**
       * @brief Deleted copy constructor.
       *
       * Actors represent unique service instances and must not be copied.
       */
      Actor(const Actor&) = delete;

      /**
       * @brief Deleted move constructor.
       *
       * Prevents accidental transfer of actor ownership.
       */
      Actor(Actor&&) = delete;

      /**
       * @brief Deleted copy assignment operator.
       */
      Actor& operator=(const Actor&) = delete;

      /**
       * @brief Deleted move assignment operator.
       */
      Actor& operator=(Actor&&) = delete;

      /**
       * @brief Virtual destructor.
       *
       * Ensures proper destruction of derived classes when deleted through
       * a base pointer. Derived classes should override if cleanup is needed.
       */
      virtual ~Actor() = default;

      /**
       * @brief Starts the actor's operation.
       *
       * This method initiates the actor's internal service (e.g., binding ports,
       * starting threads, entering event loop). It must **not block indefinitely**
       * unless the actor is designed to run until `shutdown()` is called.
       *
       * Completion (success or failure) is signaled via `sig_started`.
       *
       * @param sig_started [in,out] Signal set when startup completes.
       *                     Must remain valid until the signal is triggered.
       * @param status [out] Filled with result:
       *                     - `Status::Ok()` on success
       *                     - `Status::Error(Error::Kind::Timeout)` on timeout
       *                     - Other errors as defined by implementation
       *
       * @note This function may return **before** startup completes.
       *       Use `sig_started->wait()` to synchronize.
       * @warning In long-running actors (e.g. gRPC), this call may **not return**
       *          until `shutdown()` is invoked from another thread.
       *
       * @see shutdown(), Status
       */
      virtual void start(Signal* sig_started, Status* status) = 0;

      /**
       * @brief Initiates graceful shutdown of the actor.
       *
       * Requests the actor to stop processing new requests, complete in-flight work,
       * and terminate cleanly. Completion is signaled via `sig_stopped`.
       *
       * @param sig_stopped [in,out] Signal set when shutdown is fully complete.
       *                     Must remain valid until triggered.
       *
       * @note This is typically called from a separate thread.
       * @see start()
       */
      virtual void shutdown(Signal* sig_stopped) = 0;

    /**
     * @brief Processes a single request and produces a reply.
     *
     * This is the **core message handling entry point**. Implementations should:
     * - Validate the `request`
     * - Perform necessary work
     * - Populate `reply` with results
     * - Return appropriate `Status`
     *
     * @param request [in] Input message to process (moved-from if possible).
     * @param reply [out] Output message populated with response.
     *
     * @return Status indicating success or specific error condition.
     *
     * @par Example
     * @code
     * Status s = actor->process_request(req, reply);
     * if (!s.ok()) { handle_error(s); }
     * @endcode
     *
     * @see Request, Reply, Status
     */
    virtual Status process_request(Request request, Reply reply) = 0;


    /**
     * Mimics Daemon's behavior.
     */
    virtual Status run() {
        Status status;
        Signal sig;
        start(&sig, &status);
        sig.wait();
        return status;
    }

    /**
     * Mimics Daemon's behavior.
     */
    virtual Status terminate() {
        Signal sig;
        shutdown(&sig);
        sig.wait();
        return {};
    }


  }; // class Actor

} // namespace tec
