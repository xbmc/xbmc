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
#include "CubeCodec.h"

CubeCodec::CubeCodec()
{
  m_CodecName = "Cube";
  m_adx = 0;
  m_iDataPos = -1; 
}

CubeCodec::~CubeCodec()
{
  DeInit();
}

bool CubeCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_dll.Init();

  CStdString strFileToLoad = strFile;
  strFileToLoad = "filereader://"+strFileToLoad;
  
  m_adx = m_dll.LoadADX(strFileToLoad.c_str(),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  if (!m_adx)
  {
    CLog::Log(LOGERROR,"CubeCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_TotalTime = (__int64)m_dll.GetLength(m_adx);

  return true;
}

void CubeCodec::DeInit()
{
  if (m_adx)
    m_dll.FreeADX(m_adx);
  m_adx = 0;
}

__int64 CubeCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_adx,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*m_SampleRate*m_BitsPerSample*m_Channels/8;
  
  return result;
}

int CubeCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*m_BitsPerSample*m_Channels/8)
  {
    return READ_EOF;
  }
  
  if ((*actualsize=m_dll.FillBuffer(m_adx,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool CubeCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool CubeCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "adx" || strExt == "dsp" || strExt == "adp" || strExt == "ymf" || strExt == "ast" || strExt == "afc" || strExt == "hps"  || strExt == "waa" || strExt == "wvs" || strExt == "wam" || strExt == "gcm" || strExt == "idsp" || strExt == "mpdsp" || strExt == "mss" || strExt == "spt" || strExt == "rsd")
    return true;
  
  return false;
}

