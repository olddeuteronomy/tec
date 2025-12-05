// Time-stamp: <Last changed 2025-12-06 01:11:33 by magnolia>
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
 * @file tec_json.hpp
 * @brief Simple JSON serialization.
 * @author The Emacs Cat
 * @date 2025-12-05
 */

#pragma once

#include <string>
#include <sstream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_bytes.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"


namespace tec {

/**
 * @brief Utility class for generating minimal, valid JSON strings at compile-time and runtime.
 *
 * `Json` provides a lightweight, header-only way to convert basic C++ types,
 * containers, and `tec::Serializable` objects into JSON-formatted strings.
 *
 * It is intentionally minimal — no external dependencies, no parsing, only serialization.
 * The output is valid JSON but not pretty-printed (compact form).
 *
 * Supports:
 * - Fundamental types (`int`, `float`, `bool`, etc.)
 * - `std::string`
 * - `tec::Bytes` (as hex string)
 * - Any type satisfying `is_container_v<T>` → JSON array
 * - Any type satisfying `is_map_v<T>` → JSON object
 * - Any type derived from `tec::Serializable` → nested object via `to_json()`
 *
 * @note This is not a full JSON library — it's designed for debugging, logging,
 *       network messages, and configuration where performance and simplicity matter.
 */
struct Json {

    /// Separator used between array/object elements
    static constexpr const char* sep{", "};

    /// Separator used between object key and value
    static constexpr const char* infix{": "};

    /**
     * @brief Helper to optionally emit a JSON key name.
     *
     * If `name` is non-null, outputs `"name": ` (with proper quoting and spacing).
     * If `name` is null, does nothing.
     *
     * @param os   Output stream to write to
     * @param name Optional field name (null-terminated string)
     */
    inline static void print_name(std::ostringstream& os, const char* name) {
        if(name) {
            os << "\"" << name << "\"" << infix;
        }
    }

    /**
     * @brief Serialize a std::string as a JSON string literal.
     *
     * @param val  The string value
     * @param name Optional JSON key name
     * @return JSON fragment (with optional key)
     */
    static std::string json(const std::string& val, const char* name = nullptr) {
        std::ostringstream os;
        print_name(os, name);
        os <<  "\"" << val << "\"";
        return os.str();
    }

    /**
     * @brief Serialize a Bytes object as a hex-encoded JSON string.
     *
     * @param val  The byte container
     * @param name Optional JSON key name
     * @return JSON string like "deadbeef"
     */
    static std::string json(const Bytes& val, const char* name = nullptr) {
        std::ostringstream os;
        print_name(os, name);
        os <<  "\"" << val.as_hex() << "\"";
        return os.str();
    }

    /**
     * @brief Serialize a boolean as JSON `true` or `false`.
     *
     * @param val  The boolean value
     * @param name Optional JSON key name
     * @return JSON boolean literal
     */
    static std::string json(const bool& val, const char* name = nullptr) {
        std::ostringstream os;
        print_name(os, name);
        os << (val ? "true" : "false");
        return os.str();
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Containers
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Serialize any container (with .begin()/.end()) as a JSON array.
     *
     * Elements are serialized using their own `operator<<` (or specialized `json()` if available).
     *
     * @tparam TContainer Container type satisfying `is_container_v`
     * @param c    Container instance
     * @param name Optional array name (e.g., "myArray": [...])
     * @return JSON array string
     */
    template <typename TContainer>
    static std::string json_container(const TContainer& c, const char* name) {
        std::ostringstream os;
        print_name(os, name);
        bool first{true};
        os << "[";
        for( const auto& e: c ) {
            if(first) {
                os << e;
                first = false;
            }
            else {
                os << sep << e;
            }
        }
        os << "]";
        return os.str();
    }

    /**
     * @brief Serialize unordered map-like containers as JSON objects.
     *
     * Keys and values must support `operator<<`. Structured bindings are used for iteration.
     *
     * @tparam TMap Map type satisfying `is_map_v`
     * @param m    Map instance
     * @param name Optional object name
     * @return JSON object string
     */
    template <typename TMap>
    static std::string json_map(const TMap& m, const char* name) {
        std::ostringstream os;
        print_name(os, name);
        bool first{true};
        os << "{";
        for( const auto& [k, v]: m) {
            if(first) {
                os << k << infix << v;
                first = false;
            }
            else {
                os << sep << k << infix << v;

            }
        }
        os << "}";
        return os.str();
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                      Serializable object
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Serialize a tec::Serializable-derived object as a nested JSON object.
     *
     * Calls the object's `to_json()` method and wraps it in braces.
     *
     * @tparam TObject Type derived from tec::Serializable
     * @param obj  Object instance
     * @param name Optional field name
     * @return JSON like `"name": { ...to_json output... }`
     */
    template <typename TObject>
    static std::string json_object(const TObject& obj, const char* name) {
        std::ostringstream os;
        print_name(os, name);
        os << "{" << obj.to_json() << "}";
        return os.str();
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Generic fallback
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Generic JSON serializer with automatic type dispatching.
     *
     * Dispatches to the most appropriate specialization based on type traits:
     * 1. `tec::Serializable` → `json_object()`
     * 2. Unordered map-like → `json_map()`
     * 3. Container → `json_container()`
     * 4. Scalar types → direct `operator<<`
     *
     * @tparam T        Value type
     * @param val       Value to serialize
     * @param name      Optional JSON key name
     * @return JSON fragment
     */
    template <typename T>
    static std::string json(const T& val, const char* name) {
        std::ostringstream os;
        if constexpr (is_serializable_v<T>) {
            return json_object(val, name);
        }
        else if constexpr (is_map_v<T>) {
            return json_map(val, name);
        }
        else if constexpr (is_container_v<T>) {
            return json_container(val, name);
        }
        else {
            // Any scalar
            print_name(os, name);
            os << val;
        }
        return os.str();
    }

    /**
     * @brief Functor interface — serialize with explicit name.
     *
     * Allows usage like: `Json{}(myValue, "fieldName")`
     *
     * @tparam T   Value type
     * @param val  Value to serialize
     * @param name Field name in JSON
     * @return JSON string
     */
    template <typename T>
    std::string operator()(const T& val, const char* name) {
        return json(val, name);
    }

    /**
     * @brief Functor interface — serialize without name (root value).
     *
     * Allows usage like: `Json{}(myValue)`
     *
     * @tparam T   Value type
     * @param val  Value to serialize
     * @return JSON string (no key prefix)
     */
    template <typename T>
    std::string operator()(const T& val) {
        return json(val, nullptr);
    }
};

} // namespace tec

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                              USAGE
*

struct Person: tec::Serializable {
    using json = tec::Json;
    static constexpr auto sep{tec::Json::sep};

    short age;
    std::string name;
    std::string surname;

    friend std::ostream& operator << (std::ostream& os, const Person& p) {
        os << tec::Json{}(p);
        return os;
    }

    std::string to_json() const override {
        std::ostringstream os;
        os
            << json{}(age, "age") << sep
            << json{}(name, "name") << sep
            << json{}(surname, "surname")
            ;
        return os.str();
    }
};

*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
