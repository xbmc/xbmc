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
#include "CMythFile.h"
#include "Util.h"
#include "DllLibCMyth.h"
#include "URL.h"

extern "C" {
#include "lib/libcmyth/cmyth.h"
#include "lib/libcmyth/mvp_refmem.h"
}

using namespace XFILE;
using namespace std;

static void prog_update_callback(cmyth_proginfo_t prog)
{
  CLog::Log(LOGDEBUG, "%s - prog_update_callback", __FUNCTION__);
}

void CCMythFile::OnEvent(int event, const string& data)
{
  CSingleLock lock(m_section);
  m_events.push(make_pair(event, data));
}

bool CCMythFile::HandleEvents()
{
  CSingleLock lock(m_section);

  if(m_events.empty())
    return false;

  while(!m_events.empty())
  {
    int next        = m_events.front().first;
    CStdString data = m_events.front().second;
    m_events.pop();

    lock.Leave();

    switch (next) {
    case CMYTH_EVENT_CLOSE:
      Close();
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      {
        string chainid = data.substr(strlen("LIVETV_CHAIN UPDATE "));
        if(m_recorder)
          m_dll->livetv_chain_update(m_recorder, (char*)chainid.c_str(), 4096);
      }
      break;
    }

    lock.Enter();
  }
  return true;
}

bool CCMythFile::SetupConnection(const CURL& url, bool control, bool event, bool database)
{
  if(!m_session)
    m_session =  CCMythSession::AquireSession(url);

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

bool CCMythFile::SetupRecording(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(11) != "recordings/")
    return false;

  if(!SetupConnection(url, true, false, false))
    return false;

  m_filename = path.Mid(11);

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

  /* check if this program is currently recording       *
   * sadly proginfo_get_from_basename doesn't give us   *
   * that with the new interface, maybe the myth people *
   * will fix this eventually                           */
  m_recording = false;
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
  return true;
}

bool CCMythFile::SetupLiveTV(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(9) != "channels/")
    return false;

  if(!SetupConnection(url, true, true, true))
    return false;

  CStdString channel = path.Mid(9);
  if(!CUtil::GetExtension(channel).Equals(".ts"))
  {
    CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
    return false;
  }
  CUtil::RemoveExtension(channel);

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
  m_timestamp = GetTickCount();
  if(m_program)
    m_starttime = m_dll->proginfo_rec_start(m_program);

  if(m_recording)
  {
    /* recorder was running when we started, seek to last position */
    if(!m_dll->livetv_seek(m_recorder, 0, SEEK_END))
      CLog::Log(LOGDEBUG, "%s - failed to seek to last position", __FUNCTION__);
  }

  m_filename = GetValue(m_dll->recorder_get_filename(m_recorder));
  return true;
}

bool CCMythFile::SetupFile(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(6) != "files/")
    return false;

  if(!SetupConnection(url, true, false, false))
    return false;

  m_filename = path.Mid(5);

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

bool CCMythFile::Open(const CURL& url)
{
  Close();

  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    if(!SetupRecording(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - file: size %"PRId64", start %"PRId64", ", __FUNCTION__,  m_dll->file_length(m_file), m_dll->file_start(m_file));
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

    CLog::Log(LOGDEBUG, "%s - file: size %"PRId64", start %"PRId64", ", __FUNCTION__,  m_dll->file_length(m_file), m_dll->file_start(m_file));
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


void CCMythFile::Close()
{
  if(!m_dll)
    return;

  if(m_starttime)
  {
    m_dll->ref_release(m_starttime);
    m_starttime = NULL;
  }
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
    CCMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
}

CCMythFile::CCMythFile()
{
  m_dll         = NULL;
  m_starttime   = NULL;
  m_program     = NULL;
  m_recorder    = NULL;
  m_control     = NULL;
  m_database    = NULL;
  m_file        = NULL;
  m_session     = NULL;
  m_timestamp   = 0;
  m_recording   = false;
}

CCMythFile::~CCMythFile()
{
  Close();
}

bool CCMythFile::Exists(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    if(CUtil::GetExtension(path).Equals(".tbn")
    || CUtil::GetExtension(path).Equals(".jpg"))
      return false;

    if(!SetupConnection(url, true, false, false))
      return false;

    m_filename = path.Mid(11);
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

bool CCMythFile::Delete(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    /* this will setup all interal variables */
    if(!Exists(url))
      return false;
    if(!m_program)
      return false;

    if(m_dll->proginfo_delete_recording(m_control, m_program))
      return false;
    return true;
  }
  return false;
}

__int64 CCMythFile::Seek(__int64 pos, int whence)
{
  CLog::Log(LOGDEBUG, "%s - seek to pos %"PRId64", whence %d", __FUNCTION__, pos, whence);

  if(whence == SEEK_POSSIBLE)
  {
    if(m_recorder)
      return 0;
    else
      return 1;
  }

  __int64 result;
  if(m_recorder)
    result = -1; //m_dll->livetv_seek(m_recorder, pos, whence);
  else if(m_file)
    result = m_dll->file_seek(m_file, pos, whence);
  else
    result = -1;

  return result;
}

__int64 CCMythFile::GetPosition()
{
  if(m_recorder)
    return m_dll->livetv_seek(m_recorder, 0, SEEK_CUR);
  else
    return m_dll->file_seek(m_file, 0, SEEK_CUR);
  return -1;
}

__int64 CCMythFile::GetLength()
{
  if(m_file)
    return m_dll->file_length(m_file);
  return -1;
}

unsigned int CCMythFile::Read(void* buffer, __int64 size)
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

bool CCMythFile::SkipNext()
{
  HandleEvents();
  if(m_recorder)
    return m_dll->recorder_is_recording(m_recorder) > 0;

  return false;
}

bool CCMythFile::UpdateItem(CFileItem& item)
{
  if(!m_program || !m_session)
    return false;

  return m_session->UpdateItem(item, m_program);
}

int CCMythFile::GetTotalTime()
{
  if(m_recorder && m_timestamp + 5000 < GetTickCount())
  {
    m_timestamp = GetTickCount();
    if(m_program)
      m_dll->ref_release(m_program);
    m_program = m_dll->recorder_get_cur_proginfo(m_recorder);
  }

  if(m_program && m_recorder)
    return m_dll->proginfo_length_sec(m_program) * 1000;

  return -1;
}

int CCMythFile::GetStartTime()
{
  if(m_program && m_recorder && m_starttime)
  {
    cmyth_timestamp_t start = m_dll->proginfo_start(m_program);

    double diff = difftime(m_dll->timestamp_to_unixtime(start), m_dll->timestamp_to_unixtime(m_starttime));

    m_dll->ref_release(start);

    return (int)(diff * 1000);
  }
  return 0;
}

bool CCMythFile::ChangeChannel(int direction, const CStdString &channel)
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

bool CCMythFile::NextChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_UP, "");
}

bool CCMythFile::PrevChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_DOWN, "");
}


bool CCMythFile::CanRecord()
{
  if(m_recorder || m_recording)
    return true;

  return false;
}

bool CCMythFile::IsRecording()
{
  return m_recording;
}

bool CCMythFile::Record(bool bOnOff)
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


