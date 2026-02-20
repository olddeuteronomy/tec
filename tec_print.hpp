// Time-stamp: <Last changed 2026-02-20 16:00:43 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/
/**
 * @file tec_print.hpp
 * @brief Provides variadic print and format utilities for the tec namespace.
 * Implemented for compatibility with pre-C++20 compilers.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/// @name Print and Format Functions
/// @brief Utility functions for formatted output and string creation.
/// @details These functions provide flexible printing to output streams and string
/// formatting with support for variadic arguments and placeholder-based formatting
/// using "{}" syntax.
/// @{

/**
 * @brief Outputs a single argument to the specified stream.
 * @details Writes the provided argument to the given output stream without any formatting.
 * @tparam T The type of the argument to output.
 * @param out Pointer to the output stream (e.g., std::cout or std::ostringstream).
 * @param arg The argument to output.
 */
template <typename T>
void print(std::ostream* out, const T& arg) {
    *out << arg;
}

/**
 * @brief Outputs variadic arguments to the specified stream with formatting.
 * @details Processes a format string with "{}" placeholders, replacing each placeholder
 * with the corresponding argument. Recursively handles multiple arguments.
 * @tparam T The type of the first argument.
 * @tparam Targs The types of additional arguments.
 * @param out Pointer to the output stream (e.g., std::cout or std::ostringstream).
 * @param fmt The format string containing "{}" placeholders.
 * @param value The first argument to output.
 * @param Args The remaining arguments to output.
 */
template <typename T, typename... Targs>
void print(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
    for (; *fmt != '\0'; fmt++) {
        if (*fmt == '{' && *(fmt + 1) == '}') {
            *out << value;
            if (*(fmt + 1) != '\0') ++fmt;
            print<>(out, fmt + 1, Args...); // recursive call
            return;
        }
        *out << *fmt;
    }
}

/**
 * @brief Outputs a single argument to the specified stream with a newline.
 * @details Writes the provided argument to the given output stream and appends a newline.
 * @tparam T The type of the argument to output.
 * @param out Pointer to the output stream (e.g., std::cout or std::ostringstream).
 * @param arg The argument to output.
 */
template <typename T>
void println(std::ostream* out, const T& arg) {
    *out << arg << std::endl;
}

/**
 * @brief Outputs variadic arguments to the specified stream with formatting and a newline.
 * @details Processes a format string with "{}" placeholders, replacing each placeholder
 * with the corresponding argument, and appends a newline at the end.
 * @tparam T The type of the first argument.
 * @tparam Targs The types of additional arguments.
 * @param out Pointer to the output stream (e.g., std::cout or std::ostringstream).
 * @param fmt The format string containing "{}" placeholders.
 * @param value The first argument to output.
 * @param Args The remaining arguments to output.
 */
template <typename T, typename... Targs>
void println(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
    print<>(out, fmt, value, Args...);
    *out << std::endl;
}

/**
 * @brief Outputs a single argument to std::cout.
 * @details Writes the provided argument to the standard output stream without formatting.
 * @tparam T The type of the argument to output.
 * @param arg The argument to output.
 */
template <typename T>
void print(const T& arg) {
    print<>(&std::cout, arg);
}

/**
 * @brief Outputs variadic arguments to std::cout with formatting.
 * @details Processes a format string with "{}" placeholders, replacing each placeholder
 * with the corresponding argument, outputting to the standard output stream.
 * @tparam T The type of the first argument.
 * @tparam Targs The types of additional arguments.
 * @param fmt The format string containing "{}" placeholders.
 * @param value The first argument to output.
 * @param Args The remaining arguments to output.
 */
template <typename T, typename... Targs>
void print(const char* fmt, const T& value, Targs&&... Args) {
    print<>(&std::cout, fmt, value, Args...);
}

/**
 * @brief Outputs a single argument to std::cout with a newline.
 * @details Writes the provided argument to the standard output stream and appends a newline.
 * @tparam T The type of the argument to output.
 * @param arg The argument to output.
 */
template <typename T>
void println(const T& arg) {
    println<>(&std::cout, arg);
}

/**
 * @brief Outputs variadic arguments to std::cout with formatting and a newline.
 * @details Processes a format string with "{}" placeholders, replacing each placeholder
 * with the corresponding argument, outputting to the standard output stream, and appending a newline.
 * @tparam T The type of the first argument.
 * @tparam Targs The types of additional arguments.
 * @param fmt The format string containing "{}" placeholders.
 * @param value The first argument to output.
 * @param Args The remaining arguments to output.
 */
template <typename T, typename... Targs>
void println(const char* fmt, const T& value, Targs&&... Args) {
    println<>(&std::cout, fmt, value, Args...);
}

/**
 * @brief Formats a single argument into a string.
 * @details Converts the provided argument to its string representation using an output stream.
 * @tparam T The type of the argument to format.
 * @param arg The argument to format.
 * @return std::string The string representation of the argument.
 */
template <typename T>
std::string format(const T& arg) {
    std::ostringstream buf;
    print<>(&buf, arg);
    return buf.str();
}

/**
 * @brief Formats variadic arguments into a string.
 * @details Processes a format string with "{}" placeholders, replacing each placeholder
 * with the corresponding argument, and returns the resulting string.
 * @tparam T The type of the first argument.
 * @tparam Targs The types of additional arguments.
 * @param fmt The format string containing "{}" placeholders.
 * @param value The first argument to format.
 * @param Args The remaining arguments to format.
 * @return std::string The formatted string.
 */
template <typename T, typename... Targs>
std::string format(const char* fmt, const T& value, Targs&&... Args) {
    std::ostringstream buf;
    print<>(&buf, fmt, value, Args...);
    return buf.str();
}

/// @}

} // namespace tec
