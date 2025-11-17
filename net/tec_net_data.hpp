// Time-stamp: <Last changed 2025-11-17 23:50:08 by magnolia>
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
#include "tec/tec_trace.hpp"


namespace tec {


class NetData {
public:

    struct Meta {
        // First 8 bit (0..255) are reserved to hold an element type.
        static constexpr const uint32_t kScalar{(  1 << 8)};
        static constexpr const uint32_t kSequence{(1 << 9)};
        // Bits 10..31 are reserved.
    };

    struct Tag {
        static constexpr const uint32_t kUnknown{0};

        // Scalars.
        static constexpr const uint32_t kI32{(1 | Meta::kScalar)};
        static constexpr const uint32_t kU32{(2 | Meta::kScalar)};
        static constexpr const uint32_t kI64{(3 | Meta::kScalar)};
        static constexpr const uint32_t kU64{(4 | Meta::kScalar)};
        static constexpr const uint32_t kF32{(5 | Meta::kScalar)};
        static constexpr const uint32_t kF64{(6 | Meta::kScalar)};
        static constexpr const uint16_t kBool{(7 | Meta::kScalar)};

        // Sequences.
        static constexpr const uint32_t kBytes{(8 | Meta::kScalar | Meta::kSequence)};
        static constexpr const uint32_t kString{(9 | Meta::kScalar | Meta::kSequence)};
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
        uint32_t tag;
        uint32_t size;

        ElemHeader()
            : tag{Tag::kUnknown}
            , size{0}
        {}

        ElemHeader(uint32_t _tag, uint32_t _size)
            : tag{_tag}
            , size{_size}
        {}
    };
#pragma pack(pop)

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

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            STORE
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    NetData& operator << (int _src) {
        ElemHeader hdr{Tag::kI32, sizeof(int32_t)};
        int32_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (unsigned int _src) {
        ElemHeader hdr{Tag::kU32, sizeof(uint32_t)};
        uint32_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (long int _src) {
        ElemHeader hdr{Tag::kI32, sizeof(int32_t)};
        int32_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (unsigned long int _src) {
        ElemHeader hdr{Tag::kU32, sizeof(uint32_t)};
        uint32_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (long long int _src) {
        ElemHeader hdr{Tag::kI64, sizeof(int64_t)};
        int64_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (unsigned long long int _src) {
        ElemHeader hdr{Tag::kI64, sizeof(uint64_t)};
        uint64_t src = _src;
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (float _src) {
        ElemHeader hdr{Tag::kF32, sizeof(float)};
        return write_scalar(&hdr, &_src);
    };

    NetData& operator << (double _src) {
        ElemHeader hdr{Tag::kF64, sizeof(double)};
        return write_scalar(&hdr, &_src);
    };

    NetData& operator << (bool _src) {
        ElemHeader hdr{Tag::kBool, sizeof(uint16_t)};
        uint16_t src = (_src ? 1 : 0);
        return write_scalar(&hdr, &src);
    };

    NetData& operator << (const std::string& dst) {
        ElemHeader hdr{Tag::kString, static_cast<uint32_t>(dst.size())};
        return write_scalar_sequence(&hdr, dst.data());
    }

    NetData& operator << (Bytes& dst) {
        ElemHeader hdr{Tag::kBytes, static_cast<uint32_t>(dst.size())};
        return write_scalar_sequence(&hdr, dst.data());
    }

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                            LOAD
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    NetData& operator >> (void* dst) {
        return read_data(dst);
    }

protected:

    // --------------------------- write_* -----------------------------

    virtual NetData& write_scalar(const ElemHeader* hdr, const void* dst) {
        data_.write(hdr, sizeof(ElemHeader));
        data_.write(dst, hdr->size);
        return *this;
    }

    virtual NetData& write_scalar_sequence(const ElemHeader* hdr, const void* dst) {
        return write_scalar(hdr, dst);
    }

    // --------------------------- read_* -----------------------------

    virtual NetData& read_data(void* dst) {
        ElemHeader hdr;
        data_.read(&hdr, sizeof(ElemHeader));

        if( hdr.tag & Meta::kScalar) {
            if( hdr.tag & Meta::kSequence ) {
                return read_scalar_sequence(&hdr, dst);
            }
            else {
                return read_scalar(&hdr, dst);
            }
        }

        return read_custom(&hdr, dst);
    }

    virtual NetData& read_scalar(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tag::kBool ) {
            uint16_t b16{0};
            data_.read(&b16, sizeof(uint16_t));
            bool* val = static_cast<bool*>(dst);
            *val = b16;
        }
        else {
            data_.read(dst, hdr->size);
        }
        return *this;
    }

    virtual NetData& read_scalar_sequence(const ElemHeader* hdr, void* dst) {
        if( hdr->tag == Tag::kString ) {
            std::string* str = static_cast<std::string*>(dst);
            return read_string(hdr, str);
        }
        if( hdr->tag == Tag::kBytes ) {
            Bytes* bytes = static_cast<Bytes*>(dst);
            return read_bytes(hdr, bytes);
        }
        return *this;
    }

    virtual NetData& read_string(const ElemHeader* hdr, std::string* dst) {
        dst->resize(hdr->size);
        data_.read(dst->data(), hdr->size);
        return *this;
    }

    virtual NetData& read_bytes(const ElemHeader* hdr, Bytes* dst) {
        dst->resize(hdr->size);
        data_.read(dst->data(), hdr->size);
        return *this;
    }

    virtual NetData& read_custom(const ElemHeader* hdr, void* dst) {
        // Just skip unknown element.
        data_.seek(hdr->size, SEEK_SET);
        return *this;
    }
};

} // namespace tec
