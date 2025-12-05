// Time-stamp: <Last changed 2025-12-05 15:40:04 by magnolia>
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

#include <string>
#include <sstream>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_bytes.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"


namespace tec {


struct Json {
    static constexpr const char* sep{", "};
    static constexpr const char* infix{": "};

    inline static void print_name(std::ostringstream& os, const char* name) {
        if(name) {
            os << "\"" << name << "\"" << infix;
        }
    }

    static std::string json(const std::string& val, const char* name = nullptr) {
        std::ostringstream os;
        print_name(os, name);
        os <<  "\"" << val << "\"";
        return os.str();
    }

    static std::string json(const Bytes& val, const char* name = nullptr) {
        std::ostringstream os;
        print_name(os, name);
        os <<  "\"" << val.as_hex() << "\"";
        return os.str();
    }

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

    template <typename TObject>
    static std::string json_object(const TObject& obj, const char* name) {
        std::ostringstream os;
        print_name(os, name);
        os << "{" << obj.to_json() << "}";
        return os.str();
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Generic
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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

    template <typename T>
    std::string operator()(const T& val, const char* name) {
        return json(val, name);
    }

    template <typename T>
    std::string operator()(const T& val) {
        return json(val, nullptr);
    }
};


} // namespace tec
