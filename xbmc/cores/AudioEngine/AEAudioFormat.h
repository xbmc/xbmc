#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

enum AEChannel
{
  AE_CH_NULL = -1,
  AE_CH_RAW ,

  AE_CH_FL  , AE_CH_FR , AE_CH_FC , AE_CH_LFE, AE_CH_BL , AE_CH_BR , AE_CH_FLOC,
  AE_CH_FROC, AE_CH_BC , AE_CH_SL , AE_CH_SR , AE_CH_TFL, AE_CH_TFR, AE_CH_TFC ,
  AE_CH_TC  , AE_CH_TBL, AE_CH_TBR, AE_CH_TBC,

  AE_CH_MAX
};

typedef enum AEChannel* AEChLayout;

enum AEStdChLayout
{
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


/* this list is ordered from worst to best preferred */
/* NE = Native Endian */
enum AEDataFormat
{
  AE_FMT_INVALID = -1,
  AE_FMT_U8,
  AE_FMT_S8,

  AE_FMT_S16BE,
  AE_FMT_S16LE,
  AE_FMT_S16NE,

  AE_FMT_S32BE,
  AE_FMT_S32LE,
  AE_FMT_S32NE,

  AE_FMT_S24BE4,
  AE_FMT_S24LE4,
  AE_FMT_S24NE4, /* S24 in 4 bytes */

  AE_FMT_S24BE3,
  AE_FMT_S24LE3,
  AE_FMT_S24NE3, /* S24 in 3 bytes */

  AE_FMT_DOUBLE,
  AE_FMT_FLOAT,

  /* for passthrough streams and the like */
  AE_FMT_RAW,

  AE_FMT_MAX
};

typedef struct {
  enum AEDataFormat  m_dataFormat   ;
  unsigned int       m_sampleRate   ;
  unsigned int       m_channelCount ;
  AEChLayout         m_channelLayout;

  unsigned int       m_frames       ;
  unsigned int       m_frameSamples ;
  unsigned int       m_frameSize    ;
} AEAudioFormat;

