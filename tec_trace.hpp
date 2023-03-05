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
 *   \file tec_trace.hpp
 *   \brief A simple tracer with (un)formatted output.
 *
 * Always defined in _DEBUG build.
 * Use _TEC_TRACE_RELEASE compiler flag to enable it in release build as well.
 *
*/

#pragma once

#include <chrono>
#include <string>
#include <iostream>
#include <mutex>

#include "tec_utils.hpp"


namespace tec {

//! Global synchronizer. Declare it somewhere in your code
//! using TEC_DECLARE_TRACER macro (see below),
//! usually just before your `main' procedure.
extern std::mutex tracer_mtx__;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                           Tracer
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template <typename Duration>
class Tracer
{
    using Lock = std::lock_guard<std::mutex>;

    std::string name_;

    template <typename> struct detail
    {
        static std::mutex& mtx() { return tracer_mtx__; }
    };

public:

    Tracer(const char* name):
        name_(name)
    {
    }


    template<typename Tstream>
    void tprint_enter(Tstream* out)
    {
        Lock lk(detail<Duration>::mtx());
        auto tp = Now<Duration>();
        *out << "[" << tp.count() << "] * " << name_ << " entered.\n";
    }


    template<typename Tstream>
    void tprint(Tstream* out, const char* format) // base function
    {
        Lock lk(detail<Duration>::mtx());
        auto tp = Now<Duration>().count();
        *out << "[" << tp << "] " << name_ << ": ";
        print(out, format);
    }


    template<typename Tstream, typename T, typename... Targs>
    void tprint(Tstream* out, const char* format, T value, Targs... Fargs) // recursive variadic function
    {
        Lock lk(detail<Duration>::mtx());
        auto tp = Now<Duration>().count();
        *out << "[" << tp << "] " << name_ << ": ";
        print(out, format, value, Fargs...);
    }

}; // ::Tracer class


} // ::tec


#if defined(_DEBUG) || defined(DEBUG) || defined(_TEC_TRACE_RELEASE)
  #define TEC_ENTER(name) tec::Tracer<tec::MilliSec> tracer__(name); tracer__.tprint_enter(&std::cout)
  #define TEC_TRACE(...)  tracer__.tprint(&std::cout, __VA_ARGS__)
#else // Release
  #define TEC_ENTER(name)
  #define TEC_TRACE(...)
#endif

#define TEC_DECLARE_TRACER namespace tec { std::mutex tracer_mtx__; }
