/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/VideoHdrBitstream.h"
#include "utils/StreamDetails.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavcodec/packet.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mem.h>
}

using namespace KODI::UTILS::DOLBYVISION;
using State = CVideoHdrBitstream::State;

namespace
{
// Units of real streams, from FFmpeg's FATE samples (https://fate-suite.ffmpeg.org) and from
// commercial releases. A slice is cut to its first bytes, as only its NAL unit header is read.

// clang-format off
// FATE's hevc/hdr10_plus_h265_sample.hevc: a prefix SEI with HDR10+ metadata (SMPTE ST 2094-40),
// and its picture's slice
const std::vector<uint8_t> HDR10PLUS_SEI = {
    0x4E, 0x01, 0x04, 0x40, 0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x40,
    0x00, 0x0C, 0x80, 0x1A, 0x18, 0x0B, 0x36, 0x05, 0x50, 0x00, 0x7F, 0xA4,
    0x08, 0x00, 0x78, 0x28, 0x2D, 0xF0, 0x50, 0x03, 0xFC, 0xC8, 0x01, 0x19,
    0x90, 0x14, 0xF2, 0x58, 0x19, 0x02, 0xD0, 0x1C, 0xEA, 0xF8, 0x1E, 0x7B,
    0x18, 0x2D, 0xF0, 0x01, 0x40, 0x00, 0x00, 0x24, 0x66, 0x33, 0x53, 0x36,
    0x6A, 0x00, 0x99, 0xAC, 0xDC, 0xCF, 0x9A, 0x40, 0x80,
};
const std::vector<uint8_t> HDR10PLUS_IDR{0x26, 0x01, 0xAF, 0x13};

// FATE's hevc/hdr_vivid_h265_sample.hevc: a prefix SEI with HDR Vivid metadata, ITU-T T.35 too
const std::vector<uint8_t> HDR_VIVID_SEI = {
    0x4E, 0x01, 0x04, 0x2F, 0x26, 0x00, 0x04, 0x00, 0x05, 0x01, 0x00, 0x0B,
    0xE6, 0x5F, 0xFF, 0xFF, 0xEB, 0x4A, 0xB3, 0x33, 0x11, 0x98, 0x00, 0x52,
    0x88, 0x01, 0x1C, 0x00, 0x00, 0xFF, 0xBF, 0xEF, 0xF0, 0x41, 0x59, 0x99,
    0x88, 0xCC, 0x00, 0x29, 0x44, 0x00, 0x8E, 0x00, 0x00, 0x7F, 0xDF, 0xF7,
    0xFA, 0x26, 0x19, 0x80,
};

// FATE's mkv/hdr10tags-both.mkv, HDR10: its hvcC record, with parameter sets and a mastering
// display SEI, and the slices of an IDR picture and a trailing one
const std::vector<uint8_t> HDR10_HVCC = {
    0x01, 0x02, 0x20, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5D, 0xF0, 0x00, 0xFF, 0xFD, 0xFA, 0xFA, 0x00, 0x00, 0x0F, 0x04, 0xA0,
    0x00, 0x01, 0x00, 0x19, 0x40, 0x01, 0x0C, 0x01, 0xFF, 0xFF, 0x02, 0x20,
    0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
    0x5D, 0x95, 0x8A, 0x02, 0x40, 0xA1, 0x00, 0x01, 0x00, 0x2C, 0x42, 0x01,
    0x01, 0x02, 0x20, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x03, 0x00, 0x5D, 0xA0, 0x04, 0x62, 0x01, 0x41, 0x36, 0x59, 0x58,
    0xA9, 0x24, 0x49, 0xAE, 0x6A, 0x12, 0x20, 0x12, 0x08, 0x00, 0x00, 0x1F,
    0x48, 0x00, 0x03, 0xA9, 0x80, 0x40, 0xA2, 0x00, 0x01, 0x00, 0x07, 0x44,
    0x01, 0xC1, 0x72, 0xB6, 0x62, 0x40, 0x27, 0x00, 0x01, 0x00, 0x1E, 0x4E,
    0x01, 0x89, 0x18, 0x21, 0x34, 0x9B, 0xAA, 0x19, 0x96, 0x08, 0xFC, 0x8A,
    0x48, 0x39, 0x07, 0x3D, 0x12, 0x40, 0x42, 0x00, 0x98, 0x96, 0x80, 0x00,
    0x00, 0x03, 0x00, 0x0A, 0x80,
};
const std::vector<uint8_t> HDR10_IDR{0x28, 0x01, 0xAF, 0xCA};
const std::vector<uint8_t> HDR10_TRAIL{0x02, 0x01, 0xD0, 0x31};

// FATE's mov/dovi-p81.mp4 and mov/dovi-p7.mp4, Dolby Vision profiles 8.1 and 7 of the same
// pictures: the hvcC record of the first, which gives NAL units four byte sizes and holds no
// parameter sets, and the slices of an IDR picture and a trailing one
const std::vector<uint8_t> HVCC = {
    0x01, 0x02, 0x20, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x78, 0xF0, 0x00, 0xFC, 0xFD, 0xFA, 0xFA, 0x00, 0x00, 0x0F, 0x00,
};
const std::vector<uint8_t> DOVI_IDR{0x28, 0x01, 0xAC, 0x19};
const std::vector<uint8_t> DOVI_TRAIL{0x02, 0x01, 0xD0, 0x21};

// The RPU of every picture in the enhancement layer track of mov/dovi-p7.mp4, a FEL
const std::vector<uint8_t> FEL_RPU = {
    0x7C, 0x01, 0x19, 0x08, 0x09, 0x08, 0x40, 0x61, 0x36, 0x50, 0xAE, 0x20,
    0x00, 0x20, 0x08, 0x02, 0x00, 0x80, 0x20, 0x08, 0x02, 0x00, 0x7F, 0x80,
    0x1F, 0xFC, 0x00, 0xFF, 0xC0, 0x01, 0xFF, 0xFA, 0x00, 0x00, 0x03, 0x01,
    0x00, 0x00, 0x03, 0x00, 0xD0, 0x00, 0x00, 0x08, 0x00, 0x00, 0x06, 0x80,
    0x00, 0x00, 0x40, 0x00, 0x00, 0x34, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00,
    0x03, 0x01, 0xA0, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x03,
    0x00, 0x80, 0x00, 0x00, 0x68, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0x03,
    0x40, 0x00, 0x00, 0x20, 0x00, 0x00, 0x0A, 0x86, 0xD9, 0xE6, 0x7B, 0x2F,
    0x86, 0x3E, 0xFC, 0x18, 0x9E, 0x60, 0x29, 0xE9, 0x8C, 0x06, 0xB8, 0xC5,
    0x66, 0xA7, 0xCD, 0x5F, 0x8D, 0xFF, 0xCB, 0x6D, 0x8C, 0x39, 0xE3, 0x8A,
    0x99, 0x81, 0xB8, 0x6C, 0xA6, 0x79, 0x0F, 0x40, 0x5F, 0x43, 0x4C, 0xB7,
    0xD8, 0x0E, 0x9E, 0x04, 0xC3, 0x49, 0xFD, 0x3B, 0xB7, 0x7E, 0xC2, 0xCD,
    0x5D, 0xE3, 0xA1, 0x47, 0xD4, 0x4C, 0x0A, 0x2E, 0x80, 0x62, 0x16, 0x4A,
    0x02, 0x9B, 0x5D, 0x59, 0x50, 0x0E, 0xD3, 0x8F, 0x58, 0xC4, 0x6C, 0x4E,
    0xC1, 0x3A, 0x14, 0x58, 0x2C, 0xFF, 0x30, 0x23, 0x53, 0x6D, 0x57, 0xBB,
    0xB6, 0xE4, 0x11, 0x48, 0x3B, 0x92, 0x96, 0x64, 0x13, 0x69, 0x8F, 0xCE,
    0x6B, 0xC3, 0x1B, 0xD6, 0x20, 0xFA, 0xDD, 0x40, 0x5B, 0xBE, 0xF4, 0x64,
    0x31, 0x60, 0xB1, 0xF0, 0x1B, 0xF2, 0xEE, 0xFB, 0xF2, 0xA5, 0xE2, 0xF3,
    0xCC, 0xB4, 0xAB, 0xF3, 0xCC, 0x39, 0x83, 0x77, 0x9E, 0x75, 0x68, 0x31,
    0x1B, 0x00, 0x48, 0x00, 0x00, 0x40, 0x04, 0x00, 0x40, 0x00, 0x00, 0x40,
    0x12, 0x00, 0x00, 0x10, 0x01, 0x00, 0x10, 0x00, 0x00, 0x10, 0x04, 0x80,
    0x00, 0x04, 0x00, 0x40, 0x04, 0x00, 0x00, 0x07, 0x25, 0x66, 0x00, 0x00,
    0x35, 0xEA, 0x25, 0x66, 0xF9, 0xFC, 0xEB, 0x1C, 0x25, 0x66, 0x44, 0xCA,
    0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x03,
    0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x1C, 0x36, 0x22, 0x43, 0x01, 0x86,
    0x0A, 0x5E, 0x30, 0x8E, 0x05, 0x14, 0x00, 0x00, 0x03, 0x01, 0xA6, 0x3E,
    0x5A, 0xFF, 0xFF, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x60, 0x20, 0x0F, 0x80, 0xE1, 0x51, 0x80, 0x30, 0x08, 0x00,
    0x59, 0xCA, 0x12, 0x00, 0xC0, 0x28, 0x21, 0x8D, 0xF8, 0x25, 0x80, 0x08,
    0x00, 0x61, 0x40, 0x00, 0x02, 0x02, 0x2A, 0x51, 0x20, 0x88, 0x05, 0x00,
    0x00, 0x03, 0x00, 0x02, 0x28, 0x11, 0x50, 0x12, 0x0C, 0x07, 0xD0, 0x00,
    0x02, 0x0D, 0x60, 0x01, 0x5E, 0xB8, 0xF2, 0x9D, 0x82, 0x80, 0x00,
};

// FATE's mkv/hdr10_plus_vp9_sample.webm: the start of its VP9 frame, and its BlockAdditional, the
// HDR10+ metadata as an ITU-T T.35 message
const std::vector<uint8_t> VP9_FRAME{0x92, 0x49, 0x83, 0x42};
const std::vector<uint8_t> VP9_BLOCK_ADDITIONAL = {
    0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x40, 0x00, 0x0C, 0x80, 0x1A,
    0x18, 0x0B, 0x36, 0x05, 0x50, 0x00, 0x7F, 0xA4, 0x08, 0x00, 0x78, 0x28,
    0x2D, 0xF0, 0x50, 0x03, 0xFC, 0xC8, 0x01, 0x19, 0x90, 0x14, 0xF2, 0x58,
    0x19, 0x02, 0xD0, 0x1C, 0xEA, 0xF8, 0x1E, 0x7B, 0x18, 0x2D, 0xF0, 0x01,
    0x40, 0x00, 0x00, 0x24, 0x66, 0x33, 0x53, 0x36, 0x6A, 0x00, 0x99, 0xAC,
    0xDC, 0xCF, 0x9A, 0x00,
};

// An HDR10+ WEB-DL: the SEI of its pictures, and the slices of an IDR picture and a trailing one
const std::vector<uint8_t> WEB_HDR10PLUS_SEI = {
    0x4E, 0x01, 0x04, 0x40, 0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x40,
    0x00, 0x0C, 0x81, 0x2F, 0x30, 0xA4, 0x48, 0x6D, 0xBF, 0x80, 0x5C, 0xA4,
    0x08, 0x00, 0x00, 0x28, 0xA1, 0xF0, 0x50, 0x01, 0x68, 0xC8, 0x00, 0x01,
    0x90, 0x02, 0x52, 0x58, 0x07, 0x4A, 0xD0, 0x0C, 0xE6, 0xF8, 0x11, 0x3F,
    0x18, 0xA0, 0x50, 0x00, 0x40, 0x68, 0x10, 0xE4, 0xDB, 0x7F, 0x2F, 0x5C,
    0xA7, 0x52, 0xDC, 0xF9, 0x0E, 0xA3, 0xBA, 0x00, 0x80,
};
const std::vector<uint8_t> WEB_IDR{0x28, 0x01, 0xAC, 0x2B};
const std::vector<uint8_t> WEB_TRAIL{0x02, 0x01, 0xD0, 0x29};

// A WEB-DL with Dolby Vision profile 8.1 and HDR10+: the HDR10+ SEI and the slice of its first
// picture
const std::vector<uint8_t> HYBRID_HDR10PLUS_SEI = {
    0x4E, 0x01, 0x04, 0x40, 0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x40,
    0x00, 0x0F, 0xA0, 0x89, 0x40, 0x25, 0x4F, 0x12, 0xA7, 0x80, 0x09, 0x64,
    0x08, 0x00, 0x00, 0x28, 0x4B, 0xEC, 0x50, 0x01, 0x8C, 0xC8, 0x00, 0x01,
    0x90, 0x00, 0x02, 0x58, 0x00, 0x02, 0xD0, 0x00, 0x02, 0xF8, 0x05, 0x77,
    0x18, 0x45, 0x58, 0x00, 0x40, 0x00, 0x00, 0x24, 0x66, 0x33, 0x53, 0x36,
    0x6A, 0x00, 0x99, 0xAC, 0xDC, 0xCF, 0x9A, 0x00, 0x80,
};
const std::vector<uint8_t> HYBRID_IDR{0x26, 0x01, 0xAC, 0x3B};

// A UHD Blu-ray with Dolby Vision profile 7 in one track, where a probe seeks: the slices of a CRA
// picture, of a leading picture a decoder skips after the seek and of a trailing picture, each with
// a unit of the enhancement layer after it, and the CRA picture's RPU, whose layer is a MEL
const std::vector<uint8_t> MEL_CRA{0x2A, 0x01, 0xAC, 0x01};
const std::vector<uint8_t> MEL_CRA_EL{0x7E, 0x01, 0x2A, 0x01};
const std::vector<uint8_t> MEL_RASL{0x10, 0x01, 0xE0, 0x0C};
const std::vector<uint8_t> MEL_RASL_EL{0x7E, 0x01, 0x10, 0x01};
const std::vector<uint8_t> MEL_TRAIL{0x02, 0x01, 0xD0, 0x03};
const std::vector<uint8_t> MEL_TRAIL_EL{0x7E, 0x01, 0x02, 0x01};
const std::vector<uint8_t> MEL_RPU = {
    0x7C, 0x01, 0x19, 0x08, 0x09, 0x08, 0x40, 0x61, 0x36, 0x50, 0xAF, 0x00,
    0x3F, 0xF8, 0x01, 0xFF, 0xC0, 0x0F, 0xFC, 0x00, 0x1F, 0xFF, 0xA0, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x03, 0x00, 0x80, 0x00, 0x00,
    0x68, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x03,
    0x02, 0x00, 0x00, 0x03, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x39, 0x2B, 0x30, 0x00, 0x01,
    0xAF, 0x51, 0x2B, 0x37, 0xCF, 0xE7, 0x58, 0xE1, 0x2B, 0x32, 0x26, 0x50,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x40, 0x00, 0x00, 0x03, 0x00,
    0x40, 0x00, 0x00, 0x03, 0x00, 0xE1, 0xB1, 0x12, 0x18, 0x0C, 0x30, 0x52,
    0xF1, 0x84, 0x70, 0x28, 0xA0, 0x00, 0x00, 0x0D, 0x31, 0xF2, 0xD7, 0xFF,
    0xF8, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x03,
    0x01, 0x00, 0x7C, 0x07, 0x0A, 0x84, 0x00, 0x30, 0x08, 0x00, 0x42, 0x13,
    0x02, 0x00, 0xC0, 0x28, 0x21, 0x72, 0x68, 0x01, 0x4F, 0x58, 0x00, 0x80,
    0x04, 0x00, 0x01, 0x80, 0x56, 0x46, 0xFF, 0xD0, 0x20, 0xEE, 0x30, 0x01,
    0x00, 0x08, 0x00, 0x03, 0x00, 0xB0, 0x1E, 0x00, 0xA0, 0x01, 0xFF, 0xA0,
    0x02, 0x00, 0x10, 0x00, 0x08, 0x08, 0xBE, 0x41, 0xDC, 0x20, 0x14, 0x00,
    0x00, 0x03, 0x00, 0x08, 0xA0, 0x45, 0x40, 0x48, 0x30, 0x1F, 0x40, 0x00,
    0x08, 0x1C, 0xD0, 0x19, 0x48, 0x07, 0xD2, 0xB0, 0x9E, 0x80,
};

// AV1 that libaom encoded from the HDR10+ WEB-DL above, in Matroska: its sequence header OBU, the
// metadata OBU with the HDR10+ metadata of every temporal unit, and frame OBUs cut to two bytes of
// their payload, with sizes to match. The first unit has a key frame; the next ones have frames
// that aren't shown, then the metadata, then the frame they show.
const std::vector<uint8_t> AV1_SEQUENCE_HEADER = {
    0x0A, 0x0E, 0x00, 0x00, 0x00, 0x0C, 0xC4, 0xFF, 0x09, 0x36, 0xBE, 0x54,
    0x24, 0x40, 0x24, 0x10,
};
const std::vector<uint8_t> AV1_HDR10PLUS = {
    0x2A, 0x42, 0x04, 0xB5, 0x00, 0x3C, 0x00, 0x01, 0x04, 0x01, 0x40, 0x00,
    0x0C, 0x80, 0x66, 0xE0, 0x2A, 0x38, 0x14, 0x2C, 0x00, 0x0A, 0x24, 0x08,
    0x00, 0x00, 0x28, 0x9F, 0xA4, 0x50, 0x01, 0x90, 0xC8, 0x00, 0x01, 0x90,
    0x00, 0x6A, 0x58, 0x00, 0xFA, 0xD0, 0x01, 0x8E, 0xF8, 0x01, 0xF7, 0x18,
    0x9D, 0x40, 0x00, 0x40, 0x74, 0x12, 0xA4, 0xE4, 0x7C, 0x6E, 0x6C, 0xAF,
    0x57, 0xDE, 0xF9, 0x8E, 0xBF, 0xBC, 0x00, 0x80,
};
const std::vector<uint8_t> AV1_KEY_FRAME{0x32, 0x02, 0x10, 0x00};
const std::vector<uint8_t> AV1_HIDDEN_FRAMES = {
    0x32, 0x02, 0x20, 0x19,
    0x32, 0x02, 0x28, 0x0C,
    0x32, 0x02, 0x28, 0x06,
    0x32, 0x02, 0x28, 0x03,
};
const std::vector<uint8_t> AV1_SHOWN_FRAME{0x32, 0x02, 0x30, 0x03};
// clang-format on

std::vector<uint8_t> Joined(std::initializer_list<std::vector<uint8_t>> parts)
{
  std::vector<uint8_t> data;
  for (const std::vector<uint8_t>& part : parts)
    data.insert(data.end(), part.begin(), part.end());
  return data;
}

std::vector<uint8_t> WithStartCodes(std::initializer_list<std::vector<uint8_t>> nals)
{
  std::vector<uint8_t> data;
  for (const std::vector<uint8_t>& nal : nals)
  {
    data.insert(data.end(), {0x00, 0x00, 0x00, 0x01});
    data.insert(data.end(), nal.begin(), nal.end());
  }
  return data;
}

std::vector<uint8_t> WithSizes(std::initializer_list<std::vector<uint8_t>> nals)
{
  std::vector<uint8_t> data;
  for (const std::vector<uint8_t>& nal : nals)
  {
    const auto size{static_cast<uint32_t>(nal.size())};
    data.insert(data.end(), {static_cast<uint8_t>(size >> 24), static_cast<uint8_t>(size >> 16),
                             static_cast<uint8_t>(size >> 8), static_cast<uint8_t>(size)});
    data.insert(data.end(), nal.begin(), nal.end());
  }
  return data;
}

DemuxPacket Packet(std::vector<uint8_t>& data, AVPacketSideData* sideData = nullptr)
{
  DemuxPacket packet;
  packet.pData = data.data();
  packet.iSize = static_cast<int>(data.size());
  packet.pSideData = sideData;
  packet.iSideDataElems = sideData ? 1 : 0;
  return packet;
}

// Adds a packet a number of times, and returns the state after the last
State Add(CVideoHdrBitstream& bitstream, std::vector<uint8_t> data, int times = 1)
{
  State state{bitstream.GetState()};
  for (int i = 0; i < times; ++i)
    state = bitstream.Add(Packet(data));
  return state;
}

//! The HDR10+ metadata FFmpeg's Matroska demuxer gives with a packet from its BlockAdditional
class CHdr10PlusSideData
{
public:
  explicit CHdr10PlusSideData(const std::vector<uint8_t>& blockAdditional)
  {
    size_t size{0};
    m_metadata.reset(av_dynamic_hdr_plus_alloc(&size));
    // The message after its country, provider and provider oriented codes and its application
    EXPECT_EQ(0, av_dynamic_hdr_plus_from_t35(m_metadata.get(), blockAdditional.data() + 6,
                                              blockAdditional.size() - 6));
    m_sideData.data = reinterpret_cast<uint8_t*>(m_metadata.get());
    m_sideData.size = size;
    m_sideData.type = AV_PKT_DATA_DYNAMIC_HDR10_PLUS;
  }

  AVPacketSideData* Get() { return &m_sideData; }

private:
  std::unique_ptr<AVDynamicHDRPlus, decltype(&av_free)> m_metadata{nullptr, &av_free};
  AVPacketSideData m_sideData{};
};

//! A video stream as its demuxer gives it
class CStream : public CDemuxStreamVideo
{
public:
  CStream(AVCodecID codecId, StreamHdrType hdrType, const std::vector<uint8_t>& extra = {})
  {
    codec = codecId;
    hdr_type = hdrType;
    if (!extra.empty())
      extraData = FFmpegExtraData(extra.data(), extra.size());
  }

  //! A Dolby Vision stream, of a profile and a base layer's compatibility
  CStream(AVCodecID codecId, int profile, int compatibility, const std::vector<uint8_t>& extra = {})
    : CStream(codecId, StreamHdrType::HDR_TYPE_DOLBYVISION, extra)
  {
    dovi.dv_profile = static_cast<uint8_t>(profile);
    dovi.dv_bl_signal_compatibility_id = static_cast<uint8_t>(compatibility);
  }
};

// The packets of a stream from a keyframe on: runs of a packet, each repeated
std::vector<std::vector<uint8_t>> Packets(
    std::initializer_list<std::pair<std::vector<uint8_t>, int>> runs)
{
  std::vector<std::vector<uint8_t>> packets;
  for (const auto& [packet, times] : runs)
    packets.insert(packets.end(), times, packet);
  return packets;
}

//! What a probe stores of a video stream's HDR, and how it learnt it
struct Probed
{
  bool read{false}; //!< whether a picture is read for HDR10+ and the enhancement layer
  bool told{false}; //!< whether its packets told, or the picture is to be decoded
  int packets{0}; //!< the packets read
  std::string type{};
  std::string alt{};
  std::string detail{};

  bool operator==(const Probed& other) const = default;
};

std::ostream& operator<<(std::ostream& out, const Probed& probed)
{
  return out << "read " << probed.read << ", told " << probed.told << ", " << probed.packets
             << " packets, \"" << probed.type << "\", \"" << probed.alt << "\", \"" << probed.detail
             << "\"";
}

//! Probes a stream's HDR as CDVDFileInfo::DemuxerToStreamDetails() does, its packets given
Probed Probe(const CDemuxStreamVideo& stream,
             std::vector<std::vector<uint8_t>> packets = {},
             AVPacketSideData* sideData = nullptr)
{
  Probed probed;
  CStreamDetailVideo details;
  probed.read = CDVDFileInfo::SetStreamHdrDetails(stream, details);
  if (probed.read)
  {
    const auto readPackets = [&](const std::function<bool(const DemuxPacket&)>& take)
    {
      for (std::vector<uint8_t>& data : packets)
      {
        ++probed.packets;
        if (!take(Packet(data, sideData)))
          break;
      }
    };
    probed.told = CDVDFileInfo::SetPictureHdrDetails(stream, "", readPackets, details);
  }
  probed.type = details.m_strHdrType;
  probed.alt = details.m_strHdrTypeAlt;
  probed.detail = details.m_strHdrDetail;
  return probed;
}
} // namespace

// Each probe stores what decoding a picture of the same file stored: its HDR type, second HDR type
// and Dolby Vision detail.

TEST(TestDVDFileInfoHdr, StreamsWithoutHdr10PlusOrAnEnhancementLayerArentRead)
{
  // FATE's mkv/hdr10_plus_vp9_sample.webm isn't tagged as HDR, though its packets carry HDR10+
  CHdr10PlusSideData sideData(VP9_BLOCK_ADDITIONAL);
  EXPECT_EQ(Probed{}, Probe(CStream(AV_CODEC_ID_VP9, StreamHdrType::HDR_TYPE_NONE),
                            Packets({{VP9_FRAME, 20}}), sideData.Get()));

  // HLG: FATE's hevc/hdr_vivid_h265_sample.hevc
  EXPECT_EQ(Probed{.type = "hlg"}, Probe(CStream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HLG)));

  // Dolby Vision profile 5: FATE's mov/dovi-p5.mp4
  EXPECT_EQ((Probed{.type = "dolbyvision", .detail = "5"}), Probe(CStream(AV_CODEC_ID_HEVC, 5, 0)));

  // Profile 8.4, with an HLG base layer: FATE's hevc/dv84.mov
  EXPECT_EQ((Probed{.type = "dolbyvision", .alt = "hlg", .detail = "8.4"}),
            Probe(CStream(AV_CODEC_ID_HEVC, 8, 4)));

  // A profile past 10, of which there is no sample
  EXPECT_EQ((Probed{.type = "dolbyvision", .detail = "20"}),
            Probe(CStream(AV_CODEC_ID_HEVC, 20, 0)));
}

TEST(TestDVDFileInfoHdr, Hdr10IsToldBySixteenPictures)
{
  // FATE's mkv/hdr10tags-both.mkv. The decoder has output its first picture by the 16th.
  const CStream stream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HDR10, HDR10_HVCC);
  EXPECT_EQ((Probed{.read = true, .told = true, .packets = 16, .type = "hdr10"}),
            Probe(stream, Packets({{WithSizes({HDR10_IDR}), 1}, {WithSizes({HDR10_TRAIL}), 19}})));
  EXPECT_EQ((Probed{.read = true, .packets = 15, .type = "hdr10"}),
            Probe(stream, Packets({{WithSizes({HDR10_IDR}), 1}, {WithSizes({HDR10_TRAIL}), 14}})));
}

TEST(TestDVDFileInfoHdr, Hdr10PlusIsReadFromAnHevcPrefixSei)
{
  // FFmpeg's HEVC decoder keeps the metadata for every later picture, so the first tells
  const Probed hdr10Plus{.read = true, .told = true, .packets = 1, .type = "hdr10plus"};

  // With start codes: FATE's hevc/hdr10_plus_h265_sample.hevc
  EXPECT_EQ(hdr10Plus, Probe(CStream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HDR10),
                             Packets({{WithStartCodes({HDR10PLUS_SEI, HDR10PLUS_IDR}), 20}})));

  // With the sizes of an hvcC record: a WEB-DL
  EXPECT_EQ(hdr10Plus, Probe(CStream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HDR10, HVCC),
                             Packets({{WithSizes({WEB_HDR10PLUS_SEI, WEB_IDR}), 1},
                                      {WithSizes({WEB_HDR10PLUS_SEI, WEB_TRAIL}), 19}})));
}

TEST(TestDVDFileInfoHdr, DolbyVisionProfile8HasHdr10PlusAsItsSecondType)
{
  // FATE's mov/dovi-p81.mp4. Profile 8's RPUs aren't read, and are left out.
  const CStream profile81(AV_CODEC_ID_HEVC, 8, 1, HVCC);
  EXPECT_EQ(
      (Probed{.read = true, .told = true, .packets = 16, .type = "dolbyvision", .detail = "8.1"}),
      Probe(profile81, Packets({{WithSizes({DOVI_IDR}), 1}, {WithSizes({DOVI_TRAIL}), 19}})));

  // A WEB-DL with HDR10+ too
  EXPECT_EQ((Probed{.read = true,
                    .told = true,
                    .packets = 1,
                    .type = "dolbyvision",
                    .alt = "hdr10plus",
                    .detail = "8.1"}),
            Probe(profile81, Packets({{WithSizes({HYBRID_HDR10PLUS_SEI, HYBRID_IDR}), 20}})));
}

TEST(TestDVDFileInfoHdr, DolbyVisionProfile7HasTheEnhancementLayerOfItsRpus)
{
  // A UHD Blu-ray, its enhancement layer in the same track. The decoder skips the leading pictures.
  EXPECT_EQ(
      (Probed{.read = true, .told = true, .packets = 18, .type = "dolbyvision", .detail = "7MEL"}),
      Probe(CStream(AV_CODEC_ID_HEVC, 7, 6, HVCC),
            Packets({{WithSizes({MEL_CRA, MEL_CRA_EL, MEL_RPU}), 1},
                     {WithSizes({MEL_RASL, MEL_RASL_EL, MEL_RPU}), 2},
                     {WithSizes({MEL_TRAIL, MEL_TRAIL_EL, MEL_RPU}), 17}})));

  // FATE's mov/dovi-p7.mp4, whose enhancement layer has a track of its own with the RPUs. The
  // demuxer gives the base layer's track as HDR10, which has no RPUs to read.
  EXPECT_EQ(
      (Probed{.read = true, .told = true, .packets = 16, .type = "dolbyvision", .detail = "7FEL"}),
      Probe(
          CStream(AV_CODEC_ID_HEVC, 7, 6, HVCC),
          Packets({{WithSizes({DOVI_IDR, FEL_RPU}), 1}, {WithSizes({DOVI_TRAIL, FEL_RPU}), 19}})));
  EXPECT_EQ((Probed{.read = true, .told = true, .packets = 16, .type = "hdr10"}),
            Probe(CStream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HDR10, HVCC),
                  Packets({{WithSizes({DOVI_IDR}), 1}, {WithSizes({DOVI_TRAIL}), 19}})));
}

TEST(TestDVDFileInfoHdr, AnRpuThatCantBeReadIsLeftToTheDecoder)
{
  // Kodi's decoder doesn't have FFmpeg check an RPU's CRC
  std::vector<uint8_t> rpu{MEL_RPU};
  rpu[rpu.size() - 2] ^= 0x01;
  EXPECT_EQ(
      (Probed{.read = true, .packets = 1, .type = "dolbyvision", .detail = "7"}),
      Probe(CStream(AV_CODEC_ID_HEVC, 7, 6, HVCC), Packets({{WithSizes({MEL_CRA, rpu}), 20}})));
}

TEST(TestDVDFileInfoHdr, Hdr10PlusIsReadFromAnAv1MetadataObu)
{
  const CStream stream(AV_CODEC_ID_AV1, StreamHdrType::HDR_TYPE_HDR10);
  EXPECT_EQ(
      (Probed{.read = true, .told = true, .packets = 16, .type = "hdr10plus"}),
      Probe(stream, Packets({{Joined({AV1_SEQUENCE_HEADER, AV1_HDR10PLUS, AV1_KEY_FRAME}), 1},
                             {Joined({AV1_HIDDEN_FRAMES, AV1_HDR10PLUS, AV1_SHOWN_FRAME}), 19}})));

  // libaom's encode without the metadata
  EXPECT_EQ((Probed{.read = true, .told = true, .packets = 16, .type = "hdr10"}),
            Probe(stream, Packets({{Joined({AV1_SEQUENCE_HEADER, AV1_KEY_FRAME}), 1},
                                   {Joined({AV1_HIDDEN_FRAMES, AV1_SHOWN_FRAME}), 19}})));
}

TEST(TestDVDFileInfoHdr, Hdr10PlusIsReadFromAPacketsSideData)
{
  // FATE's mkv/hdr10_plus_vp9_sample.webm, as an HDR10 stream
  const CStream stream(AV_CODEC_ID_VP9, StreamHdrType::HDR_TYPE_HDR10);
  CHdr10PlusSideData sideData(VP9_BLOCK_ADDITIONAL);
  EXPECT_EQ((Probed{.read = true, .told = true, .packets = 16, .type = "hdr10plus"}),
            Probe(stream, Packets({{VP9_FRAME, 20}}), sideData.Get()));
  EXPECT_EQ((Probed{.read = true, .told = true, .packets = 16, .type = "hdr10"}),
            Probe(stream, Packets({{VP9_FRAME, 20}})));
}

TEST(TestDVDFileInfoHdr, OtherCodecsAreLeftToTheDecoder)
{
  // The start of a packet of testdata/h264_8bit_320x240.mkv
  const std::vector<uint8_t> packet{0x00, 0x00, 0x02, 0xAE, 0x06, 0x05};
  EXPECT_EQ(
      (Probed{.read = true, .type = "hdr10"}),
      Probe(CStream(AV_CODEC_ID_H264, StreamHdrType::HDR_TYPE_HDR10), Packets({{packet, 20}})));
}

TEST(TestDVDFileInfoHdr, TheStreamIsntUsedOnceItsPacketsAreRead)
{
  // The demuxer may replace it while they are read (CDVDDemuxFFmpeg::Read(), when its size changes)
  CStream stream(AV_CODEC_ID_HEVC, StreamHdrType::HDR_TYPE_HDR10);
  CStreamDetailVideo details;
  ASSERT_TRUE(CDVDFileInfo::SetStreamHdrDetails(stream, details));
  std::vector<uint8_t> data{WithStartCodes({HDR10PLUS_SEI, HDR10PLUS_IDR})};
  const auto readPackets = [&](const std::function<bool(const DemuxPacket&)>& take)
  {
    take(Packet(data));
    stream.hdr_type = StreamHdrType::HDR_TYPE_NONE;
  };
  EXPECT_TRUE(CDVDFileInfo::SetPictureHdrDetails(stream, "", readPackets, details));
  EXPECT_EQ("hdr10plus", details.m_strHdrType);
}

TEST(TestVideoHdrBitstream, Hdr10PlusMetadataFromALaterPictureDoesntTell)
{
  CVideoHdrBitstream bitstream(AV_CODEC_ID_HEVC, HVCC, false);
  Add(bitstream, WithSizes({WEB_IDR}));
  EXPECT_EQ(State::UNDECIDED, Add(bitstream, WithSizes({WEB_HDR10PLUS_SEI, WEB_TRAIL})));
}

TEST(TestVideoHdrBitstream, Hdr10PlusMetadataLibavutilRejectsDoesntTell)
{
  // No processing windows
  std::vector<uint8_t> sei{HDR10PLUS_SEI};
  sei[11] = 0x00;
  CVideoHdrBitstream bitstream(AV_CODEC_ID_HEVC, {}, false);
  EXPECT_EQ(State::UNDECIDED, Add(bitstream, WithStartCodes({sei, HDR10PLUS_IDR})));
}

TEST(TestVideoHdrBitstream, OnlyTheHdr10PlusMetadataFFmpegTakesCounts)
{
  // FFmpeg reads it from a prefix SEI only
  std::vector<uint8_t> suffix{HDR10PLUS_SEI};
  suffix[0] = 0x50;
  CVideoHdrBitstream inSuffix(AV_CODEC_ID_HEVC, {}, false);
  EXPECT_EQ(State::DONE, Add(inSuffix, WithStartCodes({suffix, HDR10PLUS_IDR}), 16));
  EXPECT_FALSE(inSuffix.HasHdr10Plus());

  // and of application 4
  std::vector<uint8_t> otherApplication{HDR10PLUS_SEI};
  otherApplication[9] = 0x05;
  CVideoHdrBitstream ofOtherApplication(AV_CODEC_ID_HEVC, {}, false);
  EXPECT_EQ(State::DONE,
            Add(ofOtherApplication, WithStartCodes({otherApplication, HDR10PLUS_IDR}), 16));
  EXPECT_FALSE(ofOtherApplication.HasHdr10Plus());

  // but of any version
  std::vector<uint8_t> otherVersion{HDR10PLUS_SEI};
  otherVersion[10] = 0x02;
  CVideoHdrBitstream ofOtherVersion(AV_CODEC_ID_HEVC, {}, false);
  EXPECT_EQ(State::DONE, Add(ofOtherVersion, WithStartCodes({otherVersion, HDR10PLUS_IDR})));
  EXPECT_TRUE(ofOtherVersion.HasHdr10Plus());

  // Other ITU-T T.35 messages are read, but aren't HDR10+
  CVideoHdrBitstream hdrVivid(AV_CODEC_ID_HEVC, {}, false);
  EXPECT_EQ(State::DONE, Add(hdrVivid, WithStartCodes({HDR_VIVID_SEI, HDR10PLUS_IDR}), 16));
  EXPECT_FALSE(hdrVivid.HasHdr10Plus());
}

TEST(TestVideoHdrBitstream, AnRpuThatIsAPacketsFirstUnitIsntRead)
{
  CVideoHdrBitstream bitstream(AV_CODEC_ID_HEVC, HVCC, true);
  EXPECT_EQ(State::DONE, Add(bitstream, WithSizes({MEL_RPU, MEL_TRAIL}), 16));
  EXPECT_EQ(EnhancementLayer::NONE, bitstream.GetEnhancementLayer());
}

TEST(TestVideoHdrBitstream, PicturesOfDifferentEnhancementLayersDontTell)
{
  CVideoHdrBitstream bitstream(AV_CODEC_ID_HEVC, HVCC, true);
  Add(bitstream, WithSizes({MEL_CRA, MEL_RPU}));
  EXPECT_EQ(State::UNDECIDED, Add(bitstream, WithSizes({MEL_TRAIL, FEL_RPU})));
}

TEST(TestVideoHdrBitstream, Av1Hdr10PlusMetadataAfterTheShownFrameDoesntTell)
{
  // dav1d holds it for the next temporal unit's frame
  CVideoHdrBitstream bitstream(AV_CODEC_ID_AV1, {}, false);
  EXPECT_EQ(State::UNDECIDED,
            Add(bitstream, Joined({AV1_SEQUENCE_HEADER, AV1_KEY_FRAME, AV1_HDR10PLUS})));
}

TEST(TestVideoHdrBitstream, Av1T35MessagesThatDav1dVersionsReadApartDontTell)
{
  // dav1d 1.0.0 keeps only the last message before a frame, and later versions keep them all
  const std::vector<uint8_t> otherT35{0x2A, 0x04, 0x04, 0x01, 0x02, 0x80};
  CVideoHdrBitstream twoMessages(AV_CODEC_ID_AV1, {}, false);
  EXPECT_EQ(State::UNDECIDED, Add(twoMessages, Joined({AV1_SEQUENCE_HEADER, AV1_HDR10PLUS, otherT35,
                                                       AV1_KEY_FRAME})));

  // dav1d 1.0.0 takes a message whose last nonzero byte isn't 0x80, and later versions drop it
  std::vector<uint8_t> unterminated{AV1_HDR10PLUS};
  unterminated.back() = 0x81;
  CVideoHdrBitstream withoutItsTrailingBits(AV_CODEC_ID_AV1, {}, false);
  EXPECT_EQ(State::UNDECIDED, Add(withoutItsTrailingBits,
                                  Joined({AV1_SEQUENCE_HEADER, unterminated, AV1_KEY_FRAME})));
}

TEST(TestVideoHdrBitstream, OnlyHevcIsReadForTheEnhancementLayer)
{
  EXPECT_EQ(State::UNDECIDED, CVideoHdrBitstream(AV_CODEC_ID_AV1, {}, true).GetState());
  EXPECT_EQ(State::UNDECIDED, CVideoHdrBitstream(AV_CODEC_ID_VP9, {}, true).GetState());
}
