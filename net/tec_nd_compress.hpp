// Time-stamp: <Last changed 2026-01-23 14:22:02 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 * @brief Compression wrapper for NetData objects with pluggable backends.
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

/**
 * @brief Compression wrapper for NetData objects with pluggable backends.
 *
 * Currently supports only **zlib** (deflate) when `_TEC_USE_ZLIB` / `ZLIB_VERSION` is defined.
 * Designed to be used as a strategy object — either as a member or passed by const reference.
 *
 * Main features:
 *   - Configurable compression type, level, and minimum size threshold
 *   - In-place modification of `NetData` (moves internal buffer when compression occurs)
 *   - Skips compression for small payloads (controlled by `min_size_`)
 *   - Sets appropriate fields in `NetData::header` (compression type, level, uncompressed size)
 *
 * @note All public methods are `const` — the object is immutable after construction.
 * @note Currently only zlib is implemented; other algorithms can be added by extending
 *       the `compress()` / `uncompress()` dispatch logic.
 *
 * **Basic usage examples**
 * @code
 * // 1. Default (usually zlib or no-op depending on build)
 * NdCompress compressor;
 * NetData msg = ...;           // filled message
 * compressor.compress(msg);    // may compress in-place
 *
 * // Later on receiver side
 * compressor.uncompress(msg);  // restores original if was compressed
 *
 * // 2. Explicit zlib with custom level
 * NdCompress fast_compressor(
 *     CompressionParams::kCompressionZlib,
 *     3,                           // fast compression
 *     512                          // don't compress < 512 bytes
 * );
 *
 * NetData large_payload = create_large_payload();
 * auto st = fast_compressor.compress(large_payload);
 * if (st) {
 *     send_over_network(large_payload);
 * }
 * @endcode
 */
class NdCompress {
protected:
    int type_; ///< Compression algorithm identifier (from `CompressionParams`)
    int level_; ///< Compression level (meaning depends on backend — usually -1..9 for zlib)
    size_t min_size_; ///< Minimum payload size (in bytes) below which compression is skipped

public:
    /**
     * @brief Default constructor — uses library-wide default compression settings
     *
     * Initializes with:
     *   - type  = `CompressionParams::kDefaultCompression`
     *   - level = `CompressionParams::kDefaultCompressionLevel`
     *   - min_size = `CompressionParams::kMinSize`
     *
     * @code
     * NdCompress compressor;  // most common usage
     * @endcode
     */
    NdCompress()
        : type_{CompressionParams::kDefaultCompression}
        , level_{CompressionParams::kDefaultCompressionLevel}
        , min_size_{CompressionParams::kMinSize}
    {}

    /**
     * @brief Constructs compressor with explicit settings
     * @param _type      Compression algorithm (e.g. `CompressionParams::kCompressionZlib`)
     * @param _level     Compression level (backend-specific; default = library default)
     * @param _min_size  Smallest payload size worth compressing (default = library minimum)
     *
     * @code
     * // Aggressive compression for archival/large messages
     * NdCompress archiver(CompressionParams::kCompressionZlib, 9, 1024);
     *
     * // Very fast compression, skip tiny messages
     * NdCompress low_latency(CompressionParams::kCompressionZlib, 1, 256);
     * @endcode
     */
    explicit NdCompress(int _type,
                        int _level = CompressionParams::kDefaultCompressionLevel,
                        size_t _min_size = CompressionParams::kMinSize)
        : type_{_type}
        , level_{_level}
        , min_size_{_min_size}
    {}

    /**
     * @brief Compresses the payload of a `NetData` object in-place (if configured)
     * @param nd  Network data object to compress (modified in-place on success)
     * @return `Status::OK` on success, error otherwise
     *
     * Behavior:
     *   - If payload size < `min_size_`, sets compression = `kNoCompression` and returns
     *   - Otherwise delegates to backend-specific implementation (currently only zlib)
     *   - On success: updates header fields (`compression`, `compression_level`, `size_uncompressed`)
     *   - On failure: `nd` remains unchanged
     *
     * @code
     * NdCompress compressor(CompressionParams::kCompressionZlib, 6);
     * NetData frame = build_video_frame();
     *
     * Status st = compressor.compress(frame);
     * if (!st) {
     *     LOG(ERROR) << "Compression failed: " << st.message();
     *     return;
     * }
     *
     * // frame is now potentially smaller + header updated
     * network.send(frame);
     * @endcode
     */
    virtual Status compress(NetData& nd) const {
        TEC_ENTER("NdCompress::compress");
#if defined (ZLIB_VERSION)
        if (type_ == CompressionParams::kCompressionZlib) {
            TEC_TRACE("Type={} Level={} MinSize={}", type_, level_, min_size_);
            return compress_zlib(nd);
        }
#endif
        // No compression required.
        TEC_TRACE("OFF");
        nd.header.set_compression(CompressionParams::kNoCompression);
        return {};
    }

    /**
     * @brief Decompresses the payload of a `NetData` object in-place (if compressed)
     * @param nd  Network data object to decompress (modified in-place on success)
     * @return `Status::OK` on success, error otherwise
     *
     * Behavior:
     *   - If header indicates `kNoCompression` → returns immediately (no-op)
     *   - Otherwise delegates to backend-specific decompressor
     *   - On success: clears `size_uncompressed` and updates `size`
     *   - On failure: `nd` remains unchanged
     *
     * @code
     * // Receiver side
     * NdCompress compressor;  // same settings as sender (or just default)
     * NetData received;
     * network.receive(received);
     *
     * Status st = compressor.uncompress(received);
     * if (!st) {
     *     LOG(ERROR) << "Decompression failed: " << st.message();
     *     return;
     * }
     *
     * // received.payload() now contains original uncompressed data
     * process_message(received);
     * @endcode
     */
    virtual Status uncompress(NetData& nd) const {
        int compression = nd.header.get_compression();
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

    /**
     * @brief zlib-specific compression implementation (in-place)
     * @param nd  `NetData` object to compress (modified on success)
     * @return `Status::OK` or detailed zlib error
     *
     * Skips compression if payload is smaller than `min_size_`.
     * Uses `compress2()` with the configured level.
     * Allocates temporary buffer sized via `compressBound()`.
     * Moves result into original `nd` on success.
     */
    virtual Status compress_zlib(NetData& nd) const {
        TEC_ENTER("NdCompress::compress_zlib");
        if (nd.header.size < min_size_) {
            // Data too small -- no compression required.
            nd.header.set_compression(CompressionParams::kNoCompression);
            return {};
        }
        //
        // Calculate output size.
        //
        auto size_compressed = ::compressBound(nd.header.size);
        if (size_compressed == 0) {
            return{EINVAL, Error::Kind::RuntimeErr};
        }
        //
        // Prepare temp NetData.
        //
        NetData tmp;
        tmp.header = nd.header;
        tmp.header.size_uncompressed = nd.header.size;
        tmp.header.set_compression(type_);
        tmp.header.set_compression_level(level_);
        // Prepare output buffer.
        tmp.bytes().resize(size_compressed);
        //
        // Compressing.
        //
        TEC_TRACE("Compressing {} bytes...", nd.header.size);
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
        tmp.header.size = size_compressed;
        nd.move_from(std::move(tmp), size_compressed);
        TEC_TRACE("Compressed to {} bytes with ratio {}.",
            size_compressed,
                  (double)nd.header.size_uncompressed / (double)nd.header.size);
        return {};
    }

    /**
     * @brief zlib-specific decompression implementation (in-place)
     * @param nd  `NetData` object to decompress (modified on success)
     * @return `Status::OK` or detailed zlib error
     *
     * Uses pre-stored `size_uncompressed` value from header to size output buffer.
     * Calls `uncompress()` and moves result back into original `nd`.
     */
    virtual Status uncompress_zlib(NetData& nd) const {
        TEC_ENTER("NdCompress::uncompress_zlib");
        if (nd.header.get_compression() == CompressionParams::kNoCompression) {
            // No uncompression required.
            return {};
        }
        //
        // Prepare temp NetData.
        //
        NetData tmp;
        tmp.header = nd.header;
        // Prepare output buffer.
        uLongf dest_len = nd.header.size_uncompressed;
        tmp.bytes().resize(dest_len);
        //
        // Uncompressing.
        //
        TEC_TRACE("Uncompressing {} bytes...", nd.size());
        auto result = ::uncompress(
            (Bytef*)tmp.data(), &dest_len,
            (const Bytef*)nd.data(), nd.size());
        if (result != Z_OK) {
            return {EILSEQ, Error::Kind::RuntimeErr};
        }
        //
        // Update input NetData inplace.
        //
        tmp.header.size = dest_len;
        tmp.header.size_uncompressed = 0;
        nd.move_from(std::move(tmp));
        TEC_TRACE("Upcompressed to {} bytes.", nd.size());
        return {};
    }

#endif // defined(ZLIB_VERSION)
};

} // namespace tec
