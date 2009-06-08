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
#include "PassthroughAudioClient.h"

bool CPassthroughAudioClient::OpenAC3Stream()
{
  CStreamDescriptor desc;
  desc.SetFormat(MA_STREAM_FORMAT_IEC61937);
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;

  pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED,true);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
  pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_AC3);
  pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,48000);

  return CAudioManagerClient::OpenStream(&desc);
}

bool CPassthroughAudioClient::OpenDTSStream()
{
  CStreamDescriptor desc;
  desc.SetFormat(MA_STREAM_FORMAT_IEC61937);
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;

  pAtts->SetFlag(MA_ATT_TYPE_STREAM_FLAGS,MA_STREAM_FLAG_LOCKED, true);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_SEC,192000);
  pAtts->SetUInt(MA_ATT_TYPE_BYTES_PER_FRAME,6144);
  pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_DTS);
  pAtts->SetUInt(MA_ATT_TYPE_SAMPLERATE,48000);

  return CAudioManagerClient::OpenStream(&desc);
}

