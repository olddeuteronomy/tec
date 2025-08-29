// Time-stamp: <Last changed 2025-08-24 01:34:01 by magnolia>
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
 *   @file  tec_daemon.hpp
 *   @brief Declares a Daemon interface.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_signal.hpp"
#include "tec/tec_message.hpp"


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Daemon Interface
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      A Daemon interface.
 *
 * @details A daemon, by definition, is a process or thread that
 * runs continuously as a background process and wakes up to handle
 * periodic service requests.
 *
 * The Daemon interface defines the minimum set of methods that should
 * be implemented, such as `run()`, `terminate()`, as well as required
 * signals.
 */
class Daemon {
public:
    Daemon() = default;
    Daemon(const Daemon&) = delete;
    Daemon(Daemon&&) = delete;
    virtual ~Daemon() = default;

    /**
     * @brief      Start the Daemon.
     * @return     Status
     */
    virtual Status run() = 0;

    /**
     * @brief      Terminate the Daemon.
     * @return     Status
     */
    virtual Status terminate() = 0;

    //! Send a control message.
    virtual bool send(const Message&) = 0;

    //! Signals the Daemon gets started.
    virtual const Signal& sig_running() const = 0;

    //! Signals the Daemon gets initialized (possible, with error).
    virtual const Signal& sig_inited() const = 0;

    //! Signals the Daemon gets terminated.
    virtual const Signal& sig_terminated() const = 0;

public:
    /**
     * @brief      Creates the Daemon using a derived class.
     * @details Used to hide the detailed implementation of
     * a Daemon-derived class.
     *
     * @param      params Derived::Params
     * @return     std::unique_ptr<Daemon>
     */
    template <typename Derived>
    struct Builder {
        std::unique_ptr<Daemon> operator()(typename Derived::Params const& params) {
            static_assert(std::is_base_of<Daemon, Derived>::value,
                "not derived from tec::Daemon class");
            return std::make_unique<Derived>(params);
        }
    };
};


} // ::tec
