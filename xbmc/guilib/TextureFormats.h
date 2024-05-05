/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// clang-format off
enum XB_FMT
{
  XB_FMT_UNKNOWN     = 0x0,
  XB_FMT_DXT1        = 0x1,
  XB_FMT_DXT3        = 0x2,
  XB_FMT_DXT5        = 0x4,
  XB_FMT_DXT5_YCoCg  = 0x8,
  XB_FMT_DXT_MASK    = 0xF,

  XB_FMT_A8R8G8B8    = 0x10, // texture.xbt byte order (matches BGRA8)
  XB_FMT_A8          = 0x20,
  XB_FMT_RGBA8       = 0x40,
  XB_FMT_RGB8        = 0x80,
  XB_FMT_MASK        = 0xFFFF,
  XB_FMT_OPAQUE      = 0x10000,
};

enum KD_TEX_FMT
{
  KD_TEX_FMT_UNKNOWN            = 0x0000,

  // Legacy XB_FMT formats family
  KD_TEX_FMT_LEGACY             = 0x0000,

  // SDR texture family
  KD_TEX_FMT_SDR                = 0x1000,
  KD_TEX_FMT_SDR_R8             = 0x1000, // 8bpp, single channel
  KD_TEX_FMT_SDR_RG8            = 0x1100, // 16bpp, dual channel
  KD_TEX_FMT_SDR_R5G6B5         = 0x1200, // 16bpp, 5/6 bit per color channel
  KD_TEX_FMT_SDR_RGB5_A1        = 0x1300, // 16bpp, 5 bit per color channel, pt-alpha
  KD_TEX_FMT_SDR_RGBA4          = 0x1400, // 16bpp, 4 bit per channel
  KD_TEX_FMT_SDR_RGB8           = 0x1500, // 24bpp, 8 bit per channel, no alpha (unsuitable for GPUs!)
  KD_TEX_FMT_SDR_RGBA8          = 0x1600, // 32bpp, 8 bit per channel, RGBA order
  KD_TEX_FMT_SDR_BGRA8          = 0x1700, // 32bpp, 8 bit per channel, BGRA order

  // HDR texture family
  KD_TEX_FMT_HDR                = 0x2000, 
  KD_TEX_FMT_HDR_R16f           = 0x2100, // 16bpp, single channel float
  KD_TEX_FMT_HDR_RG16f          = 0x2200, // 32bpp, dual channel float
  KD_TEX_FMT_HDR_R11F_G11F_B10F = 0x2300, // 32bpp, 6e5/5e5 per color channel
  KD_TEX_FMT_HDR_RGB9_E5        = 0x2400, // 32bpp, 9 bit color, shared 5 bit exponent
  KD_TEX_FMT_HDR_RGB10_A2       = 0x2500, // 32bpp, 10 bit color, 2 bit alpha
  KD_TEX_FMT_HDR_RGBA16f        = 0x2600, // 64bpp, four channel float

  // YUV texture family
  KD_TEX_FMT_YUV                = 0x3000,
  KD_TEX_FMT_YUV_YUYV8          = 0x3000, // 16bpp, 4:2:2 packed 

  // S3TC texture family
  KD_TEX_FMT_S3TC               = 0x4000,
  KD_TEX_FMT_S3TC_RGB8          = 0x4000, // 4bpp, RGB (BC1)
  KD_TEX_FMT_S3TC_RGB8_A1       = 0x4100, // 4bpp, RGB, pt-alpha (BC1)
  KD_TEX_FMT_S3TC_RGB8_A4       = 0x4200, // 8bpp, RGB, 4 bit alpha (BC2)
  KD_TEX_FMT_S3TC_RGBA8         = 0x4300, // 8bpp, RGBA (BC3)

  // RGTC (LATC) texture family
  KD_TEX_FMT_RGTC               = 0x5000,
  KD_TEX_FMT_RGTC_R11           = 0x5000, // 4bpp, single channel (BC4)
  KD_TEX_FMT_RGTC_RG11          = 0x5100, // 8bpp, dual channel (BC5)

  // BPTC texture family
  KD_TEX_FMT_BPTC               = 0x6000,
  KD_TEX_FMT_BPTC_RGB16F        = 0x6000, // 8bpp, HDR (BC6H float)
  KD_TEX_FMT_BPTC_RGBA8         = 0x6100, // 8bpp, LDR (BC7 unorm)

  // ETC1 texture family
  KD_TEX_FMT_ETC1               = 0x7000,
  KD_TEX_FMT_ETC1_RGB8          = 0x7000, // 4bpp, RGB

  // ETC2 texture family
  KD_TEX_FMT_ETC2               = 0x8000,
  KD_TEX_FMT_ETC2_R11           = 0x8100, // 4bpp, single channel (EAC)
  KD_TEX_FMT_ETC2_RG11          = 0x8200, // 8bpp, dual channel (EAC)
  KD_TEX_FMT_ETC2_RGB8          = 0x8300, // 4bpp, RGB
  KD_TEX_FMT_ETC2_RGB8_A1       = 0x8400, // 4bpp, RGB, pt-alpha
  KD_TEX_FMT_ETC2_RGBA8         = 0x8500, // 8bpp, RGB, alpha EAC

  // ASTC LDR texture family
  // Bitrate varies from 8bpp (4x4 tile) to 0.89bpp (12x12 tile).
  KD_TEX_FMT_ASTC_LDR           = 0x9000,
  KD_TEX_FMT_ASTC_LDR_4x4       = 0x9000,
  KD_TEX_FMT_ASTC_LDR_5x4       = 0x9100,
  KD_TEX_FMT_ASTC_LDR_5x5       = 0x9200,
  KD_TEX_FMT_ASTC_LDR_6x5       = 0x9300,
  KD_TEX_FMT_ASTC_LDR_6x6       = 0x9400,
  KD_TEX_FMT_ASTC_LDR_8x5       = 0x9500,
  KD_TEX_FMT_ASTC_LDR_8x6       = 0x9600,
  KD_TEX_FMT_ASTC_LDR_8x8       = 0x9700,
  KD_TEX_FMT_ASTC_LDR_10x5      = 0x9800,
  KD_TEX_FMT_ASTC_LDR_10x6      = 0x9900,
  KD_TEX_FMT_ASTC_LDR_10x8      = 0x9A00,
  KD_TEX_FMT_ASTC_LDR_10x10     = 0x9B00,
  KD_TEX_FMT_ASTC_LDR_12x10     = 0x9C00,
  KD_TEX_FMT_ASTC_LDR_12x12     = 0x9D00,

  // ASTC HDR texture family
  // Bitrate varies from 8bpp (4x4 tile) to 0.89bpp (12x12 tile).
  KD_TEX_FMT_ASTC_HDR           = 0xA000,
  KD_TEX_FMT_ASTC_HDR_4x4       = 0xA000,
  KD_TEX_FMT_ASTC_HDR_5x4       = 0xA100,
  KD_TEX_FMT_ASTC_HDR_5x5       = 0xA200,
  KD_TEX_FMT_ASTC_HDR_6x5       = 0xA300,
  KD_TEX_FMT_ASTC_HDR_6x6       = 0xA400,
  KD_TEX_FMT_ASTC_HDR_8x5       = 0xA500,
  KD_TEX_FMT_ASTC_HDR_8x6       = 0xA600,
  KD_TEX_FMT_ASTC_HDR_8x8       = 0xA700,
  KD_TEX_FMT_ASTC_HDR_10x5      = 0xA800,
  KD_TEX_FMT_ASTC_HDR_10x6      = 0xA900,
  KD_TEX_FMT_ASTC_HDR_10x8      = 0xAA00,
  KD_TEX_FMT_ASTC_HDR_10x10     = 0xAB00,
  KD_TEX_FMT_ASTC_HDR_12x10     = 0xAC00,
  KD_TEX_FMT_ASTC_HDR_12x12     = 0xAD00,

  KD_TEX_FMT_TYPE_MASK          = 0xF000,

  KD_TEX_FMT_MASK               = 0xFFFF,
};

// Alpha handling
enum KD_TEX_ALPHA
{
  KD_TEX_ALPHA_STRAIGHT         = 0x00000, // Straight (unmultiplied) alpha
  KD_TEX_ALPHA_OPAQUE           = 0x10000, // No alpha
  KD_TEX_ALPHA_PREMULTIPLIED    = 0x20000, // Premultiplied alpha

  KD_TEX_ALPHA_MASK             = 0xF0000,
};
 
// Texture component swizzle or effect
enum KD_TEX_SWIZ
{
  KD_TEX_SWIZ_RGBA              = 0x000000, // No swizzling
  KD_TEX_SWIZ_RGB1              = 0x100000, // Normal swizzle, ignoring alpha
  KD_TEX_SWIZ_RRR1              = 0x200000, // Luminance
  KD_TEX_SWIZ_111R              = 0x300000, // Alpha
  KD_TEX_SWIZ_RRRG              = 0x400000, // Luminance-Alpha
  KD_TEX_SWIZ_RRRR              = 0x500000, // Intensity
  KD_TEX_SWIZ_GGG1              = 0x600000, // Luminance (ETC1/BC1)
  KD_TEX_SWIZ_111G              = 0x700000, // Alpha (ETC1/BC1)
  KD_TEX_SWIZ_GGGA              = 0x800000, // Luminance-Alpha (BC2/BC3)
  KD_TEX_SWIZ_GGGG              = 0x900000, // Intensity (ETC1/BC1)

  KD_TEX_SWIZ_SDF               = 0xa00000, // Red channel contains a SDF
  KD_TEX_SWIZ_RGB_SDF           = 0xb00000, // RGB8 texture with a SDF packed in the alpha channel
  KD_TEX_SWIZ_MSDF              = 0xc00000, // Encoded MSDF in the color channels, Alpha is ignored

  KD_TEX_SWIZ_MASK              = 0xF00000,
};

// Color space
enum KD_TEX_COL
{
  KD_TEX_COL_REC709             = 0x0000000, // REC709/sRGB color space
  KD_TEX_COL_REC2020            = 0x1000000, // REC2020 color space

  KD_TEX_COL_MASK               = 0xF000000,
};

// Transfer function
enum KD_TEX_TRANSFER
{
  KD_TEX_TRANSFER_SRGB          = 0x00000000,
  KD_TEX_TRANSFER_REC709        = 0x10000000,
  KD_TEX_TRANSFER_HLG           = 0x20000000,
  KD_TEX_TRANSFER_LINEAR        = 0x30000000,
  KD_TEX_TRANSFER_SQUARED       = 0x40000000,
  KD_TEX_TRANSFER_PQ            = 0x50000000,

  KD_TEX_TRANSFER_MASK          = 0xF0000000,
};

// clang-format on
