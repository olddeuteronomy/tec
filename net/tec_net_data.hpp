// Time-stamp: <Last changed 2026-02-11 16:17:56 by magnolia>
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
 * @file tec_net_data.hpp
 * @brief Lightweight binary serialization optimized for network communication.
 * @author The Emacs Cat
 * @date 2025-11-16
 */

#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_memfile.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"
#include "tec/net/tec_nd_types.hpp"


namespace tec {

/**
 * @brief Lightweight binary serialization container optimized for network communication
 *
 * NetData provides efficient serialization and deserialization of various C++ types
 * including scalars, strings, containers, maps and user-defined serializable objects.
 * The format consists of ElemHeader + payload for each element, allowing flexible
 * and relatively compact binary representation suitable for network transmission.
 */
class NetData: public NdTypes {
public:
    /// Global message header
    Header header;

protected:
    /// Internal storage of serialized binary data
    Bytes data_;

public:
    /**
     * @brief Default constructor - creates empty NetData object
     */
    NetData() = default;

    // /**
    //  * @brief Disabled - use explicit `copy_from()`
    //  */
    // NetData(const NetData&) = delete;

    // /**
    //  * @brief Disabled - use explicit `move_from()`
    //  */
    // NetData(NetData&&) = delete;

    /**
     * @brief Virtual destructor
     */
    virtual ~NetData() = default;

    /**
     * @brief Returns const reference to internal byte buffer
     * @return const reference to the serialized bytes
     */
    const Bytes& bytes() const {
        return data_;
    }

    /**
     * @brief Returns const pointer to the beginning of serialized data
     * @return const pointer to raw data
     */
    const void* data() const {
        return data_.data();
    }

    /**
     * @brief Performs deep copy of content from another NetData instance
     * @param src source NetData object to copy from
     */
    void copy_from(const NetData& src) {
        header = src.header;
        data_.copy_from(src.bytes());
    }

    /**
     * @brief Efficiently moves content from another NetData instance
     * @param src rvalue reference to source object (will be left in moved-from state)
     * @param size_to_shrink optional target capacity after move (0 = don't shrink)
     */
    void move_from(NetData&& src, size_t size_to_shrink = 0) {
        header = src.header;
        data_.move_from(std::move(src.bytes()), size_to_shrink);
    }

    /**
     * @brief Resets read position to the beginning of the buffer
     */
    void rewind() {
        data_.rewind();
    }

    /**
     * @brief Returns current logical size of the message (according to header)
     * @return number of bytes of valid serialized data
     */
    size_t size() const {
        return header.size;
    }

    /**
     * @brief Returns current capacity of internal buffer
     * @return allocated size of the internal byte storage
     */
    size_t capacity() const {
        return data_.capacity();
    }

    /**
     * @brief Returns mutable reference to internal byte buffer
     * @warning Direct manipulation may corrupt serialization format
     * @return mutable reference to bytes container
     */
    Bytes& bytes() {
        return data_;
    }

    /**
     * @brief Returns mutable pointer to the beginning of serialized data
     * @warning Direct manipulation may corrupt serialization format
     * @return mutable pointer to raw data
     */
    void* data() {
        return data_.data();
    }

public:

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            STORE
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Serialization operator (stream-like syntax)
     * @tparam T type of value to serialize
     * @param val value to be serialized
     * @return reference to self for operation chaining
     *
     * Supports most fundamental types, strings, containers, maps and
     * custom types that satisfy is_serializable_v<> trait.
     */
    template <typename T>
    NetData& operator << (const T& val) {
        ElemHeader hdr = get_info<T>(val);
        if( hdr.tag & Meta::Sequence ) {
            write_sequence(&hdr, &val);
        }
        else if(hdr.tag & Meta::Scalar) {
            write_scalar(&hdr, &val);
        }
        else if(hdr.tag == Tags::Object) {
            write_object(&hdr, val);
        }
        else if(hdr.tag == Tags::Map) {
            write_map(&hdr, val);
        }
        else if(hdr.tag == Tags::Container) {
            write_container(&hdr, val);
        }

        header.size = data_.size();
        return *this;
    }

protected:

    /**
     * @brief Serializes map-like container (std::unordered_map)
     * @tparam TMap map-like container type
     * @param hdr prepared element header
     * @param map container instance to serialize
     * @return number of bytes written for map contents (without header)
     */
    template <typename TMap>
    size_t write_map(ElemHeader* hdr, const TMap& map) {
        if constexpr (is_map_v<TMap>) {
            ElemHeader* hdr_ptr = (ElemHeader*)data_.ptr(data_.tell());
            // Write the map header.
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write all elements of a container.
            for( const auto& [key, value]: map ) {
                *this << key << value;
            }
            // Set actual container size.
            auto bytes_written = data_.tell() - cur_pos;
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    /**
     * @brief Serializes sequence container (vector, deque, list, ...)
     * @tparam TContainer sequence container type
     * @param hdr prepared element header
     * @param container container instance to serialize
     * @return number of bytes written for container contents (without header)
     */
    template <typename TContainer>
    size_t write_container(ElemHeader* hdr, const TContainer& container) {
        if constexpr (is_container_v<TContainer>) {
            ElemHeader* hdr_ptr = (ElemHeader*)data_.ptr(data_.tell());
            // Write the container header.
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write all elements of a container.
            for( const auto& e: container ) {
                *this << e;
            }
            // Set actual container size.
            auto bytes_written = data_.tell() - cur_pos;
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    /**
     * @brief Serializes custom object that implements store() method
     * @tparam TObject type of serializable object
     * @param hdr prepared element header
     * @param obj object instance to serialize
     * @return number of bytes written for object contents (without header)
     */
    template <typename TObject>
    size_t write_object(ElemHeader* hdr, const TObject& obj) {
        if constexpr (is_serializable_v<TObject>) {
            if constexpr (is_root_v<TObject>) {
                    header.id = obj.id();
            }
            ElemHeader* hdr_ptr = (ElemHeader*)data_.ptr(data_.tell());
            // Write the object header.
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write an object
            obj.store(std::ref(*this));
            // Set actual object size.
            auto bytes_written = data_.tell() - cur_pos;
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    /**
     * @brief Special handling for long double when sizeof(long double) == 8 (on MS Windows)
     * @param hdr element header
     * @param d64 pointer to double value (used as source)
     * @return number of bytes written
     */
    virtual size_t write_long_double_64(ElemHeader* hdr, const double* d64) {
        double dst[2]{*d64, 0.0};
        data_.write(dst, hdr->size);
        return hdr->size;
    }

    /**
     * @brief Writes single scalar value with its header
     * @param hdr element header
     * @param p pointer to source value
     * @return number of bytes written
     */
    virtual size_t write_scalar(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if(sizeof(long double) == 8 && hdr->tag == Tags::F128) {
            // On Windows, sizeof(long double) == sizeof(double) == 8,
            // but we always use 16-byte for storing long double.
            return write_long_double_64(hdr, static_cast<const double*>(p));
        }
        data_.write(p, hdr->size);
        return hdr->size;
    }

    /**
     * @brief Writes string or blob sequence (header + raw bytes)
     * @param hdr element header
     * @param p pointer to source (std::string or Blob)
     * @return number of bytes written
     */
    virtual size_t write_sequence(ElemHeader* hdr, const void* p) {
        // Write the sequence header.
        data_.write(hdr, sizeof(ElemHeader));
        if (hdr->size) {
            if( hdr->tag == Tags::SChar ) {
                const std::string* s = static_cast<const std::string*>(p);
                data_.write(s->data(), hdr->size);
            }
            else if( hdr->tag == Tags::SByte ) {
                const Blob* bs = static_cast<const Blob*>(p);
                data_.write(bs->data(), hdr->size);
            }
        }
        return hdr->size;
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            LOAD
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

    /**
     * @brief Deserialization operator (stream-like syntax)
     * @tparam T type of value to deserialize
     * @param val [out] destination variable
     * @return reference to self for operation chaining
     */
    template <typename T>
    NetData& operator >> (T& val) {
        ElemHeader hdr;
        data_.read(&hdr, sizeof(ElemHeader));
        // Object.
        if (hdr.tag == Tags::Object) {
            if constexpr(is_serializable_v<T>) {
                val.load(std::ref(*this));
            }
        }
        // Map.
        else if (hdr.tag == Tags::Map) {
            read_map(&hdr, val);
        }
        // Container (but not a String!).
        else if (hdr.tag == Tags::Container) {
            read_container(&hdr, val);
        }
        // Scalar (including sequences).
        else {
            read(&hdr, &val);
        }
        return *this;
    }

protected:

    /**
     * @brief Reads map-like container from stream
     * @tparam TMap map-like container type
     * @param hdr element header already read
     * @param map [out] destination container
     */
    template <typename TMap>
    void read_map(ElemHeader* hdr, TMap& map) {
        if constexpr(is_map_v<TMap>) {
            for( size_t n = 0 ; n < hdr->count ; ++n ) {
                typename TMap::key_type k;
                typename TMap::mapped_type e;
                *this >> k >> e;
                map[k] = e;
            }
        }
    }

    /**
     * @brief Reads sequence container from stream
     * @tparam TContainer sequence container type
     * @param hdr element header already read
     * @param c [out] destination container
     */
    template <typename TContainer>
    void read_container(ElemHeader* hdr, TContainer& c) {
        if constexpr(
            is_container_v<TContainer>  &&
            !is_map_v<TContainer> &&
            !std::is_same_v<TContainer, String>
            ) {
            for( size_t n = 0 ; n < hdr->count ; ++n ) {
                typename TContainer::value_type e;
                *this >> e;
                c.push_back(e);
            }
        }
    }

    /**
     * @brief Generic dispatcher for reading scalar/sequence values
     * @param hdr element header already read
     * @param dst [out] destination memory
     */
    virtual void read(ElemHeader* hdr, void* dst) {
        if( hdr->tag & Meta::Scalar) {
            if( hdr->tag & Meta::Sequence ) {
                read_sequence(hdr, dst);
            }
            else {
                read_scalar(hdr, dst);
            }
        }
        else {
            // Skip unknown element.
            data_.seek(hdr->size, SEEK_CUR);
        }
    }

    /**
     * @brief Platform-specific reading of MSWindows' `long double` (64 bit) stored as 128-bit double.
     * @param hdr element header
     * @param d64 [out] destination double
     */
    virtual void read_long_double_64(const ElemHeader* hdr, double* d64) {
        double d[2]{0.0, 0.0};
        data_.read(d, hdr->size);
        *d64 = d[0];
    }

    /**
     * @brief Reads single scalar value
     * @param hdr element header
     * @param dst [out] destination memory
     */
    virtual void read_scalar(const ElemHeader* hdr, void* dst) {
        if(sizeof(long double) == 8 && hdr->tag == Tags::F128) {
            // On Windows, sizeof(long double) == sizeof(double) == 8
            read_long_double_64(hdr, static_cast<double*>(dst));
        }
        else {
            data_.read(dst, hdr->size);
        }
    }

    /**
     * @brief Reads string or blob sequence data
     * @param hdr element header
     * @param dst [out] destination string/blob
     */
    virtual void read_sequence(const ElemHeader* hdr, void* dst) {
        if (hdr->size) {
            if( hdr->tag == Tags::SChar ) {
                std::string* str = static_cast<String*>(dst);
                str->resize(hdr->size);
                // To avoid silly GCC warning.
                data_.read(&str->at(0), hdr->size);
            }
            else if( hdr->tag == Tags::SByte ) {
                Blob* bytes = static_cast<Blob*>(dst);
                bytes->resize(hdr->size);
                data_.read(bytes->data(), hdr->size);
            }
        }
    }

public:

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                         NetData streams
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    /**
     * @brief Helper type for ADL customization of input operations
     */
    struct StreamIn {
        NetData* nd;

        StreamIn()
            : nd{nullptr}
            {}

        explicit StreamIn(NetData* _nd)
            : nd{_nd}
            {}
    };

    /**
     * @brief Helper type for ADL customization of output operations
     */
    struct StreamOut {
        NetData* nd;

        StreamOut()
            : nd{nullptr}
            {}

        explicit StreamOut(NetData* _nd)
            : nd{_nd}
            {}
    };


}; // class NetData


} // namespace tec
