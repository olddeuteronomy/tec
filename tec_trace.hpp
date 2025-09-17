// Time-stamp: <Last changed 2025-09-17 14:54:33 by magnolia>
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
 * @file tec_tracer.hpp
 * @brief Provides a thread-safe tracing utility for debugging in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
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

/**
 * @struct trace_mutex
 * @brief Provides a global mutex for synchronizing trace output.
 * @details Manages a static mutex to ensure thread-safe tracing operations.
 */
struct trace_mutex {
    /**
     * @brief Retrieves the global trace mutex.
     * @details Returns a reference to a static mutex used for synchronizing trace output.
     * @return std::mutex& The global trace mutex.
     */
    static std::mutex& mtx() {
        static std::mutex __mtx_trace;
        return __mtx_trace;
    }
};

} // namespace details

/**
 * @class Tracer
 * @brief A thread-safe tracer for logging entry, exit, and custom messages.
 * @details Logs messages to an output stream with timestamps, typically for debugging.
 * Tracing is enabled only when the _TEC_TRACE_ON macro is defined. The tracer is
 * thread-safe, using a global mutex for synchronized output.
 * @tparam Duration The duration type for timestamps (defaults to MilliSec).
 * @snippet snp_tracer.cpp tr
 */
template <typename Duration = MilliSec>
class Tracer {
private:
    std::string name_; ///< The name of the tracer (e.g., function or context name).
    std::ostream* out_; ///< Pointer to the output stream for logging.

public:
    using Lock = std::lock_guard<std::mutex>; ///< Alias for lock guard used for thread safety.

    /**
     * @brief Constructs a tracer with a name and output stream.
     * @details Initializes the tracer with a name and an output stream for logging.
     * @param name The name of the tracer (e.g., function name).
     * @param out Pointer to the output stream (e.g., std::cout).
     */
    Tracer(const char* name, std::ostream* out)
        : name_{name}
        , out_{out}
    {}

    /**
     * @brief Destructor that logs the exit of the tracer.
     * @details Logs a message with the current timestamp and tracer name when the tracer is destroyed.
     * The output is thread-safe, using the global trace mutex.
     */
    ~Tracer() {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>();
        *out_ << "[" << tp.count() << "] - " << name_ << " exited.\n";
    }

    /**
     * @brief Logs an entry message for the tracer.
     * @details Outputs a message with the current timestamp and tracer name to indicate
     * the start of a context. The output is thread-safe.
     */
    void enter() {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>();
        *out_ << "[" << tp.count() << "] + " << name_ << " entered.\n";
    }

    /**
     * @brief Logs a single argument as a trace message.
     * @details Outputs a timestamped message with the tracer name and the provided argument.
     * The output is thread-safe.
     * @tparam T The type of the argument to log.
     * @param arg The argument to include in the trace message.
     */
    template <typename T>
    void trace(const T& arg) {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>().count();
        *out_ << "[" << tp << "]     " << name_ << ": ";
        println<>(out_, arg);
    }

    /**
     * @brief Logs variadic arguments as a formatted trace message.
     * @details Outputs a timestamped message with the tracer name and formatted arguments
     * using the provided format string with "{}" placeholders. The output is thread-safe.
     * @tparam T The type of the first argument.
     * @tparam Targs The types of additional arguments.
     * @param fmt The format string containing "{}" placeholders.
     * @param value The first argument to include in the trace message.
     * @param Args The remaining arguments to include.
     */
    template <typename T, typename... Targs>
    void trace(const char* fmt, const T& value, Targs&&... Args) {
        Lock lk(details::trace_mutex::mtx());
        auto tp = now<Duration>().count();
        *out_ << "[" << tp << "]     " << name_ << ": ";
        println<>(out_, fmt, value, Args...);
    }
}; // class Tracer

} // namespace tec

/**
 * @defgroup TracerMacros Tracer Macros
 * @brief Macros for enabling and controlling tracing functionality.
 * @details These macros provide a convenient interface for tracing when _TEC_TRACE_ON
 * is defined. They are disabled otherwise to avoid performance overhead.
 * @{
 */

#if defined(_DEBUG) || defined(DEBUG)
  #define _TEC_DEBUG 1
#endif

#if defined(_TEC_TRACE_ON)

/**
 * @def TEC_ENTER(name)
 * @brief Logs an entry message for a named context (e.g., function).
 * @details Creates a Tracer object and calls its enter() method to log a timestamped
 * entry message. On Windows, the namespace is omitted for compatibility.
 * @param name The name of the context to trace (e.g., function name).
 */
#if defined(__TEC_WINDOWS__)
  #define TEC_ENTER(name) Tracer<> tracer__(name, &std::cout); tracer__.enter()
#else
  #define TEC_ENTER(name) tec::Tracer<> tracer__(name, &std::cout); tracer__.enter()
#endif

/**
 * @def TEC_TRACE(format_string, args...)
 * @brief Logs a formatted trace message.
 * @details Calls the trace() method on a Tracer object to log a timestamped message
 * with the provided format string and arguments.
 * @param format_string The format string containing "{}" placeholders.
 * @param args The arguments to include in the trace message.
 */
#define TEC_TRACE(...) tracer__.trace(__VA_ARGS__)

#else
/**
 * @def TEC_ENTER(name)
 * @brief Disabled tracing macro for entry logging.
 * @details Does nothing when _TEC_TRACE_ON is not defined.
 */
#define TEC_ENTER(name)

/**
 * @def TEC_TRACE(...)
 * @brief Disabled tracing macro for formatted messages.
 * @details Does nothing when _TEC_TRACE_ON is not defined.
 */
#define TEC_TRACE(...)
#endif

/// @}
