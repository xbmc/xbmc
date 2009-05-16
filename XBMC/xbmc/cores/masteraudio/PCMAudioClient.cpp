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
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;

  pAtts->SetInt(MA_ATT_TYPE_STREAM_FORMAT,MA_STREAM_FORMAT_PCM);
  pAtts->SetInt(MA_ATT_TYPE_CHANNELS,channels);
  pAtts->SetInt(MA_ATT_TYPE_BITDEPTH,bitsPerSample);
  pAtts->SetInt(MA_ATT_TYPE_SAMPLESPERSEC,samplesPerSecond);

  __int64 layout = 0;
  if (channels == 6)
    __int64 layout = 0xffffffffff543210; // As provided by dvdaudio
  else if (channels == 2)
    __int64 layout = 0xffffffffffffff10; // As provided by dvdaudio
  else
    return false;

  pAtts->SetInt64(MA_ATT_TYPE_CHANNEL_MAP,layout);

  return CAudioManagerClient::OpenStream(&desc);
}
