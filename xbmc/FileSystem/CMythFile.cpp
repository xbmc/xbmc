#include "stdafx.h"
#include "CMythFile.h"
#include "Util.h"
#include "DllLibCMyth.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

using namespace XFILE;
using namespace std;

static void prog_update_callback(cmyth_proginfo_t prog)
{
  CLog::Log(LOGDEBUG, "%s - prog_update_callback", __FUNCTION__);
}

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif

void CCMythFile::OnEvent(int event, const string& data)
{
  CSingleLock lock(m_section);
  m_events.push(std::make_pair<int, string>(event, data));
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
  return true;
}

bool CCMythFile::SetupRecording(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(11) != "recordings/")
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
  return true;
}

bool CCMythFile::SetupLiveTV(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(9) != "channels/")
    return false;

  CStdString channel = path.Mid(9);
  if(!CUtil::GetExtension(channel).Equals(".ts"))
  {
    CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
    return false;
  }
  CUtil::RemoveExtension(channel);

  m_recorder = m_dll->conn_get_free_recorder(m_control);
  if(!m_recorder)
  {
    CLog::Log(LOGERROR, "%s - unable to get recorder", __FUNCTION__);
    return false;
  }

  if(!m_dll->recorder_is_recording(m_recorder))
  {
    CLog::Log(LOGDEBUG, "%s - recorder isn't running, let's start it", __FUNCTION__);
    char* msg = NULL;
    if(!(m_recorder = m_dll->spawn_live_tv(m_recorder, 16*1024, 4096, prog_update_callback, &msg)))
    {
      CLog::Log(LOGERROR, "%s - unable to spawn live tv: %s", __FUNCTION__, msg ? msg : "");
      return false;
    }
  }

  if(!ChangeChannel(CHANNEL_DIRECTION_SAME, channel.c_str()))
    return false;

  if(!m_dll->recorder_is_recording(m_recorder))
  {
    CLog::Log(LOGERROR, "%s - recorder hasn't started", __FUNCTION__);
    return false;
  }
  char * filename = m_dll->recorder_get_filename(m_recorder);
  m_filename = filename;
  m_dll->ref_release(filename);
  return true;
}

bool CCMythFile::Open(const CURL& url, bool binary)
{
  if(!binary)
    return false;

  Close();

  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    if(!SetupConnection(url, true, false, false))
      return false;

    if(!SetupRecording(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - file: size %"PRId64", start %"PRId64", ", __FUNCTION__,  m_dll->file_length(m_file), m_dll->file_start(m_file));
  } 
  else if (path.Left(9) == "channels/")
  {
    if(!SetupConnection(url, true, true, false))
      return false;

    if(!SetupLiveTV(url))
      return false;

    CLog::Log(LOGDEBUG, "%s - recorder has started on filename %s", __FUNCTION__, m_filename.c_str());

    m_program = m_dll->recorder_get_cur_proginfo(m_recorder);
    if(!m_program)
      CLog::Log(LOGWARNING, "%s - failed to get current program info", __FUNCTION__);
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
  m_program     = NULL;
  m_recorder    = NULL;
  m_control     = NULL;
  m_file        = NULL;
  m_session     = NULL;
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
    if(!SetupConnection(url, true, false, false))
      return false;

    m_filename = path.Mid(11);
    m_program = m_dll->proginfo_get_from_basename(m_control, m_filename.c_str());
    if(!m_program)
    {
      CLog::Log(LOGERROR, "%s - unable to get find selected file", __FUNCTION__);
      return false;
    }
    return true;
  }
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
  struct timeval to;
  to.tv_sec = 10;
  to.tv_usec = 0;
  int ret;
  unsigned long remain;

  /* check for any events */
  HandleEvents();

  /* file might have gotten closed */
  if(!m_recorder && !m_file)
    return 0;

  while(true)
  {
    if(m_recorder)
      ret = m_dll->livetv_request_block(m_recorder, (unsigned long)size);
    else
      ret = m_dll->file_request_block(m_file, (unsigned long)size);

    if(ret <= 0)
    {
      CLog::Log(LOGERROR, "%s - error requesting block of data (%d)", __FUNCTION__, ret);
      if(HandleEvents())
        continue;
      return 0;
    }
    break;
  }

  remain = (unsigned long)ret;
  size = 0;
  do
  {
    if(m_recorder)
      ret = m_dll->livetv_select(m_recorder, &to);
    else
      ret = m_dll->file_select(m_file, &to);

    if(ret <= 0)
    {
      CLog::Log(LOGERROR, "%s - timeout waiting for data (%d)", __FUNCTION__, ret);
      return 0;
    }

    if(m_recorder)
      ret = m_dll->livetv_get_block(m_recorder, (char*)buffer+size, remain);
    else
      ret = m_dll->file_get_block(m_file,       (char*)buffer+size, remain);

    if(ret <= 0)
    {
      CLog::Log(LOGERROR, "%s - failed to retrieve block (%d)", __FUNCTION__, ret);
      return 0;
    }

    if(ret > (int)remain)
    {
      CLog::Log(LOGERROR, "%s - potential buffer overrun", __FUNCTION__);
      return (unsigned long)size;
    }
    remain -= ret;
    size   += ret;
  } while(remain > 0);

  return (unsigned long)size;
}

bool CCMythFile::SkipNext()
{
  HandleEvents();
  if(m_recorder)
    return m_dll->recorder_is_recording(m_recorder) > 0;

  return false;
}

CVideoInfoTag* CCMythFile::GetVideoInfoTag()
{
  if(m_program)
  {
    char *str;

    if((str = m_dll->proginfo_chanstr(m_program)))
    {
      m_infotag.m_strTitle = str;
      m_dll->ref_release(str);
    }

    if((str = m_dll->proginfo_title(m_program)))
    {
      m_infotag.m_strTitle    += " : ";
      m_infotag.m_strTitle    += str;
      m_infotag.m_strShowTitle = str;
      m_dll->ref_release(str);
    }

    if((str = m_dll->proginfo_description(m_program)))
    {
      m_infotag.m_strPlotOutline = str;
      m_infotag.m_strPlot        = str;
      m_dll->ref_release(str);
    }

    m_infotag.m_iSeason  = 1; /* set this so xbmc knows it's a tv show */
    m_infotag.m_iEpisode = 1;
    return &m_infotag;
  }
  return NULL;
}

int CCMythFile::GetTotalTime()
{
  if(m_program && m_recorder)
    return m_dll->proginfo_length_sec(m_program) * 1000;

  return -1;
}

int CCMythFile::GetStartTime()
{
  if(m_program && m_recorder)
  {
    cmyth_timestamp_t start = m_dll->proginfo_start(m_program);
    cmyth_timestamp_t end = m_dll->proginfo_rec_start(m_program);

    double diff = difftime(m_dll->timestamp_to_unixtime(end), m_dll->timestamp_to_unixtime(start));

    m_dll->ref_release(start);
    m_dll->ref_release(end);
    return (int)(diff * 1000);
  }
  return 0;
}

bool CCMythFile::ChangeChannel(int direction, const char* channel)
{
  CLog::Log(LOGDEBUG, "%s - channel change started", __FUNCTION__);

  if(m_dll->recorder_pause(m_recorder) < 0)
  {
    CLog::Log(LOGDEBUG, "%s - failed to pause recorder", __FUNCTION__);
    return false;
  }

  if(channel)
  {
    CLog::Log(LOGDEBUG, "%s - chainging channel to %s", __FUNCTION__, channel);
    if(m_dll->recorder_set_channel(m_recorder, (char*)channel) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - chainging channel direction %d", __FUNCTION__, direction);
    if(m_dll->recorder_change_channel(m_recorder, (cmyth_channeldir_t)direction) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
      return false;
    }
  }

  if(m_program)
    m_dll->ref_release(m_program);
  m_program = m_dll->recorder_get_cur_proginfo(m_recorder);

  if(!m_dll->livetv_chain_switch_last(m_recorder))
    CLog::Log(LOGDEBUG, "%s - failed to change to last item in chain", __FUNCTION__);

  CLog::Log(LOGDEBUG, "%s - channel change done", __FUNCTION__);
  return true;
}

bool CCMythFile::NextChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_UP, NULL);
}

bool CCMythFile::PrevChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_DOWN, NULL);
}
