/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2024 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \file tec_worker_std.hpp
 *   \brief Declares a Worker class.
 *
 * Manages a Windows-ish thread using Messages.
 *
*/

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_utils.hpp"
#include "tec/tec_queue.hpp"
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

//! Null message.
template <typename TMessage>
TMessage zero() { TMessage msg; msg.command = Message::QUIT; return msg; }


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
    std::unique_ptr<std::thread> thread_;
    id_t thread_id_;

    //! Signals.
    Signal sig_begin_;
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
     *  @return tec::Result
     *  @sa tec::Worker::run()
     */
    Worker(const TWorkerParams& params)
        : thread_{new std::thread(detail<TWorkerParams>::thread_proc, std::ref(*this))}
        , params_{params}
        , flag_running_{false}
        , flag_terminated_{false}
    {}

    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;

    virtual ~Worker() {
        terminate();
    }

    //! Worker thread attribute.
    id_t id() const { return thread_id_; }

    //! Result of execution.
    Result result() {
        Lock lk(mtx_result_);
        return result_;
    }

    //! User parameters.
    const TWorkerParams& params() const { return params_; }

    //! Wait for Worker thread started.
    void wait_for_running() const {
        sig_begin_.wait();
    }

    //! Wait for initialization completed (possible with error).
    void wait_for_inited() const {
        sig_inited_.wait();
    }

    //! Wait for the Worker thread terminated.
    void wait_for_terminated() const {
        sig_terminated_.wait();
    }

private:
    void set_result(Result result) {
        Lock lk(mtx_result_);
        result_ = result;
    }

    void send_private(const TMessage& msg) {
        mq_.enqueue(msg);
    }

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename>
    struct detail {
        static void thread_proc(Worker& worker) {
            TEC_ENTER("Worker::thread_proc");

            // We obtain thread_id and then we are waiting for sig_begin
            // to resume the thread, see run().
            worker.thread_id_ = std::this_thread::get_id();
            worker.sig_begin_.wait();
            TEC_TRACE("`sig_begin' received.");

            // Message to be received, initially QUIT.
            TMessage msg{zero<TMessage>()};

            // Initialize the worker and set the result.
            auto init_result = worker.init();
            TEC_TRACE("init() returned %.", init_result);
            worker.set_result(init_result);
            if( !init_result) {
                // If error, send QUIT.
                worker.send_private(msg);
            }

            // Signals that the worker is inited, no matter if initialization failed.
            worker.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            // Start message polling.
            TEC_TRACE("entering message loop.");
            while( worker.mq_.poll(msg) ) {
                TEC_TRACE("received Message [cmd=%].", msg.command);
                // Process a user-defined message
                worker.process(msg);
            }
            TEC_TRACE("leaving message loop.");

            // Finalize the worker if it has been inited successfully.
            if( worker.result() ) {
                auto fin_result = worker.finalize();
                TEC_TRACE("finalize() returned %.", fin_result);
                worker.set_result(fin_result);
            }

            // Notify that the worker is being teminated.
            TEC_TRACE("`sig_terminated' signalled.");
            worker.sig_terminated_.set();
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

        if( !thread_ ) {
            // No thread exists, possible system failure.
            return {"Worker::run(): no thread exists", Result::Kind::System};
        }

        if( flag_running_ ) {
            TEC_TRACE("WARNING: Worker thread is running already!");
            return {};
        }

        // Resume the thread
        flag_running_ = true;
        sig_begin_.set();
        TEC_TRACE("`sig_begin' signalled.");

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
        if( thread_ ) {
            mq_.enqueue(msg);
            TEC_TRACE("Message [cmd=%] sent.", msg.command);
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

        if( !thread_ ) {
            return {"No thread exists", Result::Kind::RuntimeErr};
        }

        // Send Message::QUIT.
        send(zero<TMessage>());
        TEC_TRACE("QUIT sent.");

        // Wait for thread's `sig_terminated` signalled.
        TEC_TRACE("waiting for `sig_terminated' ...");
        sig_terminated_.wait();

        thread_->join();
        TEC_TRACE("Worker thread terminated OK.");

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
