/*
 *      Copyright (C) 2009 Team XBMC
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

#include "stdafx.h"
#include "PCMAudioClient.h"

bool CPCMAudioClient::OpenStream(int channels, int bitsPerSample, int samplesPerSecond)
{
  CStreamDescriptor desc;
  desc.SetFormat(MA_STREAM_FORMAT_LPCM);
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;
  
  // Required Attributes
  pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_NONE,false);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,channels * (bitsPerSample >> 3));
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC, samplesPerSecond * channels * (bitsPerSample >> 3));
  
  // LPCM Attributes
  pAtts->SetFlag(MA_ATT_TYPE_LPCM_FLAGS,MA_LPCM_FLAG_INTERLEAVED,true);
  pAtts->SetInt(MA_ATT_TYPE_SAMPLE_TYPE,MA_SAMPLE_TYPE_SINT);
  pAtts->SetUInt(MA_ATT_TYPE_BITDEPTH,bitsPerSample);
  pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,samplesPerSecond);
  pAtts->SetUInt(MA_ATT_TYPE_CHANNEL_COUNT,channels);

  if (channels == 6)
  {
    int channelLayout[6] = {MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_REAR_LEFT, MA_CHANNEL_REAR_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE};
    pAtts->SetArray(MA_ATT_TYPE_CHANNEL_LAYOUT, stream_attribute_int, 6, channelLayout);
  }

  // Invoke the base class member
  return CAudioManagerClient::OpenStream(&desc);
}
