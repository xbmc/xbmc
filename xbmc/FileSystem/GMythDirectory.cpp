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

#define XLOG(level, message, args...) CLog::Log(level, "CGMyth::%s - " message, __FUNCTION__, ##args)

using namespace DIRECTORY;


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
        XLOG(LOGDEBUG, "Skipped Channel ""%s""", channel->channel_name->str);
        continue;
      }

      CStdString name, path;
      name = channel->channel_num->str;
      name+= " - ";
      name+= channel->channel_name->str;

      path.Format("%s/%s.ts", base.c_str(), channel->channel_num->str);

      if(channel->channel_icon)
        XLOG(LOGDEBUG, "Channel ""%s"" Icon: ""%s""", name.c_str(), channel->channel_icon->str);

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
        XLOG(LOGERROR, "failed to connect to server on %s", base.c_str());
        g_object_unref(scheduler);
        g_object_unref(info);
        return false;
    }

    length = gmyth_scheduler_get_recorded_list(scheduler, &clist);
    if (length < 0) 
    {
        XLOG(LOGERROR, "failed to retreive list of recordings from %s", base.c_str());
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

#endif
