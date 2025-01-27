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
 *   @brief Declares a Worker class.
 *
 * Manages a Windows-ish thread using message loop.
 *
*/

#pragma once

#include <mutex>
#include <string>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_utils.hpp"
#include "tec/tec_queue.hpp"
#include "tec/tec_semaphore.hpp"
#include "tec/tec_trace.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Abstract Worker - Daemon
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct WorkerParams {
};


class Daemon
{
public:
    Daemon() = default;
    virtual ~Daemon() = default;

    virtual Result run() = 0;
    virtual Result terminate() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Message
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Message implements quit() to indicate that
//! message polling should stop.
//!
//! @sa SafeQueue::poll() in tec_queue.hpp.
struct Message {
    typedef unsigned long cmd_t ;

    //! Quit message loop command.
    static constexpr const cmd_t QUIT{0};

    cmd_t command; //!< A command.

    //! Indicates that message loop should be terminated.
    inline bool quit() const { return (command == Message::QUIT); }
};

//! Null message. Send it to quit the message loop.
template <typename TMessage>
TMessage quit() { TMessage msg; msg.command = Message::QUIT; return msg; }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Worker thread
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TWorkerParams, typename TMessage = Message, typename Duration = MilliSec>
class Worker: public Daemon {

public:
    using id_t = std::thread::id;

protected:
    using Lock = std::lock_guard<std::mutex>;

    //! Worker internal thread.
    std::thread thread_;
    id_t thread_id_;

    //! Signals.
    Signal sig_running_;
    Signal sig_inited_;
    Signal sig_terminated_;

    //! Message queue.
    SafeQueue<TMessage> mq_;

    //! Worker parameters.
    TWorkerParams params_;

    //! Result of execution.
    Result result_;
    //! Result synchronization.
    std::mutex mtx_result_;

    //! run() synchronization.
    bool flag_running_;
    std::mutex mtx_running_;

    //! terminate() synchronization.
    bool flag_terminated_;
    std::mutex mtx_terminated_;

public:

    /**
     *  @brief Create the worker thread in the suspended state.
     *
     *  Use `run()` to start the worker thread.
     *
     *  @sa tec::Worker::run()
     */
    Worker(const TWorkerParams& params)
        : thread_{detail<TWorkerParams>::thread_proc, std::ref(*this)}
        , params_{params}
        , flag_running_{false}
        , flag_terminated_{false}
    {}

    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;

    virtual ~Worker() {
        if( thread_.joinable() ) {
            terminate();
        }
    }

    //! Worker thread attribute.
    id_t id() const { return thread_id_; }

    //! Result of execution.
    Result result() {
        Lock lk(mtx_result_);
        return result_;
    }

    //! Custom parameters.
    const TWorkerParams& params() const { return params_; }

    //! Signals that Worker thread has started.
    const Signal& sig_running() const { return sig_running_; }

    //! Signals that Worker thread has inited (possible, with error).
    const Signal& sig_inited() const { return sig_inited_; }

    //! Signals that Worker thread has terminated.
    const Signal& sig_terminated() const { return sig_terminated_; }

private:
    void set_result(Result result) {
        Lock lk(mtx_result_);
        result_ = result;
    }

    void send_private(const TMessage& msg) {
        mq_.enqueue(msg);
    }


    struct OnExit {
        Signal& sig;
        OnExit(Signal& _sig): sig{_sig} {}
        ~OnExit() { sig.set(); }
    };

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename>
    struct detail {
        static void thread_proc(Worker& worker) {
            TEC_ENTER("Worker::thread_proc");

            // `sig_terminated' will signal on exit.
            OnExit on_exit(worker.sig_terminated_);

            // We obtain thread_id and then we are waiting for sig_begin
            // to resume the thread, see run().
            worker.thread_id_ = std::this_thread::get_id();
            TEC_TRACE("thread {} created.", worker.id());
            worker.sig_running_.wait();
            TEC_TRACE("`sig_running' received.");

            // Initialize the worker and set the result.
            TEC_TRACE("init() called ...");
            auto init_result = worker.init();
            TEC_TRACE("init() returned {}.", init_result);
            worker.set_result(init_result);
            if( !init_result) {
                // If error, send QUIT immediately.
                worker.send_private(quit<TMessage>());
            }

            // Signals the worker is inited, no matter if initialization failed.
            worker.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            // Start message polling.
            TEC_TRACE("entering message loop.");
            TMessage msg;
            while( worker.mq_.poll(msg) ) {
                TEC_TRACE("received Message [cmd={}].", msg.command);
                // Process a user-defined message
                worker.process(msg);
            }
            TEC_TRACE("leaving message loop.");

            // Finalize the worker if it has been inited successfully.
            if( worker.result() ) {
                TEC_TRACE("finalize() called ...");
                auto fin_result = worker.finalize();
                TEC_TRACE("finalize() returned {}.", fin_result);
                worker.set_result(fin_result);
            }

            // `sig_terminated' signals on exit.
        }
    };

public:

    /**
     *  @brief Start the worker thread with message polling.
     *
     *  Resumes the working thread by setting the `sig_begin` signal,
     *  then waits for init() callback completed.
     *
     *  @return tec::Result
     */
    Result run() override {
        Lock lk{mtx_running_};
        TEC_ENTER("Worker::run");

        if( !thread_.joinable() ) {
            // No thread exists, possible system failure.
            TEC_TRACE("no active thread.");
            return {"no active thread", Result::Kind::System};
        }

        if( flag_running_ ) {
            TEC_TRACE("WARNING: Worker thread is running already!");
            return {};
        }

        // Resume the thread
        flag_running_ = true;
        sig_running_.set();
        TEC_TRACE("`sig_running' signalled.");

        // Wait for thread init() completed
        TEC_TRACE("waiting for `sig_inited' signalled ...");
        sig_inited_.wait();

        return result();
    }


    /**
     *  @brief  Send a message to the worker thread.
     *
     *  If no Worker thread exists,
     *  the message will not be sent, returning `false`.
     *
     *  @param  msg a message to send.
     *  @return bool
     */
    virtual bool send(const TMessage& msg) {
        TEC_ENTER("Worker::send");
        if( thread_.joinable() ) {
            mq_.enqueue(msg);
            TEC_TRACE("Message [cmd={}] sent.", msg.command);
            return true;
        }
        else {
            return false;
        }
    }


    /**
     *  @brief Terminate the worker thread.
     *
     *  Sends Message::QUIT message to the message queue
     *  stopping the message polling, then waits for
     *  `sig_terminated` signalled to close the working thread.
     *
     *  Returns a Result of thread initialization.
     *
     *  @return tec::Result
     */
    Result terminate() override {
        Lock lk{mtx_terminated_};
        TEC_ENTER("Worker::terminate");

        if( flag_terminated_ ) {
            TEC_TRACE("WARNING: Worker thread already terminated!");
            return result();
        }

        if( !thread_.joinable() ) {
            return {"No active thread", Result::Kind::RuntimeErr};
        }

        // Send Message::QUIT.
        send(quit<TMessage>());
        TEC_TRACE("QUIT sent.");

        // Waits for the thread to finish its execution
        TEC_TRACE("waiting for thread {} to finish ...", id());
        thread_.join();
        TEC_TRACE("thread {} finished OK.", id());

        flag_terminated_ = true;
        return result();
    }

protected:
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Worker callbacks
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     *  @brief Called on worker initialization.
     *
     *  @note If init() returns Result other than Kind::Ok,
     *  finalize() callback **will not be called**.
     *
     *  @return tec::Result
     *  @sa tec::Worker::finalize()
     */
    virtual Result init() {
        return {};
    }

    /**
     *  @brief Processes a user-defined message.
     *
     *  @param msg A user-defined message.
     */
    virtual void process(const TMessage& msg) {
    }


    /**
     *  @brief Called on exiting from the thread.
     *
     *  @note This callback *is not called*
     *  if init() returned Result other than Kind::Ok.
     *
     *  @return tec::Result
     *  @sa tec::Worker::init()
     */
    virtual Result finalize() {
        return {};
    }

}; // ::Worker


} // ::tec
