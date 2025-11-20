// Time-stamp: <Last changed 2025-11-20 13:30:18 by magnolia>
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

#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/types.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_buffer.hpp"


namespace tec {


class NetData {
public:
    using Tag =   uint16_t;
    using Count = uint16_t;
    using Size =  uint32_t;

    using BoolSize = uint16_t;

    struct Meta {
        // First 8 bit (0..255) are reserved to hold an element type.
        static constexpr const Tag kScalar{(  1 << 8)};
        static constexpr const Tag kFloat{(   1 << 9)};
        static constexpr const Tag kSequence{(1 << 10)};
        // Bits 11..15 are reserved.
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
        static constexpr const Tag kBool{(7 | Meta::kScalar)};

        // Sequences.
        static constexpr const Tag kBytes{(8 | Meta::kScalar | Meta::kSequence)};
        static constexpr const Tag kString{(9 | Meta::kScalar | Meta::kSequence)};
    };

#pragma pack(push, 1)
    struct Header {
        static constexpr const uint32_t kMagic{0x041b00};

        uint32_t magic;
        uint32_t size;
        uint16_t version;
        uint16_t reserved16;
        uint32_t reserved32;

        Header()
            : magic(kMagic)
            , size{0}
            , version{0x0100}
            , reserved16{0}
            , reserved32{0}
        {}

        inline bool is_valid() const {
            return magic == kMagic;
        }
    };
#pragma pack(pop)

protected:
    Bytes data_;

#pragma pack(push, 1)
    struct ElemHeader {
        Tag tag;
        Count count; //!< For iterable objects like vector or list.
        Size size;   //!< Size of an element in bytes.

        ElemHeader()
            : tag{Tags::kUnknown}
            , count{0}
            , size{0}
        {}

        ElemHeader(Tag _tag, Count _count, Size _size)
            : tag{_tag}
            , count{_count}
            , size{_size}
        {}
    };
#pragma pack(pop)

protected:

    ElemHeader get_info(int)
        { return {Tags::kI32, 1, 4}; }
    ElemHeader get_info(unsigned int)
        { return {Tags::kI32, 1, 4}; }
    ElemHeader get_info(long)
        { return {Tags::kI32, 1, 4}; }
    ElemHeader get_info(unsigned long)
        { return {Tags::kI32, 1, 4}; }
    ElemHeader get_info(long long)
        { return {Tags::kI64, 1, 8}; }
    ElemHeader get_info(unsigned long long)
        { return {Tags::kI64, 1, 8}; }

    ElemHeader get_info(bool)
        { return {Tags::kI16, 1, sizeof(BoolSize)}; }

    ElemHeader get_info(float)
        { return {Tags::kF32, 1, 4}; }
    ElemHeader get_info(double)
        { return {Tags::kF64, 1, 8}; }

    ElemHeader get_info(const std::string& s)
        { return {Tags::kString, 1, static_cast<Size>(s.size())}; }
    ElemHeader get_info(const Bytes& s)
        { return {Tags::kBytes, 1, static_cast<Size>(s.size())}; }

public:

    NetData() {}
    virtual ~NetData() = default;

    void rewind() {
        data_.rewind();
    }

    size_t size() const {
        // Without Header.
        return data_.size();
    }

    size_t capacity() const {
        return data_.capacity();
    }

    void* data() {
        return data_.data();
    }

    const void* data() const {
        return data_.data();
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            STORE
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

    template <typename T>
    NetData& operator << (T val) {
        ElemHeader hdr = get_info(val);
        if( hdr.tag & Meta::kSequence ) {
            return write_sequence(&hdr, &val);
        }
        return write_scalar(&hdr, &val);
    }

    virtual NetData& write_scalar(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if( hdr->tag == Tags::kBool ) {
            const bool* val = static_cast<const bool*>(p);
            BoolSize b = *val ? 1 : 0;
            data_.write(&b, hdr->size);
        }
        else {
            data_.write(p, hdr->size);
        }
        return *this;
    }

    virtual NetData& write_sequence(ElemHeader* hdr, const void* p) {
        data_.write(hdr, sizeof(ElemHeader));
        if( hdr->tag == Tags::kString ) {
            const std::string* s = static_cast<const std::string*>(p);
            data_.write(s->data(), hdr->size);
        }
        else if( hdr->tag == Tags::kBytes ) {
            const Bytes* bs = static_cast<const Bytes*>(p);
            data_.write(bs->data(), hdr->size);
        }
        return *this;
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            LOAD
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

    NetData& operator >> (void* dst) {
        read(dst);
        return *this;
    }

    virtual void read(void* dst) {
        ElemHeader hdr;
        data_.read(&hdr, sizeof(ElemHeader));

        if( hdr.tag & Meta::kScalar) {
            if( hdr.tag & Meta::kSequence ) {
                read_sequence(&hdr, dst);
            }
            else {
                read_scalar(&hdr, dst);
            }
        }
    }

protected:

    virtual void read_scalar(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tags::kBool ) {
            BoolSize b{0};
            data_.read(&b, sizeof(b));
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
