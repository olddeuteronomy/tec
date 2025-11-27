// Time-stamp: <Last changed 2025-11-27 18:49:43 by magnolia>
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
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_buffer.hpp"
#include "tec/tec_container.hpp"
#include "tec/tec_serialize.hpp"


namespace tec {


class NetData {
public:
    using Tag =  uint32_t;
    using Size = uint32_t;

    using Bool = uint16_t;
    using String = std::string;

    struct Meta {
        // First 8 bits (0..255) are reserved to hold an element type.
        // ...
        // Bits 9..23 are reserved for the meta flags.
        static constexpr const Tag kScalar{(  1 << 8)};
        static constexpr const Tag kFloat{(   1 << 9)};
        static constexpr const Tag kSigned{(  1 << 10)};
        static constexpr const Tag kSequence{(1 << 11)};
        static constexpr const Tag kContainer{(1 << 12)};
        static constexpr const Tag kObject{(1 << 13)};
        // Bits 24..31 are reserved.
    };

    struct Tags {
        static constexpr const Tag kUnknown{0};

        // Scalars.
        static constexpr const Tag kI8{(1 | Meta::kScalar)};
        static constexpr const Tag kI16{(2 | Meta::kScalar)};
        static constexpr const Tag kI32{(3 | Meta::kScalar)};
        static constexpr const Tag kI64{(4 | Meta::kScalar)};
        static constexpr const Tag kF32{(5 | Meta::kScalar | Meta::kFloat)};
        static constexpr const Tag kF64{(6 | Meta::kScalar | Meta::kFloat)};
        static constexpr const Tag kF128{(7 | Meta::kScalar | Meta::kFloat)};
        static constexpr const Tag kBool{(8 | Meta::kScalar)};

        // Scalar sequences.
        static constexpr const Tag kBytes{(32 | Meta::kScalar | Meta::kSequence)};
        static constexpr const Tag kString{(33 | Meta::kScalar | Meta::kSequence)};

        // Any container.
        static constexpr const Tag kContainer{(Meta::kContainer)};

        // Any serizalizable.
        static constexpr const Tag kObject{(Meta::kObject)};
    };

#pragma pack(push, 1)
    struct Header {
        static constexpr const uint32_t kMagic{0x041b00};
        static constexpr const uint16_t kDefaultVersion{0x0100};

        // 16 bytes.
        uint32_t magic;
        uint32_t size;
        uint16_t version;
        uint16_t reserved16;
        uint32_t reserved32;

        Header()
            : magic(kMagic)
            , size{0}
            , version{kDefaultVersion}
            , reserved16{0}
            , reserved32{0}
        {}

        inline bool is_valid() const {
            return magic == kMagic;
        }
    };

    struct ElemHeader {
        // 12 bytes.
        Tag tag;
        Size size;
        Size count;

        ElemHeader()
            : tag{Tags::kUnknown}
            , size{0}
            , count{0}
        {}

        ElemHeader(Tag _tag, Size _size, Size _count = 1)
            : tag{_tag}
            , size{_size}
            , count{_count}
        {}
    };
#pragma pack(pop)

    // Integers.
    ElemHeader get_scalar_info(const char&)
        { return {Tags::kI8 | Meta::kSigned, 1}; }
    ElemHeader get_get_scalar_info(const unsigned char&)
        { return {Tags::kI8, 1}; }

    ElemHeader get_scalar_info(const short&)
        { return {Tags::kI16 | Meta::kSigned, 2}; }
    ElemHeader get_get_scalar_info(const unsigned short&)
        { return {Tags::kI16, 2}; }

    ElemHeader get_scalar_info(const int&)
        { return {Tags::kI32 | Meta::kSigned, 4}; }
    ElemHeader get_scalar_info(const unsigned int&)
        { return {Tags::kI32, 4}; }

    ElemHeader get_scalar_info(const long&)
        { return {Tags::kI32 | Meta::kSigned, 4}; }
    ElemHeader get_scalar_info(const unsigned long&)
        { return {Tags::kI32, 4}; }

    ElemHeader get_scalar_info(const long long&)
        { return {Tags::kI64 | Meta::kSigned, 8}; }
    ElemHeader get_scalar_info(const unsigned long long&)
        { return {Tags::kI64, 8}; }

    // Boolean.
    ElemHeader get_scalar_info(const bool&)
        { return {Tags::kI16, sizeof(Bool)}; }

    // Floating point.
    ElemHeader get_scalar_info(const float&)
        { return {Tags::kF32 | Meta::kSigned, 4}; }
    ElemHeader get_scalar_info(const double&)
        { return {Tags::kF64 | Meta::kSigned, 8}; }
    ElemHeader get_scalar_info(const long double&)
        { return {Tags::kF128 | Meta::kSigned, 16}; }

    // Sequence: string.
    ElemHeader get_seq_info(const String& s)
        { return {Tags::kString, static_cast<Size>(s.size())}; }
    // Sequence: Bytes.
    ElemHeader get_seq_info(const Bytes& s)
        { return {Tags::kBytes, static_cast<Size>(s.size())}; }

    // Flat container.
    template <typename TContainer>
    ElemHeader get_container_info(const TContainer& c)
        { return {Tags::kContainer, 0, static_cast<Size>(c.size())}; }

    // Serializable object.
    template <typename TObject>
    ElemHeader get_object_info(const TObject&)
        { return {Tags::kObject, 0}; }

    // Generic types.
    template <typename T>
    ElemHeader get_info(const T& val) {
        if constexpr (std::is_arithmetic_v<T>) {
            return get_scalar_info(val);
        }
        else if constexpr (
            std::is_same_v<T, String> ||
            std::is_same_v<T, Bytes>
            ) {
            return get_seq_info(val);
        }
        else if constexpr (is_serializable_v<T>) {
            return get_object_info(val);
        }
        else if constexpr (is_container_v<T>) {
            return get_container_info(val);
        }
        else {
            return {};
        }
    }

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
        ElemHeader hdr = get_info(val);
        if( hdr.tag & Meta::kSequence ) {
            write_sequence(&hdr, &val);
        }
        else if( hdr.tag & Meta::kScalar ) {
            write_scalar(&hdr, &val);
        }
        else if( hdr.tag & Meta::kContainer ) {
            write_container(&hdr, val);
        }
        else if( hdr.tag & Meta::kObject ) {
            write_object(&hdr, val);
        }
        hdr_ptr_->size = data_.tell() - sizeof(Header);
        return *this;
    }

protected:

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
        if constexpr (tec::is_serializable_v<TObject>) {
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

    virtual size_t write_scalar(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if( hdr->tag == Tags::kBool ) {
            const bool* val = static_cast<const bool*>(p);
            Bool b = *val ? 1 : 0;
            data_.write(&b, hdr->size);
        }
        else {
            data_.write(p, hdr->size);
        }
        return hdr->size;
    }

    virtual size_t write_sequence(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if( hdr->tag == Tags::kString ) {
            const std::string* s = static_cast<const std::string*>(p);
            data_.write(s->data(), hdr->size);
        }
        else if( hdr->tag == Tags::kBytes ) {
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
    NetData& operator >> (T* val) {
        ElemHeader hdr;
        data_.read(&hdr, sizeof(ElemHeader));
        if constexpr (is_serializable_v<T>) {
            if( hdr.tag & Meta::kObject ) {
                val->load(std::ref(*this));
            }
        }
        else if constexpr (
            is_container_v<T>  &&
            !std::is_same_v<T, String>
            ) {
            read_container(&hdr, val);
        }
        else {
            return read(&hdr, val);
        }
        return *this;
    }

protected:

    template <typename TContainer>
    NetData& read_container(ElemHeader* hdr, TContainer* c) {
        if constexpr (
            is_container_v<TContainer>  &&
            !std::is_same_v<TContainer, String>
            ) {
            for( size_t n = 0 ; n < hdr->count ; ++n ) {
                typename TContainer::value_type e;
                *this >> &e;
                c->push_back(e);
            }
        }
        return *this;
    }

    virtual NetData& read(ElemHeader* hdr, void* dst) {
        if( hdr->tag & Meta::kScalar) {
            if( hdr->tag & Meta::kSequence ) {
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
        return *this;
    }

    virtual void read_scalar(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tags::kBool ) {
            Bool b{0};
            data_.read(&b, sizeof(Bool));
            bool* val = static_cast<bool*>(dst);
            *val = (b == 0 ? false : true);
        }
        else {
            data_.read(dst, hdr->size);
        }
    }

    virtual void read_sequence(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tags::kString ) {
            std::string* str = static_cast<std::string*>(dst);
            str->resize(hdr->size);
            data_.read(str->data(), hdr->size);
        }
        else if( hdr->tag == Tags::kBytes ) {
            Bytes* bytes = static_cast<Bytes*>(dst);
            bytes->resize(hdr->size);
            data_.read(bytes->data(), hdr->size);
        }
    }

}; // class NetData

} // namespace tec
