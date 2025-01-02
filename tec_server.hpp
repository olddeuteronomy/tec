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
 *   \file tec_server.hpp
 *   \brief A Documented file.
 *
 *  Detailed description
 *
*/

#pragma once

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
    static constexpr const int kServerErr{51000};

    enum Status {
        ERROR_SERVER_START = kServerErr
        ,ERROR_SERVER_SHUTDOWN
    };

    Server() = default;
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    virtual ~Server() = default;

    virtual Result<> start() = 0;
    virtual void shutdown() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   Abstract Client Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Timeout defaults.
constexpr const MilliSec kClientConnectTimeout = MilliSec{5000};

struct ClientParams {
    MilliSec connect_timeout;

    ClientParams()
        : connect_timeout(kClientConnectTimeout)
    {}
};


class Client {
public:
    static constexpr const int kClientErr{51100};

    enum Status {
        ERROR_CLIENT_CONNECT = kClientErr
    };

    Client() = default;
    Client(const Client&) = delete;
    Client(Client&&) = delete;
    virtual ~Client() = default;

    virtual Result<> connect() = 0;
    virtual void close() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Worker: default timeouts
static constexpr const MilliSec kWorkerStartTimeout    = MilliSec{250};
static constexpr const MilliSec kWorkerShutdownTimeout = MilliSec{10000};

struct ServerWorkerParams: public WorkerParams
{
    MilliSec start_timeout;
    MilliSec shutdown_timeout;

    ServerWorkerParams()
        : start_timeout(kWorkerStartTimeout)
        , shutdown_timeout(kWorkerShutdownTimeout)
    {}
};

//! NOTE: ServerParams MUST be a descendant of WorkerParams, see tec_worker.hpp.
struct ServerParams: public ServerWorkerParams {
};



template <typename TServerParams, typename TServer>
class ServerWorker: public Worker<TServerParams> {
public:
    static constexpr const int kServerWorkerErr{51200};

    enum Status
    {
        NO_SERVER_PROVIDED = kServerWorkerErr  //!< No server found.
        ,SERVER_ALREADY_ATTACHED               //!< Worker already has a server attached.
    };

protected:
    TServerParams params_;
    std::unique_ptr<TServer> server_;

private:
    std::unique_ptr<std::thread> server_thread_;
    Signal sig_started_;
    Signal sig_stopped_;
    Result<> result_started_;

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

    virtual Result<> attach_server(TServer* server) {
        if( server == nullptr ) {
            return{Status::NO_SERVER_PROVIDED, "No Server provided", Result<>::Kind::RuntimeErr};
        }
        if( server_ ) {
            return{Status::SERVER_ALREADY_ATTACHED, "Server already attached", Result<>::Kind::RuntimeErr};
        }
        server_.reset(server);
        return{};
    }

protected:
    Result<> init() override {
        TEC_ENTER("ServerWorker::init");

        if( !server_ ) {
            return{Status::NO_SERVER_PROVIDED, "No Server exists", Result<>::Kind::RuntimeErr};
        }

        server_thread_.reset(
            new std::thread([&]{
                result_started_ = server_->start();
                sig_started_.set();
            }));
        if( !sig_started_.wait_for(params_.start_timeout) ) {
            // Timeout! But that means the server is started...
            return{};
        }
        else {
            // Server finished too quickly, possible with error -
            // something wrong happened, release the server thread.
            server_thread_->join();
            if( result_started_.ok() ) {
                return {Server::ERROR_SERVER_START, "Server cannot start", Result<>::Kind::RuntimeErr};
            }
            return result_started_;
        }
    }


    Result<> finalize() override {
        TEC_ENTER("ServerWorker::finalize");

        if( !server_ ) {
            return{Status::NO_SERVER_PROVIDED, "No Server exists", Result<>::Kind::RuntimeErr};
        }

        if( !server_thread_ ) {
            // Already finished, perhaps there was an error.
            return{};
        }

        Result result;
        std::thread shutdown_thread([&]{
            server_->shutdown();
            sig_stopped_.set();});

        if( !sig_stopped_.wait_for(params_.shutdown_timeout) ) {
            // Timeout! Detach the server thread.
            server_thread_->detach();
            // Report timeout.
            TEC_TRACE("error: timeout.\n");
            result = {Server::ERROR_SERVER_SHUTDOWN, "Server shutdown timeout", Result<>::Kind::RuntimeErr};
        }
        else {
            // Release the server thread.
            server_thread_->join();
        }

        shutdown_thread.join();
        return result;
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
