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
 *   \file tec_rpc.hpp
 *   \brief Base server/client stuff definitions.
 *
 *  Base RPC server/client stuff definitions.
 *
*/

#pragma once

#include "tec/tec_worker.hpp"


namespace tec {

namespace rpc {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                RPC status of both server and client
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

enum Status {
    OK = 0,
    RPC_ERROR_SERVER_START = 53001,
    RPC_ERROR_SERVER_SHUTDOWN,
    RPC_ERROR_CLIENT_CONNECT
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                    Abstract Server Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct ServerParams {
};


class Server {
public:
    Server() {}
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    virtual ~Server() {}

    virtual Result run() = 0;
    virtual void shutdown() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                   Abstract Client Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Timeout defaults.
static const MilliSec RPC_CLIENT_CONNECT_TIMEOUT = MilliSec(5000);

struct ClientParams {
    MilliSec connect_timeout;

    ClientParams()
        : connect_timeout(RPC_CLIENT_CONNECT_TIMEOUT)
    {}
};


class Client {
public:
    Client() {}
    Client(const Client&) = delete;
    Client(Client&&) = delete;
    virtual ~Client() {}

    virtual Result connect() = 0;
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Server Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Worker: default timeouts
static const MilliSec RPC_WORKER_START_TIMEOUT    = MilliSec(250);
static const MilliSec RPC_WORKER_SHUTDOWN_TIMEOUT = MilliSec(10000);

struct ServerWorkerParams
{
    MilliSec start_timeout;
    MilliSec shutdown_timeout;

    ServerWorkerParams()
        : start_timeout(RPC_WORKER_START_TIMEOUT)
        , shutdown_timeout(RPC_WORKER_SHUTDOWN_TIMEOUT)
    {}
};


template <typename TWorkerParams, typename TServer>
class ServerWorker: public Worker<TWorkerParams> {
protected:
    TWorkerParams params_;
    std::unique_ptr<TServer> server_;

private:
    std::unique_ptr<std::thread> server_thread_;
    Signal sig_started_;
    Signal sig_stopped_;
    Result result_started_;

public:
    ServerWorker(const TWorkerParams& params, TServer* server)
        : Worker<TWorkerParams>(params)
        , params_(params)
        , server_(server) // Now Worker owns the server!
    {}


    virtual ~ServerWorker() {}

protected:
    virtual Result init() override {
        TEC_ENTER("ServerWorker::init");

        server_thread_.reset(
            new std::thread([&]{
                result_started_ = server_->run();
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
                return {RPC_ERROR_SERVER_START, "RPC server unknown error"};
            }
            return result_started_;
        }
    }


    virtual Result finalize() override {
        TEC_ENTER("ServerWorker::finalize");

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
            result = {RPC_ERROR_SERVER_SHUTDOWN, "RPC Server shutdown timeout"};
        }
        else {
            // Release the server thread.
            server_thread_->join();
        }

        shutdown_thread.join();
        return result;
    }
};


} // ::rpc

} // tec
