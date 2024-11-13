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

    virtual Result create() = 0;
    virtual Result run() = 0;
    virtual Result terminate() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Message
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Message *should* implement quit() to indicate that
//! message polling should stop.
//!
//! \sa poll() in tec_queue.hpp.
struct BasicMessage {
    typedef unsigned long cmd_t ;

    //! Quit message loop command.
    static const cmd_t QUIT = 0;

    cmd_t command; //!< A command.

    //! Indicates that message loop should be terminated.
    inline bool quit() const { return (command == BasicMessage::QUIT); }
};

//! Null message.
template <typename TMessage>
TMessage zero() { TMessage msg; msg.command = BasicMessage::QUIT; return msg; }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Worker thread
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TWorkerParams, typename TMessage = BasicMessage, typename Duration = MilliSec>
class Worker: public Daemon {

public:
    using id_t = std::thread::id;

    //! Worker error codes.
    enum Status {
        OK = 0,
        ERR_MIN = 50000,
        NO_THREAD,
        THREAD_ALREADY_EXISTS,
        ERR_MAX
    };

    //! Statistics.
    struct Metrics {
        std::string units;
        Duration time_total;
        Duration time_init;
        Duration time_exec;
        Duration time_finalize;

        Metrics()
            : units(time_unit(Duration(0)))
            , time_total(0)
            , time_init(0)
            , time_exec(0)
            , time_finalize(0)
            {}
    };

protected:

    //! Worker members.
    std::unique_ptr<std::thread> thread_;
    id_t thread_id_;
    Result result_;

    //! Signals.
    Signal sig_begin_;
    Signal sig_running_;
    Signal sig_inited_;
    Signal sig_terminated_;

    //! Metrics.
    Metrics metrics_;

    //! Message queue.
    SafeQueue<TMessage> mq_;

    //! Worker parameters.
    TWorkerParams params_;

public:

    Worker(const TWorkerParams& params)
        : thread_(nullptr)
        , params_(params)
    {}

    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;

    virtual ~Worker() = default;

    //! Worker attributes
    inline id_t id() const { return thread_id_; }
    inline int exit_code() const { return result_.code(); }
    inline const std::string& error_str() const { return result_.str(); }
    inline Result result() const { return result_; }

    //! Statistics
    const Metrics& metrics() const { return metrics_; }

    // User specified parameters
    TWorkerParams& params() { return params_; }

private:

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename>
    struct detail {
        static void thread_proc(Worker& worker) {
            TEC_ENTER("Worker::thread_proc");

            Timer<Duration> t_total(worker.metrics_.time_total);

            // We obtained thread_id and then we are waiting for sig_begin
            // to resume the thread, see run().
            worker.thread_id_ = std::this_thread::get_id();
            worker.sig_begin_.wait();
            TEC_TRACE("`sig_begin' received.");

            // Signals sig_running, it will be reset on the thread exits.
            worker.sig_running_.set();
            TEC_TRACE("`sig_running' signalled.");

            // Initialize the worker
            Timer<Duration> t_init(worker.metrics_.time_init);
            worker.result_ = worker.init();
            t_init.stop();

            // Signals that the worker is inited, no matter if initialization failed
            worker.sig_inited_.set();
            TEC_TRACE("`sig_inited' signalled.");

            // Start message polling
            TEC_TRACE("entering message loop.");
            TMessage msg = zero<TMessage>();
            Timer<Duration> t_exec(worker.metrics_.time_exec);
            while( worker.mq_.poll(msg) )
            {
                TEC_TRACE("received Message [cmd=%].", msg.command);
                // Process a user-defined message
                worker.process(msg);
            }
            t_exec.stop();
            TEC_TRACE("leaving message loop.");

            // Finalize the worker if it had been inited successfully
            if( worker.result_.ok() )
            {
                Timer<Duration> t_finalize(worker.metrics_.time_finalize);
                worker.result_ = worker.finalize();
                t_finalize.stop();
            }

            // Notify that the worker is being teminated
            worker.sig_terminated_.set();
            t_total.stop();
            TEC_TRACE("`sig_terminated' signalled.");

            // We are leaving the thread now
            worker.sig_running_.reset();
        }
    };

public:

    /**
     *  \brief Create the worker thread in the "suspended" state.
     *
     *  The newly created worker thread waits for sig_begin signalled.
     *  Use run() to start the worker thread.
     *
     *  \param none
     *  \return tec::Result
     *  \sa Worker::run()
     */
    Result create() override {
        TEC_ENTER("Worker::create");
        if( thread_ != nullptr) {
            result_ = {Status::THREAD_ALREADY_EXISTS, "thread already exists"};
            return result_;
        }
        thread_.reset(new std::thread(detail<TWorkerParams>::thread_proc, std::ref(*this)));
        return {};
    }


    /**
     *  \brief Start the worker thread with message polling.
     *
     *  Resumes the working thread by setting the sig_begin signal,
     *  then waits for init() callback completed.
     *
     *  \param none
     *  \return tec::Result
     */
    Result run() override {
        TEC_ENTER("Worker::run");

        if( !thread_ ) {
            // No thread exists yet, see create()
            result_ = {Status::NO_THREAD};
            return result_;
        }

        // Resume the thread
        sig_begin_.set();
        TEC_TRACE("`sig_begin' signalled.");

        // Wait for thread init() completed
        TEC_TRACE("waiting for `sig_inited' signalled ...");
        sig_inited_.wait();

        return result_;
    }


    /**
     *  \brief Send a user-defined message to the worker thread.
     *
     *  Returns false if no working thread exists.
     *
     *  \param msg a user-defined message to handle
     *  \return bool
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
     *  \brief Terminate the worker thread.
     *
     *  Sends Message::terminate message to the message queue
     *  stopping the message polling, then waits for
     *  sig_terminated signal to close the working thread.
     *
     *  \param none
     *  \return tec::Result
     */
    Result terminate() override {
        TEC_ENTER("Worker::terminate");
        if( !thread_ ) {
            result_ = Status::NO_THREAD;
            return result_;
        }

        // Terminate message loop
        send(zero<TMessage>());
        TEC_TRACE("Message::terminate sent.");

        // Wait for thread sig_terminated signalled
        TEC_TRACE("waiting for `sig_terminated' ...");
        sig_terminated_.wait();

        thread_->join();
        TEC_TRACE("Worker thread joined OK.");

        return result_;
    }

protected:
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Worker callbacks
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     *  \brief Called on worker initialization.
     *
     *  NOTE: if init() returns Status different from OK,
     *  finalize() callback *would not be called*.
     *
     *  NOTE: Worker::exit_code() returns this Status.
     *
     *  \param none
     *  \return Status::OK (0) if succeeded
     *  \sa Worker::exit_code()
     *  \sa Worker::finalize()
     */
    virtual Result init() {
        return{};
    }

    /**
     *  \brief Called on user-defined message.
     *
     *  Processes a user-defined message.
     *
     *  \param msg a user-defined message
     *  \return none
     */
    virtual void process(const TMessage& msg) {
    }


    /**
     *  \brief Called on exiting from the thread.
     *
     *  NOTE: this callback *is not called*
     *  if init() returned status other than Status::OK.
     *
     *  \param none
     *  \return Status::OK (0) if succeeded
     *  \sa Worker::init()
     *  \sa Worker::exit_code()
     */
    virtual Result finalize() {
        return{};
    }

}; // ::Worker


} // ::tec
