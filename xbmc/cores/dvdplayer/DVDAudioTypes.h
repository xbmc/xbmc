#pragma once
/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
#include "utils/PCMRemap.h"

// TODO: Clean up format definitions
enum DVDAudioStreamType
{
  DVDAudioStreamType_None, // Drop all data
  DVDAudioStreamType_Encoded,  // Encoded frames (not PCM)
  DVDAudioStreamType_PCM // Linear PCM (interleaved)
};

enum DVDAudioPCMSampleType
{
  DVDAudioPCMSampleType_None,
  DVDAudioPCMSampleType_S16LE,
  DVDAudioPCMSampleType_S24LE,
  DVDAudioPCMSampleType_IEEEFloat
};

enum DVDAudioEncodingType
{
  DVDAudioEncodingType_None,
  DVDAudioEncodingType_Raw,
  DVDAudioEncodingType_IEC958,
  DVDAudioEncodingType_AC3,
  DVDAudioEncodingType_DTS,
  DVDAudioEncodingType_AAC,
  DVDAudioEncodingType_MP1,
  DVDAudioEncodingType_MP2,
  DVDAudioEncodingType_MP3
};

typedef struct stDVDAudioFormat
{
  unsigned int bitrate;
  DVDAudioStreamType streamType;
  union
  {
    // PCM
    struct
    {
      DVDAudioPCMSampleType sampleType;
      int channels;
      PCMChannels* channel_map;
    } pcm;
    // Encoded
    struct
    {
      DVDAudioEncodingType encodingType;
      DVDAudioEncodingType containedType;
    } encoded;
  };  
} DVDAudioFormat;

typedef struct stDVDAudioFrame
{
  unsigned int size;
  void* data;
  double pts;
  double duration;
  DVDAudioFormat format;
} DVDAudioFrame;