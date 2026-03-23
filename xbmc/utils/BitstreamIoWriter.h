/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <type_traits>

class BitstreamIoWriter {

public:
    explicit BitstreamIoWriter(size_t capacity = 0);

    void write(bool v);
    void write_bytes(const uint8_t* data, size_t count);

    template<typename T>
    void write_n(T v, uint32_t n);

    template<typename T>
    void write_signed_n(T v, uint32_t n);

    void write_ue(uint64_t v);
    void write_se(int64_t v);

    bool is_aligned() const;
    void byte_align();

    const uint8_t* as_slice() const;
    size_t as_slice_size() const;

    std::vector<uint8_t> into_inner();

    size_t size() const;

private:
    std::vector<uint8_t> buffer;
    size_t bit_position = 0;

    void ensure_capacity(size_t bits_needed);
    uint64_t signed_to_unsigned(int64_t v);
};
