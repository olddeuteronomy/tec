// Time-stamp: <Last changed 2025-04-01 13:45:30 by magnolia>
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
 *   @brief Defines an abstract Server.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_utils.hpp"
#include "tec/tec_semaphore.hpp"


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Server Parameters
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
*                    Abstract Server Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename TParams>
class Server {

public:
    typedef TParams Params;

protected:
    Params params_;

public:
    Server(const Params& params)
        : params_{params}
    {
    }

    Server(const Server&) = delete;
    Server(Server&&) = delete;

    virtual ~Server() = default;

    constexpr Params params() const { return params_; }

    virtual void start(Signal&, Status&) = 0;
    virtual void shutdown(Signal&) = 0;

}; // ::Server


} // ::tec
