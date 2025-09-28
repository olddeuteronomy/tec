// Time-stamp: <Last changed 2025-09-28 02:38:17 by magnolia>
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
 * @file tec_server_worker.hpp
 * @brief Defines a worker class that manages an extended server instance.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <any>
#include <typeinfo>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_message.hpp"
#include "tec/tec_signal.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_server_worker.hpp"


namespace tec {


template <typename TParams, typename TServer>
class ServerWorkerEx: public ServerWorker<TParams, TServer> {
public:
    using Params = TParams;

protected:
    struct Payload {
        Signal* ready;
        Status* status;
        Request request;
        Reply   reply;
    };

public:
    ServerWorkerEx(const Params& params, std::unique_ptr<TServer> server)
        : ServerWorker<Params, TServer>(params, std::move(server))
    {
        this-> template register_callback<ServerWorkerEx<Params, TServer>, Payload*>(
            this, &ServerWorkerEx<TParams, TServer>::on_request);
    }

    template <typename TRequest, typename TReply>
    Status request(const TRequest* req, TReply* rep) {
        Signal ready;
        Status status;
        Payload  payload{&ready, &status, {req}, {rep}};
        if( !this->send({&payload}) ) {
            status = {"Error sending request", Error::Kind::RuntimeErr};
        }
        ready.wait();
        return status;
    }

protected:
    virtual void on_request(const Message& msg) {
        TEC_ENTER("WorkerServerEx::on_request");
        if( msg.type() == typeid(Payload*) ) {
            TEC_TRACE("Payload received: {}", msg.type().name());
            Payload* payload = std::any_cast<Payload*>(msg);
            *(payload->status) = this->server_->process_request(payload->request, payload->reply);
            payload->ready->set();
        }
    }

}; // class ServerWorkerEx

} // namespace tec
