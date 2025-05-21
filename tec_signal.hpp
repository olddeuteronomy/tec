// Time-stamp: <Last changed 2025-05-21 15:04:39 by magnolia>
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
 *   @file tec_signal.hpp
 *   @brief A simple implementation of a signal based on mutex/cv.
 *
 *   Borrowed from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool.
*/

#pragma once

#include <mutex>
#include <condition_variable>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Signal
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! A simple implementation of signal based on mutex/cv.
//! Borrowed from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool.
class Signal {
    using Lock = std::lock_guard<std::mutex>;
    using ULock = std::unique_lock<std::mutex>;

    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    mutable bool flag_;

public:
    //! In the unsignalled state initially.
    Signal()
        : flag_(false)
    {}

    //! Set signalled state and notify all threads that are waiting for.
    void set() {
        {
            Lock lock(m_);
            flag_ = true;
        }
        cv_.notify_all();
    }

    //! Wait for the signal is set, unconditionally.
    void wait() const {
        ULock ulock(m_);
        cv_.wait(ulock, [this] { return flag_; });
    }

    /**
     * @brief      Wait for the signal is set for the specified amount of time.
     * @param      dur Duration Amount of time to wait for the signal is set.
     * @return     bool `false` if timeout otherwise `true`.
     * @sa wait()
     */
    template <typename Duration>
    bool wait_for(Duration dur) const {
        ULock ulock(m_);
        return cv_.wait_for(ulock, dur, [this] { return flag_; });
    }

}; // ::Signal


} // ::tec
