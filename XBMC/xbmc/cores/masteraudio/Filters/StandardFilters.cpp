/*
 *      Copyright (C) 2009 Team XBMC
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
#include "StandardFilters.h"

CChannelRemapFilter::CChannelRemapFilter() :
  m_pSlice(NULL),
  m_Channels(0),
  m_BitDepth(0),
  m_pWorkBuffer(NULL)
{

}

CChannelRemapFilter::~CChannelRemapFilter()
{
  Close();
}

void CChannelRemapFilter::Close()
{
  Flush();
  m_Channels = 0;
  m_BitDepth = 0;
  delete m_pWorkBuffer;
  m_pWorkBuffer = NULL;
}

// IAudioSink
MA_RESULT CChannelRemapFilter::TestInputFormat(CStreamDescriptor* pDesc)
{
  if (!pDesc)
    return MA_ERROR;

  int format = 0;
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_STREAM_FORMAT, &format);
  if (format != MA_STREAM_FORMAT_LPCM)
    return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::SetInputFormat(CStreamDescriptor* pDesc)
{
  MA_RESULT test = TestInputFormat(pDesc);
  if (MA_SUCCESS != test)
    return test;

  int channels = 0;
  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_CHANNEL_COUNT, &channels))
    return MA_ERROR;

  int bitdepth = 0;
  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_BITDEPTH, &bitdepth))
    return MA_ERROR;

  if (m_Channels) // We already have an output format
  {
    if (m_Channels != channels || m_BitDepth != bitdepth)
      return MA_NOT_SUPPORTED;
  }
  else
  {
    m_Channels = channels;
    m_BitDepth = bitdepth;
  }
 
  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt64(MA_ATT_TYPE_CHANNEL_MAP, &m_InputMap))
    return MA_ERROR;

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::GetInputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps)
    return MA_ERROR;

  pProps->transfer_alignment = (m_BitDepth >> 3) * m_Channels; // Bytes per frame
  pProps->preferred_transfer_size = pProps->transfer_alignment; // Prefer one frame
  pProps->max_transfer_size = 0; // Any size

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::AddSlice(audio_slice* pSlice)
{
  if (!pSlice)
    return MA_ERROR;

  if (m_pSlice)
    return MA_BUSYORFULL;

  m_pSlice = pSlice;
  return MA_SUCCESS;
}

float CChannelRemapFilter::GetMaxLatency()
{
  return 0.0f;
}

void CChannelRemapFilter::Flush()
{
  delete m_pSlice;
  m_pSlice = NULL;
}

// IAudioSource
MA_RESULT CChannelRemapFilter::TestOutputFormat(CStreamDescriptor* pDesc)
{
  if (!pDesc)
    return MA_ERROR;

  int format = 0;
  pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_STREAM_FORMAT, &format);
  if (format != MA_STREAM_FORMAT_LPCM)
    return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::SetOutputFormat(CStreamDescriptor* pDesc)
{

  int channels = 0;
  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_CHANNEL_COUNT, &channels))
    return MA_ERROR;

  int bitdepth = 0;
  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt(MA_ATT_TYPE_BITDEPTH, &bitdepth))
    return MA_ERROR;

  if (m_Channels) // We already have an input format
  {
    if (m_Channels != channels || m_BitDepth != bitdepth) // Input/Ouput formats do not match
      return MA_NOT_SUPPORTED;
  }
  else
  {
    m_Channels = channels;
    m_BitDepth = bitdepth;
  }

  if (MA_SUCCESS != pDesc->GetAttributes()->GetInt64(MA_ATT_TYPE_CHANNEL_MAP, &m_OutputMap))
    return MA_ERROR;

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::GetOutputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps)
    return MA_ERROR;

  pProps->transfer_alignment = (m_BitDepth >> 3) * m_Channels; // Bytes per frame
  pProps->preferred_transfer_size = pProps->transfer_alignment; // Prefer one frame
  pProps->max_transfer_size = 0; // Any size

  return MA_SUCCESS;
}

MA_RESULT CChannelRemapFilter::GetSlice(audio_slice** ppSlice)
{
  if (!m_pSlice)
    return MA_NEED_DATA;

  RemapChannels(m_pSlice);
  *ppSlice = m_pSlice;
  m_pSlice = NULL;

  return MA_SUCCESS;
}

// TODO: This can be MUCH more efficient
void CChannelRemapFilter::RemapChannels(audio_slice* pSlice)
{
  if (!pSlice)
    return;

  size_t frameLen = (m_BitDepth >> 3) * m_Channels;
  if (!m_pWorkBuffer)
    m_pWorkBuffer = new unsigned char[frameLen];

  unsigned __int16* pWork = (unsigned __int16*)m_pWorkBuffer;
  size_t bytesUsed = 0;
  for (unsigned __int16* pIn = (unsigned __int16*)pSlice->get_data(); bytesUsed < pSlice->header.data_len; pIn += m_Channels, bytesUsed+=frameLen)
  {
    for (int chan = 0; chan < MA_MAX_CHANNELS; chan++)
    {
      unsigned char outPos = ((unsigned char*)&m_OutputMap)[chan/2];
      if (chan % 2)
        outPos >>= 4;
      outPos &= 0x0F;

      if (outPos ^ 0xF) // See if this channel is used in the output. 0xF means not active
      {
        unsigned char inPos = ((unsigned char*)&m_InputMap)[chan/2];
        if (chan % 2)
          inPos >>= 4;
        inPos &= 0x0F;
        pWork[outPos] = pIn[inPos];
      }
    }
    memcpy(pIn,m_pWorkBuffer,frameLen);
  }
}