// Time-stamp: <Last changed 2025-08-24 02:20:24 by magnolia>
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
 *   @file tec_server.hpp
 *   @brief Declares a Server interface.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_signal.hpp"


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Generic Server Parameters
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct ServerParams {
    ///{@ Default timeouts.
    static constexpr const MilliSec kStartTimeout{Seconds{5}};
    static constexpr const MilliSec kShutdownTimeout{Seconds{10}};
    //}@

    MilliSec start_timeout;    //!< Start timeout in milliseconds.
    MilliSec shutdown_timeout; //!< Shutdown timeout in milliseconds.

    ServerParams()
        : start_timeout{kStartTimeout}
        , shutdown_timeout{kShutdownTimeout}
    {}
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                        Server Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

class Server {
public:
    Server() = default;
    Server(const Server&) = delete;
    Server(Server&&) = delete;

    virtual ~Server() = default;

    /**
     * @brief      Start the server.
     *
     * @param      sig_started Signalled after server gets started, possible with error.
     * @param      status Error::Kind::Timeout, or any other error, or Ok.
     *
     * @note In gRPC, if started successfully, `start()` doesn't
     * return until `shutdown()` called from another thread. In this
     * case, as well as in the case of timeout, `start()` **shouldn't**
     * modify the `status` argument.
     *
     * @sa Signal
     * @sa Status
     */
    virtual void start(Signal& sig_started, Status& status) = 0;

    /**
     * @brief      Shutdown the server.
     *
     * @param      sig_stopped Signalled after the server gets stopped.
     *
     * @sa Signal
     */
    virtual void shutdown(Signal& sig_stopped) = 0;

}; // ::Server


} // ::tec
