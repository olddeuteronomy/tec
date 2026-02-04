// Time-stamp: <Last changed 2026-02-04 16:07:19 by magnolia>
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
 * @file tec_actor_worker.hpp
 * @brief Template class combining `Worker` and `Actor` for synchronous request processing in a dedicated thread.
 *
 * The `tec::ActorWorker<TParams, TActor>` class is a **composite worker** that:
 * - Owns a concrete `Actor` instance.
 * - Runs it in a **dedicated thread**.
 * - Exposes synchronous **request-reply** processing via `Payload`.
 * - Manages full **lifecycle** (`start` → `shutdown`) with **timeout-aware signals**.
 *
 * It bridges the asynchronous `Actor` interface with the synchronous `Worker`/`Daemon` model.
 *
 * @tparam TParams Parameter type (must derive from `ActorParams` or similar).
 * @tparam TActor  Concrete actor type (must derive from `tec::Actor`).
 *
 * @note Thread-safe request handling via internal mutex.
 * @see Worker, Actor, Daemon, Signal, Status
 * @author The Emacs Cat
 * @date 2025-10-28
 */

#pragma once

#include <cassert>
#include <memory>
#include <mutex>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_signal.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_actor.hpp"


namespace tec {

/**
 * @class ActorWorker
 * @brief A `Worker` that owns and runs an `Actor` in a dedicated thread.
 *
 * This class integrates the **actor model** (asynchronous lifecycle) with the
 * **worker model** (synchronous request handling via `Daemon`). It:
 * - Starts the actor in a background thread during `on_init()`.
 * - Routes incoming `Daemon::Payload` messages to `actor_->process_request()`.
 * - Ensures thread-safe, synchronous replies.
 * - Gracefully shuts down the actor and joins threads during `on_exit()`.
 *
 * @tparam TParams Configuration parameters (passed to both worker and actor).
 * @tparam TActor  Concrete actor type derived from `tec::Actor`.
 *
 * @par Threading Model
 * - One dedicated thread runs the actor (`actor_thread_`).
 * - Request processing is serialized via `mtx_request_`.
 * - `on_init()` and `on_exit()` are called from the daemon's control thread.
 */
template <typename TParams, typename TActor>
class ActorWorker : public Worker<TParams> {
  public:
      /// @brief Alias for the parameter type.
      using Params = TParams;

  protected:
      /// @brief Owned actor instance.
      std::unique_ptr<TActor> actor_;

  private:
      /// @brief Thread in which the actor runs.
      std::thread actor_thread_;

      /// @brief Signal set when actor startup completes.
      Signal sig_started_;

      /// @brief Signal set when actor shutdown completes.
      Signal sig_stopped_;

      /// @brief Status of the startup operation.
      Status status_started_;

      /// @brief Status of the shutdown operation.
      Status status_stopped_;

      /// @brief Mutex protecting synchronous request handling.
      std::mutex mtx_request_;

public:
    /**
       * @brief Constructs an `ActorWorker` with parameters and actor ownership.
       *
       * Takes ownership of a fully constructed `TActor` instance.
       * Registers a callback to handle `Payload*` messages synchronously.
       *
       * @param params Configuration for the worker and actor.
       * @param actor  Unique pointer to the concrete actor instance.
       *
       * @pre `TActor` must derive from `tec::Actor`.
       *
       * @par Example
       * @code
       * auto worker = std::make_unique<ActorWorker<Params, GrpcActor>>(
       *     params, std::move(std::make_unique<GrpcActor>(params)));
       * @endcode
       */
    ActorWorker(const Params& params, std::unique_ptr<TActor> actor)
        : Worker<Params>(params)
        , actor_{std::move(actor)}
    {
        static_assert(
            std::is_base_of_v<Actor, TActor>,
            "ActorWorker::TActor must derive from tec::Actor");
        //
        // Register synchronous request handler.
        //
        this->template register_callback<ActorWorker, Payload*>(
            this, &ActorWorker::on_request);
    }

    /**
     * @brief Deleted copy constructor.
     *
     * `ActorWorker` manages unique resources (thread, actor) and cannot be copied.
     */
    ActorWorker(const ActorWorker&) = delete;

    /**
     * @brief Deleted move constructor.
     *
     * Move semantics are disabled to prevent thread/actor ownership issues.
     */
    ActorWorker(ActorWorker&&) = delete;

    /**
     * @brief Destructor.
     *
     * Ensures the actor thread is joined if still running.
     * @note `on_exit()` should be called explicitly for graceful shutdown.
     */
    ~ActorWorker() override {
        if (actor_thread_.joinable()) {
            actor_thread_.join();
        }
    }

protected:
    /**
     * @brief Initializes the worker by starting the actor in a background thread.
     *
     * Spawns `actor_thread_` and calls `actor_->start()`. Waits for `sig_started_`
     * to be set (with optional timeout in derived classes).
     *
     * @return Status
     *   - `Status::Ok()` on success
     *   - `Error::Kind::RuntimeErr` if already running
     *   - Actor-reported error (e.g., timeout, bind failure)
     *
     * @see on_exit(), Actor::start()
     */
    Status on_init() override {
        TEC_ENTER("ActorWorker::on_init");
        if (actor_thread_.joinable()) {
            return Status{"Actor is already running", Error::Kind::RuntimeErr};
        }
        //
        // Launch Actor in dedicated thread.
        //
        actor_thread_ = std::thread([this] {
            actor_->start(&sig_started_, &status_started_);
        });
        //
        // Block until startup completes.
        //
        sig_started_.wait();
        TEC_TRACE("Actor thread {} started with status: {}", actor_thread_.get_id(), status_started_);
        return status_started_;
    }

    /**
     * @brief Shuts down the actor and joins all threads.
     *
     * Initiates shutdown in a temporary thread to avoid deadlock if the actor
     * is blocked in `start()`. Waits for `sig_stopped_` and joins both threads.
     *
     * @return Status Always returns `Status::Ok()` (shutdown is best-effort).
     *
     * @par Trace Events
     * - Logs shutdown initiation and completion.
     *
     * @see on_init(), Actor::shutdown()
     */
    Status on_exit() override {
        TEC_ENTER("ActorWorker::on_exit");
        if (!actor_thread_.joinable()) {
            return {}; // Already stopped
        }
        //
        // Launch Actor's shutdown in a separate thread.
        //
        std::thread shutdown_thread([this] {
            actor_->shutdown(&sig_stopped_);
        });
        TEC_TRACE("Actor thread {} is stopping...", actor_thread_.get_id());
        sig_stopped_.wait();
        TEC_TRACE("Actor thread {} stopped.", actor_thread_.get_id());

        shutdown_thread.join();
        actor_thread_.join();
        return {};
    }

    /**
     * @brief Handles incoming `Payload` requests synchronously.
     *
     * Called by the daemon when a message of type `Payload*` arrives.
     * Forwards the request to the actor and populates the reply.
     *
     * @param msg The incoming message (contains `Daemon::Payload*`).
     *
     * @par Thread Safety
     * Protected by `mtx_request_` to ensure only one request is processed at a time.
     *
     * @par RAII Signal
     * Uses `Signal::OnExit` to auto-set `payload->ready` on exit.
     *
     * @see Daemon::Payload, Actor::process_request()
     */
    virtual void on_request(const Message& msg) {
        typename Worker<TParams>::Lock lk{mtx_request_};
        TEC_ENTER("ActorWorker::on_request");
        TEC_TRACE("Payload received: {}", msg.type().name());
        //
        auto payload = std::any_cast<Payload*>(msg);
        Signal::OnExit on_exit(payload->ready);
        *(payload->status) = actor_->process_request(
            std::move(payload->request), std::move(payload->reply));
    }

public:
    /// @name Factory Builders
    /// @brief Type-safe factory patterns for creating `ActorWorker` instances.
    /// @{

    /**
     * @struct Builder
     * @brief Factory for constructing `ActorWorker` as a `Daemon` pointer.
     *
     * Enables polymorphic creation of derived worker/actor pairs while
     * preserving type safety via `static_assert`.
     *
     * @tparam WorkerDerived Final `ActorWorker` specialization.
     * @tparam ActorDerived  Final `Actor` implementation.
     */
    template <typename WorkerDerived, typename ActorDerived>
    struct Builder {
        /**
         * @brief Creates a `std::unique_ptr<Daemon>` owning the worker.
         *
         * @param params Parameters used to construct both worker and actor.
         * @return std::unique_ptr<Daemon> Polymorphic pointer to the worker.
         *
         * @pre `WorkerDerived` must inherit from `ActorWorker`.
         * @pre `ActorDerived` must inherit from `Actor`.
         */
        std::unique_ptr<Daemon>
        operator()(const typename WorkerDerived::Params& params) const {
            static_assert(
                std::is_base_of_v<ActorWorker, WorkerDerived>,
                "WorkerDerived must derive from tec::ActorWorker");
            static_assert(
                std::is_base_of_v<Actor, ActorDerived>,
                "ActorDerived must derive from tec::Actor");

            return std::make_unique<WorkerDerived>(
                params,
                std::move(std::make_unique<ActorDerived>(params)));
        }
    };
    /// @}

}; // class ActorWorker

} // namespace tec

