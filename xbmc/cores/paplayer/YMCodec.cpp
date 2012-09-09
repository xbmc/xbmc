/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "YMCodec.h"
#include "cores/DllLoader/DllLoader.h"
#include "utils/log.h"

YMCodec::YMCodec()
{
  m_CodecName = "YM";
  m_ym = 0;
  m_iDataPos = -1;
}

YMCodec::~YMCodec()
{
  DeInit();
}

bool YMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously

  m_ym = m_dll.LoadYM(strFile.c_str());
  if (!m_ym)
  {
    CLog::Log(LOGERROR,"YMCodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_Channels = 1;
  m_SampleRate = 44100;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  m_TotalTime = m_dll.GetLength(m_ym)*1000;

  return true;
}

void YMCodec::DeInit()
{
  if (m_ym)
    m_dll.FreeYM(m_ym);
  m_ym = 0;
}

int64_t YMCodec::Seek(int64_t iSeekTime)
{
  return m_dll.Seek(m_ym,(unsigned long)iSeekTime);
}

int YMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if ((*actualsize=m_dll.FillBuffer(m_ym,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool YMCodec::CanInit()
{
  return m_dll.CanLoad();
}

