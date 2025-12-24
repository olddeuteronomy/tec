// Time-stamp: <Last changed 2025-12-24 02:59:33 by magnolia>
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
#include <cstdint>
#include <zlib.h>

#include "tec/tec_status.hpp"
#include "tec/tec_trace.hpp"
#include "tec/net/tec_net_data.hpp"


namespace tec {


class NdCompress {
public:
    static constexpr uint32_t kMinSize{256};

protected:
    int type_;
    int level_;

public:
    NdCompress()
        : type_{NetData::Header::kDefaultCompression}
        , level_{NetData::Header::kDefaultCompressionLevel}
    {}

    explicit NdCompress(int _type, int _level=NetData::Header::kDefaultCompressionLevel)
        : type_{_type}
        , level_{_level}
    {}

    // Compress NetData inplace.
    virtual Status compress(NetData& nd) const {
        TEC_ENTER("NdCompress::compress");
        NetData::Header hdr = *nd.header();
        if (type_ == NetData::Header::kNoCompression  ||  hdr.size < kMinSize) {
            // No compression required.
            nd.header()->set_compression(NetData::Header::kNoCompression);
            return {};
        }
        //
        // Calculate output size.
        //
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
        nd.move_from(tmp, size_compressed);
        TEC_TRACE("Compressed to {} bytes with ratio {}.",
            size_compressed, (double)hdr.size / (double)size_compressed);
        return {};
    }


    // Uncompress NetData inplace.
    virtual Status uncompress(NetData& nd) const {
        NetData::Header hdr = *nd.header();
        if (hdr.get_compression() == NetData::Header::kNoCompression  ||  hdr.size_uncompressed == 0) {
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
        nd.move_from(tmp);
        return {};
    }
};

} // namespace tec
