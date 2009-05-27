/*
 *      Copyright (C) 2009 Team XBMC
 *
 *  This Program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DSPFilterInterleaving.h"

CDSPFilterInterleaving::CDSPFilterInterleaving(unsigned int inputBusses /* = 1*/, unsigned int outputBusses /* = 1*/) :
  CDSPFilterLPCM(inputBusses, outputBusses),
  m_Mode(MODE_INTERLEAVE)
{
  // Allocate our input container array
  m_pInputContainer = (ma_audio_container**)calloc(inputBusses, sizeof(ma_audio_container*));
}

CDSPFilterInterleaving::~CDSPFilterInterleaving()
{
  // Free all our input containers
  for(unsigned int i = 0; i < m_InputBusses; ++i)
    ma_free_container(m_pInputContainer[i]);
  free(m_pInputContainer);
}

// IAudioSink
MA_RESULT CDSPFilterInterleaving::TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilterLPCM::TestInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  // If the output format has been set, then compare the formats to ensure compatibility
  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    return CompareFormats(pDesc, GetLPCMOutputAttributes(bus));

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterInterleaving::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilterLPCM::SetInputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  if (GetOutputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
  {
    if (GetLPCMOutputAttributes(bus)->m_IsInterleaved)
    {
      if (GetLPCMInputAttributes(bus)->m_IsInterleaved)
        m_Mode = MODE_NONE;
      else
        m_Mode = MODE_INTERLEAVE;
    }
  }

  return MA_SUCCESS;
}

// IAudioSource
MA_RESULT CDSPFilterInterleaving::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilterLPCM::TestOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  // If the input format has been set, then compare the formats to ensure compatibility
  if (GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
    return CompareFormats(pDesc, GetLPCMInputAttributes(bus));

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterInterleaving::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  MA_RESULT ret;
  if ((ret = CDSPFilterLPCM::SetOutputFormat(pDesc, bus)) != MA_SUCCESS)
    return ret;

  if (GetInputAttributes(bus)->m_StreamFormat != MA_STREAM_FORMAT_UNKNOWN)
  {
    if (GetLPCMInputAttributes(bus)->m_IsInterleaved)
    {
      if (GetLPCMOutputAttributes(bus)->m_IsInterleaved)
        m_Mode = MODE_NONE;
      else
        m_Mode = MODE_DEINTERLEAVE;
    }
  }
  return MA_SUCCESS;
}

MA_RESULT CDSPFilterInterleaving::CompareFormats(CStreamDescriptor* pDesc, const LPCMAttributes *pLPCM)
{
  // Fetch the relevant attributes
  MA_RESULT      ret;
  LPCMAttributes LPCM;
  if ((ret = ReadLPCMAttributes(pDesc, &LPCM)) != MA_SUCCESS)
    return ret;

  if (
    (pLPCM->m_SampleType     != LPCM.m_SampleType   ) ||
    (pLPCM->m_BitDepth       != LPCM.m_BitDepth     ) ||
    (pLPCM->m_SampleRate     != LPCM.m_SampleRate   ) ||
    (pLPCM->m_ChannelCount   != LPCM.m_ChannelCount ) ||
    (memcmp(LPCM.m_ChannelLayout, pLPCM->m_ChannelLayout, sizeof(int) * LPCM.m_ChannelCount)))
    return MA_NOT_SUPPORTED;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilterInterleaving::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  // If there is only 1 channel, no changes are necessary, just pass it through
  if (GetLPCMInputAttributes(bus)->m_ChannelCount == 1 || m_Mode == MODE_NONE)
    return GetInputData(pOutput, frameCount, renderTime, renderFlags, bus);

  // Pass-off to the appropriate transform
  if (m_Mode == MODE_INTERLEAVE)
    return Interleave(pOutput, frameCount, renderTime, renderFlags, bus);
  else
    return DeInterleave(pOutput, frameCount, renderTime, renderFlags, bus);
}

MA_RESULT CDSPFilterInterleaving::Interleave(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus)
{
  unsigned int channelCount = GetLPCMInputAttributes(bus)->m_ChannelCount;
  unsigned int inFrameSize = GetInputAttributes(bus)->m_FrameSize;
  unsigned int outFrameSize = GetOutputAttributes(bus)->m_FrameSize;

  // Check/Update input container size
  if (!m_pInputContainer || m_pInputContainer[bus]->buffer[0].allocated_len < (inFrameSize * frameCount))
  {
    if (m_pInputContainer[bus]) // Save a call if it is not needed
      ma_free_container(m_pInputContainer[bus]);
    m_pInputContainer[bus] = ma_alloc_container(channelCount, inFrameSize, frameCount);
  }

  // Fetch input data
  MA_RESULT res;
  if ((res = GetInputData(m_pInputContainer[bus], frameCount, renderTime, renderFlags, bus)) != MA_SUCCESS)
    return res;

  ma_audio_container* pCont = m_pInputContainer[bus];
  for(unsigned int channel = 0; channel < channelCount; channel++)
  {
    char* pInFrame = (char*)pCont->buffer[channel].data;
    char* pOutFrame = (char*)pOutput->buffer[0].data + (channel * inFrameSize); // Offset by one channel within output buffer
    for(unsigned int frame = 0; frame < frameCount; frame++)
    {
      memcpy(pOutFrame, pInFrame, inFrameSize);
      pInFrame++;
      pOutFrame += outFrameSize; // Leapfrog over other channels
    }
  }
  pOutput->buffer[0].data_len = outFrameSize * frameCount;
  return MA_SUCCESS;
}

MA_RESULT CDSPFilterInterleaving::DeInterleave(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus)
{
  unsigned int channelCount = GetLPCMInputAttributes(bus)->m_ChannelCount;
  unsigned int inFrameSize = GetInputAttributes(bus)->m_FrameSize;
  unsigned int outFrameSize = GetOutputAttributes(bus)->m_FrameSize;

  // Check/Update input container size
  if (!m_pInputContainer || m_pInputContainer[bus]->buffer[0].allocated_len < (inFrameSize * frameCount * channelCount))
  {
    if (m_pInputContainer[bus]) // Save a call if it is not needed
      ma_free_container(m_pInputContainer[bus]);
    m_pInputContainer[bus] = ma_alloc_container(1, channelCount * inFrameSize, frameCount);
  }

  // Fetch input data
  MA_RESULT res;
  if ((res = GetInputData(m_pInputContainer[bus], frameCount, renderTime, renderFlags, bus)) != MA_SUCCESS)
    return res;

  ma_audio_container* pCont = m_pInputContainer[bus];
  for(unsigned int channel = 0; channel < channelCount; channel++)
  {
    char* pOutFrame = (char*)pCont->buffer[channel].data;
    char* pInFrame = (char*)pOutput->buffer[0].data + (channel * inFrameSize); // Offset by one channel within output buffer
    for(unsigned int frame = 0; frame < frameCount; frame++)
    {
      memcpy(pOutFrame, pInFrame, outFrameSize);
      pOutFrame++;
      pInFrame += inFrameSize; // Leapfrog over other channels
    }
    pOutput->buffer[channel].data_len = outFrameSize * frameCount;
  }
  return MA_SUCCESS;
}