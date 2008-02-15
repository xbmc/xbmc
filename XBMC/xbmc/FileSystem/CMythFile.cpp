#include "stdafx.h"
#include "CMythFile.h"
#include "Util.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

#define XLOG(level, message, args...) CLog::Log((level), "CCMyth::%s - " message, __FUNCTION__, ##args)
#define XREL(a) if(a) { ref_release(a); a = NULL; }

using namespace XFILE;

static void prog_update_callback(cmyth_proginfo_t prog)
{
  XLOG(LOGDEBUG, "prog_update_callback");
}


bool CCMythFile::HandleEvents()
{
  char buf[128];
  cmyth_event_t next;

  struct timeval to;
  to.tv_sec = 0;
  to.tv_usec = 0;

  while(true)
  {
    /* check if there are any new events */
    if(cmyth_event_select(m_event, &to) <= 0)
      return false;

    next = cmyth_event_get(m_event, buf, 128);
    switch (next) {
    case CMYTH_EVENT_UNKNOWN:
      XLOG(LOGDEBUG, "MythTV unknown event (error?)");
      break;
    case CMYTH_EVENT_CLOSE:
      XLOG(LOGDEBUG, "Event socket closed, shutting down myth");
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE:
      XLOG(LOGDEBUG, "MythTV event RECORDING_LIST_CHANGE");
      if (m_programlist)
        ref_release(m_programlist);
//      m_programlist = cmyth_proglist_get_all_recorded(m_control);
      break;
    case CMYTH_EVENT_SCHEDULE_CHANGE:
      XLOG(LOGDEBUG, "MythTV event SCHEDULE_CHANGE");
      break;
    case CMYTH_EVENT_DONE_RECORDING:
      XLOG(LOGDEBUG, "MythTV event DONE_RECORDING");
      break;
    case CMYTH_EVENT_QUIT_LIVETV:
      XLOG(LOGDEBUG, "MythTV event QUIT_LIVETV");
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
    {
      XLOG(LOGDEBUG, "MythTV event %s",buf);
      char * pfx = "LIVETV_CHAIN UPDATE ";
      char * chainid = buf + strlen(pfx);
      cmyth_livetv_chain_update(m_recorder, chainid, 4096);
      break;
    }
    case CMYTH_EVENT_SIGNAL:
      XLOG(LOGDEBUG, "MythTV event SIGNAL");
      break;
    }
  }
}

bool CCMythFile::Open(const CURL& url, bool binary)
{
  if(!binary)
    return false;

  Close();

  int port = url.GetPort();
  if(port == 0)
    port = 6543;

  m_control = cmyth_conn_connect_ctrl((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!m_control)
  {
    XLOG(LOGERROR, "unable to connect to server %s, port %d", url.GetHostName().c_str(), port);
    return false;
  }

  m_event = cmyth_conn_connect_event((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!m_event)
  {
    XLOG(LOGERROR, "unable to connect to server %s, port %d", url.GetHostName().c_str(), port);
    return false;
  }

  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    CStdString file = path.Mid(11);

    m_programlist = cmyth_proglist_get_all_recorded(m_control);
    if(!m_programlist)
    {
      XLOG(LOGERROR, "unable to get list of recordings");
      return false;
    }
    int count = cmyth_proglist_get_count(m_programlist);
    for(int i=0; i<count; i++)
    {
      cmyth_proginfo_t program = cmyth_proglist_get_item(m_programlist, i);
      if(program)
      {
        char* path = cmyth_proginfo_pathname(program);
        if(path)
        {
          CStdString recording = CUtil::GetFileName(path);
          if(file == recording)
            m_program = (cmyth_proginfo_t)ref_hold(program);
          ref_release(path);
        }
        ref_release(program);
      }
      if(m_program)
        break;
    }

    if(!m_program)
    {
      XLOG(LOGERROR, "unable to get find selected file");
      return false;
    }

    m_file = cmyth_conn_connect_file(m_program, m_control, 16*1024, 4096);
    if(!m_file)
    {
      XLOG(LOGERROR, "unable to connect to file");
      return false;
    }
    XLOG(LOGDEBUG, "file: size %lld, start %lld, ",  cmyth_file_length(m_file), cmyth_file_start(m_file));
  } 
  else if (path.Left(9) == "channels/")
  {
    CStdString channel = path.Mid(9);
    if(!CUtil::GetExtension(channel).Equals(".ts"))
    {
      XLOG(LOGERROR, "invalid channel url %s", channel.c_str());
      return false;
    }
    CUtil::RemoveExtension(channel);

    m_recorder = cmyth_conn_get_free_recorder(m_control);
    if(!m_recorder)
    {
      XLOG(LOGERROR, "unable to get recorder");
      return false;
    }

    if(!cmyth_recorder_is_recording(m_recorder))
    {
      XLOG(LOGDEBUG, "recorder isn't running, let's start it");
      char* msg = NULL;
      if(!(m_recorder = cmyth_spawn_live_tv(m_recorder, 16*1024, 4096, prog_update_callback, &msg)))
      {
        XLOG(LOGERROR, "unable to spawn live tv: %s", msg ? msg : "");
        return false;
      }
    }

    if(!ChangeChannel(CHANNEL_DIRECTION_SAME, channel.c_str()))
      return false;

    if(!cmyth_recorder_is_recording(m_recorder))
    {
      XLOG(LOGERROR, "recorder hasn't started");
      return false;
    }
    m_filename = cmyth_recorder_get_filename(m_recorder);

    XLOG(LOGDEBUG, "recorder has started on filename %s", m_filename.c_str());

    m_program = cmyth_recorder_get_cur_proginfo(m_recorder);
    if(!m_program)
      XLOG(LOGWARNING, "failed to get current program info");
  }
  else
  {
    XLOG(LOGERROR, "invalid path specified %s", path.c_str());
    return false;
  }

  /* check for any events */
  HandleEvents();

  return true;
}


void CCMythFile::Close()
{
  if(m_program)
  {
    ref_release(m_program);
    m_program = NULL;
  }
  if(m_programlist)
  {
    ref_release(m_programlist);
    m_programlist = NULL;
  }
  if(m_recorder)
  {
    cmyth_recorder_stop_livetv(m_recorder);
    ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_file)
  {
    ref_release(m_file);
    m_file = NULL;
  }
  if(m_control)
  {
    ref_release(m_control);
    m_control = NULL;
  }
}

CCMythFile::CCMythFile()
{
  m_program     = NULL;
  m_programlist = NULL;
  m_recorder    = NULL;
  m_control     = NULL;
  m_file        = NULL;
  m_remain      = 0;
  cmyth_dbg_level(CMYTH_DBG_DETAIL); 
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

  }
  return false;
}

bool CCMythFile::Delete(const CURL& url) 
{ 
  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
  }
  return false;
}

__int64 CCMythFile::Seek(__int64 pos, int whence)
{
  XLOG(LOGDEBUG, "seek to pos %lld, whence %d", pos, whence);

  if(whence == SEEK_POSSIBLE)
    return true;

  if(m_recorder)
    return cmyth_livetv_seek(m_recorder, pos, whence);
  else
    return cmyth_file_seek(m_file, pos, whence);

  return -1;
}

__int64 CCMythFile::GetPosition()
{
  if(m_recorder)
    return cmyth_livetv_seek(m_recorder, 0, SEEK_CUR);
  else
    return cmyth_file_seek(m_file, 0, SEEK_CUR);
  return -1;
}

__int64 CCMythFile::GetLength()
{
  if(m_file)
    return cmyth_file_length(m_file);
  return -1;
}

unsigned int CCMythFile::Read(void* buffer, __int64 size)
{ 
  /* check for any events */
  HandleEvents();

  struct timeval to;
  to.tv_sec = 0;
  to.tv_usec = 10;
  int ret;

  if(m_remain == 0)
  {
    if(m_recorder)
      ret = cmyth_livetv_request_block(m_recorder, size);
    else
      ret = cmyth_file_request_block(m_file, size);
    if(ret <= 0)
    {
      XLOG(LOGERROR, "error requesting block of data (%d)", ret);
      return 0;
    }
    m_remain = ret;
  }

  if(m_recorder)
    ret = cmyth_livetv_select(m_recorder, &to);
  else
    ret = cmyth_file_select(m_file, &to);
  if(ret <= 0)
  {
    XLOG(LOGERROR, "timeout waiting for data (%d)", ret);
    return 0;
  }

  size = min(m_remain, (int)size);
  if(m_recorder)
    ret = cmyth_livetv_get_block(m_recorder, (char*)buffer, size);
  else
    ret = cmyth_file_get_block(m_file, (char*)buffer, size);
  if(ret <= 0)
  {
    XLOG(LOGERROR, "failed to retrieve block (%d)", ret);
    return 0;
  }
  if(ret > m_remain)
  {
    XLOG(LOGERROR, "received more data than we requested, probably a buffer overrun");
    m_remain = 0;
    return 0;
  }
  m_remain -= ret;
  return ret;
}

bool CCMythFile::SkipNext()
{
  if(m_recorder)
    return cmyth_recorder_is_recording(m_recorder) > 0;

  return false;
}

CVideoInfoTag* CCMythFile::GetVideoInfoTag()
{
  if(m_program)
  {
    char *str;

    if((str = cmyth_proginfo_chanstr(m_program)))
    {
      m_infotag.m_strTitle = str;
      ref_release(str);
    }

    if((str = cmyth_proginfo_title(m_program)))
    {
      m_infotag.m_strTitle    += " : ";
      m_infotag.m_strTitle     = str;
      m_infotag.m_strShowTitle = str;
      ref_release(str);
    }

    if((str = cmyth_proginfo_description(m_program)))
    {
      m_infotag.m_strPlotOutline = str;
      m_infotag.m_strPlot        = str;
      ref_release(str);
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
    return cmyth_proginfo_length_sec(m_program) * 1000;

  return -1;
}

int CCMythFile::GetStartTime()
{
  if(m_program && m_recorder)
  {
    cmyth_timestamp_t start = cmyth_proginfo_start(m_program);
    cmyth_timestamp_t end = cmyth_proginfo_rec_start(m_program);

    double diff = difftime(cmyth_timestamp_to_unixtime(end), cmyth_timestamp_to_unixtime(start));

    ref_release(start);
    ref_release(end);
    return (int)(diff * 1000);
  }
  return 0;
}

bool CCMythFile::ChangeChannel(int direction, const char* channel)
{
  if(cmyth_recorder_pause(m_recorder) < 0)
  {
    XLOG(LOGDEBUG, "failed to pause recorder");
    return false;
  }

  if(channel)
  {
    if(cmyth_recorder_set_channel(m_recorder, (char*)channel) < 0)
    {
      XLOG(LOGDEBUG, "failed to change channel");
      return false;
    }
  }
  else
  {
    if(cmyth_recorder_change_channel(m_recorder, (cmyth_channeldir_t)direction) < 0)
    {
      XLOG(LOGDEBUG, "failed to change channel");
      return false;
    }
  }

  if(m_program)
    ref_release(m_program);
  m_program = cmyth_recorder_get_cur_proginfo(m_recorder);

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
