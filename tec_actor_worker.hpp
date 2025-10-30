// Time-stamp: <Last changed 2025-10-30 14:18:50 by magnolia>
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
 * @brief Defines a worker class that manages an actor instance in the tec namespace.
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
 * @brief A worker that owns and manages an actor instance.
 * @details Extends the Worker class to start and shut down an actor instance,
 * handling actor lifecycle within a dedicated thread. It uses signals and status
 * objects to manage actor startup and shutdown with timeout support.
 * @tparam TParams The type of parameters, typically derived from ActorParams.
 * @tparam TServer The server type, derived from Server.
 * @see Worker
 * @see Server
 */
template <typename TParams, typename TActor>
class ActorWorker : public Worker<TParams> {
public:
    using Params = TParams; ///< Type alias for worker parameters.

protected:
    std::unique_ptr<TActor> actor_; ///< The actor instance owned by the worker.

private:
    std::thread actor_thread_; ///< Thread for running the actor.
    Signal sig_started_; ///< Signal indicating the actor has started.
    Signal sig_stopped_; ///< Signal indicating the actor has stopped.
    Status status_started_; ///< Status of the actor startup operation.
    Status status_stopped_; ///< Status of the actor shutdown operation.
    std::mutex mtx_request_;

public:
    /**
     * @brief Constructs an ActorWorker with parameters and an actor instance.
     * @details Initializes the Worker base class with parameters and takes ownership of
     * the actor instance.
     * Ensures TActor is derived from Actor via static assertion.
     * @param params The configuration parameters for the worker.
     * @param server A unique pointer to the actor instance.
     */
    ActorWorker(const Params& params, std::unique_ptr<TActor> actor)
        : Worker<Params>(params)
        , actor_{std::move(actor)}
    {
        static_assert(
            std::is_base_of<Actor, TActor>::value,
            "ActorWorker::TActor must derive from tec::Actor");

        static_assert(
            std::is_base_of<ActorParams, TParams>::value,
            "ActorWorker::TParams must derive from tec::ActorParams");

        // Registers a special callback to manage requests to the actor synchronously.
        this->template register_callback<ActorWorker<Params, TActor>, Daemon::Payload*>(
            this, &ActorWorker<TParams, TActor>::on_request);
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    ActorWorker(const ActorWorker&) = delete;

    /**
     * @brief Deleted move constructor to prevent moving.
     */
    ActorWorker(ActorWorker&&) = delete;

    /**
     * @brief Destructor that ensures proper server thread cleanup.
     * @details Joins the server thread if it is joinable to ensure clean shutdown.
     */
    virtual ~ActorWorker() {
        if (actor_thread_.joinable()) {
            actor_thread_.join();
        }
    }

protected:

    /**
     * @brief Initializes the worker by starting the server thread.
     * @details Signals the server thread to start and waits for the server to signal
     * completion or timeout. Logs events using tracing if enabled.
     * @return Status Error::Kind::Ok if successful, otherwise an error status (e.g., TimeoutErr).
     * @see Worker::on_init()
     */
    Status on_init() override {
        TEC_ENTER("ActorWorker::on_init");

        if( actor_thread_.joinable() ) {
            return {"Actor is already running", Error::Kind::RuntimeErr};
        }

        // Start the actor thread.
        actor_thread_ = std::thread([&] {
            actor_->start(&sig_started_, &status_started_);
        });

        // Wait for the actor started.
        sig_started_.wait();
        TEC_TRACE("Actor thread {} started with {}.", actor_thread_.get_id(), status_started_);

        return status_started_;
    }

    /**
     * @brief Shuts down the actor during worker exit.
     * @details Initiates actor shutdown in a separate thread and waits for completion
     * or timeout. Logs events using tracing if enabled.
     * @return Status Error::Kind::Ok if successful, otherwise an error status (e.g., TimeoutErr).
     * @see Worker::on_exit()
     */
    Status on_exit() override {
        TEC_ENTER("ActorWorker::on_exit");

        if ( !actor_thread_.joinable()) {
            // Already finished or not started.
            return {};
        }

        // Start a dedicated thread to shut down the actor.
        std::thread shutdown_thread([&] {
            actor_->shutdown(&sig_stopped_);
        });

        // Wait for the actor to shut down.
        TEC_TRACE("Actor thread {} is stopping...", actor_thread_.get_id());
        sig_stopped_.wait();
        TEC_TRACE("Actor thread {} stopped.", actor_thread_.get_id());

        shutdown_thread.join();
        actor_thread_.join();
        return {};
    }

protected:

    virtual void on_request(const Message& msg) {
        typename Worker<TParams>::Lock lk{mtx_request_};
        TEC_ENTER("ActorWorker::on_request");
        TEC_TRACE("Payload received: {}", msg.type().name());
        auto payload = std::any_cast<Daemon::Payload*>(msg);
        Actor::SignalOnExit on_exit(payload->ready);
        *(payload->status) = actor_->process_request(payload->request, payload->reply);
    }

public:

    /// @name ActorWorker Builders
    /// @brief Factory structs for creating ActorWorker instances.
    /// @details Provide builders to create ActorWorker instances as Daemon pointers,
    /// ensuring type safety through static assertions.
    /// @{

    /**
     * @struct Builder
     * @brief Factory for creating ActorWorker as a Daemon.
     * @details Creates a ActorWorker instance with a derived worker and actor,
     * returning it as a Daemon pointer.
     * @tparam WorkerDerived The derived ActorWorker class type.
     * @tparam ActorDerived The derived Actor class type.
     */
    template <typename WorkerDerived, typename ActorDerived>
    struct Builder {
        /**
         * @brief Creates a unique pointer to a ActorWorker as a Daemon.
         * @details Constructs a WorkerDerived instance with an ActorDerived instance,
         * ensuring type constraints via static assertions.
         * @param params The parameters for constructing the worker and actor.
         * @return std::unique_ptr<Daemon> A unique pointer to the created worker.
         */
        std::unique_ptr<Daemon>
        operator()(typename WorkerDerived::Params const& params) {
            static_assert(
                std::is_base_of<ActorWorker, WorkerDerived>::value,
                "WorkerDerived must derive from tec::ActorWorker");
            static_assert(
                std::is_base_of<Actor, ActorDerived>::value,
                "ActorDerived must derive from tec::Actor");
            static_assert(
                std::is_base_of<ActorParams, typename WorkerDerived::Params>::value,
                "Params must derive from tec::ActorParams");

            return std::make_unique<WorkerDerived>(params, std::move(std::make_unique<ActorDerived>(params)));
        }
    };

    /// @}

}; // class ActorWorker

} // namespace tec
