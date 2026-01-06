// Time-stamp: <Last changed 2026-01-06 15:23:46 by magnolia>
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
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_memfile.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"
#include "tec/net/tec_compression.hpp"


namespace tec {

/**
 * @struct NdTypes
 * @brief Core type definitions and metadata for a lightweight binary serialization format.
 *
 * This structure defines tags, sizes, and helper functions used to describe
 * serialized data elements in a compact, platform-independent way.
 */
struct NdTypes {

    /// Unique identifier type for serialized objects/messages.
    using ID = uint16_t;

    /// Tag type used to encode element kind and properties.
    using Tag = uint16_t;

    /// Size type for individual element payload (in bytes).
    using Size = uint32_t;

    /// Count type for number of elements in sequences/containers.
    using Count = uint16_t;

    /// Boolean storage type (always 1 byte).
    using Bool = uint8_t;

    /// String storage type.
    using String = std::string;

    /**
     * @struct Meta
     * @brief Bit-flag definitions for interpreting the Tag field.
     *
     * The lower 8 bits (0–7) encode the base element type.
     * Bits 9–15 are reserved for meta flags describing properties.
     */
    struct Meta {
        /// Indicates a scalar value (single primitive).
        static constexpr Tag Scalar{(1 << 8)};

        /// Indicates a floating-point scalar.
        static constexpr Tag Float{(1 << 9)};

        /// Indicates a signed integer scalar.
        static constexpr Tag Signed{(1 << 10)};

        /// Indicates a sequence of scalar elements (e.g., byte array, string).
        static constexpr Tag Sequence{(1 << 11)};
    };

    /**
     * @struct Tags
     * @brief Concrete tag values for all supported data types.
     */
    struct Tags {
        /// Unknown or uninitialized element.
        static constexpr Tag Unknown{0};

        // Scalars

        /// Signed 8-bit integer.
        static constexpr Tag I8{(1 | Meta::Scalar)};

        /// Signed/unsigned 16-bit integer.
        static constexpr Tag I16{(2 | Meta::Scalar)};

        /// Signed/unsigned 32-bit integer.
        static constexpr Tag I32{(3 | Meta::Scalar)};

        /// Signed/unsigned 64-bit integer.
        static constexpr Tag I64{(4 | Meta::Scalar)};

        /// Boolean value.
        static constexpr Tag IBool{(5 | Meta::Scalar)};

        /// 32-bit IEEE-754 float.
        static constexpr Tag F32{(6 | Meta::Scalar | Meta::Float)};

        /// 64-bit IEEE-754 double.
        static constexpr Tag F64{(7 | Meta::Scalar | Meta::Float)};

        /// 128-bit floating point (platform-dependent).
        static constexpr Tag F128{(8 | Meta::Scalar | Meta::Float)};

        // Scalar sequences

        /// Sequence of raw bytes (Blob).
        static constexpr Tag SByte{('B' | Meta::Scalar | Meta::Sequence)};

        /// Sequence of characters (std::string).
        static constexpr Tag SChar{('A' | Meta::Scalar | Meta::Sequence)};

        // Containers

        /// Flat container (vector, array, etc.).
        static constexpr Tag Container{'C'}; // 67

        /// Associative map/container.
        static constexpr Tag Map{'M'}; // 77

        // Object

        /// User-defined serializable object.
        static constexpr Tag Object{'O'}; // 79
    };

#pragma pack(push, 1)
    /**
     * @struct Header
     * @brief Global header placed at the start of every serialized buffer.
     *
     * Fixed size: 24 bytes. Contains magic number, total size, version, root object ID,
     * status, compression info, and uncompressed size.
     *
     * ### Byte layout
     *
     * | Byte   | Size    | Field                | Type      | Description                                                                 |
     * | Offset | (bytes) |                      |           |                                                                             |
     * |--------|---------|----------------------|-----------|-----------------------------------------------------------------------------|
     * | 0–3    | 4       | `magic`              | uint32_t  | Magic number identifying valid data. Fixed value: `0x00041b00`.             |
     * | 4–7    | 4       | `size`               | uint32_t  | Total size of the entire serialized message **including** this header.      |
     * | 8–9    | 2       | `version`            | uint16_t  | Protocol version. Default: `0x0100` (version 1.0).                          |
     * | 10–11  | 2       | `id`                 | uint16_t  | User-defined message or object identifier.                                  |
     * | 12–13  | 2       | `status`             | int16_t   | Application-specific status code (e.g., success or error code).             |
     * | 14–15  | 2       | `compression_flags`  | uint16_t  | Bits 0–3: compression algorithm (0–15)<br>Bits 4–7: compression level (0–15)|
     * | 16–19  | 4       | `size_uncompressed`  | uint32_t  | Original payload size before compression (for verification/decompression).  |
     * | 20–23  | 4       | `reserved`           | uint32_t  | Reserved for future use; must be zero.                                      |
     *
     * This layout is guaranteed to be contiguous and exactly the sizes shown on all platforms due to the packing directive.
     *
     */
    struct Header {
        /// Magic constant identifying valid serialized data.
        static constexpr uint32_t kMagic{0x00041b00};

        /// Default protocol version.
        static constexpr uint16_t kDefaultVersion{0x0100};

        /// Magic identifier (must match kMagic).
        uint32_t magic;

        /// Total size of the serialized payload including this header (bytes).
        uint32_t size;

        /// Protocol version.
        uint16_t version;

        /// User-defined root object identifier.
        uint16_t id;

        /// Status code (application-specific).
        int16_t  status;

        /// Compression type and level (low 4 bits: type, high 4 bits: level).
        uint16_t compression_flags;

        /// Original uncompressed payload size (for verification).
        uint32_t size_uncompressed;

        /// Reserved for future use (must be zero).
        uint32_t reserved;

        /**
         * @brief Default constructor initializing a valid empty header.
         */
        Header()
            : magic(kMagic)
            , size{0}
            , version{kDefaultVersion}
            , id{0}
            , status{0}
            , compression_flags{CompressionParams::kNoCompression}
            , size_uncompressed{0}
            , reserved{0}
        {}

        /**
         * @brief Check if the header appears valid.
         * @return true if magic and version are acceptable.
         */
        inline constexpr bool is_valid() const {
            return (magic == kMagic && version >= kDefaultVersion);
        }

        /**
         * @brief Extract compression algorithm type.
         * @return Compression type (0–15).
         */
        inline constexpr int get_compression() const {
            return (0xF & compression_flags);
        }

        /**
         * @brief Set compression algorithm type.
         * @param comp_type Type value (0–15).
         */
        inline constexpr void set_compression(int comp_type) {
            compression_flags |= (0xF & comp_type);
        }

        /**
         * @brief Extract compression level.
         * @return Level value (0–15, typically 0–9).
         */
        inline constexpr int get_compression_level() const {
            return (compression_flags & 0xF0) >> 4;
        }

        /**
         * @brief Set compression level.
         * @param nlevel Level value (0–15).
         */
        inline constexpr void set_compression_level(int nlevel) {
            compression_flags |= ((0xF & nlevel) << 4);
        }
    };

    /**
     * @struct ElemHeader
     * @brief Per-element header describing the following data item.
     *
     * Fixed size: 8 bytes. Precedes every serialized element.
     *
     * ### Byte layout
     *
     * | Byte   | Size    | Field    | Type            | Description                                                                                                   |
     * | Offset | (bytes) |          |                 |                                                                                                               |
     * |--------|---------|----------|-----------------|---------------------------------------------------------------------------------------------------------------|
     * | 0–1    | 2       | `tag`    | Tag (uint16_t)  | Type and property tag (encodes scalar/float/signed/sequence/container/object etc.).                           |
     * | 2–5    | 4       | `size`   | Size (uint32_t) | Payload size in bytes **excluding** this ElemHeader (0 for containers/objects that compute size dynamically). |
     * | 6–7    | 2       | `count`  | Count (uint16_t)| Number of elements:<br>- 1 for scalars<br>- Actual count for sequences/containers (clamped to 65535)          |
     *
     * This layout is guaranteed to be contiguous and exactly the sizes shown on all platforms due to the packing directive.
     *
     */
    struct ElemHeader {
        /// Type and property tag.
        Tag tag;

        /// Payload size in bytes (excluding this header).
        Size size;

        /// Number of elements (for containers/sequences; 1 for scalars).
        Count count;

        /**
         * @brief Default constructor (unknown element).
         */
        ElemHeader()
            : tag{Tags::Unknown}
            , size{0}
            , count{0}
        {}

        /**
         * @brief Construct with explicit tag, size, and count.
         */
        ElemHeader(Tag _tag, Size _size, Count _count = 1)
            : tag{_tag}
            , size{_size}
            , count{_count}
        {}
    };
#pragma pack(pop)

    /**
     * @brief Convert a size_t count to Count, clamping at maximum.
     * @param count Original count.
     * @return Clamped Count value.
     */
    inline constexpr Count to_count(size_t count) {
        return ((count > __UINT16_MAX__) ? __UINT16_MAX__ : static_cast<Count>(count));
    }

    /** @brief Get element info for signed char. */
    ElemHeader get_scalar_info(const char&) { return {Tags::I8 | Meta::Signed, 1}; }

    /** @brief Get element info for unsigned char. */
    ElemHeader get_scalar_info(const unsigned char&) { return {Tags::I8, 1}; }

    /** @brief Get element info for signed short. */
    ElemHeader get_scalar_info(const short&) { return {Tags::I16 | Meta::Signed, 2}; }

    /** @brief Get element info for unsigned short. */
    ElemHeader get_scalar_info(const unsigned short&) { return {Tags::I16, 2}; }

    /** @brief Get element info for signed int. */
    ElemHeader get_scalar_info(const int&) { return {Tags::I32 | Meta::Signed, 4}; }

    /** @brief Get element info for unsigned int. */
    ElemHeader get_scalar_info(const unsigned int&) { return {Tags::I32, 4}; }

    /** @brief Get element info for signed long. */
    ElemHeader get_scalar_info(const long&) { return {Tags::I32 | Meta::Signed, 4}; }

    /** @brief Get element info for unsigned long. */
    ElemHeader get_scalar_info(const unsigned long&) { return {Tags::I32, 4}; }

    /** @brief Get element info for signed long long. */
    ElemHeader get_scalar_info(const long long&) { return {Tags::I64 | Meta::Signed, 8}; }

    /** @brief Get element info for unsigned long long. */
    ElemHeader get_scalar_info(const unsigned long long&) { return {Tags::I64, 8}; }

    /** @brief Get element info for bool. */
    ElemHeader get_scalar_info(const bool&) { return {Tags::IBool, sizeof(Bool)}; }

    /** @brief Get element info for float. */
    ElemHeader get_scalar_info(const float&) { return {Tags::F32 | Meta::Signed, 4}; }

    /** @brief Get element info for double. */
    ElemHeader get_scalar_info(const double&) { return {Tags::F64 | Meta::Signed, 8}; }

    /** @brief Get element info for long double (platform-dependent size). */
    ElemHeader get_scalar_info(const long double&) { return {Tags::F128 | Meta::Signed, 16}; }

    /** @brief Get element info for std::string (character sequence). */
    ElemHeader get_seq_info(const String& str) { return {Tags::SChar, static_cast<Size>(str.size())}; }

    /** @brief Get element info for Blob (raw byte sequence). */
    ElemHeader get_seq_info(const Blob& bytes) { return {Tags::SByte, static_cast<Size>(bytes.size())}; }

    /**
     * @brief Get element info for flat containers (vector, array, etc.).
     * @tparam TContainer Container type.
     */
    template <typename TContainer>
    ElemHeader get_container_info(const TContainer& c) { return {Tags::Container, 0, to_count(c.size())}; }

    /**
     * @brief Get element info for map-like containers.
     * @tparam TMap Map type.
     */
    template <typename TMap>
    ElemHeader get_map_info(const TMap& m) { return {Tags::Map, 0, to_count(m.size())}; }

    /**
     * @brief Get element info for user-defined serializable objects.
     * @tparam TObject Serializable type.
     */
    template <typename TObject>
    ElemHeader get_object_info(const TObject&) { return {Tags::Object, 0}; }

    /**
     * @brief Generic dispatcher to determine ElemHeader for any supported type.
     *
     * Uses compile-time trait checks to route to the appropriate specialization.
     *
     * @tparam T Value type.
     * @param val Reference to the value (used only for overload resolution).
     * @return Corresponding ElemHeader.
     */
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
            std::is_same_v<T, Blob>
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

}; // struct NdTypes

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

On Windows, sizeof(long double) == 8.
On Linux, macOS, sizeof(long double) == 16.

On Windows, Linux, macOS, sizeof(bool) == 1.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
