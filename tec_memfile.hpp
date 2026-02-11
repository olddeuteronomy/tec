// Time-stamp: <Last changed 2026-02-12 01:52:18 by magnolia>
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
 * @file tec_memfile.hpp
 * @brief A byte buffer class with stream-like read/write semantics.
 * @author The Emacs Cat
 * @date 2025-12-17
 */

#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <sstream>

#include <memory.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_trace.hpp"


namespace tec {


/**
 * @brief A byte buffer class with stream-like read/write semantics.
 *
  * The `MemFile` class provides a dynamically resizing buffer for sequential
 * reading and writing of elements of type `char`. It mimics the behavior of file
 * streams using `seek`, `tell`, `read`, and `write` operations. The buffer grows
 * in blocks of a configurable size to minimize reallocations.
 */
class MemFile {
public:
    struct _Char2 {
        char c0;
        char c1;
    };

    /**
     * @brief Converts a byte to a 2-character representation suitable for hex dumps.
     *
     * Returns a 2-element struct containing either:
     * - The two hexadecimal digits (uppercase) representing the byte value, or
     * - The literal ASCII character preceded by a space, **but only for printable ASCII characters**
     *   in the range 0x21–0x7E (`!` to `~`).
     *   Space (0x20) is deliberately treated as non-printable and rendered in hex.
     *
     * This format is commonly used in classic hex dump tools (e.g. `hexdump -C`, `xxd`, debug output)
     * where printable characters are shown directly and everything else in hex.
     *
     * @param value The byte to convert.
     *
     * @return struct _Char2
     *         A struct with two characters:
     *         - For printable ASCII (except space): `{ ' ', actual_char }`
     *         - For all other values (including space, control chars, and high bytes): `{ '0'..'F', '0'..'F' }`
     *
     * @par Examples
     * @code
     *     to_hex_chars(65);   // 'A'  → {' ', 'A'}
     *     to_hex_chars(32);   // ' '  → {'2', '0'}   (space is forced to hex)
     *     to_hex_chars(10);   // '\n' → {'0', 'A'}
     *     to_hex_chars(255);  // 0xFF → {'F', 'F'}
     * @endcode
     *
     * @note The function is `constexpr` and `inline`, making it usable in both
     *       runtime and compile-time contexts with zero overhead.
     *
     * @see as_hex()
     */
    inline static constexpr _Char2 to_hex_chars(unsigned char ch) noexcept {
        constexpr char table[] = "0123456789ABCDEF";
        if (0x20 < ch && ch < 0x7F) {
            return {' ', static_cast<char>(ch)};
        } else {
            return {table[ch >> 4], table[ch & 0x0F]};
        }
    }

public:

    /** @brief Default block size for buffer expansion, matches `BUFSIZ` from `<stdio.h>`.
        Usually 8192 bytes.
     */
    static constexpr size_t kDefaultBlockSize{BUFSIZ};

private:

    /** @brief Underlying storage for buffer elements. */
    std::string buffer_;

    /** @brief Size of each allocation block used when expanding the buffer. */
    size_t blk_size_;

    /** @brief Current read/write position within the buffer (like a file pointer). */
    long pos_;

private:

    /** @brief Calculates required capacity to hold extra `len` elemants starting from `pos`. */
    size_t calc_required_capacity(long pos, size_t len) const {
       return buffer_.capacity()
                + blk_size_
                + ((pos + len) / blk_size_) * blk_size_;
    }

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
    MemFile()
        : buffer_(kDefaultBlockSize, 0)
        , blk_size_{kDefaultBlockSize}
        , pos_{0}
    {
        buffer_.resize(0);
    }

    /**
     * @brief Constructs a MemFile with a specified block size for preallocation.
     *
     * Initializes the internal buffer with the given block size as capacity,
     * but sets the initial size to 0. This allows for efficient appending
     * without frequent reallocations.
     *
     * @param block_size The initial capacity and block size for allocation.
     */
    explicit MemFile(size_t block_size)
        : buffer_(block_size, 0)
        , blk_size_{block_size}
        , pos_{0}
    {
        buffer_.resize(0);
    }

    /**
     * @brief Constructs a MemFile from a std::string.
     *
     * Uses a default block size for allocation and writes the contents
     * of the provided string into the buffer.
     *
     * @param s The string to initialize the buffer with.
     */
    explicit MemFile(const std::string& s)
        : buffer_(kDefaultBlockSize, 0)
        , blk_size_{kDefaultBlockSize}
        , pos_{0}
    {
        buffer_.resize(0);
        write(s.data(), s.size());
    }

    /**
     * @brief Constructs a MemFile from a raw memory buffer.
     *
     * Uses a default block size for allocation and writes the specified
     * length of data from the source pointer into the buffer.
     *
     * @param src Pointer to the source data.
     * @param len Length of the data to copy.
     */
    MemFile(const void* src, size_t len)
        : buffer_(kDefaultBlockSize, 0)
        , blk_size_{kDefaultBlockSize}
        , pos_{0}
    {
        buffer_.resize(0);
        write(src, len);
    }

    virtual ~MemFile() = default;


    /**
     * @brief Copies data from another MemFile instance.
     *
     * Rewinds the current position and writes the entire contents
     * of the source MemFile into this one.
     *
     * @param src The source MemFile to copy from.
     */
    void copy_from(const MemFile& src) {
        rewind();
        write(src.data(), src.size());
    }

    /**
     * @brief Moves data from another MemFile instance.
     *
     * Transfers ownership of the buffer and state from the source,
     * optionally shrinking the buffer to a specified size if smaller
     * than the current size.
     *
     * @param src The rvalue reference to the source MemFile.
     * @param size_to_shrink Optional size to resize the buffer to (default 0, no shrink).
     */
    void move_from(MemFile&& src, size_t size_to_shrink=0) {
        blk_size_ = src.blk_size_;
        pos_ = src.pos_;
        buffer_ = std::move(src.buffer_);
        if (size_to_shrink > 0 && size_to_shrink < size()) {
            buffer_.resize(size_to_shrink);
        }
    }

    /**
     * @brief Returns a const reference to the internal string buffer.
     *
     * @return Const reference to the buffer.
     */
    const std::string& str() const noexcept {
        return buffer_;
    }

    /**
     * @brief Returns a const pointer to the buffer's data.
     *
     * Assumes the buffer is non-empty; uses at(0) to access, which may throw
     * if empty. This avoids certain compiler warnings related to empty strings.
     *
     * @return Const pointer to the data.
     */
    const void* data() const {
        // To prevent silly GCC warning?
        return &buffer_.at(0);
    }

    /**
     * @brief Returns a mutable pointer to the buffer's data.
     *
     * Assumes the buffer is non-empty; uses at(0) to access, which may throw
     * if empty. This avoids certain compiler warnings related to empty strings.
     *
     * @return Mutable pointer to the data.
     */
    void* data() {
        // To prevent silly GCC warning?
        return &buffer_.at(0);
    }

    /**
     * @brief Returns a const pointer to a specific position in the buffer.
     *
     * No bounds checking is performed; caller must ensure pos is valid.
     *
     * @param pos The position offset.
     * @return Const pointer to the position.
     */
    const char* ptr(long pos) const {
        return buffer_.data() + pos;
    }

    /**
     * @brief Returns a mutable pointer to a specific position in the buffer.
     *
     * No bounds checking is performed; caller must ensure pos is valid.
     *
     * @param pos The position offset.
     * @return Mutable pointer to the position.
     */
    char* ptr(long pos) {
        return buffer_.data() + pos;
    }

    /**
     * @brief Returns the block size used for buffer expansion.
     * @return The current block size in number of elements.
     */
    constexpr size_t block_size() const noexcept {
        return blk_size_;
    }

    /**
     * @brief Returns the logical size of the data in the buffer.
     *
     * This is the amount of data that has been written and is readable.
     *
     * @return Number of bytes currently stored.
     */
    size_t size() const noexcept {
        return buffer_.size();
    }

    /**
     * @brief Returns the current capacity of the underlying storage.
     * @return Total number of bytes the buffer can hold without reallocation.
     */
    size_t capacity() const noexcept {
        return buffer_.capacity();
    }

    /**
     * @brief Returns the current read/write position.
     * @return The current position as a signed offset from the start.
     */
    constexpr long tell() const noexcept {
        return pos_;
    }

    /**
     * @brief Resets the read/write position to the beginning of the buffer.
     */
    constexpr void rewind() noexcept {
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
     * @return A new read/write position, -1 if seeking before start, -2 if seeking beyond end.
     */
    long seek(long offset, int whence) {
        long origin = (whence == SEEK_CUR ? pos_ : (whence == SEEK_END ? size() : 0));
        size_t new_pos = origin + offset;
        if( new_pos < 0) {
            return -1;
        }
        if( new_pos > size() ) {
            return -2;
        }
        pos_ = new_pos;
        return pos_;
    }

    void resize(size_t len) {
        size_t new_size = pos_ + len;
        if (new_size > size()) {
            if (new_size > buffer_.capacity()) {
                TEC_ENTER("MemFile:resize");
                size_t new_cap = calc_required_capacity(pos_, new_size);
                TEC_TRACE("Cap: {}->{}, Size: {}->{}",
                          buffer_.capacity(), new_cap,
                          size(), new_size);
                buffer_.reserve(new_cap);
            }
            buffer_.resize(new_size);
        }
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
        if (pos_ + len > size()) {
            resize(len);
        }
        ::memcpy(buffer_.data() + pos_, src, len);
        pos_ += len;
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
     *         or if `len == 0` or no data.
     */
    size_t read(void* dst, size_t len) {
        if( len == 0 ) {
            return 0;
        }
        if( pos_ + len > size() ) {
            // Reading out of bound.
            return 0;
        }
        if (buffer_.empty()) {
            return 0;
        }
        ::memcpy(dst, buffer_.data() + pos_, len);
        pos_ += len;
        return len;
    }


    /**
     * @brief Returns a human-readable hex+ASCII dump string.
     *
     * Each byte is represented by exactly two characters:
     * - Printable ASCII characters in the range 0x21–0x7E (`!` to `~`)
     *   are shown as space + the character itself.
     * - All other bytes (including space 0x20, control characters, DEL,
     *   and values ≥ 0x7F) are shown as two uppercase hexadecimal digits.
     *
     * This format is extremely useful for debugging binary data,
     * network packets, or memory contents, because it makes text-like
     * regions immediately recognizable while still showing exact byte
     * values.
     *
     * @return std::string
     *         A string of length exactly `2 * size()`,
     *         containing the mixed hex/ASCII representation.
     *
     * @par Complexity
     *      O(N) where N = `size()`, with a very tight constant factor.
     *
     * @par Example
     * @code
     *     const char hello[] = "Hello\x01\x02World\xFF\0";
     *     MemFile data(hello, strlen(hello));
     *     // data contains: 48 65 6C 6C 6F 01 02 57 6F 72 6C 64 FF
     *     std::cout << data.as_hex() << '\n';
     *     // Output: " H e l l o010203 W o r l dFF"
     * @endcode
     *
     * @note Space (0x20) is deliberately rendered as hex "20", not as ' ' + ' ',
     *       which matches classic hex dump tools and avoids ambiguity.
     *
     * @see to_hex_chars()
     */
    std::string as_hex() const {
        std::ostringstream os;
        if (size() > 0) {
            for (unsigned char c: buffer_) {
                auto ch = to_hex_chars(c);
                os << ch.c0 << ch.c1;

            }
        }
        return os.str();
    }

};


/** An alias. */
using Blob = MemFile;

/** An alias. */
using Bytes = MemFile;


} // namespace tec
