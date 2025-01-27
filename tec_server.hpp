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
 *   \file tec_server.hpp
 *   \brief A Documented file.
 *
 *  Detailed description
 *
*/

#pragma once

#include "tec/tec_trace.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_worker.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                    Abstract Server Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class Server {
public:
    Server() = default;
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    virtual ~Server() = default;

    virtual void start(Signal&, Result&) = 0;
    virtual void shutdown(Signal&) = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   Abstract Client Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct ClientParams {
    //! Default client connection timeout (5 sec).
    static constexpr const MilliSec kConnectTimeout{Seconds{5}};

    MilliSec connect_timeout;

    ClientParams()
        : connect_timeout(kConnectTimeout)
    {}
};


class Client {
public:
    Client() = default;
    Client(const Client&) = delete;
    Client(Client&&) = delete;
    virtual ~Client() = default;

    virtual Result connect() = 0;
    virtual void close() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct ServerWorkerParams: public WorkerParams
{
    //! Worker: default timeouts.
    static constexpr const MilliSec kStartTimeout{Seconds{2}};
    static constexpr const MilliSec kShutdownTimeout{Seconds{10}};

    MilliSec start_timeout;
    MilliSec shutdown_timeout;

    ServerWorkerParams()
        : start_timeout(kStartTimeout)
        , shutdown_timeout(kShutdownTimeout)
    {}
};

//! NOTE: ServerParams MUST be a descendant of WorkerParams, see tec_worker.hpp.
struct ServerParams: public ServerWorkerParams {
};



template <typename TServerParams, typename TServer>
class ServerWorker: public Worker<TServerParams> {
protected:
    TServerParams params_;
    std::unique_ptr<TServer> server_;

private:
    std::unique_ptr<std::thread> server_thread_;
    Signal sig_started_;
    Signal sig_stopped_;
    Result result_started_;
    Result result_stopped_;

public:
    //! Initialize with a Server.
    ServerWorker(const TServerParams& params, std::unique_ptr<TServer> server)
        : Worker<TServerParams>(params)
        , params_(params)
        , server_(std::move(server))    // Now ServerWorker owns the server!
    {}

    //! No Server provided in constructor, use attach_server() later.
    ServerWorker(const TServerParams& params)
        : Worker<TServerParams>(params)
        , params_(params)
    {}

    virtual ~ServerWorker() {}

    virtual Result attach_server(TServer* server) {
        if( server == nullptr ) {
            return{"No server provided", Result::Kind::Invalid};
        }
        if( server_ ) {
            return{"Server already attached", Result::Kind::Invalid};
        }
        server_.reset(server);
        return{};
    }

protected:
    Result init() override {
        TEC_ENTER("ServerWorker::init");

        if( !server_ ) {
            return {"No server exists", Result::Kind::Invalid};
        }

        // A new thread to start the server.
        server_thread_.reset(
            new std::thread([&]{
                server_->start(sig_started_, result_started_);
            }));

        if( !sig_started_.wait_for(params_.start_timeout) ) {
            // Timeout!
            TEC_TRACE("!!! Error: server start timeout");
            server_thread_->detach();
            return {"Server start timeout", Result::Kind::TimeoutErr};
        }
        if( !result_started_ ) {
            // Something wrong happened, release the server thread.
            server_thread_->join();
            return result_started_;
        }

        // Everything is OK.
        return {};
    }


    Result finalize() override {
        TEC_ENTER("ServerWorker::finalize");

        if( !server_ ) {
            return {"No server exists", Result::Kind::Invalid};
        }

        if( !server_thread_ ) {
            // Already finished or not started.
            return {};
        }

        std::thread shutdown_thread([&]{
            server_->shutdown(sig_stopped_);
            });

        if( !sig_stopped_.wait_for(params_.shutdown_timeout) ) {
            // Timeout! Detach the server thread.
            server_thread_->detach();
            // Report timeout.
            TEC_TRACE("!!! Error: Server shutdown timeout.");
            result_stopped_ = {"Server shutdown timeout", Result::Kind::TimeoutErr};
        }
        else {
            server_thread_->join();
        }

        shutdown_thread.join();
        return result_stopped_;
    }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Daemon Builder
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! A functor that creates a server daemon.
template <typename TServerParams>
struct DaemonBuilder {
    using Func = std::unique_ptr<Daemon>(*)(const TServerParams&);
    Func fptr;

    DaemonBuilder(Func func)
        : fptr{func}
    {}

    std::unique_ptr<Daemon> operator()(const TServerParams& server_params) {
        if( nullptr == fptr ) {
            return nullptr;
        }
        else {
            return fptr(server_params);
        }
    }
};


} // ::tec
