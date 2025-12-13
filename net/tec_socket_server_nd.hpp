// Time-stamp: <Last changed 2025-12-13 16:50:51 by magnolia>
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

#include <sys/types.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_serialize.hpp"
#include "tec/net/tec_socket.hpp"
#include "tec/net/tec_net_data.hpp"
#include "tec/net/tec_socket_nd.hpp"
#include "tec/net/tec_socket_server.hpp"
#include "tec/tec_trace.hpp"


namespace tec {

template <typename TParams>
class SocketServerNd: public SocketServer<TParams> {
public:
    using Params = TParams;

public:
    SocketServerNd(const Params& params)
        : SocketServer<Params>(params)
    {
        this->params_.mode = SocketServerParams::kModeNetData;
    }

    virtual ~SocketServerNd() = default;


    void on_net_data(Socket* s) override {
        TEC_ENTER("SocketServerNd::on_net_data");
        SocketNd sock(s->fd, s->addr, s->port);

        // Echo server.
        NetData* data_in;
        auto status = SocketNd::recv_nd(data_in, &sock);
        if (!status) {
            // Send error header.
            NetData::Header hdr;
            hdr.status = static_cast<decltype(hdr.status)>(status.code.value_or(0));
            write(sock.fd, &hdr, sizeof(hdr));
            return;
        }
    }

};

}
