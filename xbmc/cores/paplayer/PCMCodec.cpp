/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PCMCodec.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "utils/StringUtils.h"

PCMCodec::PCMCodec()
{
  m_CodecName = "PCM";
  m_TotalTime = 0;
  m_SampleRate = 44100;
  m_Channels = 2;
  m_BitsPerSample = 16;
  m_Bitrate = m_SampleRate * m_Channels * m_BitsPerSample;
}

PCMCodec::~PCMCodec()
{
  DeInit();
}

bool PCMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Close();
  if (!m_file.Open(strFile, READ_CACHED))
  {
    CLog::Log(LOGERROR, "PCMCodec::Init - Failed to open file");
    return false;
  }

  int64_t length = m_file.GetLength();

  if (m_Bitrate)
    m_TotalTime = 1000 * 8 * length / m_Bitrate;

  m_file.Seek(0, SEEK_SET);

  return true;
}

void PCMCodec::DeInit()
{
  m_file.Close();
}

int64_t PCMCodec::Seek(int64_t iSeekTime)
{
  m_file.Seek((iSeekTime / 1000) * (m_Bitrate / 8));
  return iSeekTime;
}

int PCMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = 0;

  int iAmountRead = m_file.Read(pBuffer, 2 * (size / 2));
  if (iAmountRead > 0)
  {
    uint16_t *buffer = (uint16_t*) pBuffer;

    iAmountRead = 2 * (iAmountRead / 2);

    for (int i = 0; i < (iAmountRead / 2); i++)
      buffer[i] = Endian_SwapBE16(buffer[i]);   // L16 PCM is in network byte order (Big Endian)

    *actualsize = iAmountRead;

    return READ_SUCCESS;
  }
  return READ_ERROR;
}

bool PCMCodec::CanInit()
{
  return true;
}

void PCMCodec::SetMimeParams(const CStdString& strMimeParams)
{
  CStdStringArray mimeParams;

  // if there are no parameters, the default is 2 channels, 44100 samples/sec
  m_Channels = 2;
  m_SampleRate = 44100;

  StringUtils::SplitString(strMimeParams, ";", mimeParams);
  for (size_t i = 0; i < mimeParams.size(); i++)
  {
    CStdStringArray thisParam;
    StringUtils::SplitString(mimeParams[i], "=", thisParam, 2);
    if (thisParam.size() > 1)
    {
      if (thisParam[0] == "rate")
      {
        m_SampleRate = atoi(thisParam[1].Trim());
      }
      else if (thisParam[0] == "channels")
      {
        m_Channels = atoi(thisParam[1].Trim());
      }
    }
  }

  m_Bitrate = m_SampleRate * m_Channels * m_BitsPerSample;
}
