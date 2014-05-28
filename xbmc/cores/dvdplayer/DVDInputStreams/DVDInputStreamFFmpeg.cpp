/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMFFMPEG_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMFFMPEG_H_INCLUDED
#include "DVDInputStreamFFmpeg.h"
#endif

#ifndef DVDINPUTSTREAMS_XBMC_PLAYLISTS_PLAYLISTM3U_H_INCLUDED
#define DVDINPUTSTREAMS_XBMC_PLAYLISTS_PLAYLISTM3U_H_INCLUDED
#include "xbmc/playlists/PlayListM3U.h"
#endif

#ifndef DVDINPUTSTREAMS_SETTINGS_SETTINGS_H_INCLUDED
#define DVDINPUTSTREAMS_SETTINGS_SETTINGS_H_INCLUDED
#include "settings/Settings.h"
#endif

#ifndef DVDINPUTSTREAMS_UTIL_H_INCLUDED
#define DVDINPUTSTREAMS_UTIL_H_INCLUDED
#include "Util.h"
#endif

#ifndef DVDINPUTSTREAMS_UTILS_LOG_H_INCLUDED
#define DVDINPUTSTREAMS_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif


using namespace XFILE;

CDVDInputStreamFFmpeg::CDVDInputStreamFFmpeg()
  : CDVDInputStream(DVDSTREAM_TYPE_FFMPEG)
  , m_can_pause(false)
  , m_can_seek(false)
  , m_aborted(false)
{

}

CDVDInputStreamFFmpeg::~CDVDInputStreamFFmpeg()
{
  Close();
}

bool CDVDInputStreamFFmpeg::IsEOF()
{
  if(m_aborted)
    return true;
  else
    return false;
}

bool CDVDInputStreamFFmpeg::Open(const char* strFile, const std::string& content)
{
  CFileItem item(strFile, false);
  std::string selected;
  if (item.IsInternetStream() && item.IsType(".m3u8"))
  {
    // get the available bandwidth and  determine the most appropriate stream
    int bandwidth = CSettings::Get().GetInt("network.bandwidth");
    if(bandwidth <= 0)
      bandwidth = INT_MAX;
    selected = PLAYLIST::CPlayListM3U::GetBestBandwidthStream(strFile, bandwidth);
    if (selected.compare(strFile) != 0)
    {
      CLog::Log(LOGINFO, "CDVDInputStreamFFmpeg: Auto-selecting %s based on configured bandwidth.", selected.c_str());
      strFile = selected.c_str();
    }
  }

  if (!CDVDInputStream::Open(strFile, content))
    return false;

  m_can_pause = true;
  m_can_seek  = true;
  m_aborted   = false;

  if(strnicmp(strFile, "udp://", 6) == 0
  || strnicmp(strFile, "rtp://", 6) == 0)
  {
    m_can_pause = false;
    m_can_seek  = false;
  }

  if(strnicmp(strFile, "tcp://", 6) == 0)
  {
    m_can_pause = true;
    m_can_seek  = false;
  }
  return true;
}

// close file and reset everyting
void CDVDInputStreamFFmpeg::Close()
{
  CDVDInputStream::Close();
}

int CDVDInputStreamFFmpeg::Read(uint8_t* buf, int buf_size)
{
  return -1;
}

int64_t CDVDInputStreamFFmpeg::GetLength()
{
  return 0;
}

int64_t CDVDInputStreamFFmpeg::Seek(int64_t offset, int whence)
{
  return -1;
}

