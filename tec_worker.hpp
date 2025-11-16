// Time-stamp: <Last changed 2025-11-17 00:06:10 by magnolia>
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
 * @file Worker.hpp
 * @brief Defines a worker class for processing messages in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_queue.hpp"
#include "tec/tec_message.hpp"
#include "tec/tec_daemon.hpp"


namespace tec {

/**
 * @class Worker
 * @brief A class implementing message processing as a daemon.
 * @details Implements the Daemon interface to manage a worker thread that processes
 * messages from a SafeQueue. It supports registering callbacks for specific message types,
 * provides default initialization and exit callbacks, and manages thread lifecycle with
 * signals for running, initialization, and termination.
 * @tparam TParams The type of parameters used to configure the worker.
 * @note Workers are **non-copyable** and **non-movable** to ensure unique ownership.
 * @see Daemon
 */
template <typename TParams>
class Worker : public Daemon {
public:
    using Params = TParams; ///< Type alias for worker parameters.
    using id_t = std::thread::id; ///< Type alias for thread ID.
    using Lock = std::lock_guard<std::mutex>; ///< Type alias for mutex lock guard.

    /**
     * @brief Type alias for a callback function to process messages.
     * @details Defines a function that takes a Worker pointer and a Message to process.
     */
    using CallbackFunc = std::function<void(Worker<Params>*, const Message&)>;

protected:
    Params params_; ///< Configuration parameters for the worker.

private:
    /**
     * @struct Slot
     * @brief Stores a callback and its associated worker for message handling.
     * @details Used in the callback table to map message types to their handlers.
     */
    struct Slot {
        Worker<Params>* worker; ///< Pointer to the worker instance.
        CallbackFunc callback; ///< Callback function for message processing.

        /**
         * @brief Constructs a Slot with a worker and callback.
         * @param w Pointer to the worker instance.
         * @param cb The callback function.
         */
        explicit Slot(Worker<Params>* w, CallbackFunc cb)
            : worker{w}
            , callback{cb}
        {}

        /**
         * @brief Destructor for Slot.
         * @details Default destructor for cleanup.
         */
        ~Slot() = default;
    };

    std::unordered_map<std::type_index, std::unique_ptr<Slot>> slots_; ///< Table mapping message types to callbacks.
    std::mutex mtx_slots_; ///< Mutex for thread-safe access to the callback table.
    Signal sig_running_; ///< Signal indicating the worker is running.
    Signal sig_inited_; ///< Signal indicating the worker is initialized (possibly with error).
    Signal sig_terminated_; ///< Signal indicating the worker has terminated.
    SafeQueue<Message> mq_; ///< Thread-safe queue for incoming messages.
    Status status_; ///< Current execution status of the worker.
    std::atomic_bool flag_running_; ///< Flag indicating if the worker thread is running.
    std::atomic_bool flag_exited_; ///< Flag indicating if the message loop has exited.
    std::mutex mtx_thread_proc_; ///< Mutex for synchronizing run/terminate operations.
    std::thread thread_; ///< The worker's thread for message processing.
    id_t thread_id_; ///< ID of the worker thread.

public:
    /**
     * @brief Constructs a worker in a suspended state.
     * @details Initializes the worker with the provided parameters.
     * The worker thread is created by calling run().
     * @param params The configuration parameters for the worker.
     * @see run()
     */
    explicit Worker(const Params& params)
        : Daemon()
        , params_{params}
        , flag_running_{false}
        , flag_exited_{false}
    {}

    /**
     * @brief Destructor that ensures proper thread termination.
     * @details Calls terminate() if the thread is joinable to ensure clean shutdown.
     */
    virtual ~Worker() {
        if (thread_.joinable()) {
            terminate();
        }
    }

    /**
     * @brief Retrieves the worker thread's ID.
     * @return id_t The ID of the worker thread.
     */
    id_t id() const { return thread_id_; }

    /**
     * @brief Retrieves the worker's configuration parameters.
     * @return const Params& The worker's parameters.
     */
    constexpr const Params& params() const { return params_; }

    /**
     * @brief Retrieves the signal indicating the worker is running.
     * @return const Signal& The running signal.
     * @see Daemon::sig_running()
     */
    const Signal& sig_running() const override { return sig_running_; }

    /**
     * @brief Retrieves the signal indicating the worker is initialized.
     * @return const Signal& The initialization signal.
     * @see Daemon::sig_inited()
     */
    const Signal& sig_inited() const override { return sig_inited_; }

    /**
     * @brief Retrieves the signal indicating the worker has terminated.
     * @return const Signal& The termination signal.
     * @see Daemon::sig_terminated()
     */
    const Signal& sig_terminated() const override { return sig_terminated_; }

    /**
     * @brief Sends a message to the worker's queue.
     * @details Enqueues a message for processing if the worker thread is joinable.
     * Logs the operation using tracing if enabled.
     * @param msg The message to send.
     * @return bool True if the message was sent, false if no thread exists.
     * @see Daemon::send()
     */
    bool send(const Message& msg) override {
        TEC_ENTER("Worker::send");
        if (thread_.joinable()) {
            mq_.enqueue(msg);
            TEC_TRACE("Message [{}] sent.", name(msg));
            return true;
        } else {
            // No worker thread.
            return false;
        }
    }

    /**
     * @brief Registers a callback for a specific message type.
     * @details Associates a member function of a derived class with a message type.
     * The callback is stored in the slots table and invoked when a matching message is received.
     * Should not be called while the worker is running.
     * @tparam Derived The derived worker class type.
     * @tparam T The message type to associate with the callback.
     * @param worker Pointer to the derived worker instance.
     * @param callback The member function to call for the message type.
     * @see dispatch()
     */
    template <typename Derived, typename T>
    void register_callback(Derived* worker, void (Derived::*callback)(const Message& msg)) {
        Lock lk{mtx_slots_};
        TEC_ENTER("Worker::register_callback");
        // Ensure Derived is actually derived from Worker<Params>.
        static_assert(std::is_base_of_v<Worker<Params>, Derived>,
                      "Derived must inherit from tec::Worker");
        std::type_index ndx = std::type_index(typeid(T));

        // Remove existing handler.
        if (auto slot = slots_.find(ndx); slot != slots_.end()) {
            slots_.erase(ndx);
        }

        // Set the slot.
        slots_[ndx] = std::make_unique<Slot>(
            worker,
            [callback](Worker<Params>* worker, const Message& msg) {
                // Safely downcast to Derived.
                auto derived = dynamic_cast<Derived*>(worker);
                (derived->*callback)(msg);
            });
        TEC_TRACE("Callback {} registered.", typeid(T).name());
    }

protected:
    /**
     * @brief Dispatches a message to its registered callback.
     * @details Looks up the callback for the message's type in the slots table and
     * invokes it if found.
     * @param msg The message to dispatch.
     * @see register_callback()
     */
    virtual void dispatch(const Message& msg) {
        auto ndx = std::type_index(msg.type());
        if (auto slot = slots_.find(ndx); slot != slots_.end()) {
            slot->second->callback(slot->second->worker, msg);
        }
    }

private:
    /**
     * @struct OnExit
     * @brief Helper struct to signal termination on exit.
     * @details Automatically sets the termination signal when destroyed.
     */
    struct OnExit {
        Signal& sig_; ///< Reference to the termination signal.

        /**
         * @brief Constructs an OnExit helper with a termination signal.
         * @param sig The termination signal to set on destruction.
         */
        explicit OnExit(Signal& sig) : sig_{sig} {}

        /**
         * @brief Destructor that sets the termination signal.
         */
        ~OnExit() { sig_.set(); }
    };

    /**
     * @struct details
     * @brief Internal details for the worker thread procedure.
     * @details Contains a static thread procedure to handle message polling and lifecycle
     * management for the worker.
     * @tparam Params The worker's parameter type.
     */
    template <typename Params>
    struct details {
        /**
         * @brief The worker thread's main procedure.
         * @details Manages the worker thread's lifecycle, including initialization,
         * message polling, and termination. Uses tracing for debugging and signals
         * for state transitions.
         * @param wt Reference to the worker instance.
         */
        static void thread_proc(Worker<Params>& wt) {
            TEC_ENTER("Worker::thread_proc");

            // Signal termination on exit.
            OnExit on_terminating{std::ref(wt.sig_terminated_)};

            // Obtain thread ID and wait for the running signal.
            wt.thread_id_ = std::this_thread::get_id();
            TEC_TRACE("thread {} created.", wt.id());
            wt.sig_running_.wait();
            TEC_TRACE("`sig_running' received.");

            // Exit immediately if flagged.
            if (wt.flag_exited_) {
                return;
            }

            // Initialize the worker.
            TEC_TRACE("on_init() called ...");
            wt.status_ = wt.on_init();
            TEC_TRACE("on_init() returned {}.", wt.status_);

            // Signal initialization complete.
            wt.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            if (wt.status_) {
                // Process messages if initialized successfully.
                TEC_TRACE("entering message loop.");
                bool stop = false;
                do {
                    auto msg = wt.mq_.dequeue();
                    TEC_TRACE("received Message [{}].", name(msg));
                    if (is_null(msg)) {
                        stop = true;
                        wt.flag_exited_ = true;
                    } else {
                        wt.dispatch(msg);
                    }
                } while (!stop);
                TEC_TRACE("leaving message loop, {} message(s) left in queue...", wt.mq_.size());
            }

            // Finalize if it was initialized successfully.
            if (wt.status_) {
                TEC_TRACE("on_exit() called ...");
                wt.status_ = wt.on_exit();
                TEC_TRACE("on_exit() returned {}.", wt.status_);
            }
        }
    };

protected:
    /**
     * @brief Default callback invoked during worker thread initialization.
     * @details Called before entering the message loop. If it returns a status other
     * than Error::Kind::Ok, the worker stops and skips the message loop and on_exit().
     * @return Status Error::Kind::Ok by default.
     */
    virtual Status on_init() { return {}; }

    /**
     * @brief Default callback invoked when exiting the worker thread.
     * @details Called after the message loop, only if on_init() succeeded.
     * @return Status Error::Kind::Ok by default.
     */
    virtual Status on_exit() { return {}; }

public:
    /**
     * @brief Starts the worker thread's message polling.
     * @details Creates the worker thread, signals sig_running_, and waits for
     * initialization to complete. Returns the initialization status.
     * @return Status The result of the initialization.
     * @see Daemon::run()
     */
    Status run() override {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("Worker::run");

        if (!thread_.joinable()) {
            thread_ = std::thread(details<Params>::thread_proc, std::ref(*this));
        }
        else {
            TEC_TRACE("`Worker::thread_proc' is already running.");
            return {};
        }

        // Resume the thread.
        flag_running_ = true;
        sig_running_.set();
        TEC_TRACE("`sig_running' signalled.");

        // Wait for thread initialization completed.
        TEC_TRACE("waiting for `sig_inited' signalled ...");
        sig_inited_.wait();

        // Return the result of thread initialization.
        return status_;
    }

    /**
     * @brief Terminates the worker thread.
     * @details Sends a null message to stop the message loop if initialized successfully,
     * waits for termination, and joins the thread. Should not be called from the worker thread.
     * @return Status The final status of the worker.
     * @see Daemon::terminate()
     * @see nullmsg()
     */
    Status terminate() override {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("Worker::terminate");

        if (!thread_.joinable()) {
            TEC_TRACE("WARNING: no thread exists!");
            return status_;
        }

        if (!flag_running_) {
            TEC_TRACE("Exiting the suspended thread...");
            flag_exited_ = true;
            sig_running_.set();
        }

        // Send the null message on normal exiting.
        if (status_ && !flag_exited_) {
            send(nullmsg());
            TEC_TRACE("QUIT sent.");
        }

        // Wait for the thread to finish its execution.
        TEC_TRACE("waiting for thread {} to finish ...", id());
        thread_.join();
        TEC_TRACE("thread {} finished OK.", id());

        return status_;
    }

    /**
     * @struct Builder
     * @brief Factory for creating worker instances.
     * @details Provides a templated builder to create instances of derived worker classes,
     * ensuring type safety through static assertions.
     * @tparam Derived The derived worker class type.
     */
    template <typename Derived>
    struct Builder {
        /**
         * @brief Creates a unique pointer to a derived worker instance.
         * @details Constructs a new instance of the specified Derived class using the
         * provided parameters. Ensures Derived is a subclass of Worker.
         * @param params The parameters for constructing the Derived worker.
         * @return std::unique_ptr<Worker<typename Derived::Params>> A unique pointer to the created worker.
         */
        std::unique_ptr<Worker<typename Derived::Params>>
        operator()(typename Derived::Params const& params) {
            static_assert(std::is_base_of<Worker<typename Derived::Params>, Derived>::value,
                          "Not derived from tec::Worker class");
            return std::make_unique<Derived>(params);
        }
    };
}; // class Worker

} // namespace tec
