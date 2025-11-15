// Time-stamp: <Last changed 2025-11-15 09:21:52 by magnolia>
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
 * @file tec_buffer.hpp
 * @brief A generic binary buffer class with stream-like read/write semantics.
 * @author The Emacs Cat
 * @date 2025-11-14
 */


#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <memory.h>

#include <vector>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @brief A generic binary buffer class with stream-like read/write semantics.
 *
 * The `Buffer<T>` class provides a dynamically resizing buffer for sequential
 * reading and writing of elements of type `T`. It mimics the behavior of file
 * streams using `seek`, `tell`, `read`, and `write` operations. The buffer grows
 * in blocks of a configurable size to minimize reallocations.
 *
 * @tparam T The element type stored in the buffer (typically `uint8_t` for bytes).
 */
template <typename T>
class Buffer {
public:
    /** @brief Default block size for buffer expansion, matches `BUFSIZ` from `<stdio.h>`. */
    static constexpr const size_t kDefaultBlockSize{BUFSIZ};

private:
    /** @brief Underlying storage for buffer elements. */
    std::vector<T> buffer_;

    /** @brief Size of each allocation block used when expanding the buffer. */
    size_t blk_size_;

    /** @brief Current read/write position within the buffer (like a file pointer). */
    long pos_;

    /** @brief Logical size of the data written into the buffer (may be less than capacity). */
    size_t size_;

public:
    /**
     * @brief Constructs a buffer with the specified block size.
     *
     * The initial capacity is set to one block. The buffer starts empty with
     * position at 0.
     *
     * @param block_size Size of each growth block. Must be greater than 0.
     *                   Defaults to `kDefaultBlockSize`.
     */
    explicit Buffer(size_t block_size = kDefaultBlockSize)
        : buffer_(block_size)
        , blk_size_{block_size}
        , pos_{0}
        , size_{0}
    {}

    virtual ~Buffer() = default;

    T* data() const {
        return buffer_.data();
    }

    /**
     * @brief Returns the block size used for buffer expansion.
     * @return The current block size in number of elements.
     */
    size_t block_size() const {
        return blk_size_;
    }

    /**
     * @brief Returns the logical size of the data in the buffer.
     *
     * This is the amount of data that has been written and is readable.
     * It may be less than `capacity()`.
     *
     * @return Number of elements currently stored.
     */
    size_t size() const {
        return size_;
    }

    /**
     * @brief Returns the current capacity of the underlying storage.
     * @return Total number of elements the buffer can hold without reallocation.
     */
    size_t capacity() const {
        return buffer_.capacity();
    }

    /**
     * @brief Returns the current read/write position.
     * @return The current position as a signed offset from the start.
     */
    long tell() const {
        return pos_;
    }

    /**
     * @brief Resets the read/write position to the beginning of the buffer.
     */
    void rewind() {
        pos_ = 0;
    }

    /**
     * @brief Moves the read/write position relative to a reference point.
     *
     * Behaves like `fseek`. Supports `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`.
     *
     * @param offset Number of elements to move (can be negative).
     * @param whence Reference point:
     *               - `SEEK_SET`: from beginning of buffer
     *               - `SEEK_CUR`: from current position
     *               - `SEEK_END`: from end of data
     * @return 0 on success, -1 if seeking before start, -2 if seeking beyond end.
     */
    int seek(long offset, int whence) {
        long origin = (whence == SEEK_CUR ? pos_ : (whence == SEEK_END ? size_ : 0));
        long new_pos = origin + offset;
        if( new_pos < 0) {
            return -1;
        }
        if( new_pos > size_) {
            return -2;
        }
        pos_ = new_pos;
        return 0;
    }

    /**
     * @brief Writes data into the buffer at the current position.
     *
     * The buffer automatically expands if necessary. Expansion occurs in
     * multiples of `blk_size_` to reduce reallocations.
     *
     * @param src Pointer to the source data.
     * @param len Number of elements to write.
     * @return Number of elements actually written. Returns 0 on failure
     *         (e.g., out of memory or `len == 0`).
     */
    size_t write(const void* src, size_t len) {
        if( len == 0 ) {
            return 0;
        }
        if( pos_ + len >= buffer_.capacity() ) {
            // Expand the buffer with extra blocks.
            size_t new_cap = buffer_.capacity()
                + blk_size_
                + ((pos_ + len) / blk_size_) * blk_size_;
            if( new_cap > buffer_.max_size() ) {
                return 0;
            }
            buffer_.reserve(new_cap);
        }
        memcpy(buffer_.data() + pos_, src, len * sizeof(T));
        pos_ += len;
        if( pos_ > size_ ) {
            size_ = pos_;
        }
        return len;
     }

    /**
     * @brief Reads data from the buffer starting at the current position.
     *
     * Does not read past the logical end of data (`size_`).
     *
     * @param dst Pointer to the destination buffer.
     * @param len Maximum number of elements to read.
     * @return Number of elements actually read. Returns 0 if at end of data
     *         or if `len == 0`.
     */
    size_t read(void* dst, size_t len) {
        if( len == 0 ) {
            return 0;
        }
        if( pos_ + len > size_ ) {
            // Reading out of bound.
            return 0;
        }
        memcpy(dst, buffer_.data() + pos_, len * sizeof(T));
        pos_ += len;
        return len;
    }
};


/** @brief Convenience alias for a byte-oriented buffer (`uint8_t`). */
using ByteBuffer = Buffer<uint8_t>;

} // namespace tec
