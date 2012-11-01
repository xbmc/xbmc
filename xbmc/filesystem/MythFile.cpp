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

#include "threads/SystemClock.h"
#include "MythFile.h"
#include "XBDateTime.h"
#include "FileItem.h"
#include "utils/URIUtils.h"
#include "DllLibCMyth.h"
#include "URL.h"
#include "DirectoryCache.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

extern "C" {
#include "cmyth/include/cmyth/cmyth.h"
#include "cmyth/include/refmem/refmem.h"
}

using namespace XFILE;
using namespace std;

static void prog_update_callback(cmyth_proginfo_t prog)
{
  CLog::Log(LOGDEBUG, "%s - prog_update_callback", __FUNCTION__);
}

void CMythFile::OnEvent(int event, const string& data)
{
  CSingleLock lock(m_section);
  m_events.push(make_pair(event, data));
}

bool CMythFile::HandleEvents()
{
  CSingleLock lock(m_section);

  if(m_events.empty())
    return false;

  while(!m_events.empty())
  {
    int event   = m_events.front().first;
    string data = m_events.front().second;
    m_events.pop();

    lock.Leave();

    switch (event) {
    case CMYTH_EVENT_CLOSE:
      Close();
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      {
        if(m_recorder)
          m_dll->livetv_chain_update(m_recorder, (char*)data.c_str(), 4096);
      }
      break;
    }

    lock.Enter();
  }
  return true;
}

bool CMythFile::SetupConnection(const CURL& url, bool control, bool event, bool database)
{
  if(!m_session)
    m_session =  CMythSession::AquireSession(url);

  if(!m_session)
    return false;

  if(!m_dll)
  {
    m_dll = m_session->GetLibrary();
    if(!m_dll)
      return false;
  }

  if(control && !m_control)
  {
    m_control = m_session->GetControl();
    if(!m_control)
      return false;
  }
  if(event)
  {
    if(!m_session->SetListener(this))
      return false;
  }
  if(database && !m_database)
  {
    m_database = m_session->GetDatabase();
    if(!m_database)
      return false;
  }

  return true;
}

bool CMythFile::SetupRecording(const CURL& url)
{
  if (url.GetFileName().Left(11) != "recordings/" &&
      url.GetFileName().Left(7)  != "movies/" &&
      url.GetFileName().Left(8)  != "tvshows/")
    return false;

  if(!SetupConnection(url, true, false, false))
    return false;

  m_filename = url.GetFileNameWithoutPath();

  m_program = m_dll->proginfo_get_from_basename(m_control, m_filename.c_str());
  if(!m_program)
  {
    CLog::Log(LOGERROR, "%s - unable to get find selected file", __FUNCTION__);
    return false;
  }

  m_file = m_dll->conn_connect_file(m_program, m_control, 16*1024, 4096);
  if(!m_file)
  {
    CLog::Log(LOGERROR, "%s - unable to connect to file", __FUNCTION__);
    return false;
  }

  /*
   * proginfo_get_from_basename doesn't return the recording status. Hopefully this will be added to
   * mythbackend eventually.
   *
   * Since cycling through the recorders to check if the program is recording takes some time
   * (depending on the MythTV backend configuration), make some assumptions based on the recording
   * end time since nearly all recordings opened won't be recording.
   */
  m_recording = false;
  CDateTime start = GetValue(m_dll->proginfo_rec_start(m_program));
  CDateTime end   = GetValue(m_dll->proginfo_rec_end(m_program));
  if (end > start // Assume could be recording if empty date comes back as the epoch
  &&  end < CDateTime::GetCurrentDateTime())
    CLog::Log(LOGDEBUG, "%s - Assumed not recording since recording end time before current time: %s",
              __FUNCTION__, end.GetAsLocalizedDateTime().c_str());
  else
  {
    CLog::Log(LOGDEBUG, "%s - Checking recording status using tuners since recording end time NULL or before current time: %s",
              __FUNCTION__, end.GetAsLocalizedDateTime().c_str());
    for(int i=0;i<16 && !m_recording;i++)
    {
      cmyth_recorder_t recorder = m_dll->conn_get_recorder_from_num(m_control, i);
      if(!recorder)
        continue;
      if(m_dll->recorder_is_recording(recorder))
      {
        cmyth_proginfo_t program = m_dll->recorder_get_cur_proginfo(recorder);

        if(m_dll->proginfo_compare(program, m_program) == 0)
          m_recording = true;
        m_dll->ref_release(program);
      }
      m_dll->ref_release(recorder);
    }
  }

  if (m_recording)
    CLog::Log(LOGDEBUG, "%s - Currently recording: %s", __FUNCTION__, m_filename.c_str());

  return true;
}

bool CMythFile::SetupLiveTV(const CURL& url)
{
  if (url.GetFileName().Left(9) != "channels/")
    return false;

  if(!SetupConnection(url, true, true, true))
    return false;

  CStdString channel = url.GetFileNameWithoutPath();
  if(!URIUtils::GetExtension(channel).Equals(".ts"))
  {
    CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
    return false;
  }
  URIUtils::RemoveExtension(channel);

  for(int i=0;i<16;i++)
  {
    m_recorder = m_dll->conn_get_recorder_from_num(m_control, i);
    if(!m_recorder)
      continue;

    if(m_dll->recorder_is_recording(m_recorder))
    {
      /* for now don't allow reuse of tuners, we would have to change tuner on channel *
       * and make sure we don't stop the tuner when stopping playback as that affects  *
       * other clients                                                                 */
#if 0
      /* if already recording, check if it is this channel */
      cmyth_proginfo_t program;
      program = m_dll->recorder_get_cur_proginfo(m_recorder);
      if(program)
      {
        if(channel == GetValue(m_dll->proginfo_chanstr(program)))
        {
          m_dll->ref_release(program);
          break;
        }
        m_dll->ref_release(program);
      }
#endif
    }
    else
    {
      /* not recording, check if it supports this channel */
      if(m_dll->recorder_check_channel(m_recorder, (char*)channel.c_str()) == 0)
        break;
    }
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }

  if(!m_recorder)
  {
    CLog::Log(LOGERROR, "%s - unable to get recorder", __FUNCTION__);
    return false;
  }

  m_recording = !!m_dll->recorder_is_recording(m_recorder);
  if(!m_recording)
    CLog::Log(LOGDEBUG, "%s - recorder isn't running, let's start it", __FUNCTION__);

  char* msg = NULL;
  if(!(m_recorder = m_dll->spawn_live_tv(m_recorder, 16*1024, 4096, prog_update_callback, &msg, (char*)channel.c_str())))
  {
    CLog::Log(LOGERROR, "%s - unable to spawn live tv: %s", __FUNCTION__, msg ? msg : "");
    return false;
  }

  m_program = m_dll->recorder_get_cur_proginfo(m_recorder);
  m_timestamp = XbmcThreads::SystemClockMillis();

  if(m_recording)
  {
    /* recorder was running when we started, seek to last position */
    if(!m_dll->livetv_seek(m_recorder, 0, SEEK_END))
      CLog::Log(LOGDEBUG, "%s - failed to seek to last position", __FUNCTION__);
  }

  m_filename = GetValue(m_dll->recorder_get_filename(m_recorder));
  return true;
}

bool CMythFile::SetupFile(const CURL& url)
{
  if (url.GetFileName().Left(6) != "files/")
    return false;

  if(!SetupConnection(url, true, false, false))
    return false;

  m_filename = url.GetFileName().Mid(6);

  m_file = m_dll->conn_connect_path((char*)m_filename.c_str(), m_control, 16*1024, 4096);
  if(!m_file)
  {
    CLog::Log(LOGERROR, "%s - unable to connect to file", __FUNCTION__);
    return false;
  }

  if(m_dll->file_length(m_file) == 0)
  {
    CLog::Log(LOGERROR, "%s - file is empty, probably doesn't even exist", __FUNCTION__);
    return false;
  }

  return true;
}

bool CMythFile::Open(const CURL& url)
{
  Close();

  CStdString path(url.GetFileName());

  if (path.Left(11) == "recordings/" ||
      path.Left(7)  == "movies/" ||
      path.Left(8)  == "tvshows/")
  {
    if(!SetupRecording(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - file: size %"PRIu64", start %"PRIu64", ", __FUNCTION__,  (int64_t)m_dll->file_length(m_file), (int64_t)m_dll->file_start(m_file));
  }
  else if (path.Left(9) == "channels/")
  {

    if(!SetupLiveTV(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - recorder has started on filename %s", __FUNCTION__, m_filename.c_str());
  }
  else if (path.Left(6) == "files/")
  {
    if(!SetupFile(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - file: size %"PRId64", start %"PRId64", ", __FUNCTION__,  (int64_t)m_dll->file_length(m_file), (int64_t)m_dll->file_start(m_file));
  }
  else
  {
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, path.c_str());
    return false;
  }

  /* check for any events */
  HandleEvents();

  return true;
}


void CMythFile::Close()
{
  if(!m_dll)
    return;

  if(m_program)
  {
    m_dll->ref_release(m_program);
    m_program = NULL;
  }
  if(m_recorder)
  {
    m_dll->recorder_stop_livetv(m_recorder);
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_file)
  {
    m_dll->ref_release(m_file);
    m_file = NULL;
  }
  if(m_session)
  {
    m_session->SetListener(NULL);
    CMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
}

CMythFile::CMythFile()
{
  m_dll         = NULL;
  m_program     = NULL;
  m_recorder    = NULL;
  m_control     = NULL;
  m_database    = NULL;
  m_file        = NULL;
  m_session     = NULL;
  m_timestamp   = 0;
  m_recording   = false;
}

CMythFile::~CMythFile()
{
  Close();
}

bool CMythFile::Exists(const CURL& url)
{
  CStdString path(url.GetFileName());

  /*
   * mythbackend provides access to the .mpg or .nuv recordings. The associated thumbnails
   * (*.mpg.png or *.nuv.png) and channel icons, which are an arbitrary image format, are requested
   * through the files/ path.
   */
  if ((path.Left(11) == "recordings/"
    || path.Left(7)  == "movies/"
    || path.Left(8)  == "tvshows/")
    && (URIUtils::GetExtension(path).Equals(".mpg")
    ||  URIUtils::GetExtension(path).Equals(".nuv")))
  {
    if(!SetupConnection(url, true, false, false))
      return false;

    m_filename = url.GetFileNameWithoutPath();
    m_program = m_dll->proginfo_get_from_basename(m_control, m_filename.c_str());
    if(!m_program)
    {
      CLog::Log(LOGERROR, "%s - unable to get find %s", __FUNCTION__, m_filename.c_str());
      return false;
    }
    return true;
  }
  else if(path.Left(6) == "files/")
    return true;

  return false;
}

bool CMythFile::Delete(const CURL& url)
{
  CStdString path(url.GetFileName());

  if (path.Left(11) == "recordings/" ||
      path.Left(7)  == "movies/" ||
      path.Left(8)  == "tvshows/")
  {
    /* this will setup all interal variables */
    if(!Exists(url))
      return false;
    if(!m_program)
      return false;

    if(m_dll->proginfo_delete_recording(m_control, m_program))
    {
      CLog::Log(LOGDEBUG, "%s - Error deleting recording: %s", __FUNCTION__, url.GetFileName().c_str());
      return false;
    }

    if (path.Left(8) == "tvshows/")
    {
      /*
       * Clear the directory cache for the TV Shows folder so the listing is accurate if this was
       * the last TV Show in the current directory that was deleted.
       */
      CURL tvshows(url);
      tvshows.SetFileName("tvshows/");
      g_directoryCache.ClearDirectory(tvshows.Get());
    }

    /*
     * Reset the recorded programs cache so the updated list is retrieved from mythbackend.
     */
    m_session->ResetAllRecordedPrograms();

    return true;
  }
  return false;
}

int64_t CMythFile::Seek(int64_t pos, int whence)
{
  CLog::Log(LOGDEBUG, "%s - seek to pos %"PRId64", whence %d", __FUNCTION__, pos, whence);

  if(m_recorder) // Live TV
    return -1; // Seeking not possible. Eventually will use m_dll->livetv_seek(m_recorder, pos, whence);

  if(m_file) // Recording
  {
    if (whence == 16) // SEEK_POSSIBLE = 0x10 = 16
      return 1;
    else
      return m_dll->file_seek(m_file, pos, whence);
  }
  return -1;
}

int64_t CMythFile::GetPosition()
{
  if(m_recorder)
    return m_dll->livetv_seek(m_recorder, 0, SEEK_CUR);
  else
    return m_dll->file_seek(m_file, 0, SEEK_CUR);
  return -1;
}

int64_t CMythFile::GetLength()
{
  if(m_file)
    return m_dll->file_length(m_file);
  return -1;
}

unsigned int CMythFile::Read(void* buffer, int64_t size)
{
  /* check for any events */
  HandleEvents();

  /* file might have gotten closed */
  if(!m_recorder && !m_file)
    return 0;

  int ret;
  if(m_recorder)
    ret = m_dll->livetv_read(m_recorder, (char*)buffer, (unsigned long)size);
  else
    ret = m_dll->file_read(m_file, (char*)buffer, (unsigned long)size);

  if(ret < 0)
  {
    CLog::Log(LOGERROR, "%s - cmyth read returned error %d", __FUNCTION__, ret);
    return 0;
  }
  return ret;
}

bool CMythFile::SkipNext()
{
  HandleEvents();
  if(m_recorder)
    return m_dll->recorder_is_recording(m_recorder) > 0;

  return false;
}

bool CMythFile::UpdateItem(CFileItem& item)
{
  /*
   * UpdateItem should only return true if a LiveTV item has changed via a channel change, or the
   * program being aired on the current channel has changed requiring the UI to update the currently
   * playing information.
   *
   * Check by comparing the current title with the new title.
   */
  if (!m_recorder)
    return false;

  if (!m_program || !m_session)
    return false;

  CStdString title = item.m_strTitle;
  m_session->SetFileItemMetaData(item, m_program);
  return title != item.m_strTitle;
}

int CMythFile::GetTotalTime()
{
  if(m_recorder && (XbmcThreads::SystemClockMillis() - m_timestamp) > 5000 )
  {
    m_timestamp = XbmcThreads::SystemClockMillis();
    if(m_program)
      m_dll->ref_release(m_program);
    m_program = m_dll->recorder_get_cur_proginfo(m_recorder);
  }

  if(m_program && m_recorder)
    return m_dll->proginfo_length_sec(m_program) * 1000;

  return -1;
}

int CMythFile::GetStartTime()
{
  if(m_program && m_recorder)
  {
    cmyth_timestamp_t start = m_dll->proginfo_start(m_program);
      
    CDateTimeSpan time;
    time  = CDateTime::GetCurrentDateTime()
          - CDateTime(m_dll->timestamp_to_unixtime(start));

    m_dll->ref_release(start);
    return time.GetDays()    * 1000 * 60 * 60 * 24
         + time.GetHours()   * 1000 * 60 * 60
         + time.GetMinutes() * 1000 * 60
         + time.GetSeconds() * 1000;
  }
  return 0;
}

bool CMythFile::ChangeChannel(int direction, const CStdString &channel)
{
  CLog::Log(LOGDEBUG, "%s - channel change started", __FUNCTION__);

  if(direction == CHANNEL_DIRECTION_SAME)
  {
    if(!m_program || channel != GetValue(m_dll->proginfo_chanstr(m_program)))
    {
      if(m_dll->recorder_pause(m_recorder) < 0)
      {
        CLog::Log(LOGDEBUG, "%s - failed to pause recorder", __FUNCTION__);
        return false;
      }

      CLog::Log(LOGDEBUG, "%s - chainging channel to %s", __FUNCTION__, channel.c_str());
      if(m_dll->recorder_set_channel(m_recorder, (char*)channel.c_str()) < 0)
      {
        CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
        return false;
      }
    }
  }
  else
  {
    if(m_dll->recorder_pause(m_recorder) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to pause recorder", __FUNCTION__);
      return false;
    }

    CLog::Log(LOGDEBUG, "%s - chainging channel direction %d", __FUNCTION__, direction);
    if(m_dll->recorder_change_channel(m_recorder, (cmyth_channeldir_t)direction) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
      return false;
    }
  }

  if(!m_dll->livetv_chain_switch_last(m_recorder))
    CLog::Log(LOGDEBUG, "%s - failed to change to last item in chain", __FUNCTION__);

  if(m_program)
    m_dll->ref_release(m_program);
  m_program = m_dll->recorder_get_cur_proginfo(m_recorder);

  CLog::Log(LOGDEBUG, "%s - channel change done", __FUNCTION__);
  return true;
}

bool CMythFile::NextChannel(bool preview)
{
  return ChangeChannel(CHANNEL_DIRECTION_UP, "");
}

bool CMythFile::PrevChannel(bool preview)
{
  return ChangeChannel(CHANNEL_DIRECTION_DOWN, "");
}

bool CMythFile::SelectChannel(unsigned int channel)
{
  return ChangeChannel(CHANNEL_DIRECTION_SAME,""+channel);
}

bool CMythFile::CanRecord()
{
  if(m_recorder || m_recording)
    return true;

  return false;
}

bool CMythFile::IsRecording()
{
  return m_recording;
}

bool CMythFile::Record(bool bOnOff)
{
  if(m_recorder)
  {
    if(!m_database)
      return false;

    int ret;
    if(bOnOff)
      ret = m_dll->livetv_keep_recording(m_recorder, m_database, 1);
    else
      ret = m_dll->livetv_keep_recording(m_recorder, m_database, 0);

    if(ret < 0)
    {
      CLog::Log(LOGERROR, "%s - failed to turn on recording", __FUNCTION__);
      return false;
    }

    m_recording = bOnOff;
    return true;
  }
  else
  {
    if(m_recording)
    {
      if(m_dll->proginfo_stop_recording(m_control, m_program) < 0)
        return false;

      m_recording = false;
      return true;
    }
  }
  return false;
}

bool CMythFile::GetCommBreakList(cmyth_commbreaklist_t& commbreaklist)
{
  if (m_program)
  {
    commbreaklist = m_dll->get_commbreaklist(m_control, m_program);
    return true;
  }
  return false;
}

bool CMythFile::GetCutList(cmyth_commbreaklist_t& commbreaklist)
{
  if (m_program)
  {
    commbreaklist = m_dll->get_cutlist(m_control, m_program);
    return true;
  }
  return false;
}

int CMythFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
  {
    if(m_recorder)
      return 0;
    else
      return 1;
  }
  return -1;
}
