#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/**
 * The possible channels
 */
enum AEChannel
{
  AE_CH_NULL = -1,
  AE_CH_RAW ,

  AE_CH_FL  , AE_CH_FR , AE_CH_FC , AE_CH_LFE, AE_CH_BL  , AE_CH_BR  , AE_CH_FLOC,
  AE_CH_FROC, AE_CH_BC , AE_CH_SL , AE_CH_SR , AE_CH_TFL , AE_CH_TFR , AE_CH_TFC ,
  AE_CH_TC  , AE_CH_TBL, AE_CH_TBR, AE_CH_TBC, AE_CH_BLOC, AE_CH_BROC,

  /* p16v devices */
  AE_CH_UNKNOWN1 , AE_CH_UNKNOWN2 , AE_CH_UNKNOWN3 , AE_CH_UNKNOWN4 ,
  AE_CH_UNKNOWN5 , AE_CH_UNKNOWN6 , AE_CH_UNKNOWN7 , AE_CH_UNKNOWN8 ,
  AE_CH_UNKNOWN9 , AE_CH_UNKNOWN10, AE_CH_UNKNOWN11, AE_CH_UNKNOWN12,
  AE_CH_UNKNOWN13, AE_CH_UNKNOWN14, AE_CH_UNKNOWN15, AE_CH_UNKNOWN16,
  AE_CH_UNKNOWN17, AE_CH_UNKNOWN18, AE_CH_UNKNOWN19, AE_CH_UNKNOWN20,
  AE_CH_UNKNOWN21, AE_CH_UNKNOWN22, AE_CH_UNKNOWN23, AE_CH_UNKNOWN24,
  AE_CH_UNKNOWN25, AE_CH_UNKNOWN26, AE_CH_UNKNOWN27, AE_CH_UNKNOWN28,
  AE_CH_UNKNOWN29, AE_CH_UNKNOWN30, AE_CH_UNKNOWN31, AE_CH_UNKNOWN32,
  AE_CH_UNKNOWN33, AE_CH_UNKNOWN34, AE_CH_UNKNOWN35, AE_CH_UNKNOWN36,
  AE_CH_UNKNOWN37, AE_CH_UNKNOWN38, AE_CH_UNKNOWN39, AE_CH_UNKNOWN40,
  AE_CH_UNKNOWN41, AE_CH_UNKNOWN42, AE_CH_UNKNOWN43, AE_CH_UNKNOWN44,
  AE_CH_UNKNOWN45, AE_CH_UNKNOWN46, AE_CH_UNKNOWN47, AE_CH_UNKNOWN48,
  AE_CH_UNKNOWN49, AE_CH_UNKNOWN50, AE_CH_UNKNOWN51, AE_CH_UNKNOWN52,
  AE_CH_UNKNOWN53, AE_CH_UNKNOWN54, AE_CH_UNKNOWN55, AE_CH_UNKNOWN56,
  AE_CH_UNKNOWN57, AE_CH_UNKNOWN58, AE_CH_UNKNOWN59, AE_CH_UNKNOWN60,
  AE_CH_UNKNOWN61, AE_CH_UNKNOWN62, AE_CH_UNKNOWN63, AE_CH_UNKNOWN64,

  AE_CH_MAX
};

/**
 * Standard channel layouts
 */
enum AEStdChLayout
{
  AE_CH_LAYOUT_INVALID = -1,

  AE_CH_LAYOUT_1_0 = 0,
  AE_CH_LAYOUT_2_0,
  AE_CH_LAYOUT_2_1,
  AE_CH_LAYOUT_3_0,
  AE_CH_LAYOUT_3_1,
  AE_CH_LAYOUT_4_0,
  AE_CH_LAYOUT_4_1,
  AE_CH_LAYOUT_5_0,
  AE_CH_LAYOUT_5_1,
  AE_CH_LAYOUT_7_0,
  AE_CH_LAYOUT_7_1,

  AE_CH_LAYOUT_MAX
};

/**
 * The various data formats
 * LE = Little Endian, BE = Big Endian, NE = Native Endian
 * @note This is ordered from the worst to best preferred formats
 */
enum AEDataFormat
{
  AE_FMT_INVALID = -1,

  AE_FMT_U8,

  AE_FMT_S16BE,
  AE_FMT_S16LE,
  AE_FMT_S16NE,

  AE_FMT_S32BE,
  AE_FMT_S32LE,
  AE_FMT_S32NE,

  AE_FMT_S24BE4,
  AE_FMT_S24LE4,
  AE_FMT_S24NE4,    // 24 bits in lower 3 bytes
  AE_FMT_S24NE4MSB, // S32 with bits_per_sample < 32

  AE_FMT_S24BE3,
  AE_FMT_S24LE3,
  AE_FMT_S24NE3, /* S24 in 3 bytes */

  AE_FMT_DOUBLE,
  AE_FMT_FLOAT,

  /* Bitstream formats */
  AE_FMT_AAC,
  AE_FMT_AC3,
  AE_FMT_DTS,
  AE_FMT_EAC3,
  AE_FMT_TRUEHD,
  AE_FMT_DTSHD,
  AE_FMT_LPCM,

  /* planar formats */
  AE_FMT_U8P,
  AE_FMT_S16NEP,
  AE_FMT_S32NEP,
  AE_FMT_S24NE4P,
  AE_FMT_S24NE4MSBP,
  AE_FMT_S24NE3P,
  AE_FMT_DOUBLEP,
  AE_FMT_FLOATP,

  AE_FMT_MAX
};
