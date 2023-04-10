/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \brief Defines a Worker class.
 *
 * Manage a Windows-ish thread using Messages.
 *
*/

#pragma once

#include <iostream>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>

#include "tec/tec_def.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_queue.hpp"
#include "tec/tec_trace.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Worker thread
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TWorkerParams, typename Duration = MilliSec>
class Worker {
public:
    typedef std::thread::id id_t;
    typedef Duration duration_t;

#if __TEC_PTR__ == 64
    typedef int64_t arg_t;
#elif __TEC_PTR__ == 32
    typedef int32_t arg_t;
#else
#error Unknown pointer size __TEC_PTR__
#endif

#if __TEC_LONG__ == 64
    typedef uint64_t cmd_t;
#elif __TEC_LONG__ == 32
    typedef uint32_t cmd_t;
#endif

    //! Worker system error codes.
    enum Status {
        OK = 0,
        ERR_MIN = 50000,
        NO_THREAD,
        ERR_MAX
    };

    //! Message *should* implement quit() to indicate that
    //! message polling should stop.
    //!
    //! \sa poll() in tec_queue.hpp.
    struct Message {
        enum Command {
            QUIT = 0  //!< RESERVED, do not use it directly.
        };

        cmd_t command;
        arg_t arg;

        inline const bool quit() const { return (command == Command::QUIT); }

        //! Null message.
        static Message zero() { return{Command::QUIT, 0}; }
        //! Terminate message loop.
        static Message terminate() { return zero(); }
    };

    struct Metrics {
        std::string units;
        duration_t time_total;
        duration_t time_init;
        duration_t time_exec;
        duration_t time_finalize;

        Metrics()
            : units(time_unit(duration_t(0)))
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
    SafeQueue<Message> mq_;

    //! Worker parameters.
    TWorkerParams params_;

public:

    Worker(const TWorkerParams& params)
        : thread_(nullptr)
        , params_(params)
    {
    }

    //! Worker attributes
    inline id_t id() const { return thread_id_; }
    inline int exit_code() const { return result_.code(); }
    inline const std::string& error_str() const { return result_.str(); }
    inline Result result() const { return result_; }

    //! Statistics
    const Metrics& metrics() const { return metrics_; }

    // User specified parameters
    TWorkerParams& params() { return params_; }

    virtual ~Worker() {}

private:

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename>
    struct detail {
        static void thread_proc(Worker& worker) {
            TEC_ENTER("Worker::thread_proc");

            Timer<duration_t> t_total(worker.metrics_.time_total);

            // We obtained thread_id and then we are waiting for sig_begin
            // to resume the thread, see run().
            worker.thread_id_ = std::this_thread::get_id();
            worker.sig_begin_.wait();
            TEC_TRACE("sig_begin received.\n");

            // Signals sig_running, it will be reset on the thread exits.
            worker.sig_running_.set();
            TEC_TRACE("sig_running signalled.\n");

            // Initialize the worker
            Timer<duration_t> t_init(worker.metrics_.time_init);
            worker.result_ = worker.init();
            t_init.stop();

            // Signals that the worker is inited, no matter if initialization failed
            worker.sig_inited_.set();
            TEC_TRACE("sig_inited signalled.\n");

            // Start message polling
            TEC_TRACE("entering message loop.\n");
            Message msg = Message::zero();
            Timer<duration_t> t_exec(worker.metrics_.time_exec);
            while( worker.mq_.poll(msg) )
            {
                TEC_TRACE("received Message{%, %}.\n", msg.command, msg.arg);
                // Process a user-defined message
                worker.process(msg);
            }
            t_exec.stop();
            TEC_TRACE("leaving message loop on Message{%, %}.\n", msg.command, msg.arg);

            // Finalize the worker if it had been inited successfully
            if( worker.result_.ok() )
            {
                Timer<duration_t> t_finalize(worker.metrics_.time_finalize);
                worker.result_ = worker.finalize();
                t_finalize.stop();
            }

            // Notify that the worker is being teminated
            worker.sig_terminated_.set();
            t_total.stop();
            TEC_TRACE("sig_terminated signalled\n");

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
     *  \return self&
     *  \sa Worker::run()
     */
    virtual Worker& create() {
        TEC_ENTER("Worker::create");
        thread_.reset(new std::thread(detail<TWorkerParams>::thread_proc, std::ref(*this)));
        return *this;
    }


    /**
     *  \brief Start the worker thread with message polling.
     *
     *  Resumes the working thread by setting the sig_begin signal,
     *  then waits for init() callback completed.
     *
     *  \param none
     *  \return self&
     */
    virtual Worker& run() {
        TEC_ENTER("Worker::run");

        if( !thread_ ) {
            // No thread exists yet, see create()
            result_ = {Status::NO_THREAD};
            return *this;
        }

        // Resume the thread
        sig_begin_.set();
        TEC_TRACE("sig_begin signalled.\n");

        // Wait for thread init() completed
        TEC_TRACE("waiting for sig_inited signalled ...\n");
        sig_inited_.wait();

        return *this;
    }


    /**
     *  \brief Send a user-defined message to the worker thread.
     *
     *  Returns false if no working thread exists.
     *
     *  \param msg a user-defined message to handle
     *  \return bool
     */
    virtual bool send(const Message& msg) {
        if( thread_ ) {
            mq_.enqueue(msg);
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
     *  \return self&
     */
    virtual Worker& terminate() {
        TEC_ENTER("Worker::terminate");
        if( !thread_ ) {
            result_ = Status::NO_THREAD;
            return *this;
        }

        // Terminate message loop
        send(Message::terminate());
        TEC_TRACE("Message::terminate sent.\n");

        // Wait for thread sig_terminated signalled
        TEC_TRACE("waiting for sig_terminated ...\n");
        sig_terminated_.wait();

        thread_->join();
        TEC_TRACE("Worker thread joined OK.\n");

        return *this;
    }

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
     *  NOTE: Worker::get_exit_code() will return this Status.
     *
     *  \param none
     *  \return Status::OK (0) if succeeded
     *  \sa Worker::get_exit_code()
     *  \sa Worker::finalize()
     */
    virtual Result init() {
        return{OK};
    }

    /**
     *  \brief Called on user-defined message.
     *
     *  Processes a user-defined message.
     *
     *  \param msg _Message_ user-defined message
     *  \return none
     */
    virtual void process(const Message& msg)
    {
    }


    /**
     *  \brief Called on exiting from the thread.
     *
     *  NOTE: if init() returned Status different from OK,
     *  this callback *is not called*.
     *
     *  NOTE: Worker::get_exit_code() will return this Status.
     *
     *  \param none
     *  \return Status::OK (0) if succeeded
     *  \sa Worker::init()
     *  \sa Worker::get_exit_code()
     */
    virtual Result finalize()
    {
        return{OK};
    }

}; // ::Worker


} // ::tec
