// Time-stamp: <Last changed 2025-11-17 02:20:16 by magnolia>
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

#include "tec/tec_buffer.hpp"


namespace tec {


class NetData {
public:
    struct Meta {
        // First 12 bit (<4096) are reserved to hold a primitive element size.
        static constexpr const uint32_t mINT{(   1 << 12)};
        static constexpr const uint32_t mFLOAT{( 1 << 13)};
        static constexpr const uint32_t mSIGNED{(1 << 14)};
        // Bits 15-23 are reserved.
        static constexpr const uint32_t mNUMERIC{(1 << 24)};
        static constexpr const uint32_t mSTRING{( 1 << 25)};
        static constexpr const uint32_t mBYTES{(  1 << 26)};
        // Bits 27-31 are reserved.
    };
    struct Tag {

        // Tags.
        static constexpr const uint32_t tUINT32{( 32 | Meta::mNUMERIC | Meta::mINT)};
        static constexpr const uint32_t tINT32{(  32 | Meta::mNUMERIC | Meta::mINT | Meta::mSIGNED)};
        static constexpr const uint32_t tUINT64{( 64 | Meta::mNUMERIC | Meta::mINT)};
        static constexpr const uint32_t tINT64{(  64 | Meta::mNUMERIC | Meta::mINT | Meta::mSIGNED)};
        static constexpr const uint32_t tFLOAT32{(32 | Meta::mNUMERIC | Meta::mFLOAT | Meta::mSIGNED)};
        static constexpr const uint32_t tFLOAT64{(64 | Meta::mNUMERIC | Meta::mFLOAT | Meta::mSIGNED)};

        static constexpr const uint32_t tSTRING{(1 | Meta::mSTRING)};
        static constexpr const uint32_t tBYTES{( 1 | Meta::mBYTES)};

        static constexpr const uint32_t tUNKNOWN{0};
    };

#pragma pack(push, 1)

    struct ElemHeader {
        uint32_t tag;
        uint32_t size;

        ElemHeader()
            : tag{Tag::tUNKNOWN}
            , size{0}
        {}

        ElemHeader(uint32_t _tag, uint32_t _size)
            : tag{_tag}
            , size{_size}
        {}
    };

#pragma pack(pop)

protected:
    Bytes data;

public:
    NetData() {}
    virtual ~NetData() = default;

    // -------------------- Write fundamental data ---------------------

    NetData& operator << (int32_t src) {
        ElemHeader hdr{Tag::tINT32, sizeof(int32_t)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (uint32_t src) {
        ElemHeader hdr{Tag::tUINT32, sizeof(uint32_t)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (int64_t src) {
        ElemHeader hdr{Tag::tINT64, sizeof(int64_t)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (uint64_t src) {
        ElemHeader hdr{Tag::tUINT64, sizeof(uint64_t)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (float src) {
        ElemHeader hdr{Tag::tFLOAT32, sizeof(float)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (double src) {
        ElemHeader hdr{Tag::tFLOAT64, sizeof(double)};
        return write_numeric(&hdr, &src);
    };

    NetData& operator << (const std::string& dst) {
        ElemHeader hdr{Tag::tSTRING, static_cast<uint32_t>(dst.size())};
        return *this;
}

    // ------------------------ Read data ------------------------------

    NetData operator >> (void* dst) {
        ElemHeader hdr;
        data.read(&hdr, sizeof(ElemHeader));
        if( hdr.tag & Meta::mNUMERIC) {
            return read_numeric(&hdr, dst);
        }
        if( hdr.tag & Meta::mSTRING ) {
            return read_string(&hdr, static_cast<std::string*>(dst));
        }
        if( hdr.tag & Meta::mBYTES) {
            return read_bytes(&hdr, static_cast<Bytes*>(dst));
        }

        return read_extended(&hdr, dst);
    }

    // --------------------------- Helpers -----------------------------

protected:
    virtual NetData& write_numeric(const ElemHeader* hdr, const void* dst) {
        data.write(hdr, sizeof(ElemHeader));
        data.write(dst, hdr->size);
        return *this;
    }

    virtual NetData& write_string(const ElemHeader* hdr, const std::string& dst) {
        data.write(hdr, sizeof(ElemHeader));
        data.write(dst.data(), hdr->size);
        return *this;
    }

    virtual NetData& read_numeric(const ElemHeader* hdr, void* dst) {
        data.read(dst, hdr->size);
        return *this;
    }

    virtual NetData& read_string(const ElemHeader* hdr, std::string* dst) {
        dst->resize(hdr->size);
        data.read(dst, hdr->size);
        return *this;
    }

    virtual NetData& read_bytes(const ElemHeader* hdr, Bytes* dst) {
        dst->resize(hdr->size);
        data.read(dst, hdr->size);
        return *this;
    }

    virtual NetData& read_extended(const ElemHeader* hdr, void* dst) {
        // Just skip unknown element.
        data.seek(hdr->size, SEEK_SET);
        return *this;
    }
};

} // namespace tec
