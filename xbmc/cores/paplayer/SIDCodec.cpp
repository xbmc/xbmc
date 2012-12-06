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

#include "SIDCodec.h"
#include "cores/DllLoader/DllLoader.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

SIDCodec::SIDCodec()
{
  m_CodecName = "sid";
  m_sid = 0;
  m_iTrack = -1;
  m_iDataPos = -1;
}

SIDCodec::~SIDCodec()
{
  DeInit();
}

bool SIDCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously

  m_dll.Init();

  CStdString strFileToLoad = strFile;
  m_iTrack = 0;
  CStdString strExtension;
  URIUtils::GetExtension(strFile,strExtension);
  strExtension.MakeLower();
  if (strExtension==".sidstream")
  {
    //  Extract the track to play
    CStdString strFileName=URIUtils::GetFileName(strFile);
    int iStart=strFileName.ReverseFind('-')+1;
    m_iTrack = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str());
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFile;
    URIUtils::GetDirectory(strPath, strFileToLoad);
    URIUtils::RemoveSlashAtEnd(strFileToLoad); // we want the filename
  }

  m_sid = m_dll.LoadSID(strFileToLoad.c_str());
  if (!m_sid)
  {
    CLog::Log(LOGERROR,"SIDCodec: error opening file %s!",strFile.c_str());
    return false;
  }

  m_Channels = 1;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = 4*60*1000;
  m_DataFormat = AE_FMT_S16NE;

  return true;
}

void SIDCodec::DeInit()
{
  if (m_sid)
    m_dll.FreeSID(m_sid);
  m_sid = 0;
}

int64_t SIDCodec::Seek(int64_t iSeekTime)
{
  char temp[3840*2];
  if (m_iDataPos > iSeekTime/1000*48000*2)
  {
    m_dll.StartPlayback(m_sid,m_iTrack);
    m_iDataPos = 0;
  }

  while (m_iDataPos < iSeekTime/1000*48000*2)
  {
    int64_t iRead = iSeekTime/1000*48000*2-m_iDataPos;
    if (iRead > 3840*2)
    {
      m_dll.SetSpeed(m_sid,32*100);
      iRead = 3840*2;
    }
    else
      m_dll.SetSpeed(m_sid,100);

    iRead = m_dll.FillBuffer(m_sid,temp,int(iRead));
    if (!iRead)
      break; // get out of here
    if (iRead == 3840*2)
      m_iDataPos += iRead*32;
    else m_iDataPos += iRead;
  }
  return iSeekTime;
}

int SIDCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_dll.StartPlayback(m_sid,m_iTrack);
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*48000*2)
    return READ_EOF;

  m_dll.SetSpeed(m_sid,100);
  if ((*actualsize=m_dll.FillBuffer(m_sid,pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool SIDCodec::CanInit()
{
  return m_dll.CanLoad();
}

CAEChannelInfo SIDCodec::GetChannelInfo()
{
  static enum AEChannel map[1][2] = {
    {AE_CH_FC, AE_CH_NULL}
  };

  if (m_Channels > 1)
    return CAEUtil::GuessChLayout(m_Channels);

  return CAEChannelInfo(map[m_Channels - 1]);
}