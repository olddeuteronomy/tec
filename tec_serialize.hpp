// Time-stamp: <Last changed 2025-12-03 13:13:38 by magnolia>
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

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "tec/tec_container.hpp"


namespace tec {


template<typename T, typename = void>
struct is_serializable : std::false_type {};

template<typename T>
struct is_serializable<T,
    std::void_t<decltype(std::declval<T>().serializable())>>
    : std::true_type {};

template<typename T>
inline constexpr bool is_serializable_v = is_serializable<T>::value;


// Forward reference.
class NetData;

struct Serializable {
    constexpr bool serializable() { return true; }
    static constexpr const char* sep{", "};

    Serializable() = default;
    virtual ~Serializable() = default;

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                       Binary serialization
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    virtual NetData& store(NetData& s) const = 0;
    virtual NetData& load(NetData& s) = 0;

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                       JSON serialization
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    static std::string json(const std::string& val, const char* name = nullptr) {
        std::ostringstream os;
        if(name) os << name << ": ";
        os <<  "\"" << val << "\"";
        return os.str();
    }

    static std::string json(const bool& val, const char* name = nullptr) {
        std::ostringstream os;
        if(name) os << name << ": ";
        os << (val ? "true" : "false");
        return os.str();
    }

    template <typename TContainer>
    static std::string json_container(const TContainer& c, const char* name = nullptr) {
        std::ostringstream os;
        bool first{true};
        os << "[";
        for( const auto& e: c ) {
            if(first) {
                os << e;
                first = false;
            }
            else {
                os << ", " << e;
            }
        }
        os << "]";
        return os.str();
    }

    template <typename TMap>
    static std::string json_map(const TMap& m, const char* name = nullptr) {
        std::ostringstream os;
        bool first{true};
        os << "{";
        for( const auto& [k, v]: m) {
            if(first) {
                os << k << ": " << v;
                first = false;
            }
            else {
                os << ", " << k << ": " << v;

            }
        }
        os << "}";
        return os.str();
    }

    template <typename TObject>
    static std::string json_object(const TObject& obj, const char* name = nullptr) {
        std::ostringstream os;
        os << "{" << obj.to_json() << "}";
        return os.str();
    }

    template <typename T>
    static std::string json(const T& val, const char* name = nullptr) {
        std::ostringstream os;
        if(name) os << name << ": ";
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
            os << val;
        }
        return os.str();
    }
};


} // namespace tec
