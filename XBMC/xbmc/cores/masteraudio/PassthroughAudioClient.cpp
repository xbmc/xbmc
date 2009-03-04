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
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;

  pAtts->SetInt(MA_ATT_TYPE_STREAM_FORMAT,MA_STREAM_FORMAT_ENCODED);
  pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_AC3);
  pAtts->SetBool(MA_ATT_TYPE_PASSTHROUGH,true);

  return CAudioManagerClient::OpenStream(&desc);
}

bool CPassthroughAudioClient::OpenDTSStream()
{
  CStreamDescriptor desc;
  CStreamAttributeCollection* pAtts = desc.GetAttributes();
  if(!pAtts)
    return false;

  pAtts->SetInt(MA_ATT_TYPE_STREAM_FORMAT,MA_STREAM_FORMAT_ENCODED);
  pAtts->SetInt(MA_ATT_TYPE_ENCODING,MA_STREAM_ENCODING_DTS);
  pAtts->SetBool(MA_ATT_TYPE_PASSTHROUGH,true);

  return CAudioManagerClient::OpenStream(&desc);
}

