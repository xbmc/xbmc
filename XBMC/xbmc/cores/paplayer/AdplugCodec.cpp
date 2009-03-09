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
#include "AdplugCodec.h"

AdplugCodec::AdplugCodec()
{
  m_CodecName = "Adplug";
  m_adl = 0;
  m_iTrack = -1;
  m_iDataPos = -1;
}

AdplugCodec::~AdplugCodec()
{
  DeInit();
}

bool AdplugCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously

  m_dll.Init();

  CStdString strFileToLoad = strFile;
  /*m_iTrack = 0;
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  strExtension.MakeLower();
  if (strExtension==".adplugstream")
  {
    //  Extract the track to play
    CStdString strFileName=CUtil::GetFileName(strFile);
    int iStart=strFileName.ReverseFind("-")+1;
    m_iTrack = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str());
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFile;
    CUtil::GetDirectory(strPath, strFileToLoad);
    CUtil::RemoveSlashAtEnd(strFileToLoad); // we want the filename
  }*/

  m_adl = m_dll.LoadADL(strFileToLoad.c_str());
  if (!m_adl)
  {
    CLog::Log(LOGERROR,"AdplugCodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)m_dll.GetLength(m_adl);

  return true;
}

void AdplugCodec::DeInit()
{
  if (m_adl)
    m_dll.FreeADL(m_adl);
  m_adl = 0;
}

__int64 AdplugCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_adl,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*48000*4;

  return result;
}

int AdplugCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*48000*4)
    return READ_EOF;

  if ((*actualsize=m_dll.FillBuffer(m_adl,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool AdplugCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool AdplugCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "a2m" || strExt == "amd" || strExt == "bam" || strExt == "cff" || strExt == "cfm" || strExt == "xad"
      || strExt == "d00" || strExt == "dfm" || strExt == "dmo" || strExt == "dro" || strExt == "dtm" || strExt == "hsc"
      || strExt == "hsp" || strExt == "imf" || strExt == "ksm" || strExt == "laa" || strExt == "lds" || strExt == "m"
      || strExt == "mad" || strExt == "mkj" || strExt == "mtk" || strExt == "rad" || strExt == "raw"
      || strExt == "rol" || strExt == "s3m" || strExt == "sa2" || strExt == "sat" || strExt == "sci" || strExt == "sng"
      || strExt == "xms" || strExt == "xsm" || strExt == "adplug")
    return true;

  return false;
}

