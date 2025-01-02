/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2024 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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

#include <chrono>
#include <cstdio>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <mutex>
#include <condition_variable>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

//! Default is monotonic clock that is most suitable for measuring intervals.
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
Duration Now() { return std::chrono::duration_cast<Duration>(Clock::now() - std::chrono::time_point<Clock, Duration>()); }

//! Returns Duration since `start'.
template <typename Duration>
Duration Since(Duration start) { return Now<Duration>() - start; }

//! Duration unit as string.
constexpr const char* time_unit(Seconds) { return "s"; }
constexpr const char* time_unit(MilliSec) { return "ms"; }
constexpr const char* time_unit(MicroSec) { return "mu"; }

//! Some duration constants.
constexpr const Seconds one_hour() { return Seconds(60 * 60); }
constexpr const Seconds one_day() { return Seconds(24 * 60 * 60); }

//! String.
#ifdef UNICODE
typedef std::wstring String;
#else
typedef std::string  String;
#endif


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Result of execution
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
struct Result {
    enum Result {
        OK = 0
    };

    int code_;
    std::string errstr_;

x    Result(): code_(OK) {}
    Result(int c): code_(c) {}
    Result(int c, const std::string& e): code_(c), errstr_(e) {}

    bool ok() const { return (code_ == OK); }
    operator bool() const { return ok(); }
    const std::string& str() const { return errstr_; }
    int code() const { return code_; }
};
*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Execution Result
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename ECode=int, typename EDesc=std::string>
struct Result {
    enum class Kind: int {
        Ok
        , Err  //!< Generic error.
        , IOErr //!< IO failure,
        , RuntimeErr //!< Runtime error.
        , NetErr //!< Network error.
        , GrpcErr //!< gRPC error.
        , TimeoutErr //!< Timeout error.
        , DataErr //!< Invalid data or state.
    };

    Kind kind;
    std::optional<ECode> code; //!< Error code.
    std::optional<EDesc> desc; //!< Error description.

    bool ok() const { return kind == Kind::Ok; }
    operator bool() const { return ok(); }

    Result()
        : kind{Kind::Ok}
    {}

    Result(const ECode& _code, Kind _kind = Kind::Err)
        : kind{_kind}
        , code{_code}
    {}

    Result(const ECode& _code, const EDesc& _desc, Kind _kind = Kind::Err)
        : kind{_kind}
        , code{_code}
        , desc{_desc}
    {}
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                  Simple print() with variadic arguments
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename T>
void print(std::ostream* out, T arg) {
    *out << arg;
}

template <typename T, typename... Targs>
void print(std::ostream* out, const char* fmt, T value, Targs&&... Args) {
    for( ; *fmt != '\0'; fmt++ ) {
        if( *fmt == '%' ) {
            *out << value;
            print<>(out, fmt + 1, Args...); // recursive call
            return;
        }
        *out << *fmt;
    }
}

template <typename T>
void println(std::ostream* out, T arg) {
    *out << arg << std::endl;
}

template <typename T, typename... Targs>
void println(std::ostream* out, const char* fmt, T value, Targs&&... Args) {
    print<>(out, fmt, value, Args...);
    *out << std::endl;
}


//! Output to terminal.
template <typename T>
void print(T& arg) {
    std::cout << arg;
}

template <typename T, typename... Targs>
void print(const char* fmt, T value, Targs&&... Args) {
    print<>(&std::cout, fmt, value, Args...);
}

template <typename T>
void println(T& arg) {
    println<>(&std::cout, arg);
}
template <typename T, typename... Targs>
void println(const char* fmt, T value, Targs&&... Args) {
    println<>(&std::cout, fmt, value, Args...);
}


//! "Print" variadic arguments to a string.
template <typename T>
std::string format(T&& arg) {
    std::ostringstream buf;
    print<>(&buf, arg);
    return buf.str();
}

// Recursive call.
template <typename T, typename... Targs>
std::string format(const char* fmt, T value, Targs&&... Args) {
    std::ostringstream buf;
    print<>(&buf, fmt, value, Args...);
    return buf.str();
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Simple Timer
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Simple timer.
template <typename Duration>
class Timer {
    Duration& src_;

public:
    Timer(Duration& src)
        : src_(src)
    {
        src_ = Now<Duration>();
    }

    Duration stop() { src_ = Now<Duration>() - src_; return src_; }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Signal
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! A classic implementation of Signal based on mutex/cv.
//! Borrowed from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool.
class Signal {
    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    bool flag_;

public:
    Signal()
        : flag_(false)
    {}

    bool is_set() const {
        std::lock_guard<std::mutex> lock(m_);
        return flag_;
    }

    void set() {
        {
            std::lock_guard<std::mutex> lock(m_);
            flag_ = true;
        }
        cv_.notify_all();
    }

    void reset() {
        {
            std::lock_guard<std::mutex> lock(m_);
            flag_ = false;
        }
        cv_.notify_all();
    }

    void wait() const {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [this] { return flag_; });
    }

    template <typename Duration>
    bool wait_for(Duration dur) const {
        std::unique_lock<std::mutex> lock(m_);
        return cv_.wait_for(lock, dur, [this] { return flag_; });
    }

}; // Signal


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* System utilities (Windows versions are in tec/mswin/tec_win_utils.hpp)
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if !defined(__TEC_WINDOWS__)

#include <unistd.h>
#include <pwd.h>


//! Return a computer name or empty string on failure.
inline String getcomputername() {
    // TODO: UNICODE support.
    char host[1024];
    if( gethostname(host, sizeof(host)) == 0)
        return host;
    else
        return "";
}


//! Return a logged user name or empty string on failure.
inline String getusername() {
    // TODO: UNICODE support.
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
// MS Windows stuff goes here
#include "tec/mswin/tec_win_utils.hpp"
#endif // __TEC_WINDOWS
