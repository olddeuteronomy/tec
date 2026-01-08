// Time-stamp: <Last changed 2026-01-08 23:29:10 by magnolia>
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
 * @file tec_nd_compress.hpp
 * @brief NetData compression.
 * @author The Emacs Cat
 * @date 2025-12-23
 */

#pragma once

#include <cerrno>
#include <cstddef>

#if defined (_TEC_USE_ZLIB)
#include <zlib.h>
#endif

#include "tec/tec_def.hpp"   // IWYU pragma: keep
#include "tec/tec_trace.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/net/tec_compression.hpp"
#include "tec/net/tec_net_data.hpp"


namespace tec {


class NdCompress {
protected:
    int type_;
    int level_;
    size_t min_size_;

public:
    NdCompress()
        : type_{CompressionParams::kDefaultCompression}
        , level_{CompressionParams::kDefaultCompressionLevel}
        , min_size_{CompressionParams::kMinSize}
    {}

    explicit NdCompress(int _type,
                        int _level = CompressionParams::kDefaultCompressionLevel,
                        size_t _min_size = CompressionParams::kMinSize)
        : type_{_type}
        , level_{_level}
        , min_size_{_min_size}
    {}


    virtual Status compress(NetData& nd) const {
        TEC_ENTER("NdCompress::compress");
        TEC_TRACE("Type={} Level={} MinSize={}", type_, level_, min_size_);
#if defined (ZLIB_VERSION)
        if (type_ == CompressionParams::kCompressionZlib) {
            return compress_zlib(nd);
        }
#endif
        // No compression required.
        nd.header()->set_compression(CompressionParams::kNoCompression);
        return {};
    }


    virtual Status uncompress(NetData& nd) const {
        int compression = nd.header()->get_compression();
        if (compression == CompressionParams::kNoCompression) {
            // Nothing to do.
            return {};
        }
#if defined (ZLIB_VERSION)
        if (compression == CompressionParams::kCompressionZlib) {
            return uncompress_zlib(nd);
        }
#endif
        return {ENOTSUP, Error::Kind::Unsupported};
    }

protected:

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        ZLIB compression
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if defined (ZLIB_VERSION)

    // Compress NetData inplace.
    virtual Status compress_zlib(NetData& nd) const {
        TEC_ENTER("NdCompress::compress_zlib");
        if (nd.header()->size < min_size_) {
            // Data too small -- no compression required.
            nd.header()->set_compression(CompressionParams::kNoCompression);
            return {};
        }
        //
        // Calculate output size.
        //
        NetData::Header hdr = *nd.header();
        auto size_compressed = ::compressBound(hdr.size);
        if (size_compressed == 0) {
            return{EINVAL, Error::Kind::RuntimeErr};
        }
        //
        // Prepare temp NetData.
        //
        NetData tmp;
        NetData::Header* tmp_hdr = tmp.header();
        *tmp_hdr = hdr;
        tmp_hdr->size_uncompressed = hdr.size;
        tmp_hdr->set_compression(type_);
        tmp_hdr->set_compression_level(level_);
        // Prepare output buffer.
        tmp.bytes().resize(size_compressed);
        //
        // Compressing.
        //
        TEC_TRACE("Compressing {} bytes...", hdr.size);
        auto result = ::compress2(
            (Bytef*)tmp.bytes().ptr(0), &size_compressed,
            (const Bytef*)nd.bytes().ptr(0), nd.size(),
            level_);
        if (result != Z_OK) {
            return {EILSEQ, Error::Kind::RuntimeErr};
        }
        //
        // Update input NetData inplace.
        //
        tmp_hdr->size = size_compressed;
        nd.move_from(std::move(tmp), size_compressed);
        TEC_TRACE("Compressed to {} bytes with ratio {}.",
            size_compressed, (double)hdr.size / (double)size_compressed);
        return {};
    }


    // Uncompress NetData inplace.
    virtual Status uncompress_zlib(NetData& nd) const {
        TEC_ENTER("NdCompress::uncompress_zlib");
        NetData::Header hdr = *nd.header();
        if (hdr.get_compression() == CompressionParams::kNoCompression) {
            // No uncompression required.
            return {};
        }
        //
        // Prepare temp NetData.
        //
        NetData tmp;
        NetData::Header* tmp_hdr = tmp.header();
        *tmp_hdr = hdr;
        // Prepare output buffer.
        uLongf dest_len = hdr.size_uncompressed;
        tmp.bytes().resize(dest_len);
        //
        // Uncompressing.
        //
        TEC_TRACE("Uncompressing {} bytes...", hdr.size);
        auto result = ::uncompress(
            (Bytef*)tmp.bytes().ptr(0), &dest_len,
            (const Bytef*)nd.bytes().ptr(0), hdr.size);
        if (result != Z_OK) {
            return {EILSEQ, Error::Kind::RuntimeErr};
        }
        //
        // Update input NetData inplace.
        //
        tmp_hdr->size = dest_len;
        tmp_hdr->size_uncompressed = 0;
        nd.move_from(std::move(tmp));
        TEC_TRACE("Upcompressed to {} bytes.", dest_len);
        return {};
    }

#endif // defined(ZLIB_VERSION)
};

} // namespace tec
