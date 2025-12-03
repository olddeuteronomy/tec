// Time-stamp: <Last changed 2025-12-04 01:53:02 by magnolia>
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
 * @file tec_net_data.hpp
 * @brief Data serialization routines.
 * @author The Emacs Cat
 * @date 2025-11-16
 */

#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_bytes.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"
#include "tec/net/tec_nd_types.hpp"


namespace tec {

class NetData: public NdTypes {

protected:
    Bytes data_;
    Header* hdr_ptr_;

public:
    NetData() {
        Header hdr;
        data_.write(&hdr, sizeof(Header));
        hdr_ptr_ = (Header*)data_.data();
    }

    virtual ~NetData() = default;

    const Bytes& bytes() const {
        return data_;
    }

    Header header() const {
        return *hdr_ptr_;
    }

    void rewind() {
        data_.seek(sizeof(Header), SEEK_SET);
    }

    size_t total_size() const {
        return data_.size();
    }

    size_t size() const {
        return hdr_ptr_->size;
    }

    size_t capacity() const {
        return data_.capacity();
    }

    void* data() {
        return data_.data() + sizeof(Header);
    }

    const void* data() const {
        return data_.data() + sizeof(Header);
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            STORE
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

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

        hdr_ptr_->size = data_.tell() - sizeof(Header);
        return *this;
    }

protected:

    template <typename TMap>
    size_t write_map(ElemHeader* hdr, const TMap& map) {
        if constexpr (is_map_v<TMap>) {
            ElemHeader* hdr_ptr = (ElemHeader*)data_.at(data_.tell());
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write all elements of a container.
            for( const auto& [key, value]: map ) {
                *this << key << value;
            }
            auto bytes_written = data_.tell() - cur_pos;
            // Set actual container size.
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    template <typename TContainer>
    size_t write_container(ElemHeader* hdr, const TContainer& container) {
        if constexpr (is_container_v<TContainer>) {
            ElemHeader* hdr_ptr = (ElemHeader*)data_.at(data_.tell());
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write all elements of a container.
            for( const auto& e: container ) {
                *this << e;
            }
            auto bytes_written = data_.tell() - cur_pos;
            // Set actual container size.
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    template <typename TObject>
    size_t write_object(ElemHeader* hdr, const TObject& obj) {
        if constexpr (is_serializable_v<TObject>) {
            ElemHeader* hdr_ptr = (ElemHeader*)data_.at(data_.tell());
            data_.write(hdr, sizeof(ElemHeader));
            size_t cur_pos = data_.tell();
            // Write an object
            obj.store(std::ref(*this));
            // Set actual object size.
            auto bytes_written = data_.tell() - cur_pos;
            // Set actual object size.
            hdr_ptr->size = bytes_written;
            return bytes_written;
        }
        return 0;
    }

    virtual size_t write_long_double_64(ElemHeader* hdr, const double* d64) {
        double dst[2]{*d64, 0.0};
        data_.write(dst, hdr->size);
        return hdr->size;
    }

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

    virtual size_t write_sequence(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if( hdr->tag == Tags::SChar ) {
            const std::string* s = static_cast<const std::string*>(p);
            data_.write(s->data(), hdr->size);
        }
        else if( hdr->tag == Tags::SByte ) {
            const Bytes* bs = static_cast<const Bytes*>(p);
            data_.write(bs->data(), hdr->size);
        }
        return hdr->size;
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            LOAD
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

    template <typename T>
    NetData& operator >> (T& val) {
        ElemHeader hdr;
        data_.read(&hdr, sizeof(ElemHeader));
        // Object.
        if constexpr (is_serializable_v<T>) {
            if(hdr.tag == Tags::Object) {
                val.load(std::ref(*this));
            }
        }
        // Map.
        else if constexpr(is_map_v<T>) {
            if(hdr.tag == Tags::Map) {
                read_map(&hdr, val);
            }
        }
        // Container (but not a String!).
        else if constexpr (
            is_container_v<T>  &&
            !std::is_same_v<T, String>
            ) {
            if(hdr.tag == Tags::Container) {
                read_container(&hdr, val);
            }
        }
        // Scalar.
        else {
            read(&hdr, &val);
        }
        return *this;
    }

protected:

    template <typename TMap>
    void read_map(ElemHeader* hdr, TMap& map) {
        if constexpr (is_map_v<TMap>) {
            for( size_t n = 0 ; n < hdr->count ; ++n ) {
                typename TMap::key_type k;
                typename TMap::mapped_type e;
                *this >> k >> e;
                map[k] = e;
            }
        }
    }

    template <typename TContainer>
    void read_container(ElemHeader* hdr, TContainer& c) {
        if constexpr (
            is_container_v<TContainer>  &&
            !std::is_same_v<TContainer, String>
            ) {
            for( size_t n = 0 ; n < hdr->count ; ++n ) {
                typename TContainer::value_type e;
                *this >> e;
                c.push_back(e);
            }
        }
    }

    void read(ElemHeader* hdr, void* dst) {
        if( hdr->tag & Meta::Scalar) {
            if( hdr->tag & Meta::Sequence ) {
                read_sequence(hdr, dst);
            }
            else {
                read_scalar(hdr, dst);
            }
        }
        else {
            // Skip inknown element.
            data_.seek(hdr->size, SEEK_CUR);
        }
    }

    virtual void read_long_double_64(const ElemHeader* hdr, double* d64) {
        double d[2]{0.0, 0.0};
        data_.read(d, hdr->size);
        *d64 = d[0];
    }

    virtual void read_scalar(const ElemHeader* hdr, void* dst) {
        if(sizeof(long double) == 8 && hdr->tag == Tags::F128) {
            // On Windows, sizeof(long double) == sizeof(double) == 8
            read_long_double_64(hdr, static_cast<double*>(dst));
        }
        else {
            data_.read(dst, hdr->size);
        }
    }

    virtual void read_sequence(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tags::SChar ) {
            std::string* str = static_cast<std::string*>(dst);
            str->resize(hdr->size);
            data_.read(str->data(), hdr->size);
        }
        else if( hdr->tag == Tags::SByte ) {
            Bytes* bytes = static_cast<Bytes*>(dst);
            bytes->resize(hdr->size);
            data_.read(bytes->data(), hdr->size);
        }
    }

}; // class NetData

} // namespace tec
