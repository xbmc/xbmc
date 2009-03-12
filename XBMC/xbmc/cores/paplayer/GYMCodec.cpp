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

#include "stdafx.h"
#include "GYMCodec.h"
#include "cores/DllLoader/DllLoader.h"

GYMCodec::GYMCodec()
{
  m_CodecName = L"GYM";
  m_iDataInBuffer = 0;
  m_szBuffer = NULL;
  m_gym = 0;
  m_iDataPos = 0;
}

GYMCodec::~GYMCodec()
{
  DeInit();
}

bool GYMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously

  m_iBufferSize = m_dll.Init();

  m_gym = m_dll.LoadGYM(strFile.c_str());
  if (!m_gym)
  {
    CLog::Log(LOGERROR,"GYMCodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = m_dll.GetLength(m_gym);
  m_szBuffer = new char[m_iBufferSize];
  m_iDataPos = 0;

  return true;
}

void GYMCodec::DeInit()
{
  if (m_gym)
    m_dll.FreeGYM(m_gym);
  m_gym = 0;

  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
}

__int64 GYMCodec::Seek(__int64 iSeekTime)
{
  m_dll.Seek(m_gym,(unsigned int) (iSeekTime/1000*60));

  m_iDataPos = iSeekTime/1000*48000*4;
  return iSeekTime;
}

int GYMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos >= m_TotalTime/1000*48000*4)
    return READ_EOF;

  if (m_iDataInBuffer <= 0)
  {
    if (!m_dll.FillBuffer(m_gym,m_szBuffer))
      return READ_ERROR;
    m_iDataInBuffer = m_iBufferSize;
    m_szStartOfBuffer = m_szBuffer;
  }

  *actualsize= size<m_iDataInBuffer?size:m_iDataInBuffer;
  memcpy(pBuffer,m_szStartOfBuffer,*actualsize);
  m_szStartOfBuffer += *actualsize;
  m_iDataInBuffer -= *actualsize;
  m_iDataPos += *actualsize;

  return READ_SUCCESS;
}

bool GYMCodec::CanInit()
{
  return m_dll.CanLoad();
}
