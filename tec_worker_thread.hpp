// Time-stamp: <Last changed 2025-04-01 14:42:44 by magnolia>
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
 *   @file tec_worker_thread.hpp
 *   @brief Implements the WorkerThread class.
 *
 * Manages a Windows-ish thread using message loop.
 *
*/

#pragma once

#include <mutex>
#include <string>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_queue.hpp"
#include "tec/tec_worker.hpp"


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Worker thread
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      function description
 *
 * @details    detailed description
 *
 * @param      param
 *
 * @return     return type
 */
template <typename Traits>
class WorkerThread: public Worker<Traits> {
public:
    using id_t = std::thread::id;

    typedef Traits traits;
    typedef typename traits::Params Params;
    typedef typename traits::Message Message;

protected:
    using Lock = std::lock_guard<std::mutex>;

private:
    // Signals.
    Signal sig_running_;
    Signal sig_inited_;
    Signal sig_terminated_;

    // Message queue.
    SafeQueue<Message> mq_;

    // Status of execution.
    Status status_;
    std::mutex mtx_status_;

    // run()/terminate() sync.
    bool flag_running_;
    bool flag_terminated_;
    std::mutex mtx_thread_proc_;

    // Internal Worker thread.
    std::thread thread_;
    id_t thread_id_;

public:

    /**
     *  @brief Creates the Worker thread in the suspended state.
     *
     *  Use `run()` to start the Worker thread.
     *
     *  @sa Worker::run()
     */
    WorkerThread(const Params& params)
        : Worker<traits>(params)
        , flag_running_{false}
        , flag_terminated_{false}
        , thread_{details<traits>::thread_proc, std::ref(*this)}
    {}

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = delete;

    virtual ~WorkerThread() {
        if( thread_.joinable() ) {
            terminate();
        }
    }

    //! Worker thread ID.
    id_t id() const { return thread_id_; }

    //! Gets result of execution.
    Status status() override final {
        Lock lk(mtx_status_);
        return status_;
    }

    //! Signals that Worker thread has started.
    const Signal& sig_running() const override final { return sig_running_; }

    //! Signals that Worker thread has inited (possible, with error).
    const Signal& sig_inited() const override final { return sig_inited_; }

    //! Signals that Worker thread has terminated.
    const Signal& sig_terminated() const override final { return sig_terminated_; }

private:
    // Internal.
    void set_status(Status status) {
        Lock lk(mtx_status_);
        status_ = status;
    }

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
        static void thread_proc(WorkerThread<traits>& wt) {
            TEC_ENTER("WorkerThread::thread_proc");

            // `sig_terminated' will signal on exit.
            OnExit on_terminating{wt.sig_terminated_};

            // We obtain thread_id and then we are waiting for `sig_begin'
            // to resume the thread, see run().
            wt.thread_id_ = std::this_thread::get_id();
            TEC_TRACE("thread {} created.", wt.id());
            wt.sig_running_.wait();
            TEC_TRACE("`sig_running' received.");

            // Initialize the Worker and set the result.
            TEC_TRACE("on_init() called ...");
            auto init_status = wt.on_init();
            TEC_TRACE("on_init() returned {}.", init_status);
            wt.set_status(init_status);

            // Signals the Worker is inited, no matter if initialization failed.
            wt.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            if( wt.status() ) {
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

            // Finalize the Worker only if it has been inited successfully.
            if( wt.status() ) {
                TEC_TRACE("on_exit() called ...");
                auto exit_status = wt.on_exit();
                TEC_TRACE("on_exit() returned {}.", exit_status);
                wt.set_status(exit_status);
            }

            // `sig_terminated' signals on exit, see OnExit struct defined above.
        }
    };

public:

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
    bool send(const Message& msg) override {
        TEC_ENTER("WorkerThread::send");
        if( status() && thread_.joinable() ) {
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
     *  @brief Starts the Worker thread message polling.
     *
     *  Resumes the Worker thread by setting the `sig_running` signal,
     *  then waits for `on_init()` callback completed.
     *
     *  @note Derived from Daemon.
     *  @return Status
     */
    Status run() override {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("WorkerThread::run");

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
        return status();
    }

    /**
     *  @brief Terminates the Worker thread.
     *
     *  Sends Message::QUIT message to the message queue
     *  to stop the message polling if inited successfully, then waits for
     *  `sig_terminated` signalled to terminate the Worker thread.
     *
     *  @note Derived from Daemon.
     *
     *  @return Status
     */
    Status terminate() override {
        Lock lk{mtx_thread_proc_};
        TEC_ENTER("WorkerThread::terminate");

        if( flag_terminated_ ) {
            TEC_TRACE("WARNING: Worker thread already terminated!");
            return status();
        }

        if( !thread_.joinable() ) {
            TEC_TRACE("WARNING: no thread exists!");
            return status();
        }

        // Send Message::QUIT if normal exiting.
        if( status() ) {
            send(quit<Message>());
            TEC_TRACE("QUIT sent.");
        }

        // Waits for the thread to finish its execution
        TEC_TRACE("waiting for thread {} to finish ...", id());
        thread_.join();
        TEC_TRACE("thread {} finished OK.", id());

        flag_terminated_ = true;
        return status();
    }

}; // ::WorkerThread


} // ::tec
