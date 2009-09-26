/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "PVRFile.h"
#include "Util.h"
#include "PVRManager.h"
#include "utils/log.h"

using namespace XFILE;
using namespace std;

CPVRFile::CPVRFile()
{
  m_isPlayRecording   = false;
  m_playingItem = -1;
}

CPVRFile::~CPVRFile()
{
}

bool CPVRFile::Open(const CURL& url)
{
  Close();

  CStdString strURL;
  url.GetURL(strURL);

  if (strURL.Left(17) == "pvr://channelstv/")
  {
    CStdString channel = strURL.Mid(17);
    CUtil::RemoveExtension(channel);
    m_playingItem = atoi(channel.c_str());

    if (!g_PVRManager.OpenLiveStream(m_playingItem, false))
    {
      return false;
    }
    m_isPlayRecording = false;

    CLog::Log(LOGDEBUG, "%s - TV Channel has started on filename %s", __FUNCTION__, strURL.c_str());
  }
  else if (strURL.Left(20) == "pvr://channelsradio/")
  {
    CStdString channel = strURL.Mid(20);
    CUtil::RemoveExtension(channel);
    m_playingItem = atoi(channel.c_str());

    if (!g_PVRManager.OpenLiveStream(m_playingItem, true))
    {
      return false;
    }
    m_isPlayRecording = false;

    CLog::Log(LOGDEBUG, "%s - Radio Channel has started on filename %s", __FUNCTION__, strURL.c_str());
  }
  else if (strURL.Left(17) == "pvr://recordings/")
  {
    CStdString recording = strURL.Mid(17);
    CUtil::RemoveExtension(recording);
    m_playingItem = atoi(recording.c_str());

    if (!g_PVRManager.OpenRecordedStream(m_playingItem))
    {
      return false;
    }
    m_isPlayRecording = true;
  
    CLog::Log(LOGDEBUG, "%s - Recording has started on filename %s", __FUNCTION__, strURL.c_str());
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

unsigned int CPVRFile::Read(void* buffer, int64_t size)
{
  return g_PVRManager.ReadStream((BYTE*)buffer, size);
}

int64_t CPVRFile::GetLength()
{
  return g_PVRManager.LengthStream();
}

int64_t CPVRFile::Seek(int64_t pos, int whence)
{
  if (whence == SEEK_POSSIBLE)
  {
    int64_t ret = g_PVRManager.SeekStream(pos, whence);

    if (ret >= 0)
    {
      return ret;
    }
    else
    {
      if (g_PVRManager.LengthStream() && g_PVRManager.SeekStream(0, SEEK_CUR) >= 0)
        return 1;
      else
        return 0;
    }
  }
  else
  {
    return g_PVRManager.SeekStream(pos, whence);
  }
  return 0;
}

int CPVRFile::GetTotalTime()
{
  if (g_PVRManager.IsTimeshifting())
    return 0;

  return g_PVRManager.GetTotalTime();
}

int CPVRFile::GetStartTime()
{
  return g_PVRManager.GetStartTime();
}

bool CPVRFile::NextChannel()
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
  if (g_PVRManager.ChannelUp(&newchannel))
  {
    m_playingItem = newchannel;
    return true;
  }
  else
  {
    return false;
  }
}

bool CPVRFile::PrevChannel()
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
  if (g_PVRManager.ChannelDown(&newchannel))
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
  if (m_isPlayRecording)
  {
    /* We are inside a recording, skip item update */
    return true;
  }

  return g_PVRManager.UpdateItem(item);
}

CStdString CPVRFile::TranslatePVRFilename(const CStdString& pathFile)
{
  int playingItem;
  CStdString ret;

  CStdString FileName = pathFile;

  if (FileName.substr(0, 6) == "pvr://")
  {
    ret = FileName;
  }
  if (FileName.substr(0, 5) == "tv://")              /* Live TV */
  {
    FileName.erase(0, 5);
    playingItem = atoi(FileName.c_str());
    ret.Format("pvr://channelstv/%i.ts", playingItem);
  }
  else if (FileName.substr(0, 8) == "radio://")     /* Live Radio */
  {
    FileName.erase(0, 8);
    playingItem = atoi(FileName.c_str());
    ret.Format("pvr://channelsradio/%i.ts", playingItem);
  }
  else if (FileName.substr(0, 9) == "record://")     /* Recorded TV */
  {
    FileName.erase(0, 9);
    playingItem = atoi(FileName.c_str());
    ret.Format("pvr://recordings/%i.ts", playingItem);
  }

  return ret;
}

bool CPVRFile::CanRecord()
{
  if (m_isPlayRecording)
  {
    return false;
  }

  return g_PVRManager.CanInstantRecording();
}

bool CPVRFile::IsRecording()
{
  return g_PVRManager.IsRecordingOnPlayingChannel();
}

bool CPVRFile::Record(bool bOnOff)
{
  return g_PVRManager.StartRecordingOnPlayingChannel(bOnOff);
}
