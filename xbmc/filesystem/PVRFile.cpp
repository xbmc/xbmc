/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "PVRFile.h"
#include "cores/dvdplayer/DVDInputStreams/DVDInputStream.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/addons/PVRClients.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace PVR;

CPVRFile::CPVRFile()
{
  m_isPlayRecording = false;
  m_playingItem     = -1;
}

CPVRFile::~CPVRFile()
{
}

bool CPVRFile::Open(const CURL& url)
{
  Close();

  if (!g_PVRManager.IsStarted())
    return false;

  std::string strURL = url.Get();

  if (StringUtils::StartsWith(strURL, "pvr://channels/tv/") ||
      StringUtils::StartsWith(strURL, "pvr://channels/radio/"))
  {
    CFileItemPtr tag = g_PVRChannelGroups->GetByPath(strURL);
    if (tag && tag->HasPVRChannelInfoTag())
    {
      if (!g_PVRManager.OpenLiveStream(*tag))
        return false;

      m_isPlayRecording = false;
      CLog::Log(LOGDEBUG, "PVRFile - %s - playback has started on filename %s", __FUNCTION__, strURL.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "PVRFile - %s - channel not found with filename %s", __FUNCTION__, strURL.c_str());
      return false;
    }
  }
  else if (StringUtils::StartsWith(strURL, "pvr://recordings/active"))
  {
    CFileItemPtr tag = g_PVRRecordings->GetByPath(strURL);
    if (tag && tag->HasPVRRecordingInfoTag())
    {
      if (!g_PVRManager.OpenRecordedStream(tag->GetPVRRecordingInfoTag()))
        return false;

      m_isPlayRecording = true;
      CLog::Log(LOGDEBUG, "%s - playback has started on recording %s (%s)", __FUNCTION__, strURL.c_str(), tag->GetPVRRecordingInfoTag()->m_strIconPath.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "PVRFile - Recording not found with filename %s", strURL.c_str());
      return false;
    }
  }
  else if (StringUtils::StartsWith(strURL, "pvr://recordings/deleted/"))
  {
    CLog::Log(LOGNOTICE, "PVRFile - Playback of deleted recordings is not possible (%s)", strURL.c_str());
    return false;
  }
  else
  {
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, strURL.c_str());
    return false;
  }

  return true;
}

void CPVRFile::Close()
{
  g_PVRManager.CloseStream();
}

ssize_t CPVRFile::Read(void* buffer, size_t size)
{
  if (size > SSIZE_MAX)
    size = SSIZE_MAX;

  if (!g_PVRManager.IsStarted())
    return -1;

  // TODO: Fix overflow in case of sizeof(int) != sizeof(size_t)
  const int ret = g_PVRClients->ReadStream((BYTE*)buffer, size);
  if (ret < 0)
    return -1;

  return ret;
}

int64_t CPVRFile::GetLength()
{
  return g_PVRManager.IsStarted() ? g_PVRClients->GetStreamLength() : 0;
}

int64_t CPVRFile::Seek(int64_t pos, int whence)
{
  return g_PVRManager.IsStarted() ? g_PVRClients->SeekStream(pos, whence) : 0;
}

int64_t CPVRFile::GetPosition()
{
  return g_PVRManager.IsStarted() ? g_PVRClients->GetStreamPosition() : 0;
}

int CPVRFile::GetTotalTime()
{
  // for recordings leave this to demuxer
  return m_isPlayRecording ? 0 : g_PVRManager.GetTotalTime();
}

int CPVRFile::GetStartTime()
{
  return g_PVRManager.GetStartTime();
}

bool CPVRFile::NextChannel(bool preview/* = false*/)
{
  unsigned int newchannel;

  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    return true;
  }

  /* Do channel switch and save new channel number, it is not always
   * increased by one in a case if next channel is encrypted or we
   * on the beginning or end of the channel list!
   */
  if (g_PVRManager.ChannelUp(&newchannel, preview))
  {
    m_playingItem = newchannel;
    return true;
  }
  else
  {
    return false;
  }
}

bool CPVRFile::PrevChannel(bool preview/* = false*/)
{
  unsigned int newchannel;

  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    return true;
  }

  /* Do channel switch and save new channel number, it is not always
   * increased by one in a case if next channel is encrypted or we
   * on the beginning or end of the channel list!
   */
  if (g_PVRManager.ChannelDown(&newchannel, preview))
  {
    m_playingItem = newchannel;
    return true;
  }
  else
  {
    return false;
  }
}

bool CPVRFile::SelectChannel(unsigned int channel)
{
  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip channelswitch */
    /** TODO:
     ** Add support for cutting keys (functions becomes the numeric keys as integer)
     **/
    return true;
  }

  if (g_PVRManager.ChannelSwitch(channel))
  {
    m_playingItem = channel;
    return true;
  }
  else
  {
    return false;
  }
}

bool CPVRFile::UpdateItem(CFileItem& item)
{
  return g_PVRManager.UpdateItem(item);
}

std::string CPVRFile::TranslatePVRFilename(const std::string& pathFile)
{
  if (!g_PVRManager.IsStarted())
    return "";

  std::string FileName = pathFile;
  if (FileName.substr(0, 14) == "pvr://channels")
  {
    CFileItemPtr channel = g_PVRChannelGroups->GetByPath(FileName);
    if (channel && channel->HasPVRChannelInfoTag())
    {
      std::string stream = channel->GetPVRChannelInfoTag()->StreamURL();
      if(!stream.empty())
      {
        if (stream.compare(6, 7, "stream/") == 0)
        {
          // pvr://stream
          // This function was added to retrieve the stream URL for this item
          // Is is used for the MediaPortal (ffmpeg) PVR addon
          // see PVRManager.cpp
          return g_PVRClients->GetStreamURL(channel->GetPVRChannelInfoTag());
        }
        else
        {
          return stream;
        }
      }
    }
  }
  return FileName;
}

bool CPVRFile::CanRecord()
{
  if (m_isPlayRecording || !g_PVRManager.IsStarted())
    return false;

  return g_PVRClients->CanRecordInstantly();
}

bool CPVRFile::IsRecording()
{
  return g_PVRManager.IsStarted() && g_PVRClients->IsRecordingOnPlayingChannel();
}

bool CPVRFile::Record(bool bOnOff)
{
  return g_PVRManager.StartRecordingOnPlayingChannel(bOnOff);
}

bool CPVRFile::Delete(const CURL& url)
{
  if (!g_PVRManager.IsStarted())
    return false;

  std::string path(url.GetFileName());
  if (StringUtils::StartsWith(path, "recordings/") && path[path.size()-1] != '/')
  {
    std::string strURL = url.Get();
    CFileItemPtr tag = g_PVRRecordings->GetByPath(strURL);
    if (tag && tag->HasPVRRecordingInfoTag())
      return tag->GetPVRRecordingInfoTag()->Delete();
  }
  return false;
}

bool CPVRFile::Rename(const CURL& url, const CURL& urlnew)
{
  if (!g_PVRManager.IsStarted())
    return false;

  std::string path(url.GetFileName());
  std::string newname(urlnew.GetFileName());

  size_t found = newname.find_last_of("/");
  if (found != std::string::npos)
    newname = newname.substr(found+1);

  if (StringUtils::StartsWith(path, "recordings/active/") && path[path.size()-1] != '/')
  {
    std::string strURL = url.Get();
    CFileItemPtr tag = g_PVRRecordings->GetByPath(strURL);
    if (tag && tag->HasPVRRecordingInfoTag())
      return tag->GetPVRRecordingInfoTag()->Rename(newname);
  }
  return false;
}

bool CPVRFile::Exists(const CURL& url)
{
  return g_PVRManager.IsStarted() &&
      g_PVRRecordings->GetByPath(url.Get())->HasPVRRecordingInfoTag();
}

int CPVRFile::IoControl(EIoControl request, void *param)
{
  if (request == IOCTRL_SEEK_POSSIBLE)
  {
    if (!g_PVRManager.IsStarted())
      return 0;
    else if (g_PVRClients->CanSeekStream())
      return 1;
    else
      return 0;
  }

  return -1;
}
