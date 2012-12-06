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

#include "threads/SystemClock.h"
#include "system.h"
#include "URL.h"
#include "FileItem.h"
#include "DllHDHomeRun.h"
#include "HDHomeRunFile.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "Util.h"

using namespace XFILE;
using namespace std;

// -------------------------------------------
// ------------------ File -------------------
// -------------------------------------------
CHomeRunFile::CHomeRunFile()
{
  m_device = NULL;
  m_pdll = new DllHdHomeRun;
  m_pdll->Load();
}

CHomeRunFile::~CHomeRunFile()
{
  Close();
  delete m_pdll;
}

bool CHomeRunFile::Exists(const CURL& url)
{
  CStdString path(url.GetFileName());

  /*
   * HDHomeRun URLs are of the form hdhomerun://1014F6D1/tuner0?channel=qam:108&program=10
   * The filename starts with "tuner" and has no extension. This check will cover off requests
   * for *.tbn, *.jpg, *.jpeg, *.edl etc. that do not exist.
   */
  if(path.Left(5) == "tuner"
  && URIUtils::GetExtension(path).IsEmpty())
    return true;

  return false;
}

int64_t CHomeRunFile::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

int CHomeRunFile::Stat(const CURL& url, struct __stat64* buffer)
{
  memset(buffer, 0, sizeof(struct __stat64));
  return 0;
}

int64_t CHomeRunFile::GetPosition()
{
  return 0;
}

int64_t CHomeRunFile::GetLength()
{
  return 0;
}

bool CHomeRunFile::Open(const CURL &url)
{
  if(!m_pdll->IsLoaded())
    return false;

  m_device = m_pdll->device_create_from_str(url.GetHostName().c_str(), NULL);
  if(!m_device)
    return false;

  m_pdll->device_set_tuner_from_str(m_device, url.GetFileName().c_str());

  if(url.HasOption("channel"))
    m_pdll->device_set_tuner_channel(m_device, url.GetOption("channel").c_str());

  if(url.HasOption("program"))
    m_pdll->device_set_tuner_program(m_device, url.GetOption("program").c_str());

  // start streaming from selected device and tuner
  if( m_pdll->device_stream_start(m_device) <= 0 )
    return false;

  return true;
}

unsigned int CHomeRunFile::Read(void* lpBuf, int64_t uiBufSize)
{
  size_t datasize;

  if(uiBufSize < VIDEO_DATA_PACKET_SIZE)
    CLog::Log(LOGWARNING, "CHomeRunFile::Read - buffer size too small, will most likely fail");

  // for now, let it it time out after 5 seconds,
  // neither of the players can be forced to
  // continue even if read return 0 as can happen
  // on live streams.
  XbmcThreads::EndTime timestamp(5000);
  while(1)
  {
    datasize = (size_t) uiBufSize;
    uint8_t* ptr = m_pdll->device_stream_recv(m_device, datasize, &datasize);
    if(ptr)
    {
      memcpy(lpBuf, ptr, datasize);
      return (unsigned int)datasize;
    }

    if(timestamp.IsTimePast())
      return 0;

    Sleep(64);
  }
  return (unsigned int)datasize;
}

void CHomeRunFile::Close()
{
  if(m_device)
  {
    m_pdll->device_stream_stop(m_device);
    m_pdll->device_destroy(m_device);
    m_device = NULL;
  }
}

int CHomeRunFile::GetChunkSize()
{
  return VIDEO_DATA_PACKET_SIZE;
}
