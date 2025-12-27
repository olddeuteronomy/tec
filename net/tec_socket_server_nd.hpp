// Time-stamp: <Last changed 2025-12-26 14:46:18 by magnolia>
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
 * @file tec_net_data.hpp
 * @brief NetData server.
 * @author The Emacs Cat
 * @date 2025-12-11
 */

#pragma once

#include <cerrno>
#include <functional>
#include <sys/socket.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/tec_serialize.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket.hpp"
#include "tec/net/tec_socket_nd.hpp"
#include "tec/net/tec_socket_server.hpp"
#include "tec/net/tec_nd_compress.hpp"


namespace tec {


template <typename TParams>
class SocketServerNd: public SocketServer<TParams> {
public:
    using Params = TParams;
    using Lock = std::lock_guard<std::mutex>;
    using ID = NetData::ID;
    using ServerNd = SocketServerNd<Params>;

    struct DataInOut {
        Status* status;
        SocketNd* sock;
        NetData* nd_in;
        NetData* nd_out;
    };

    using HandlerFunc = std::function<void (ServerNd*, DataInOut)>;

private:

    struct Slot{
        ServerNd* server;
        HandlerFunc handler;

        explicit Slot(ServerNd* _server, HandlerFunc _handler)
            : server{_server}
            , handler{_handler}
        {}
        ~Slot() = default;
    };

    std::unordered_map<ID, std::unique_ptr<Slot>> slots_;
    std::mutex mtx_slots_;

public:

    explicit SocketServerNd(const Params& params)
        : SocketServer<Params>(params)
    {
        // Register echo request handler (ID=0).
        this->template register_handler<SocketServerNd>(
            this, 0, &SocketServerNd::echo);
    }

    virtual ~SocketServerNd() = default;


    template <typename Derived>
    void register_handler(Derived* server, ID id, void (Derived::*handler)(DataInOut dio)) {
        Lock lk{mtx_slots_};
        TEC_ENTER("SocketServerNd::register_handler");
        // Ensure Derived is actually derived from ServerNd.
        static_assert(std::is_base_of_v<ServerNd, Derived>,
                      "Derived must inherit from tec::SocketServerNd");
        // Remove existing handler.
        if (auto slot = slots_.find(id); slot != slots_.end()) {
            slots_.erase(id);
        }
        // Set the slot.
        slots_[id] = std::make_unique<Slot>(
            server,
            [handler](ServerNd* server, DataInOut dio) {
                // Safely downcast to Derived.
                auto derived = dynamic_cast<Derived*>(server);
                (derived->*handler)(dio);
            });
        TEC_TRACE("NetData handler ID={} registered.", id);
    }

protected:

    virtual Status dispatch(ID id, DataInOut dio) {
        TEC_ENTER("SocketServerNd::dispatch");
        Status status;
        bool found{false};
        if (auto slot = slots_.find(id); slot != slots_.end()) {
            found = true;
            slot->second->handler(slot->second->server, dio);
        }
        if (!found) {
            auto errmsg = format("Handler for ID={} not found.", id);
            status = {ENOTSUP, errmsg, Error::Kind::NotImplemented};
            TEC_TRACE("{}", status);
        }
        else {
            status = *dio.status;
            TEC_TRACE("Dispatched with {}.", status);
        }
        return status;
    }


    virtual void reply_error(Status status, NetData::ID request_id, SocketNd* sock) {
        TEC_ENTER("SocketServerNd::reply_error");
        NetData nd;
        nd.header()->id = request_id;
        nd.header()->status = static_cast<decltype(nd.header()->status)>(status.code.value_or(0xffff));
        SocketNd::send_nd(&nd, sock);
        TEC_TRACE("Replied with errcode={}.", nd.header()->status);
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                    Pre- and postprocessing
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    virtual Status compress(NetData* nd) {
        // if (this->params_.compression) {
            NdCompress cmpr(this->params_.compression,
                            this->params_.compression_level,
                            this->params_.compression_min_size);
            return cmpr.compress(*nd);
        // }
        // return {};
    }

    virtual Status uncompress(NetData* nd) {
        if (nd->header()->get_compression()) {
            NdCompress cmpr;
            return cmpr.uncompress(*nd);
        }
        return {};
    }

    virtual Status preprocess(NetData* nd) {
        TEC_ENTER("SocketServerNd::preprocess");
        return uncompress(nd);
    }

    virtual Status postprocess(NetData* nd) {
        TEC_ENTER("SocketServerNd::postprocess");
        return compress(nd);
    }


    void on_net_data(Socket* s) override {
        TEC_ENTER("SocketServerNd::on_net_data");
        SocketNd sock(s->fd, s->addr, s->port);
        NetData nd_in;
        //
        // Read input NetData.
        //
        Status status = SocketNd::recv_nd(&nd_in, &sock);
        if (status) {
            //
            // Preprocess input inplace.
            //
            status = preprocess(&nd_in);
            //
            // Dispatch NetData.
            //
            if (status) {
                NetData nd_out;
                DataInOut dio{&status, &sock, &nd_in, &nd_out};
                status = dispatch(nd_in.header()->id, dio);
                if (status) {
                    //
                    // Postprocess output inplace.
                    //
                    status = postprocess(&nd_out);
                    if (status) {
                        //
                        // Send output to a client.
                        //
                        status = SocketNd::send_nd(&nd_out, &sock);
                    }
                }
            }
        }
        else if (status.code == EBADMSG) {
            //
            // Not a NetData header -- switch to raw char stream processing.
            //
            SocketServer<Params>::on_string(s);
            return;
        }
        //
        // Send an error to the client if not OK.
        //
        if (!status) {
            reply_error(status, nd_in.header()->id, &sock);
        }
    }


    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                       Default handler
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    virtual void echo(DataInOut dio) {
        TEC_ENTER("SocketServerNd::echo");
        //
        // Copy input to output.
        //
        dio.nd_out->copy_from(*dio.nd_in);
        *dio.status = {};
    }

};

} // namespace tec
