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

#include "OGGcodec.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "music/tags/TagLoaderTagLib.h"

OGGCodec::OGGCodec() : m_callback(m_file)
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_DataFormat = AE_FMT_INVALID;
  m_Bitrate = 0;
  m_CodecName = "ogg";
  m_TimeOffset = 0.0;
  m_CurrentStream=0;
  m_TotalTime = 0;
  m_inited = false;
  memset(&m_VorbisFile, 0, sizeof(m_VorbisFile));
}

OGGCodec::~OGGCodec()
{
  DeInit();
}

bool OGGCodec::Init(const CStdString &strFile1, unsigned int filecache)
{
  if (m_inited)
    return true;
  CStdString strFile=strFile1;
  if (!m_dll.Load())
    return false;

  m_CurrentStream=0;

  CStdString strExtension;
  URIUtils::GetExtension(strFile, strExtension);

  //  A bitstream inside a ogg file?
  if (strExtension==".oggstream")
  {
    //  Extract the bitstream to play
    CStdString strFileName=URIUtils::GetFileName(strFile);
    int iStart=strFileName.ReverseFind('-')+1;
    m_CurrentStream = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str())-1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFile;
    URIUtils::GetDirectory(strPath, strFile);
    URIUtils::RemoveSlashAtEnd(strFile); // we want the filename
  }

  CFileItem item(strFile, false);

  //  Open the file to play
  if (!m_file.Open(strFile, READ_CACHED))
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't open %s", strFile1.c_str());
    return false;
  }

  //  setup ogg i/o callbacks
  ov_callbacks oggIOCallbacks = m_callback.Get(strFile);

  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks(&m_callback, &m_VorbisFile, NULL, 0, oggIOCallbacks)!=0)
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't open decoder for %s", strFile1.c_str());
    return false;
  }

  long iStreams=m_dll.ov_streams(&m_VorbisFile);
  if (iStreams>1)
  {
    if (m_CurrentStream > iStreams)
      return false;
  }

  //  Calculate the offset in secs where the bitstream starts
  for (int i=0; i<m_CurrentStream; ++i)
    m_TimeOffset += m_dll.ov_time_total(&m_VorbisFile, i);

  //  get file info
  vorbis_info* pInfo=m_dll.ov_info(&m_VorbisFile, m_CurrentStream);
  if (!pInfo)
  {
    CLog::Log(LOGERROR, "OGGCodec: Can't get stream info from %s", strFile1.c_str());
    return false;
  }

  m_SampleRate = pInfo->rate;
  m_Channels = pInfo->channels;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  if (item.IsInternetStream())
    m_TotalTime = -1;
  else
    m_TotalTime = (int64_t)m_dll.ov_time_total(&m_VorbisFile, m_CurrentStream)*1000;
  m_Bitrate = pInfo->bitrate_nominal;
  if (m_Bitrate == 0 && m_TotalTime > 0 && !item.IsInternetStream())
    m_Bitrate = (int)(m_file.GetLength()*8 / (m_TotalTime / 1000));

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0)
  {
    CLog::Log(LOGERROR, "OGGCodec: incomplete stream info from %s, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%"PRIu64, strFile1.c_str(), m_SampleRate, m_Channels, m_BitsPerSample, m_TotalTime);
    return false;
  }

  //  Get replay gain tags
  vorbis_comment* pComments=m_dll.ov_comment(&m_VorbisFile, m_CurrentStream);
  if (pComments)
  {
    CTagLoaderTagLib tagLoaderTagLib;
    tagLoaderTagLib.Load(strFile, m_tag);
  }

  //  Seek to the logical bitstream to play
  if (m_TimeOffset>0.0)
  {
    if (m_dll.ov_time_seek(&m_VorbisFile, m_TimeOffset)!=0)
    {
      CLog::Log(LOGERROR, "OGGCodec: Can't seek to the bitstream start time (%s)", strFile1.c_str());
      return false;
    }
  }

  return true;
}

void OGGCodec::DeInit()
{
  if (m_VorbisFile.datasource)
    m_dll.ov_clear(&m_VorbisFile);
  m_VorbisFile.datasource = NULL;
  m_inited = false;
}

int64_t OGGCodec::Seek(int64_t iSeekTime)
{
  if (m_dll.ov_time_seek(&m_VorbisFile, m_TimeOffset+(double)(iSeekTime/1000.0f))!=0)
    return 0;

  return iSeekTime;
}

int OGGCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iBitStream=-1;

  //  the maximum chunk size the vorbis decoder seem to return with one call is 4096
  long lRead=m_dll.ov_read(&m_VorbisFile, (char*)pBuffer, size, 0, 2, 1, &iBitStream);

  if (lRead == OV_HOLE)
    return READ_SUCCESS;

  //  Our logical bitstream changed, we reached the eof
  if (lRead > 0 && m_CurrentStream!=iBitStream)
    lRead=0;

  if (lRead<0)
  {
    CLog::Log(LOGERROR, "OGGCodec: Read error %lu", lRead);
    return READ_ERROR;
  }
  else if (lRead==0)
    return READ_EOF;
  else
    *actualsize=lRead;

  return READ_SUCCESS;
}

bool OGGCodec::CanInit()
{
  return m_dll.CanLoad();
}

CAEChannelInfo OGGCodec::GetChannelInfo()
{
  static enum AEChannel map[8][9] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FC, AE_CH_FR, AE_CH_SL, AE_CH_SR, AE_CH_BL, AE_CH_BR, AE_CH_LFE, AE_CH_NULL}
  };

  if (m_Channels > 8)
    return CAEUtil::GuessChLayout(m_Channels);

  return CAEChannelInfo(map[m_Channels - 1]);
}
