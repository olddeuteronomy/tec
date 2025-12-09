// Time-stamp: <Last changed 2025-12-07 11:43:48 by magnolia>
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

#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_bytes.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"


namespace tec {


struct NdTypes {
    using ID = uint16_t;

    using Tag =  uint16_t;
    using Size = uint32_t;
    using Count = uint16_t;

    using Bool = uint8_t;
    using String = std::string;
    using Blob = Bytes;

    struct Meta {
        // First 8 bits 0..7 (<=255) are reserved to hold an element type.
        // ...
        // Bits 9..15 are reserved for the meta flags.
        static constexpr Tag Scalar{(  1 << 8)};
        static constexpr Tag Float{(   1 << 9)};
        static constexpr Tag Signed{(  1 << 10)};
        static constexpr Tag Sequence{(1 << 11)};
    };

    struct Tags {
        static constexpr Tag Unknown{0};

        // Scalars.
        static constexpr Tag I8{(1 | Meta::Scalar)};
        static constexpr Tag I16{(2 | Meta::Scalar)};
        static constexpr Tag I32{(3 | Meta::Scalar)};
        static constexpr Tag I64{(4 | Meta::Scalar)};
        static constexpr Tag IBool{(5 | Meta::Scalar)};
        static constexpr Tag F32{(6 | Meta::Scalar | Meta::Float)};
        static constexpr Tag F64{(7 | Meta::Scalar | Meta::Float)};
        static constexpr Tag F128{(8 | Meta::Scalar | Meta::Float)};
        // Scalar sequences.
        static constexpr Tag SByte{(32 | Meta::Scalar | Meta::Sequence)};
        static constexpr Tag SChar{(33 | Meta::Scalar | Meta::Sequence)};
        static constexpr Tag SWChar{(34 | Meta::Scalar | Meta::Sequence)};
        // Containers.
        static constexpr Tag Container{64};
        static constexpr Tag Map{65};

        // Serizalizable object.
        static constexpr Tag Object{80};
    };

#pragma pack(push, 1)
    struct Header {
        static constexpr uint32_t kMagic{0x041b00};
        static constexpr uint16_t kDefaultVersion{0x0100};

        // 16 bytes.
        uint32_t magic;
        uint32_t size;
        uint16_t version;
        uint16_t id;
        uint32_t reserved32;

        Header()
            : magic(kMagic)
            , size{0}
            , version{kDefaultVersion}
            , id{0}
            , reserved32{0}
        {}

        inline bool is_valid() const {
            return magic == kMagic;
        }
    };

    struct ElemHeader {
        // 8 bytes.
        Tag tag;
        Size size;
        Count count;

        ElemHeader()
            : tag{Tags::Unknown}
            , size{0}
            , count{0}
        {}

        ElemHeader(Tag _tag, Size _size, Count _count = 1)
            : tag{_tag}
            , size{_size}
            , count{_count}
        {}
    };
#pragma pack(pop)

    inline static Count to_count(size_t count) {
        return (count > __UINT16_MAX__) ? __UINT16_MAX__ : count;
    }

    // Integers.
    ElemHeader get_scalar_info(const char&)
        { return {Tags::I8 | Meta::Signed, 1}; }
    ElemHeader get_get_scalar_info(const unsigned char&)
        { return {Tags::I8, 1}; }

    ElemHeader get_scalar_info(const short&)
        { return {Tags::I16 | Meta::Signed, 2}; }
    ElemHeader get_get_scalar_info(const unsigned short&)
        { return {Tags::I16, 2}; }

    ElemHeader get_scalar_info(const int&)
        { return {Tags::I32 | Meta::Signed, 4}; }
    ElemHeader get_scalar_info(const unsigned int&)
        { return {Tags::I32, 4}; }

    ElemHeader get_scalar_info(const long&)
        { return {Tags::I32 | Meta::Signed, 4}; }
    ElemHeader get_scalar_info(const unsigned long&)
        { return {Tags::I32, 4}; }

    ElemHeader get_scalar_info(const long long&)
        { return {Tags::I64 | Meta::Signed, 8}; }
    ElemHeader get_scalar_info(const unsigned long long&)
        { return {Tags::I64, 8}; }

    // Boolean.
    ElemHeader get_scalar_info(const bool&)
        { return {Tags::IBool, sizeof(Bool)}; }

    // Floating point.
    ElemHeader get_scalar_info(const float&)
        { return {Tags::F32 | Meta::Signed, 4}; }
    ElemHeader get_scalar_info(const double&)
        { return {Tags::F64 | Meta::Signed, 8}; }
    ElemHeader get_scalar_info(const long double&)
        { return {Tags::F128 | Meta::Signed, 16}; }

    // Sequence: string.
    ElemHeader get_seq_info(const String& s)
        { return {Tags::SChar, static_cast<Size>(s.size())}; }
    // Sequence: Bytes.
    ElemHeader get_seq_info(const Bytes& s)
        { return {Tags::SByte, static_cast<Size>(s.size())}; }

    // Flat container.
    template <typename TContainer>
    ElemHeader get_container_info(const TContainer& c)
        { return {Tags::Container, 0, to_count(c.size())}; }

    // Map.
    template <typename TMap>
    ElemHeader get_map_info(const TMap& m)
        { return {Tags::Map, 0, to_count(m.size())}; }

    // Serializable object.
    template <typename TObject>
    ElemHeader get_object_info(const TObject&)
        { return {Tags::Object, 0}; }

    // Generic types.
    template <typename T>
    ElemHeader get_info(const T& val) {
        if constexpr (is_serializable_v<T>) {
            return get_object_info(val);
        }
        else if constexpr (std::is_arithmetic_v<T>) {
            return get_scalar_info(val);
        }
        else if constexpr (
            std::is_same_v<T, String> ||
            std::is_same_v<T, Bytes>
            ) {
            return get_seq_info(val);
        }
        else if constexpr (is_map_v<T>) {
            return get_map_info(val);
        }
        else if constexpr (is_container_v<T>) {
            return get_container_info(val);
        }
        else {
            return {};
        }
    }

}; // struct NdType


} // namespace tec

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NOTE:

 - float — single precision floating-point type. Usually IEEE-754
   binary32 format.

 - double — double precision floating-point type. Usually IEEE-754
   binary64 format.

 - long double — extended precision floating-point type. Does not
   necessarily map to types mandated by IEEE-754.

IEEE-754 binary128 format is used by some HP-UX, SPARC, MIPS, ARM64,
and z/OS implementations.

The most well known IEEE-754 binary64-extended format is x87 80-bit
extended precision format. It is used by many x86 and x86-64
implementations (a notable exception is MSVC, which implements long
double in the same format as double, i.e. binary64).

On Windows, sizeof(long long double) == 8.

On Windows and Linux, sizeof(bool) == 1.

 - char16_t — type for UTF-16 character representation, required to be
   large enough to represent any UTF-16 code unit (16 bits). It has
   the same size, signedness, and alignment as std::uint_least16_t,
   but is a distinct type.

 - char32_t — type for UTF-32 character representation, required to be
   large enough to represent any UTF-32 code unit (32 bits). It has
   the same size, signedness, and alignment as std::uint_least32_t,
   but is a distinct type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
