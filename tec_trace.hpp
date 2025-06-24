// Time-stamp: <Last changed 2025-06-25 02:50:23 by magnolia>
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
 *   @file tec_trace.hpp
 *   @brief A simple tracer utilities.
 *
 * Define `_TEC_TRACE_ON` to enable tracing.
 *
*/

#pragma once

#include <ostream>
#include <string>
#include <mutex>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_print.hpp"
#include "tec/tec_utils.hpp"


namespace tec {


namespace details {

// //! Global trace synchronizer.
struct trace_mutex {
    static std::mutex& mtx() {
        static std::mutex __mtx_trace;
        return __mtx_trace;
    }
};

} //::details


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       A simple Tracer
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      A simple Tracer.
 * @details    Define _TEC_TRACE macro to enable tracing.
 *
 * @snippet snp_tracer.cpp tr
 */
template <typename Duration=MilliSec>
class Tracer {
private:
    std::string name_;
    std::ostream* out_;

public:
    using Lock = std::lock_guard<std::mutex>;

    Tracer(const char* name, std::ostream* out)
        : name_{name}
        , out_{out}
    {}

    ~Tracer()
    {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>();
        *out_ << "[" << tp.count() << "] - " << name_ << " exited.\n";
    }

    void enter() {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>();
        *out_ << "[" << tp.count() << "] + " << name_ << " entered.\n";
    }


    template<typename T>
    void trace(const T& arg) {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>().count();
        *out_ << "[" << tp << "]     " << name_ << ": ";
        println<>(out_, arg);
    }


    template<typename T, typename... Targs>
    void trace(const char* fmt, const T& value, Targs&&... Args) {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>().count();
        *out_ << "[" << tp << "]     " << name_ << ": ";
        println<>(out_, fmt, value, Args...);
    }

}; // ::Tracer


} // ::tec


#if defined(_DEBUG) || defined(DEBUG)
  #define _TEC_DEBUG 1
#endif

#if defined(_TEC_TRACE_ON)
// Trace is enabled.

/**
@def TEC_ENTER(name)
Prints `name` (usually a function name) to the console.
*/
#if defined(__TEC_WINDOWS__)
  // Windows-specific version of TEC_ENTER.
  #define TEC_ENTER(name) Tracer<> tracer__(name, &std::cout); tracer__.enter()
#else
  #define TEC_ENTER(name) tec::Tracer<> tracer__(name, &std::cout); tracer__.enter()
#endif

/**
@def TEC_TRACE(format_string, args...)
Prints formatted log to the console.
*/
#define TEC_TRACE(...)  tracer__.trace(__VA_ARGS__)

#else
// No trace, please.
#define TEC_ENTER(name)
#define TEC_TRACE(...)
#endif
