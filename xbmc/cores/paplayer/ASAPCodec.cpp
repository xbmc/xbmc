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

#include "ASAPCodec.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"

ASAPCodec::ASAPCodec()
{
  m_CodecName = "asap";
}

ASAPCodec::~ASAPCodec()
{
}

bool ASAPCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false;

  CStdString strFileToLoad = strFile;
  int song = -1;
  CStdString strExtension;
  URIUtils::GetExtension(strFile, strExtension);
  strExtension.MakeLower();
  if (strExtension == ".asapstream")
  {
    CStdString strFileName = URIUtils::GetFileName(strFile);
    int iStart = strFileName.ReverseFind('-') + 1;
    song = atoi(strFileName.substr(iStart, strFileName.size() - iStart - 11).c_str()) - 1;
    CStdString strPath = strFile;
    URIUtils::GetDirectory(strPath, strFileToLoad);
    URIUtils::RemoveSlashAtEnd(strFileToLoad);
  }

  int duration;
  if (!m_dll.asapLoad(strFileToLoad.c_str(), song, &m_Channels, &duration))
    return false;
  m_TotalTime = duration;
  m_SampleRate = 44100;
  m_BitsPerSample = 16;
  m_DataFormat = AE_FMT_S16NE;
  return true;
}

void ASAPCodec::DeInit()
{
}

int64_t ASAPCodec::Seek(int64_t iSeekTime)
{
  m_dll.asapSeek((int) iSeekTime);
  return iSeekTime;
}

int ASAPCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = m_dll.asapGenerate(pBuffer, size);
  if (*actualsize < size)
    return READ_EOF;
  return READ_SUCCESS;
}

bool ASAPCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool ASAPCodec::IsSupportedFormat(const CStdString &strExt)
{
  if(strExt.empty())
    return false;
  CStdString ext = strExt;
  if (ext[0] == '.')
    ext.erase(0, 1);
  return ext == "sap"
    || ext == "cmc" || ext == "cmr" || ext == "dmc"
    || ext == "mpt" || ext == "mpd" || ext == "rmt"
    || ext == "tmc" || ext == "tm8" || ext == "tm2"
    || ext == "cms" || ext == "cm3" || ext == "dlt";
}
