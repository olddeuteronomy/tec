// Time-stamp: <Last changed 2025-03-14 01:41:31 by magnolia>
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
 *   @file tec_print.hpp
 *   @brief Simple formatted print (before C++20).
*/

#pragma once

#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                  Simple print() with variadic arguments
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Outputs a single `arg` to the stream.
template <typename T>
void print(std::ostream* out, const T& arg) {
    *out << arg;
}

/**
 * @brief      Prints variadic arguments to the stream.
 * @param      out *std::stream* An output stream.
 * @param      fmt *std::string* A format string.
 * @param      value *T* The first argument to output.
 * @param      Arg *Targs* Extra arguments.
 */
template <typename T, typename... Targs>
void print(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
    for( ; *fmt != '\0'; fmt++ ) {
        if( *fmt == '{' &&  *(fmt + 1) == '}' ) {
            *out << value;
            if( *(fmt + 1) != '\0' ) ++fmt;
            print<>(out, fmt + 1, Args...); // recursive call
            return;
        }
        *out << *fmt;
    }
}

//! Prints a single argument with newline to the stream.
template <typename T>
void println(std::ostream* out, const T& arg) {
    *out << arg << std::endl;
}

//! Prints variadic arguments with newline to the stream.
template <typename T, typename... Targs>
void println(std::ostream* out, const char* fmt, const T& value, Targs&&... Args) {
    print<>(out, fmt, value, Args...);
    *out << std::endl;
}

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

//! Stores representation of the single argument in a new string.
template <typename T>
std::string format(const T& arg) {
    std::ostringstream buf;
    print<>(&buf, arg);
    return buf.str();
}

//! Stores representation of the arguments in a new string.
template <typename T, typename... Targs>
std::string format(const char* fmt, const T& value, Targs&&... Args) {
    std::ostringstream buf;
    print<>(&buf, fmt, value, Args...);
    return buf.str();
}


} // ::tec
