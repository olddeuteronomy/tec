/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \file tec_utils.hpp
 *   \brief Various utility classes/functions.
 *
 *  TODO: Detailed description
 *
*/

#pragma once

#include <atomic>
#include <chrono>
#include <sstream>
#include <string>
#include <iostream>
#include <ostream>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "tec_def.hpp"


namespace tec
{

using Clock = std::chrono::steady_clock;

using Seconds = std::chrono::seconds;
using TimePointSec = std::chrono::time_point<Clock, Seconds>;

using MilliSec = std::chrono::milliseconds;
using TimePointMs = std::chrono::time_point<Clock, MilliSec>;

using MicroSec = std::chrono::microseconds;
using TimePointMu = std::chrono::time_point<Clock, MicroSec>;

//! Returns Now() as Duration.
template <typename Duration>
inline Duration Now() { return std::chrono::duration_cast<Duration>(Clock::now() - std::chrono::time_point<Clock, Duration>()); }

//! Returns Duration since `start'.
template <typename Duration>
inline Duration Since(Duration start) { return Now<Duration>() - start; }

//! Duration unit as string.
inline const char* time_unit(Seconds) {return "s"; }
inline const char* time_unit(MilliSec) {return "ms"; }
inline const char* time_unit(MicroSec) {return "mu"; }


#ifdef UNICODE
typedef std::wstring String;
#else
typedef std::string  String;
#endif


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                  Simple print() with variadic arguments
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename Tstream>
void print(Tstream* out, const char* fmt) // base function
{
    *out << fmt;
}


template <typename Tstream, typename T, typename... Targs>
void print(Tstream* out, const char* fmt, T value, Targs... Fargs) // recursive variadic function
{
    for( ; *fmt != '\0'; fmt++ )
    {
        if( *fmt == '%' )
        {
            *out << value;
            print<>(out, fmt + 1, Fargs...); // recursive call
            return;
        }
        *out << *fmt;
    }
}


//! Macro: print variadic arguments to std::cout.
#define tec_print(...) tec::print(&std::cout, __VA_ARGS__)


//! "Print" variadic arguments to string.
inline std::string format(const char* fmt)
{
    std::ostringstream buf;
    print(&buf, fmt);
    return buf.str();
}

// Recursive call.
template <typename T, typename... Targs>
inline std::string format(const char* fmt, T value, Targs... Fargs)
{
    std::ostringstream buf;
    print(&buf, fmt, value, Fargs...);
    return buf.str();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Simple Timer
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename Duration>
class Timer
{
    Duration start_;

public:
    Timer(): start_{Now<Duration>()}  {}
    Duration stop() { return Now<Duration>() - start_; }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Signal
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Atomic-based signal. Deprecated, use Signal.
class SignalAtomic
{
    std::atomic_bool sig_;
    bool reset_on_exit_;

public:
    SignalAtomic(bool signalled = false, bool reset_on_exit = false):
        sig_{signalled}
        , reset_on_exit_{reset_on_exit}
    {}

    SignalAtomic(const SignalAtomic&) = delete;
    SignalAtomic(SignalAtomic&&) = delete;

    ~SignalAtomic()
    {
        if( reset_on_exit_ ) { sig_.store(false); }
    }

    inline void set() noexcept { sig_.store(true); }
    inline void reset() noexcept { sig_.store(false); }
    inline operator bool() noexcept { return sig_.load(); }

    template<typename Duration>
    inline bool wait_for(Duration dur)
    {
        bool ok = true;
        auto start = Now<Duration>();
        while( !sig_.load() )
        {
            if( Since(start) < dur )
            {
                std::this_thread::yield();
            }
            else
            {
                ok = false;
                break;
            }
        }

        return ok;
    }

}; // SignalAtomic


//! Another implementation of Signal.
//! Borrowed from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool
class Signal
{
    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    bool flag_;

public:
    Signal()
        : flag_(false)
    {}

    bool is_set() const
    {
        std::lock_guard<std::mutex> lock(m_);
        return flag_;
    }

    void set()
    {
        {
            std::lock_guard<std::mutex> lock(m_);
            flag_ = true;
        }
        cv_.notify_all();
    }

    void reset()
    {
        {
            std::lock_guard<std::mutex> lock(m_);
            flag_ = false;
        }
        cv_.notify_all();
    }

    void wait() const
    {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [this] { return flag_; });
    }

    template <typename Duration>
    bool wait_for(const Duration& dur) const
    {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, dur, [this] { return flag_; });
    }

}; // Signal


} // ::tec
