/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamIoWriter.h"

#include <cstring>

void BitstreamIoWriter::ensure_capacity(size_t bits_needed) {
    size_t bytes_needed = (bit_position + bits_needed + 7) / 8;
    if (buffer.size() < bytes_needed) {
        buffer.resize(bytes_needed);
    }
}

uint64_t BitstreamIoWriter::signed_to_unsigned(int64_t v) {
    return (v <= 0) ? (-2 * v) : (2 * v - 1);
}

BitstreamIoWriter::BitstreamIoWriter(size_t capacity) : buffer(capacity) {}

void BitstreamIoWriter::write(bool v) {
    ensure_capacity(1);
    size_t byte_index = bit_position / 8;
    size_t bit_index = 7 - (bit_position % 8);
    
    if (v) {
        buffer[byte_index] |= (1 << bit_index);
    } else {
        buffer[byte_index] &= ~(1 << bit_index);
    }
    
    bit_position++;
}

void BitstreamIoWriter::write_bytes(const uint8_t* data, size_t count) {

    if (!data || count == 0) return;

    if (!is_aligned()) {
        for (size_t i = 0; i < count; ++i) {
            write_n<uint8_t>(data[i], 8);
        }
        return;
    }

    ensure_capacity(count * 8);
    const size_t byte_index = bit_position / 8;
    std::memcpy(buffer.data() + byte_index, data, count);
    bit_position += count * 8;
}

template<typename T>
void BitstreamIoWriter::write_n(T v, uint32_t n) {
    ensure_capacity(n);
    // Write bits in big-endian order (MSB first)
    for (int32_t i = n - 1; i >= 0; i--) {
        write((v >> i) & 1);
    }
}

template<typename T>
void BitstreamIoWriter::write_signed_n(T v, uint32_t n) {
    write_n(static_cast<typename std::make_unsigned<T>::type>(v), n);
}

void BitstreamIoWriter::write_ue(uint64_t v) {
    if (v == 0) {
        write(true);
        return;
    }
    uint64_t tmp = v + 1;
    int64_t leading_zeroes = -1;

    while (tmp > 0) {
        tmp >>= 1;
        leading_zeroes++;
    }

    for (int64_t i = 0; i < leading_zeroes; i++) {
        write(false);
    }
    write(true);

    uint64_t remaining = v + 1 - (1ULL << leading_zeroes);
    write_n(remaining, leading_zeroes);
}

void BitstreamIoWriter::write_se(int64_t v) {
    write_ue(signed_to_unsigned(v));
}

bool BitstreamIoWriter::is_aligned() const {
    return bit_position % 8 == 0;
}

void BitstreamIoWriter::byte_align() {
    if (!is_aligned()) {
        bit_position = (bit_position + 7) & ~7;
    }
}

const uint8_t* BitstreamIoWriter::as_slice() const {
    if (!is_aligned()) {
        return nullptr;
    }
    return buffer.data();
}

size_t BitstreamIoWriter::as_slice_size() const {
    return (bit_position + 7) / 8;
}

std::vector<uint8_t> BitstreamIoWriter::into_inner() {
    return std::move(buffer);
}

size_t BitstreamIoWriter::size() const {
    return (bit_position + 7) / 8;
}

// Template Explicit instantiations
template void BitstreamIoWriter::write_n<uint8_t>(uint8_t, uint32_t);
template void BitstreamIoWriter::write_n<uint16_t>(uint16_t, uint32_t);
template void BitstreamIoWriter::write_n<uint32_t>(uint32_t, uint32_t);

template void BitstreamIoWriter::write_signed_n<int16_t>(int16_t, uint32_t);