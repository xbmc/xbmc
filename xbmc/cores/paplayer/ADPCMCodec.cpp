/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "ADPCMCodec.h"
#include "utils/log.h"

ADPCMCodec::ADPCMCodec()
{
  m_CodecName = "ADPCM";
  m_adpcm = NULL;
  m_bIsPlaying = false;
}

ADPCMCodec::~ADPCMCodec()
{
  DeInit();
}

bool ADPCMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  DeInit();

  if (!m_dll.Load())
    return false; // error logged previously

  m_adpcm = m_dll.LoadXWAV(strFile.c_str());
  if (!m_adpcm)
  {
    CLog::Log(LOGERROR,"ADPCMCodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_Channels = m_dll.GetNumberOfChannels(m_adpcm);
  m_SampleRate = m_dll.GetPlaybackRate(m_adpcm);
  m_BitsPerSample = 16;//m_dll.GetSampleSize(m_adpcm);
  m_TotalTime = m_dll.GetLength(m_adpcm); // fixme?
  m_iDataPos = 0;

  return true;
}

void ADPCMCodec::DeInit()
{
  if (m_adpcm)
    m_dll.FreeXWAV(m_adpcm);

  m_adpcm = NULL;
  m_bIsPlaying = false;
}

__int64 ADPCMCodec::Seek(__int64 iSeekTime)
{
  m_dll.Seek(m_adpcm,(int)iSeekTime);
  return iSeekTime;
}

int ADPCMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_adpcm)
    return READ_ERROR;

  *actualsize  = m_dll.FillBuffer(m_adpcm,(char*)pBuffer,size);

  if (*actualsize == 0)
    return READ_ERROR;

  return READ_SUCCESS;
}

bool ADPCMCodec::CanInit()
{
  return m_dll.CanLoad();
}

