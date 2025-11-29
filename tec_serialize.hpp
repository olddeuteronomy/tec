// Time-stamp: <Last changed 2025-11-30 00:51:20 by magnolia>
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

#pragma once

#include <type_traits>

namespace tec {


template<typename T, typename = void>
struct is_serializable : std::false_type {};

template<typename T>
struct is_serializable<T,
    std::void_t<decltype(std::declval<T>().serializable())>>
    : std::true_type {};

template<typename T>
inline constexpr bool is_serializable_v = is_serializable<T>::value;


template <typename TStream>
struct Serializable {
    constexpr const bool serializable() { return true; }

    Serializable() = default;
    virtual ~Serializable() = default;

    virtual TStream& store(TStream& s) const = 0;
    virtual TStream& load(TStream& s) = 0;
} ;

} // namespace tec
