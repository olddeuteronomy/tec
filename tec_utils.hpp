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
#include <iostream>
#include <linux/limits.h>
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


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       TResult of execution
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @class TResult
 * @include{cpp} ex1.cpp
 */
template <typename ECode=int, typename EDesc=std::string>
struct TResult {
    //! Generic error codes.
    struct ErrCode {
        constexpr static const ECode Unspecified{-1}; //!< Unspecified error code.
    };

    //! Error classes
    enum class Kind: int {
        Ok  //!< Success
        , Err        //!< Generic error
        , IOErr      //!< IO failure
        , RuntimeErr //!< Runtime error
        , NetErr     //!< Network error
        , GrpcErr    //!< gRPC error
        , TimeoutErr //!< Timeout error
        , Invalid    //!< Invalid data or state
        , System     //!< System error
    };

    //! Returns Result::Kind as string.
    virtual const char* kind_as_string(Kind k) const {
        switch (k) {
            case Kind::Ok: { static char s0[]{"Success"}; return s0; }
            case Kind::Err: { static char s1[]{"Generic"}; return s1; }
            case Kind::IOErr: { static char s2[]{"IO"}; return s2; }
            case Kind::RuntimeErr: { static char s3[]{"Runtime"}; return s3; }
            case Kind::NetErr: { static char s3[]{"Network"}; return s3; }
            case Kind::GrpcErr: { static char s4[]{"gRpc"}; return s4; }
            case Kind::TimeoutErr: { static char s5[]{"Timeout"}; return s5; }
            case Kind::Invalid: { static char s6[]{"Invalid"}; return s6; }
            case Kind::System: { static char s7[]{"System"}; return s7; }
            default: { static char s8[]{"Unspecified"}; return s8; }
        }
    }

    Kind kind;                 //!< Error class.
    std::optional<ECode> code; //!< Error code (optional).
    std::optional<EDesc> desc; //!< Error description (optional).

    //! Is everything OK?
    bool ok() const { return kind == Kind::Ok; }
    //! Operator: Is everything OK?
    operator bool() const { return ok(); }

    //! Output Result to a stream.
    friend std::ostream& operator << (std::ostream& out, const TResult& result) {
        out << "[" << result.kind_as_string(result.kind) << "]"
            << " Code=" << (result.ok() ? ECode{0} : result.code.value_or(ErrCode::Unspecified))
            << " Desc=\"" << result.desc.value_or("<unspecified>") << "\"";
        return out;
    }

    //! Return Result as string.
    std::string as_string() {
        std::ostringstream buf;
        buf << *this;
        return buf.str();
    }

    /**
     * @brief      Constructs a successful TResult (class is Kind::Ok).
     */
    TResult()
        : kind{Kind::Ok}
    {}

    /**
     * @brief      Constructs an error TResult with unspecified error code.
     *
     * @param      _kind Error class.
     */
    TResult(Kind _kind)
        : kind{_kind}
        , code{ErrCode::Unspecified}
    {}

    /**
     * @brief      Constructs an unspecified error TResult with description.
     *
     * @param      _desc Error description.
     * @param      _kind Error class (default Kind::Err, a generic error).
     */
    TResult(const EDesc& _desc, Kind _kind = Kind::Err)
        : kind{_kind}
        , code{ErrCode::Unspecified}
        , desc{_desc}
    {}

    /**
     * @brief      Constructs an error TResult w/o description.
     *
     * @param      _code Error code.
     * @param      _kind Error class (default Kind::Err, a generic error).
     */
    TResult(const ECode& _code, Kind _kind = Kind::Err)
        : kind{_kind}
        , code{_code}
    {}

    /**
     * @brief      Constructs an error TResult with description.
     *
     * @param      _code Error code.
     * @param      _kind Error class (default Kind::Err, a generic error).
     * @param      _desc Error description (default std::string).
     */
    TResult(const ECode& _code, const EDesc& _desc, Kind _kind = Kind::Err)
        : kind{_kind}
        , code{_code}
        , desc{_desc}
    {}

};

using Result = TResult<>;


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                  Simple print() with variadic arguments
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename T>
void print(std::ostream* out, const T& arg) {
    *out << arg;
}

template <typename T, typename... Targs>
void print(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
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
void println(std::ostream* out, const T& arg) {
    *out << arg << std::endl;
}

template <typename T, typename... Targs>
void println(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
    print<>(out, fmt, value, Args...);
    *out << std::endl;
}


//! Output to terminal.
template <typename T>
void print(const T& arg) {
    std::cout << arg;
}

template <typename T, typename... Targs>
void print(const char* fmt, const T& value, Targs&&... Args) {
    print<>(&std::cout, fmt, value, Args...);
}

template <typename T>
void println(const T& arg) {
    println<>(&std::cout, arg);
}
template <typename T, typename... Targs>
void println(const char* fmt, const T& value, Targs&&... Args) {
    println<>(&std::cout, fmt, value, Args...);
}


//! "Print" variadic arguments to a string.
template <typename T>
std::string format(const T& arg) {
    std::ostringstream buf;
    print<>(&buf, arg);
    return buf.str();
}

// Recursive call.
template <typename T, typename... Targs>
std::string format(const char* fmt, const T& value, Targs&&... Args) {
    std::ostringstream buf;
    print<>(&buf, fmt, value, Args...);
    return buf.str();
}


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
    void start() { src_ = Now<Duration>(); }

    //! Return duration between start() and stop().
    Duration stop() { return Now<Duration>() - src_;  }
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Signal
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! A classic implementation of Signal based on mutex/cv.
//! Borrowed from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool.
class Signal {
    using Lock = std::lock_guard<std::mutex>;
    using ULock = std::unique_lock<std::mutex>;

    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    mutable bool flag_;

public:
    //! Constructs
    Signal()
        : flag_(false)
    {}

    bool is_set() const {
        Lock lock(m_);
        return flag_;
    }

    //! Set signalled state and notify all threads that are waiting for.
    void set() {
        {
            Lock lock(m_);
            flag_ = true;
        }
        cv_.notify_all();
    }

    //! Reset to unsignalled state and notify all threads.
    void reset() {
        {
            Lock lock(m_);
            flag_ = false;
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
     * @sa Milliseconds
     */
    template <typename Duration>
    bool wait_for(Duration dur) const {
        ULock ulock(m_);
        return cv_.wait_for(ulock, dur, [this] { return flag_; });
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
// MS Windows stuff goes here
#include "tec/mswin/tec_win_utils.hpp"
#endif // __TEC_WINDOWS
