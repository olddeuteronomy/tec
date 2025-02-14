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
 *   @file tec_daemon.hpp
 *   @brief Declares an asbstract Daemin class.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_result.hpp"
#include "tec/tec_semaphore.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Abstract Daemon
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      An abstract Daemon class.
 *
 * @details A daemon, by definition, is a process or thread that
 * runs continuously as a background process and wakes up to handle
 * periodic service requests.
 *
 * tec::Daemon defines the minimum set of methods that should be implemented:
 * *run* and *terminate* as well as required signals.
 */
class Daemon {
public:
    Daemon() = default;
    virtual ~Daemon() = default;

    /**
     * @brief      Start the Daemon's thread.
     * @return     tec::Result
     * @sa tec::Result
     */
    virtual Result run() = 0;

    /**
     * @brief      Terminate the Daemon's thread.
     * @return     tec::Result
     * @sa tec::Result
     */
    virtual Result terminate() = 0;

    //! Signals when the Daemon is started.
    virtual const Signal& sig_running() const = 0;

    //! Signals when the Daemon is initialized (possible, with error).
    virtual const Signal& sig_inited() const = 0;

    //! Signals when the Daemon is terminated.
    virtual const Signal& sig_terminated() const = 0;

public:
    template <typename Derived>
    struct Builder {
        std::unique_ptr<Daemon> operator()(typename Derived::Params& params) {
            static_assert(std::is_base_of<Daemon, Derived>::value,
                "not derived from tec::Daemon class");
            return std::make_unique<Derived>(params);
        }
    };
};


} // ::tec
