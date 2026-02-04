// Time-stamp: <Last changed 2026-02-05 02:43:24 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/

/**
 * @file tec_socket_client_nd.hpp
 * @brief Definition of the SocketClientNd class template.
 *
 * This file contains the implementation of the SocketClientNd class,
 * which is a specialized extension of the SocketClient class for
 * handling NetData streams. It adds support for sending and receiving
 * NetData objects, including preprocessing (e.g., compression) and
 * postprocessing (e.g., uncompression). The class uses the same
 * parameter template as SocketClient to maintain configuration
 * consistency.
 *
 * SocketClientNd overrides key methods to process NetData requests
 * and provides virtual hooks for customization of send/receive
 * operations and data processing. It integrates compression based on
 * parameters and is designed for efficient binary data transmission
 * over sockets.
 *
 * @note This class builds upon SocketClient and is part of the TEC
 *       library namespace for network operations involving structured
 *       data (NetData).
 *
 * @author The Emacs Cat
 * @date 2025-12-07
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
#include "tec/net/tec_nd_compress.hpp"


namespace tec {

/**
 * @class SocketClientNd
 * @brief Templated client socket actor for NetData stream handling.
 *
 * The SocketClientNd class extends SocketClient to support
 * NetData-based requests and replies. It handles binary data
 * transmission with optional compression, providing overrides for
 * request processing and virtual methods for send/receive and data
 * processing customization.
 *
 * Key features:
 * - Processes NetData::StreamIn and StreamOut for structured data exchange.
 * - Supports compression/uncompression based on parameters.
 * - Delegates to base class for non-NetData requests.
 * - Integrates with SocketNd for low-level NetData socket operations.
 *
 * @tparam TParams The parameter type, which must derive from SocketClientParams (inherited check).
 *
 * @see SocketClient
 * @see NetData
 * @see SocketNd
 * @see NdCompress
 */
template <typename TParams>
class SocketClientNd: public SocketClient<TParams> {
public:
    /// @brief Type alias for the template parameter TParams.
    /// This allows easy reference to the params type within the class.
    using Params = TParams;

    /**
     * @brief Constructs a SocketClientNd with the given parameters.
     *
     * Initializes the base SocketClient with the provided params.
     *
     * @param params The configuration parameters for the client.
     */
    explicit SocketClientNd(const Params& params)
        : SocketClient<Params>(params)
    {
    }

    /**
     * @brief Default destructor.
     *
     * No additional cleanup beyond the base class destructor.
     */
    virtual ~SocketClientNd() = default;

    /**
     * @brief Processes incoming requests, handling NetData types.
     *
     * This override checks for NetData::StreamIn and StreamOut types,
     * validates them, and delegates to send_recv_nd. Falls back to
     * base class processing for other request types.
     *
     * @param request The incoming request (std::any).
     * @param reply Optional reply object (std::any).
     * @return Status indicating success or error (e.g., Invalid if pointers are null).
     */
    Status process_request(Request request, Reply reply) override {
        TEC_ENTER("SocketClientNd::process_request");
        if( request.type() == typeid(const NetData::StreamIn*) &&
            reply.type() == typeid(NetData::StreamOut*)) {
            // NetData: Both request and reply are required.
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

        // Default processing.
        return SocketClient<Params>::process_request(request, reply);
    }

    /**
     * @brief Convenience method to send a NetData request and receive a response.
     *
     * Directly calls send_recv_nd to handle the input and output NetData objects.
     *
     * @param nd_in Pointer to the input NetData to send.
     * @param nd_out Pointer to the output NetData to receive.
     * @return Status indicating success or error.
     */
    Status request_nd(NetData* nd_in, NetData* nd_out) {
        TEC_ENTER("SocketClientNd::request");
        auto status =  send_recv_nd(nd_in, nd_out);
        return status;
    }

protected:

    /**
     * @brief Sends a NetData object over the socket.
     *
     * Creates a SocketNd helper and uses SocketNd::send_nd to transmit the data.
     *
     * @param nd Pointer to the NetData to send.
     * @return Status indicating success or error.
     */
    virtual Status send_nd(NetData* nd) {
        TEC_ENTER("SocketClientNd::send_nd");
        // Socket helper.
        SocketNd sock{this->sockfd_, this->params_.addr.c_str(), this->params_.port,
                      this->get_buffer(), this->get_buffer_size()};
        return SocketNd::send_nd(nd, &sock);
    }

    /**
     * @brief Receives a NetData object from the socket.
     *
     * Creates a SocketNd helper, uses SocketNd::recv_nd to receive the data, and rewinds the NetData.
     *
     * @param nd Pointer to the NetData to populate with received data.
     * @return Status indicating success or error.
     */
    virtual Status recv_nd(NetData* nd) {
        TEC_ENTER("SocketClientNd::recv_nd");
        // Socket helper.
        SocketNd sock{this->sockfd_, this->params_.addr.c_str(), this->params_.port,
                      this->get_buffer(), this->get_buffer_size()};
        auto status = SocketNd::recv_nd(nd, &sock);
        nd->rewind();
        return status;
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                    Pre- and postprocessing
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Compresses the NetData using configured parameters.
     *
     * Creates an NdCompress object with params and calls its compress method.
     *
     * @param nd Pointer to the NetData to compress.
     * @return Status indicating success or error.
     */
    virtual Status compress(NetData* nd) {
        NdCompress cmpr(this->params_.compression,
                        this->params_.compression_level,
                        this->params_.compression_min_size);
        return cmpr.compress(*nd);
    }

    /**
     * @brief Uncompresses the NetData using configured parameters.
     *
     * Creates an NdCompress object with params and calls its uncompress method.
     *
     * @param nd Pointer to the NetData to uncompress.
     * @return Status indicating success or error.
     */
    virtual Status uncompress(NetData* nd) {
        NdCompress cmpr(this->params_.compression,
                        this->params_.compression_level,
                        this->params_.compression_min_size);
        return cmpr.uncompress(*nd);
    }

    /**
     * @brief Preprocesses the NetData before sending (default: compress).
     *
     * Calls compress as the default preprocessing step.
     *
     * @param nd Pointer to the NetData to preprocess.
     * @return Status indicating success or error.
     */
    virtual Status preprocess(NetData* nd) {
        return compress(nd);
    }

    /**
     * @brief Postprocesses the NetData after receiving (default: uncompress).
     *
     * Calls uncompress as the default postprocessing step.
     *
     * @param nd Pointer to the NetData to postprocess.
     * @return Status indicating success or error.
     */
    virtual Status postprocess(NetData* nd) {
        return uncompress(nd);
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Data send/receive
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Sends a NetData request and receives a reply with pre/postprocessing.
     *
     * Applies preprocessing to input, sends, receives, applies
     * postprocessing to output. Terminates the connection if any step
     * fails.
     *
     * @param nd_in Pointer to the input NetData to send.
     * @param nd_out Pointer to the output NetData to receive.
     * @return Status indicating overall success or error.
     */
    virtual Status send_recv_nd(NetData* nd_in, NetData* nd_out) {
        TEC_ENTER("SocketClientNd::send_recv_nd");
        //
        // Preprocess the input.
        //
        auto status = preprocess(nd_in);
        if (status) {
            //
            // Send a request.
            //
            status = send_nd(nd_in);
            if (status) {
                //
                // Receive a reply.
                //
                status = recv_nd(nd_out);

            }
        }
        //
        // Postprocess the output.
        //
        if (status) {
            status = postprocess(nd_out);
        }
        else {
            this->terminate();
        }
        return status;
    }

}; // class SocketClientNd

} // namespace tec
