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
#include <thread>

#include "tec_def.hpp"
#include "tec_utils.hpp"
#include "tec_queue.hpp"
#include "tec_trace.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Worker thread
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TWorkerParams, typename Duration = MilliSec>
class Worker
{
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
    //typedef uint64_t errcode_t;
    typedef uint64_t cmd_t;
#elif __TEC_LONG__ == 32
    //typedef uint32_t errcode_t;
    typedef uint32_t cmd_t;
#endif

    //! Worker system error codes.
    enum Status
    {
        OK = 0,
        ERR_MIN = 50000,
        START_TIMEOUT,
        TERMINATE_TIMEOUT,
        NO_THREAD,
        ERR_MAX
    };

    //! Message should implement quit() to indicate that
    //! message polling should stop.
    //!
    //! \sa poll() in tec_queue.hpp.
    struct Message
    {
        enum Command
        {
            QUIT = 0
        };

        cmd_t command;
        arg_t arg;

        inline const bool quit() const { return (command == Command::QUIT); }

        //! Null message.
        static Message zero() { return{Command::QUIT, 0}; }
        //! Terminate message loop and set exit code.
        static Message terminate() { return zero(); }
    };

protected:

    //! Worker attributes.
    std::unique_ptr<std::thread> thread_;
    id_t thread_id_;
    Result result_;

    //! Signals.
    Signal sig_begin_;
    Signal sig_running_;
    Signal sig_inited_;
    Signal sig_terminated_;

    //! Statistics.
    duration_t stat_time_total_;
    duration_t stat_time_init_;
    duration_t stat_time_exec_;
    duration_t stat_time_finalize_;

    //! Message queue.
    SafeQueue<Message> mq_;

    //! Worker parameters.
    TWorkerParams params_;

public:

    Worker(const TWorkerParams& params)
        : thread_{nullptr}
        , stat_time_total_{0}
        , stat_time_init_{0}
        , stat_time_exec_{0}
        , stat_time_finalize_{0}
        , params_(params)
    {
    }

    //! Worker attributes
    inline id_t get_id() const { return thread_id_; }
    inline int get_exit_code() const { return result_.code(); }
    const std::string& get_error_str() const { return result_.str(); }
    Result get_result() const { return result_; }

    //! Statistics
    inline duration_t get_time_total() const { return stat_time_total_; }
    inline duration_t get_time_init() const { return stat_time_init_; }
    inline duration_t get_time_exec() const { return stat_time_exec_; }
    inline duration_t get_time_finalize() const { return stat_time_finalize_; }

    // User specified parameters
    TWorkerParams& params() { return params_; }

    virtual ~Worker()
    {
    }

private:

    //-----------------------------------------------------------------
    // WE HIDE THREAD PROCEDURE INSIDE A STRUCT TO INSTANTIATE
    // DIFFERENT STATIC PROCEDURES OWNED BY DIFFERENT WORKERS.
    //-----------------------------------------------------------------
    template <typename>
    struct detail
    {
        static void thread_proc(Worker& worker)
        {
            TEC_ENTER("thread_proc");

            // Worker* worker = (Worker*)(void*)pWorker;
            Timer<duration_t> t_total;

            // We obtained thread_id and then we are waiting for sig_begin
            // to resume the thread, see run().
            worker.thread_id_ = std::this_thread::get_id();
            worker.sig_begin_.wait();
            TEC_TRACE("sig_begin received.\n");

            // Signals sig_running, it will be reset on the thread exits.
            worker.sig_running_.set();
            TEC_TRACE("sig_running signalled.\n");

            // Initialize the worker
            Timer<duration_t> t_init;
            worker.result_ = worker.init();
            worker.stat_time_init_ = t_init.stop();

            // Signals that the worker is inited, no matter if initialization failed
            worker.sig_inited_.set();
            TEC_TRACE("sig_inited signalled.\n");

            // Start message polling
            TEC_TRACE("entering message loop.\n");
            Message msg = Message::zero();
            Timer<duration_t> t_exec;
            while( worker.mq_.poll(msg) )
            {
                TEC_TRACE("received Message{%, %}.\n", msg.command, msg.arg);
                // Process a user-defined message
                worker.process(msg);
            }
            worker.stat_time_exec_ = t_exec.stop();
            TEC_TRACE("leaving message loop on Message{%, %}.\n", msg.command, msg.arg);

            // Finalize the worker if inited successfully
            if( worker.result_.ok() )
            {
                Timer<duration_t> t_finalize;
                worker.result_ = worker.finalize();
                worker.stat_time_finalize_ = t_finalize.stop();
            }

            // Notify that the worker is teminating
            worker.sig_terminated_.set();
            worker.stat_time_total_ = t_total.stop();
            TEC_TRACE("sig_terminated signalled\n");

            // We are leaving the thread
            worker.sig_running_.reset();
        }
    };

public:

    //! \brief Creates the worker thread in suspended state.
    //! \return self&.
    Worker& create()
    {
        TEC_ENTER("create()");
        // Create a thread in "suspended" state.
        // The thread now waits to be resumed by sig_begin_ signalled. See run().
        thread_.reset(new std::thread(detail<TWorkerParams>::thread_proc, std::ref(*this)));
        return *this;
    }


    Worker& run()
    {
        TEC_ENTER("run()");

        if( !thread_ )
        {
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


    bool send(const Message& msg)
    {
        if( thread_ )
        {
            mq_.enqueue(msg);
            return true;
        }
        else
        {
            return false;
        }
    }


    Worker& terminate()
    {
        TEC_ENTER("terminate()");
        if( !thread_ )
        {
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
     *  \brief Calling on thread initialization.
     *
     *  TODO
     *
     *  \param void
     *  \return errcode_t
     */
    virtual Result init()
    {
        return{OK};
    }

    /**
     *  \brief Called on user message.
     *
     *  TODO
     *
     *  \param msg *Message* user-defined message
     *  \return void
     */
    virtual void process(const Message& msg)
    {
    }


    /**
     *  \brief Called on exiting from the thread.
     *
     *  TODO
     *
     *  \param *exit_code* errcode_t Exit code
     *  \return 0 if succeeded, Worker::Status otherwise
     *  \sa Worker::get_exit_code
     */
    virtual Result finalize()
    {
        return{OK};
    }

}; // ::Worker


} // ::tec
