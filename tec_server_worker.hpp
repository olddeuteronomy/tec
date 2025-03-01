// Time-stamp: <Last changed 2025-02-14 16:37:47 by magnolia>
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
 *   @file tec_server_worker.hpp
 *   @brief Implements the ServerWorker class.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_worker_thread.hpp"
#include "tec/tec_server.hpp"
#include <memory>


namespace tec {


template <typename Traits, typename TServer>
class ServerWorker: public WorkerThread<Traits> {
public:
    typedef Traits traits ;
    typedef typename traits::Params Params;
    typedef typename traits::Message Message;

private:
    std::unique_ptr<TServer> server_;

    std::unique_ptr<std::thread> server_thread_;
    Signal sig_run_server_thread_;

    Signal sig_started_;
    Signal sig_stopped_;
    Result result_started_;
    Result result_stopped_;

public:

    //! Initialize with a Server.
    ServerWorker(const Params& params, std::unique_ptr<TServer> server)
        : WorkerThread<Traits>{params}
        // Now ServerWorker owns the server!
        , server_{std::move(server)}
        // Start the paused server thread. Resume it by sig_run_server_thread_.set().
        , server_thread_{new std::thread([&]{
            sig_run_server_thread_.wait();
            server_->start(sig_started_, result_started_);
        })}
    {
    }

    virtual ~ServerWorker() {
        if( server_thread_->joinable() ) {
            server_thread_->join();
        }
    }

protected:

    Result on_init() override {
        TEC_ENTER("ServerWorker::on_init");

        // Resume the server thread.
        sig_run_server_thread_.set();

        // Wait for the server is started.
        if( !sig_started_.wait_for(this->params().start_timeout) ) {
            // Timeout!
            TEC_TRACE("!!! Error: server start timeout");
            server_thread_->detach();
            return {"Server start timeout", Error::Kind::TimeoutErr};
        }
        if( !result_started_ ) {
            // Something wrong happened, release the server thread.
            server_thread_->join();
            return result_started_;
        }

        // Everything is OK.
        return {};
    }

    Result on_exit() override {
        TEC_ENTER("ServerWorker::on_exit");

        if( !server_thread_->joinable() ) {
            // Already finished or not started.
            return {};
        }

        // Start the new thread that shutdowns the server attached.
        std::thread shutdown_thread([&]{
            server_->shutdown(sig_stopped_);
        });

        // Wait for the server is down.
        if( !sig_stopped_.wait_for(this->params().shutdown_timeout) ) {
            // Timeout! Detach the server thread.
            server_thread_->detach();
            // Report timeout.
            TEC_TRACE("!!! Error: Server shutdown timeout.");
            result_stopped_ = {"Server shutdown timeout", Error::Kind::TimeoutErr};
        }
        else {
            server_thread_->join();
        }

        shutdown_thread.join();
        return result_stopped_;
    }

public:
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                       Server Worker Builders
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    //! Server Worker builder.
    template <typename Derived, typename ServerDerived>
    struct Builder {
        std::unique_ptr<Worker<typename Derived::traits>>
        operator()(typename Derived::Params& params)
        {
            // Check for a Derived class is derived from the tec::Worker class.
            static_assert(
                std::is_base_of<Worker<typename Derived::traits>, Derived>::value,
                "not derived from tec::Worker class");
            // Check for a ServerDerived class is derived from the tec::Server class.
            static_assert(
                std::is_base_of<Server<typename Derived::Params>, ServerDerived>::value,
                "not derived from tec::Server class");
            // Check for a Parameters class is derived from the tec::ServerParameters class.
            static_assert(
                std::is_base_of<ServerParams, typename Derived::Params>::value,
                "not derived from tec::ServerParams class");
            return std::make_unique<Derived>(params, std::make_unique<ServerDerived>(params));
        }
    };

    //! Server Daemon builder.
    template <typename Derived, typename ServerDerived>
    struct DaemonBuilder {
        std::unique_ptr<Daemon>
        operator()(typename Derived::Params& params)
        {
            // Check for a Derived class is derived from the tec::Worker class.
            static_assert(
                std::is_base_of<Daemon, Derived>::value,
                "not derived from tec::Worker class");
            // Check for a ServerDerived class is derived from the tec::Server class.
            static_assert(
                std::is_base_of<Server<typename Derived::Params>, ServerDerived>::value,
                "not derived from tec::Server class");
            // Check for a Parameters class is derived from the tec::ServerParameters class.
            static_assert(
                std::is_base_of<ServerParams, typename Derived::Params>::value,
                "not derived from tec::ServerParams class");
            return std::make_unique<Derived>(params, std::make_unique<ServerDerived>(params));
        }
    };

}; // ::ServerWorker


} // ::tec
