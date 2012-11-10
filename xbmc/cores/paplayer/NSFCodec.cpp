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

#include "NSFCodec.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"

NSFCodec::NSFCodec()
{
  m_iTrack = 0;
  m_CodecName = "nsf";
  m_nsf = NULL;
  m_bIsPlaying = false;
  m_szBuffer = NULL;
  m_szStartOfBuffer = NULL;
  m_iDataInBuffer = 0;
  m_iDataPos = 0;
}

NSFCodec::~NSFCodec()
{
  DeInit();
}

bool NSFCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  DeInit();

  if (!m_dll.Load())
    return false; // error logged previously

  CStdString strFileToLoad = strFile;
  m_iTrack = 0;
  CStdString strExtension;
  URIUtils::GetExtension(strFile,strExtension);
  strExtension.MakeLower();
  if (strExtension==".nsfstream")
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

  m_nsf = m_dll.LoadNSF(strFileToLoad.c_str());
  if (!m_nsf)
  {
    CLog::Log(LOGERROR,"NSFCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_Channels = 1;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  m_TotalTime = 4*60*1000; // fixme?
  m_iDataPos = 0;

  return true;
}

void NSFCodec::DeInit()
{
  if (m_nsf)
    m_dll.FreeNSF(m_nsf);

  m_nsf = 0;
  m_bIsPlaying = false;
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
}

int64_t NSFCodec::Seek(int64_t iSeekTime)
{
  if (m_iDataPos > iSeekTime/1000*48000*2)
  {
    m_dll.StartPlayback(m_nsf,m_iTrack);
    m_iDataPos = 0;
  }
  while (m_iDataPos+2*48000/m_dll.GetPlaybackRate(m_nsf)*2 < iSeekTime/1000*48000*2)
  {
    m_dll.FrameAdvance(m_nsf);

    m_iDataInBuffer = 48000/m_dll.GetPlaybackRate(m_nsf)*2;
    m_szStartOfBuffer = m_szBuffer;
    m_iDataPos += 48000/m_dll.GetPlaybackRate(m_nsf)*2;
  }
  m_dll.FillBuffer(m_nsf,m_szBuffer,48000/m_dll.GetPlaybackRate(m_nsf)); // *2 since two channels
  if (iSeekTime/1000*48000*2 > 48000/m_dll.GetPlaybackRate(m_nsf)*2)
    m_iDataPos += 48000/m_dll.GetPlaybackRate(m_nsf)*2;
  else
    m_iDataPos = 0;
  m_iDataInBuffer -= int(iSeekTime/1000*48000*2-m_iDataPos);
  m_szStartOfBuffer += (iSeekTime/1000*48000*2-m_iDataPos);
  m_iDataPos = iSeekTime/1000*48000*2;

  return iSeekTime;
}

int NSFCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_nsf)
    return READ_ERROR;

  if (m_iDataPos >= m_TotalTime/1000*48000*2)
    return READ_EOF;

  if (!m_bIsPlaying)
  {
    m_dll.StartPlayback(m_nsf,m_iTrack);
    m_bIsPlaying = true;
    m_szBuffer = new char[48000/m_dll.GetPlaybackRate(m_nsf)*2];
    m_szStartOfBuffer = m_szBuffer;
    m_iDataPos = 0;
  }

  if (m_iDataInBuffer <= 0)
  {
    m_iDataInBuffer = m_dll.FillBuffer(m_nsf,m_szBuffer,48000/m_dll.GetPlaybackRate(m_nsf)); // *2 since two channels

    m_szStartOfBuffer = m_szBuffer;
  }

  *actualsize = size<m_iDataInBuffer?size:m_iDataInBuffer;
  memcpy(pBuffer,m_szStartOfBuffer,*actualsize);
  m_szStartOfBuffer += *actualsize;
  m_iDataInBuffer -= *actualsize;
  m_iDataPos += *actualsize;

  return READ_SUCCESS;
}

bool NSFCodec::CanInit()
{
  return m_dll.CanLoad();
}
