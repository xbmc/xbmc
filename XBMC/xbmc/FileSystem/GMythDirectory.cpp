#ifdef HAS_GMYTH
#include "stdafx.h"
#include "GMythDirectory.h"
#include "Util.h"

#include <glib.h>

#include "gmyth/gmyth_backendinfo.h"
#include "gmyth/gmyth_scheduler.h"
#include "gmyth/gmyth_util.h"
#include "gmyth/gmyth_epg.h"
#include "gmyth/gmyth_file_transfer.h"
#include "gmyth/gmyth_livetv.h"
#include "gmyth/gmyth_common.h"


using namespace DIRECTORY;
using namespace XFILE;

bool CGMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  if(url.GetHostName().IsEmpty())
    return false;

  static bool inited = false;
  if(!inited)
  {
    g_type_init();
    g_thread_init(NULL);
    inited = true;
  }

  GMythBackendInfo *info = gmyth_backend_info_new();
  if(!info)
    return false;

  gmyth_backend_info_set_hostname(info, url.GetHostName().c_str());
  int port = url.GetPort();
  if(port == 0)
    port = 6543;
  gmyth_backend_info_set_port(info, port);

  if (!url.GetUserName().IsEmpty())
      gmyth_backend_info_set_username(info, url.GetUserName().c_str());
  else
      gmyth_backend_info_set_username(info, "mythtv");

  if (!url.GetPassWord().IsEmpty())
      gmyth_backend_info_set_password(info, url.GetPassWord().c_str());
  else
      gmyth_backend_info_set_password(info, "mythtv");

  gmyth_backend_info_set_db_name(info, "mythconverg");

  GMythEPG       *epg;
  gint            length;
  GList          *clist, *ch;

  if(url.GetFileName().IsEmpty())
  {
    CFileItem *item;

    item = new CFileItem(base + "/channels/", true);
    item->SetLabel("Live Channels");
    item->SetLabelPreformated(true);
    items.Add(item);

    item = new CFileItem(base + "/recordings/", true);
    item->SetLabel("Recordings");
    item->SetLabelPreformated(true);
    items.Add(item);
  }
  else if(url.GetFileName() == "channels/")
  {

    epg = gmyth_epg_new();
    if (!gmyth_epg_connect(epg, info)) {
      g_object_unref(epg);
      g_object_unref(info);
      return false;
    }

    length = gmyth_epg_get_channel_list(epg, &clist);
    for (ch = clist; ch != NULL; ch = ch->next)
    {
      GMythChannelInfo *channel = (GMythChannelInfo *) ch->data;

      if ((channel->channel_name == NULL) || (channel->channel_num == NULL))
        continue;

      // skip any channels with no channel number
      if(strcmp(channel->channel_num->str, "0") == 0)
      {
        CLog::Log(LOGDEBUG, "%s - Skipped Channel ""%s"" ", __FUNCTION__, channel->channel_name->str);
        continue;
      }

      CStdString name, path;
      name = channel->channel_num->str;
      name+= " - ";
      name+= channel->channel_name->str;

      path.Format("%s/%s.ts", base.c_str(), channel->channel_num->str);

      if(channel->channel_icon)
        CLog::Log(LOGDEBUG, "%s - Channel ""%s"" Icon: ""%s""", __FUNCTION__, name.c_str(), channel->channel_icon->str);

      CFileItem *item = new CFileItem(path, false);
      item->SetLabel(name);
      item->SetLabelPreformated(true);

      items.Add(item);
    }

    gmyth_free_channel_list(clist);
    gmyth_epg_disconnect(epg);
    g_object_unref(epg);
  }
  else if(url.GetFileName() == "recordings/")
  {

    GMythScheduler *scheduler;

    scheduler = gmyth_scheduler_new();

    if (!gmyth_scheduler_connect_with_timeout(scheduler, info, 10))
    {
        CLog::Log(LOGERROR, "%s - failed to connect to server on %s", __FUNCTION__, base.c_str());
        g_object_unref(scheduler);
        g_object_unref(info);
        return false;
    }

    length = gmyth_scheduler_get_recorded_list(scheduler, &clist);
    if (length < 0) 
    {
        CLog::Log(LOGERROR, "%s - failed to retreive list of recordings from %s", __FUNCTION__, base.c_str());
        gmyth_scheduler_disconnect(scheduler);
        g_object_unref(scheduler);
        g_object_unref(info);
        return false;
    }
    gmyth_scheduler_disconnect(scheduler);

    for (ch = clist; ch != NULL; ch = ch->next)
    {
      RecordedInfo *recording = (RecordedInfo *) ch->data;
      if(!gmyth_util_file_exists(info, recording->basename->str))
        continue;

      CStdString name, path;
      if(recording->title)
        name = recording->title->str;
      else
        name = recording->basename->str;

      path.Format("%s/%s", base.c_str(), recording->basename->str);

      CFileItem *item = new CFileItem(path, false);
      item->SetLabel(name);
      item->SetLabelPreformated(true);
      items.Add(item);
    }
    gmyth_recorded_info_list_free(clist);
  }

  g_object_unref(info);
  return true;
}


bool CGMythFile::Open(const CURL& url, bool binary)
{
  if(!binary)
    return false;

  CStdString path(url.GetFileName());
  GMythBackendInfo *info = gmyth_backend_info_new();
  if(!info)
    return false;

  gmyth_backend_info_set_hostname(info, url.GetHostName().c_str());
  gmyth_backend_info_set_port(info, url.GetPort());

/*
  if (!url.GetUserName().IsEmpty())
      gmyth_backend_info_set_username(info, url.GetUserName().c_str());
  else
      gmyth_backend_info_set_username(info, "mythtv");

  if (!url.GetPassWord().IsEmpty())
      gmyth_backend_info_set_password(info, url.GetPassWord().c_str());
  else
      gmyth_backend_info_set_password(info, "mythtv");

  gmyth_backend_info_set_db_name(info, "mythconverg");
*/

  if(path.Left(11) == "recordings/")
  {
    CStdString file = path.Mid(11);

    m_file = GMYTH_FILE(gmyth_file_transfer_new(info));
    if (!m_file) 
    {
      CLog::Log(LOGERROR, "%s - failed to create transfer", __FUNCTION__);
      g_object_unref(info);
      return false;
    }
    m_filename = g_strdup(file.c_str());
    if (!gmyth_file_transfer_open(GMYTH_FILE_TRANSFER(m_file), m_filename)) 
    {
      CLog::Log(LOGERROR, "%s - failed to open transfer", __FUNCTION__);
      g_object_unref(info);
      return false;
    }
    g_object_unref(info);
    return true;
  } 
  else if (path.Left(9) == "channels/")
  {
    CStdString channel = path.Mid(9);
    if(!CUtil::GetExtension(channel).Equals(".ts"))
    {
      CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
      Close();
      return false;
    }
    CUtil::RemoveExtension(channel);

    m_livetv = gmyth_livetv_new(info);
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
    m_file = GMYTH_FILE(gmyth_livetv_create_file_transfer(m_livetv));
    if (!m_file) 
    {
      CLog::Log(LOGERROR, "%s - failed to create transfer", __FUNCTION__);
      Close();
      return false;
    }
    if (m_livetv->uri)
      m_filename = g_strdup(gmyth_uri_get_path(m_livetv->uri));
    else
      m_filename = g_strdup(m_livetv->proginfo->pathname->str);

    if (!gmyth_file_transfer_open(GMYTH_FILE_TRANSFER(m_file), m_filename)) 
    {
      CLog::Log(LOGERROR, "%s - failed to open transfer", __FUNCTION__);
      Close();
      return false;
    }
    g_object_unref(info);
    return true;
  }
  else
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, path.c_str());

  g_object_unref(info);
  return false;
}

void CGMythFile::Close()
{
  if(m_file)
  {
    gmyth_file_transfer_close(GMYTH_FILE_TRANSFER(m_file));
    g_object_unref(m_file);
    m_file = NULL;
  }
  if(m_livetv)
  {
    gmyth_recorder_close(m_livetv->recorder);
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
}

CGMythFile::CGMythFile()
{
  m_array = (GByteArray*)g_array_new(false, true, sizeof(gchar));
  m_file = NULL;
  m_livetv = NULL;
  m_used = 0;
  m_filename = NULL;
  m_channel = NULL;
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

  if(m_file)
    return gmyth_file_transfer_seek(GMYTH_FILE_TRANSFER(m_file), pos, whence);
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
    return gmyth_file_transfer_get_filesize(GMYTH_FILE_TRANSFER(m_file));
  else
    return -1;
}

unsigned int CGMythFile::Read(void* buffer, __int64 size)
{
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

    int ret = gmyth_file_transfer_read (GMYTH_FILE_TRANSFER(m_file), m_array, size, true);
    if(ret == GMYTH_FILE_READ_ERROR)
    {
      CLog::Log(LOGERROR, "%s - read failed", __FUNCTION__);
      return 0;
    }
    else if(ret == GMYTH_FILE_READ_NEXT_PROG_CHAIN)
    {
      CLog::Log(LOGINFO, "%s - next program chain", __FUNCTION__);
      continue;
    }
    else if(ret != GMYTH_FILE_READ_OK)
    {
      CLog::Log(LOGWARNING, "%s - unknown status %d", __FUNCTION__, ret);
      return 0;
    }
    break;
  }

  size = std::min(size, (__int64)m_array->len);
  memcpy(buffer, m_array->data, size);
  m_used = size;

  return size;
}
#endif
