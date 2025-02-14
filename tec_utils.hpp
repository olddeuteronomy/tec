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
 *   @file tec_utils.hpp
 *   @brief Various utility classes/functions.
*/

#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                     Time utilities
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


//! Monotonic clock is most suitable for measuring intervals.
using Clock = std::chrono::steady_clock;

//! Seconds.
using Seconds = std::chrono::seconds;
using TimePointSec = std::chrono::time_point<Clock, Seconds>;

//! Milliseconds.
using MilliSec = std::chrono::milliseconds;
using TimePointMs = std::chrono::time_point<Clock, MilliSec>;

//! Microseconds.
using MicroSec = std::chrono::microseconds;
using TimePointMu = std::chrono::time_point<Clock, MicroSec>;

//! Returns now() as Duration.
template <typename Duration>
Duration now() { return std::chrono::duration_cast<Duration>(Clock::now() - std::chrono::time_point<Clock, Duration>()); }

//! Returns Duration since `start'.
template <typename Duration>
Duration since(Duration start) { return now<Duration>() - start; }

//! Duration unit as string.
constexpr const char* time_unit(Seconds) { return "s"; }
constexpr const char* time_unit(MilliSec) { return "ms"; }
constexpr const char* time_unit(MicroSec) { return "mu"; }

//! Some duration constants.
constexpr const Seconds one_hour() { return Seconds(60 * 60); }
constexpr const Seconds one_day() { return Seconds(24 * 60 * 60); }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Simple Timer
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @class      Timer
 * @brief      Simple timer.
 */
template <typename Duration = MilliSec>
class Timer {
    Duration src_;

public:
    //! Start the timer.
    Timer() {
        start();
    }

    //! Start the timer.
    void start() { src_ = now<Duration>(); }

    //! Return duration between start() and stop().
    Duration stop() { return now<Duration>() - src_;  }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* System utilities (Windows versions are in tec/mswin/tec_win_utils.hpp)
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if !defined(__TEC_WINDOWS__)

#include <unistd.h>
#include <pwd.h>
#include <linux/limits.h>


//! Returns a computer name or empty string on failure.
//! Use UTF-8 for non-English encoding.
inline std::string getcomputername() {
    char host[PATH_MAX];
    if( gethostname(host, sizeof(host)) == 0)
        return host;
    else
        return "";
}


//! Returns a logged user name or empty string on failure.
//! Use UTF-8 for non-English encoding.
inline std::string getusername() {
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);
    if ( pw != NULL )
        return pw->pw_name;
    else
        return "";
}

#endif // !__TEC_WINDOWS__

} // ::tec


#if defined(__TEC_WINDOWS__)
// MS Windows stuff goes here. NOT YET!
#include "tec/mswin/tec_win_utils.hpp"
#endif // __TEC_WINDOWS
