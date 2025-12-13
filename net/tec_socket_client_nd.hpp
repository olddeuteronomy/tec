// Time-stamp: <Last changed 2025-12-14 00:31:43 by magnolia>
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
 * @file tec_socket_client_nd.hpp
 * @brief BSD socket client implementing NetData requests.
 * @note For BSD, macOS, Linux. Windows version is not implemented yet.
 * @author The Emacs Cat
 * @date 2025-11-10
 */

#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L   // This line fixes the "storage size of ‘hints’ isn’t known" issue.
#endif

#include <any>
#include <cerrno>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_status.hpp"
#include "tec/tec_message.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket_nd.hpp"
#include "tec/net/tec_socket_client.hpp"


namespace tec {

template <typename TParams>
class SocketClientNd: public SocketClient<TParams> {
public:
    using Params = TParams;

    explicit SocketClientNd(const Params& params)
        : SocketClient<Params>(params)
    {
    }

    virtual ~SocketClientNd() = default;


    Status process_request(Request request, Reply reply) override {
        TEC_ENTER("SocketClientNd::process_request");
        if( request.type() == typeid(const NetData::StreamIn*) &&
            reply.type() == typeid(NetData::StreamOut*)) {
            // Both request and reply are required.
            if (!request.has_value() || !reply.has_value()) {
                return {EINVAL, Error::Kind::Invalid};
            }
            const NetData::StreamIn* req = std::any_cast<const NetData::StreamIn*>(request);
            NetData::StreamOut* rep = std::any_cast<NetData::StreamOut*>(reply);
            if (req->nd == nullptr || rep->nd == nullptr) {
                return {EFAULT, Error::Kind::Invalid};
            }
            return send_recv_nd(req->nd, rep->nd);
        }

        return SocketClient<Params>::process_request(request, reply);
    }


    Status request_nd(NetData* nd_in, NetData* nd_out) {
        TEC_ENTER("SocketClientNd::request");
        return send_recv_nd(nd_in, nd_out);
    }

protected:

    virtual Status send_nd(NetData* nd) {
        TEC_ENTER("SocketClientNd::send_nd");
        // Socket helper.
        SocketNd sock{this->sockfd_, this->params_.addr, this->params_.port};
        return SocketNd::send_nd(nd, &sock);
    }


    virtual Status recv_nd(NetData* nd) {
        TEC_ENTER("SocketClientNd::recv_nd");
        // Socket helper.
        SocketNd sock{this->sockfd_, this->params_.addr, this->params_.port};
        return SocketNd::recv_nd(nd, &sock);
    }


    virtual Status send_recv_nd(NetData* nd_in, NetData* nd_out) {
        TEC_ENTER("SocketClientNd::send_recv_nd");
        auto status = send_nd(nd_in);
        if (status) {
            status = recv_nd(nd_out);
        }
        else {
            this->terminate();
        }
        return status;
    }

};

}
