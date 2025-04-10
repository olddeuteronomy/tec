// Time-stamp: <Last changed 2025-04-09 16:47:18 by magnolia>
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
 *   @file tec_worker.hpp
 *   @brief Implements a Worker class derived from Daemon.
 *
*/

#pragma once

#include <type_traits>
#include <unordered_map>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_message.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_queue.hpp"
#include "tec/tec_daemon.hpp"

namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          Worker traits
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Declares the Worker trait - a pair of TParams and TMessage types.
template <
    typename TParams,
    typename TMessage
    >
struct worker_traits {
    typedef TParams Params;
    typedef TMessage Message;
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      A Worker class implements message polling.
 *
 * @details Extends the Daemon class with the `send()` method that is used to
 * manage the Worker's thread.
 * Also,
 * 1) Defines message handlers;
 * 2) Implements default callbacks `on_init()` and `on_exit()`;
 * 3) Defines an internal thread procedure that implements message polling;
 * 4) Implements required Signals (see Daemon).
 */
template <typename Traits>
class Worker : public Daemon
{
public:

    typedef Traits traits;
    typedef typename traits::Params Params;
    typedef typename traits::Message Message;

    using id_t = std::thread::id;

    //! Message handler.
    typedef void (*Handler)(Worker<Traits>&, const Message&);

protected:

    using Lock = std::lock_guard<std::mutex>;

    // Worker parameters.
    Params params_;

private:

    // Message handlers.
    std::unordered_map<typename Message::Command, Handler> slots_;

    // Signals.
    Signal sig_running_;
    Signal sig_inited_;
    Signal sig_terminated_;

    // Message queue.
    SafeQueue<Message> mq_;

    // Status of execution.
    Status status_;

    // run()/terminate() sync.
    bool flag_running_;
    bool flag_terminated_;
    std::mutex mtx_thread_proc_;

    // Internal Worker thread.
    std::thread thread_;
    id_t thread_id_;

public:

    /**
     *  @brief Constructs s Worker thread in the suspended state.
     *
     *  Use `run()` to start the Worker thread.
     *
     *  @sa Worker::run()
     */
    Worker(const Params& params)
        : Daemon()
        , params_{params}
        , flag_running_{false}
        , flag_terminated_{false}
        , thread_{details<traits>::thread_proc, std::ref(*this)}
    {}

    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;

    virtual ~Worker() {
        if( thread_.joinable() ) {
            terminate();
        }
    }

    //! Worker thread ID.
    id_t id() const { return thread_id_; }

    //! Returns Worker parameters.
    constexpr Params params() const { return params_; }

    //! Gets result of execution.
    Status status() const  { return status_; }

    //! Signals that Worker thread has started.
    const Signal& sig_running() const override final { return sig_running_; }

    //! Signals that Worker thread has inited (possible, with error).
    const Signal& sig_inited() const override final { return sig_inited_; }

    //! Signals that Worker thread has terminated.
    const Signal& sig_terminated() const override final { return sig_terminated_; }

    /**
     *  @brief  Sends a WorkerMessage (or any class derived from WorkerMessage)
     *  to the Worker thread.
     *
     *  If no Worker thread exists,
     *  the message will not be sent, returning `false`.
     *
     *  @note Derived from Worker.
     *
     *  @param  msg WorkerMessage to send.
     *  @return bool
     */
    bool send(const Message& msg) {
        TEC_ENTER("Worker::send");
        if( status_ && thread_.joinable() ) {
            mq_.enqueue(msg);
            TEC_TRACE("Message [cmd={}] sent.", msg.command);
            return true;
        }
        else {
             // No message loop.
            return false;
        }
    }

    /**
     * @brief Registers a message handler.
     * @param cmd
     * @param handler A handler of the corresponding message.
     */
    void register_handler(typename Message::Command cmd, Handler handler) {
        // Remove existing handler.
        if( auto slot = slots_.find(cmd); slot != slots_.end() ) {
            slots_.erase(cmd);
        }
        slots_[cmd] = {handler};
    }

private:

    // Sets the signal on exiting from `thread_proc'.
    struct OnExit {
        Signal& sig_;

        OnExit(Signal& sig): sig_{sig} {}
        ~OnExit() { sig_.set(); }
    };

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename traits>
    struct details {
        static void thread_proc(Worker<traits>& wt) {
            TEC_ENTER("Worker::thread_proc");

            // `sig_terminated' will signal on exit.
            OnExit on_terminating{wt.sig_terminated_};

            // We obtain thread_id and then we are waiting for `sig_begin'
            // to resume the thread, see run().
            wt.thread_id_ = std::this_thread::get_id();
            TEC_TRACE("thread {} created.", wt.id());
            wt.sig_running_.wait();
            TEC_TRACE("`sig_running' received.");

            // Initialize the Worker and set a status.
            TEC_TRACE("on_init() called ...");
            wt.status_ = wt.on_init();
            TEC_TRACE("on_init() returned {}.", wt.status_);

            // Signals the Worker is inited, no matter if initialization failed.
            wt.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            if( wt.status_ ) {
                // Start message polling if inited successfully.
                TEC_TRACE("entering message loop.");
                bool stop = false;
                do {
                    auto msg = wt.mq_.dequeue();
                    TEC_TRACE("received Message [cmd={}].", msg.command);
                    if( !msg.quit() ) {
                        wt.dispatch(msg);
                    }
                    else {
                        stop = true;
                    }
                } while( !stop );
                TEC_TRACE("leaving message loop.");
            }

            // Finalize the Worker ONLY IF it has been inited successfully.
            if( wt.status_ ) {
                TEC_TRACE("on_exit() called ...");
                // Set exiting status.
                wt.status_ = wt.on_exit();
                TEC_TRACE("on_exit() returned {}.", wt.status_);
            }

            // `sig_terminated' signals on exit, see OnExit struct defined above.
        }
    };

protected:

    /**
     *  @brief A callback that is invoked on Worker initialization.
     *
     *  @note If `on_init()` returns Status other than `Error::Kind::Ok`,
     *  the Worker should stop message processing and quit the Worker thread
     *  immediately. `on_exit()` callback **will not be called** in this case.
     *
     *  @note Default implementation does nothing returning Error::Kind::Ok.
     *  @return Status `Error::Kind::Ok` by default.
     */
    virtual Status on_init() { return {}; }

    /**
     *  @brief A callback that is invoked on exiting from the Worker's thread.
     *
     *  @note This callback **will not be called** if `on_init()`
     *  callback returned Result other than `Error::Kind::Ok`.
     *
     *  @note Default implementation does nothing returning `Error::Kind::Ok`.
     *  @return Status `Error::Kind::Ok` by default.
     */
    virtual Status on_exit() { return {}; }


    /**
     * @brief      Dispatches a message by calling the corresponding message handler.
     * @param      msg A message to dispatch.
     * @sa register_handler()
     */
    virtual void dispatch(const Message& msg) {
        if( auto slot = slots_.find(msg.command); slot != slots_.end() ) {
            slot->second(std::ref(*this), msg);
        }
    }

public:

        /**
     *  @brief Starts the Worker thread message polling.
     *
     *  Resumes the Worker thread by setting the `sig_running` signal,
     *  then waits for `on_init()` callback completed.
     *
     *  @note Derived from Daemon.
     *  @return Status
     */
    Status run() override final {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("Worker::run");

        if( !thread_.joinable() ) {
            // No thread exists, possible system failure.
            TEC_TRACE("no active thread.");
            return {"no active thread", Error::Kind::System};
        }

        if( flag_running_ ) {
            TEC_TRACE("WARNING: Worker thread is already running!");
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
     *  @brief Terminates the Worker thread.
     *
     *  Sends Message::QUIT message to the message queue
     *  to stop the message polling if inited successfully, then waits for
     *  `sig_terminated` signalled to terminate the Worker thread.
     *
     *  @note Derived from Daemon.
     *  @return Status
     */
    Status terminate() override final {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("Worker::terminate");

        if( flag_terminated_ ) {
            TEC_TRACE("WARNING: Worker thread already terminated!");
            return status_;
        }

        if( !thread_.joinable() ) {
            TEC_TRACE("WARNING: no thread exists!");
            return status_;
        }

        // Send Message::QUIT if normal exiting.
        if( status_ ) {
            send(nullmsg<Message>());
            TEC_TRACE("QUIT sent.");
        }

        // Waits for the thread to finish its execution
        TEC_TRACE("waiting for thread {} to finish ...", id());
        thread_.join();
        TEC_TRACE("thread {} finished OK.", id());

        flag_terminated_ = true;
        return status_;
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Worker buider
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    //! Builds the Worker from a derived class (including itself).
    template <typename Derived>
    struct Builder {
        std::unique_ptr<Worker<typename Derived::traits>>
        operator()(typename Derived::Params const& params) {
            static_assert(std::is_base_of<Worker<typename Derived::traits>, Derived>::value,
                "not derived from tec::Worker class");
            return std::make_unique<Derived>(params);
        }
    };
};

} // ::tec
