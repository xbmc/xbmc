#ifdef HAS_GMYTH
#include "stdafx.h"
#include "GMythFile.h"
#include "Util.h"

#include <glib.h>

#include "gmyth/gmyth_backendinfo.h"
#include "gmyth/gmyth_scheduler.h"
#include "gmyth/gmyth_util.h"
#include "gmyth/gmyth_epg.h"
#include "gmyth/gmyth_file_transfer.h"
#include "gmyth/gmyth_livetv.h"
#include "gmyth/gmyth_common.h"

#define XLOG(level, message, args...) CLog::Log(level, "CGMyth::%s - " message, __FUNCTION__, ##args)

using namespace XFILE;

bool CGMythFile::Open(const CURL& url, bool binary)
{
  if(!binary)
    return false;

  CStdString path(url.GetFileName());
  m_info = gmyth_backend_info_new();
  if(!m_info)
    return false;

  gmyth_backend_info_set_hostname(m_info, url.GetHostName().c_str());
  gmyth_backend_info_set_port(m_info, url.GetPort());


  if (!url.GetUserName().IsEmpty())
      gmyth_backend_info_set_username(m_info, url.GetUserName().c_str());
  else
      gmyth_backend_info_set_username(m_info, "mythtv");

  if (!url.GetPassWord().IsEmpty())
      gmyth_backend_info_set_password(m_info, url.GetPassWord().c_str());
  else
      gmyth_backend_info_set_password(m_info, "mythtv");

  gmyth_backend_info_set_db_name(m_info, "mythconverg");


  if(path.Left(11) == "recordings/")
  {
    CStdString file = path.Mid(11);
    m_filename = g_strdup(file.c_str());
  } 
  else if (path.Left(9) == "channels/")
  {
    CStdString channel = path.Mid(9);
    if(!CUtil::GetExtension(channel).Equals(".ts"))
    {
      XLOG(LOGERROR, "invalid channel url %s", channel.c_str());
      Close();
      return false;
    }
    CUtil::RemoveExtension(channel);

    m_livetv = gmyth_livetv_new(m_info);
    if (!m_livetv)
    {
      Close();
      return true;
    }
    m_channel = g_strdup(channel.c_str());
    if (!gmyth_livetv_channel_name_setup(m_livetv, m_channel))
    {
      CLog::Log(LOGERROR, "%s - failed to tune live channel", __FUNCTION__);
      Close();
      return false;
    }
  }
  else
  {
    XLOG(LOGERROR, "invalid path specified %s", path.c_str());
    return false;
  }

  // setup actual transfer
  if (!SetupTransfer())
  {
    Close();
    return false;
  }

  // if we are live, print program info
  if(m_livetv && m_livetv->proginfo)
    XLOG(LOGINFO, " ** PROGRAM INFO **\n%s\n ** ************ **", gmyth_program_info_to_string(m_livetv->proginfo));

  return true;
}

bool CGMythFile::SetupTransfer()
{
  if(m_file)
  {
    gmyth_file_transfer_close(m_file);
    g_object_unref(m_file);
    m_file = NULL;
  }

  if(m_livetv)
  {
    /* live tv, reget filename as it might have changed */
    if(m_filename)
    {
      g_free(m_filename);
      m_filename = NULL;
    }

    if (m_livetv->uri)
      m_filename = g_strdup(gmyth_uri_get_path(m_livetv->uri));
    else
      m_filename = g_strdup(m_livetv->proginfo->pathname->str);

    m_file = GMYTH_FILE_TRANSFER(gmyth_livetv_create_file_transfer(m_livetv));
  }
  else
  {
    m_file = gmyth_file_transfer_new(m_info);
  }

  if (!m_file) 
  {
    XLOG(LOGERROR, "failed to create transfer");
    return false;
  }
  CLog::Log(LOGDEBUG, "%s - opening filename ""%s""",__FUNCTION__, m_filename);
  if (!gmyth_file_transfer_open(m_file, m_filename)) 
  {
    XLOG(LOGERROR, "failed to open transfer");
    return false;
  }
  m_held = false;
  return true;
}

void CGMythFile::Close()
{
  if(m_file)
  {
    gmyth_file_transfer_close(m_file);
    g_object_unref(m_file);
    m_file = NULL;
  }
  if(m_livetv)
  {
    gmyth_livetv_stop_playing(m_livetv);
    g_object_unref(m_livetv);
    m_livetv = NULL;
  }

  if(m_filename)
  {
    g_free(m_filename);
    m_filename = NULL;
  }
  if(m_channel)
  {
    g_free(m_channel);
    m_channel = NULL;
  }
  if(m_info)
  {
    g_object_unref(m_info);
    m_info = NULL;
  }
}

CGMythFile::CGMythFile()
{
  m_array = (GByteArray*)g_array_new(false, true, sizeof(gchar));
  m_info = NULL;
  m_file = NULL;
  m_livetv = NULL;
  m_used = 0;
  m_filename = NULL;
  m_channel = NULL;
  m_held = false;
}

CGMythFile::~CGMythFile()
{
  Close();
  if(m_array)
  {
    g_array_free((GArray*)m_array, true);
    m_array = NULL;
  }
}

bool CGMythFile::Exists(const CURL& url)
{
  return false;
}

__int64 CGMythFile::Seek(__int64 pos, int whence)
{
  if(whence == SEEK_POSSIBLE)
  {
    if(m_livetv)
      return false;
    else
      return true;
  }
  XLOG(LOGDEBUG, "seek to pos %lld, whence %d", pos, whence);
  if(m_file)
    return gmyth_file_transfer_seek(m_file, pos, whence);
  else
    return -1;
}

__int64 CGMythFile::GetPosition()
{
  return -1;
}

__int64 CGMythFile::GetLength()
{
  if(m_file)
    return gmyth_file_transfer_get_filesize(m_file);
  else
    return -1;
}

unsigned int CGMythFile::Read(void* buffer, __int64 size)
{
  if(m_held)
    return 0;

  // return anything added in last read
  if(m_used && m_array->len > m_used)
  {
    size = std::min(size, (__int64)(m_array->len - m_used));
    memcpy(buffer, m_array->data, size);
    m_used += size;
    return size;
  }  

  while(true)
  {
    /* why do i need to call this to get reads to work?? */
    while(g_main_context_iteration(NULL, false));

    g_array_set_size((GArray*)m_array, 0);

    int ret = gmyth_file_transfer_read (m_file, m_array, size, true);
    if(ret == GMYTH_FILE_READ_ERROR)
    {
      XLOG(LOGERROR, "read failed");
      return 0;
    }
    else if(ret == GMYTH_FILE_READ_NEXT_PROG_CHAIN)
    {
      XLOG(LOGINFO, "next program chain");

      if(!m_used)
        continue; /* first read request */

      /* file user must call skipnext to get next program */
      m_held = true;
      return 0;
    }
    else if(ret != GMYTH_FILE_READ_OK)
    {
      XLOG(LOGWARNING, "unknown status %d", ret);
      return 0;
    }
    break;
  }

  size = std::min(size, (__int64)m_array->len);
  memcpy(buffer, m_array->data, size);
  m_used = size;

  return size;
}

bool CGMythFile::SkipNext()
{
  if(!m_livetv)
    return false;

  if(!gmyth_livetv_next_program_chain(m_livetv))
  {
    XLOG(LOGERROR, "failed to get next program chain");
    return false;
  }
  m_held = false;
  return true;
}

CVideoInfoTag* CGMythFile::GetVideoInfoTag()
{
  if(m_livetv && m_livetv->proginfo)
  {
    if(m_livetv->proginfo->chanstr)
    {
      m_infotag.m_strTitle = m_livetv->proginfo->chanstr->str;
      m_infotag.m_strTitle += " : ";
    }
    if(m_livetv->proginfo->title)
      m_infotag.m_strShowTitle = m_livetv->proginfo->title->str;

    m_infotag.m_strTitle += m_infotag.m_strShowTitle;

    if(m_livetv->proginfo->description)
    {
      m_infotag.m_strPlotOutline = m_livetv->proginfo->description->str;
      m_infotag.m_strPlot = m_livetv->proginfo->description->str;
    }
    m_infotag.m_iSeason = 1; /* set this so xbmc knows it's a tv show */
    return &m_infotag;
  }
  return NULL;
}

int CGMythFile::GetTotalTime()
{
  if(m_livetv && m_livetv->proginfo)
  {
    if(m_livetv->proginfo->startts && m_livetv->proginfo->endts)
    {
      GTimeVal *start = m_livetv->proginfo->startts;
      GTimeVal *end   = m_livetv->proginfo->endts;
      return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_usec - start->tv_usec) / 1000; 
    }
  }
  return 0;
}

int CGMythFile::GetStartTime()
{
  if(m_livetv && m_livetv->proginfo)
  {
    if(m_livetv->proginfo->startts && m_livetv->proginfo->recstartts)
    {
      GTimeVal *start = m_livetv->proginfo->startts;
      GTimeVal *end   = m_livetv->proginfo->recstartts;
      return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_usec - start->tv_usec) / 1000; 
    }
  }
  return 0;
}

bool CGMythFile::NextChannel()
{
  if(!m_livetv) return false;
  if(!gmyth_recorder_pause_recording(m_livetv->recorder))
  {
    XLOG(LOGERROR, "failed to pause recording");
    return false;
  }
  if(!gmyth_recorder_change_channel(m_livetv->recorder, CHANNEL_DIRECTION_DOWN))
  {
    XLOG(LOGERROR, "failed to change channel");
    //return false;
  }
  if(!gmyth_livetv_next_program_chain(m_livetv))
    XLOG(LOGERROR, "failed to get the next program info");

  return SetupTransfer();
}

bool CGMythFile::PrevChannel()
{
  if(!m_livetv) return false;
  if(!gmyth_recorder_pause_recording(m_livetv->recorder))
  {
    XLOG(LOGERROR, "failed to pause recording");
    return false;
  }
  if(!gmyth_recorder_change_channel(m_livetv->recorder, CHANNEL_DIRECTION_UP))
  {
    XLOG(LOGERROR, "failed to change channel");
    //return false;
  }

  if(!gmyth_livetv_next_program_chain(m_livetv))
    XLOG(LOGERROR, "failed to get the next program info");

  return SetupTransfer();
}

#endif
