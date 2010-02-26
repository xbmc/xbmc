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

#ifdef _LINUX
  #include <limits.h>
  #include <string.h>
#endif

#include "PCMSampleConverter.h"

CPCMSampleConverter::CPCMSampleConverter() :
  m_pBuffer(NULL),
  m_BufferOffset(0),
  m_BufferSize(0)
{
  memset(&m_InputFormat, 0, sizeof(DVDAudioFormat));
  memset(&m_OutputFormat, 0, sizeof(DVDAudioFormat));
}

CPCMSampleConverter::~CPCMSampleConverter()
{
  free(m_pBuffer);
}

bool CPCMSampleConverter::Initialize(DVDAudioFormat& inFormat, DVDAudioFormat& outFormat)
{
  if (inFormat.streamType != DVDAudioStreamType_PCM || outFormat.streamType != DVDAudioStreamType_PCM)
    return false; // Only converting samples here...
  
  m_InputFormat = inFormat;
  m_OutputFormat = outFormat;
  
  InitBuffer(8 * 1024);
  
  return true;
}

void CPCMSampleConverter::InitBuffer(unsigned int minSize, bool copyExisting /*= false*/)
{
  if (m_BufferSize >= minSize)
    return;  // Already big enough
  
  unsigned char* pBuf = (unsigned char*)malloc(minSize);
  if (copyExisting)
    memcpy(pBuf, m_pBuffer, m_BufferOffset);
  
  free(m_pBuffer);
  m_pBuffer = pBuf;
  m_BufferOffset = 0;
  m_BufferSize = minSize;
}

bool CPCMSampleConverter::AddFrame(DVDAudioFrame& frame)
{
  // TODO: Compare frame format to input format
  // TODO: Ensure requested size is frame-aligned?

  unsigned int bytesNeeded = frame.size * GetConversionFactor();
  if (m_BufferSize - m_BufferOffset < bytesNeeded)
    InitBuffer(bytesNeeded + m_BufferOffset, true);

  // TODO: Convert based on actual input/output formats
  float* pInSamples = (float*)frame.data;
  short* pOutSamples = (short*)(m_pBuffer + m_BufferOffset);
  for (unsigned int s = frame.size; s > 0; s -= sizeof(float))
  {
    // Clamp to [-1.0,1.0] and Scale to [SHRT_MIN,SHRT_MAX]
    if(*pInSamples > 1.0f) 
      *pOutSamples++ = SHRT_MAX;
    else if (*pInSamples < -1.0f) 
      *pOutSamples++ = SHRT_MIN;
    else
      *pOutSamples++ = (short)(*pInSamples++ * SHRT_MAX);
    
    m_BufferOffset += sizeof(short);
  }
  return true;
}

bool CPCMSampleConverter::GetFrame(DVDAudioFrame& frame)
{
  // TODO: Only return frame-aligned data?
  
  frame.size = m_BufferOffset;
  frame.data = m_pBuffer;
  
  m_BufferOffset = 0;
  
  return true;
}

float CPCMSampleConverter::GetConversionFactor()
{
  // TODO: Calculate this properly
  return 0.5;
}

