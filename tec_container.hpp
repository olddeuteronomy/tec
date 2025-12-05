// Time-stamp: <Last changed 2025-12-05 15:46:49 by magnolia>
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

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {


template<typename T, typename = void>
struct is_container : std::false_type {};

template<typename T>
struct is_container<T,
    std::void_t<
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        decltype(std::declval<T>().size())
        >>
    : std::true_type {};

template<typename T>
inline constexpr bool is_container_v = is_container<T>::value;


// Matches std::unordered_map
template<typename T, typename = void>
struct is_map : std::false_type {};

template<typename T>
struct is_map<T,
    std::void_t<
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        decltype(std::declval<T>().size()),
        decltype(std::declval<T>().bucket_count())
        >>
    : std::true_type {};

template<typename T>
inline constexpr bool is_map_v = is_map<T>::value;

} // namespace tec
