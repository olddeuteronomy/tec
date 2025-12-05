// Time-stamp: <Last changed 2025-12-06 00:36:59 by magnolia>
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
 * @file tec_container.hpp
 * @brief Generic container and map traits.
 * @author The Emacs Cat
 * @date 2025-11-29
 */

#pragma once

#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @brief Trait to detect whether a type behaves like a standard container.
 *
 * This trait is true for types that provide the following member functions:
 * - `begin()` and `end()` (iterable)
 * - `size()` (sized)
 *
 * It works with most STL containers (std::vector, std::list, std::deque, std::set,
 * std::map, std::unordered_* containers, etc.) and any user-defined type that
 * follows the same interface.
 *
 * @tparam T        Type to check
 * @tparam <unnamed> SFINAE parameter used for specialization
 */
template<typename T, typename = void>
struct is_container : std::false_type {};

/**
 * @brief Specialization that activates when T has begin(), end(), and size().
 */
template<typename T>
struct is_container<T,
    std::void_t<
        decltype(std::declval<T>().begin()), ///< Must have begin()
        decltype(std::declval<T>().end()),   ///< Must have end()
        decltype(std::declval<T>().size())   ///< Must have size()
        >>
    : std::true_type {};

/**
 * @brief Convenience variable template for checking container-ness.
 *
 * Usage:
 * @code
 * static_assert(tec::is_container_v<std::vector<int>>);
 * static_assert(!tec::is_container_v<int>);
 * @endcode
 *
 * @tparam T Type to test
 */
template<typename T>
inline constexpr bool is_container_v = is_container<T>::value;


/**
 * @brief Trait to detect associative containers that behave like std::unordered_map / std::unordered_set.
 *
 * In addition to the usual container interface (begin/end/size), this trait requires
 * the presence of `bucket_count()`, which is specific to unordered associative containers
 * in the standard library.
 *
 * It will be true for:
 * - std::unordered_map
 * - std::unordered_multimap
 * - std::unordered_set
 * - std::unordered_multiset
 *
 * It will be false for ordered maps (std::map) and for non-unordered containers.
 *
 * @tparam T        Type to check
 * @tparam <unnamed> SFINAE parameter used for specialization
 */
template<typename T, typename = void>
struct is_map : std::false_type {};

/**
 * @brief Specialization that activates when T has the unordered-map-like interface.
 */
template<typename T>
struct is_map<T,
    std::void_t<
        decltype(std::declval<T>().begin()),       ///< Iterable
        decltype(std::declval<T>().end()),         ///< Iterable
        decltype(std::declval<T>().size()),        ///< Sized
        decltype(std::declval<T>().bucket_count()) ///< Unordered container specific
        >>
    : std::true_type {};

/**
 * @brief Convenience variable template for checking unordered-map-like types.
 *
 * Usage:
 * @code
 * static_assert(tec::is_map_v<std::unordered_map<std::string, int>>);
 * static_assert(!tec::is_map_v<std::map<std::string, int>>);
 * static_assert(!tec::is_map_v<std::vector<int>>);
 * @endcode
 *
 * @tparam T Type to test
 */
template<typename T>
inline constexpr bool is_map_v = is_map<T>::value;

} // namespace tec
