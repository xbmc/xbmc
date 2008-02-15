#include "stdafx.h"
#include "CMythDirectory.h"
#include "Util.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

#define XLOG(level, message, args...) CLog::Log(level, "CGMyth::%s - " message, __FUNCTION__, ##args)

using namespace DIRECTORY;


bool CCMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);
  cmyth_dbg_level(CMYTH_DBG_DETAIL); 

  if(url.GetHostName().IsEmpty())
    return false;

  int port = url.GetPort();
  if(port == 0)
    port = 6543;

  cmyth_conn_t control = cmyth_conn_connect_ctrl((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!control)
  {
    XLOG(LOGERROR, "unable to connect to server %s, port %d", url.GetHostName().c_str(), port);
    return false;
  }

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
    CStdString user(url.GetUserName());
    CStdString pass(url.GetPassWord());
    if(user == "")
      user = "mythtv";
    if(pass == "")
      pass = "mythtv";
    cmyth_database_t db = cmyth_database_init((char*)url.GetHostName().c_str(), "mythconverg", (char*)user.c_str(), (char*)pass.c_str());
    if(!db)
    {
      XLOG(LOGERROR, "unable to connect to db with url %s", strPath.c_str());
      ref_release(control);
      return false;      
    }
    cmyth_chanlist_t list = cmyth_mysql_get_chanlist(db);
    if(!list)
    {
      XLOG(LOGERROR, "unable to get list of channels with url %s", strPath.c_str());
      ref_release(db); 
      ref_release(control); 
      return false;
    }
    int count = cmyth_chanlist_get_count(list);
    for(int i = 0; i < count; i++)
    {
      cmyth_channel_t channel = cmyth_chanlist_get_item(list, i);
      if(channel)
      {
        CStdString name, path, icon;

        int num = cmyth_channel_channum(channel);
        char* str;
        if((str = cmyth_channel_name(channel)))
        {
          name.Format("%d - %s", num, str); 
          ref_release(str);
        }
        else
          name.Format("%d");

        if((str = cmyth_channel_icon(channel)))
        {
          icon = str;
          ref_release(str);
        }

        path.Format("%s/%d.ts", base.c_str(), num);

        if(num <= 0)
        {
          XLOG(LOGDEBUG, "Channel '%s' Icon '%s' - Skipped", name.c_str(), icon.c_str());
        }
        else
        {
          XLOG(LOGDEBUG, "Channel '%s' Icon '%s'", name.c_str(), icon.c_str());
          CFileItem *item = new CFileItem(path, false);
          item->SetLabel(name);
          item->SetLabelPreformated(true);
          items.Add(item);
        }
        ref_release(channel);
      }
    }
    ref_release(list);
    ref_release(db);
  }
  else if(url.GetFileName() == "recordings/")
  {
    cmyth_proglist_t list = cmyth_proglist_get_all_recorded(control);
    if(!list)
    {
      XLOG(LOGERROR, "unable to get list of recordings");
      ref_release(control);
      return false;
    }
    int count = cmyth_proglist_get_count(list);
    for(int i=0; i<count; i++)
    {
      cmyth_proginfo_t program = cmyth_proglist_get_item(list, i);
      if(program)
      {
        char* str;
        if((str = cmyth_proginfo_recgroup(program)))
        {
          if(strcmp(str, "LiveTV") == 0)
          {
            ref_release(str);
            ref_release(program);
            continue;
          }
        }

        if((str = cmyth_proginfo_pathname(program)))
        {
          CStdString recording, name, path;

          recording = CUtil::GetFileName(str);
          ref_release(str);

          if((str = cmyth_proginfo_title(program)))
          {
            name = str;
            ref_release(str);
          }
          else
            name = recording;

          XLOG(LOGDEBUG, "recording %s (%s) has status %d", name.c_str(), recording.c_str(), cmyth_proginfo_rec_status(program));
          path.Format("%s/%s", base.c_str(), recording.c_str());

          CFileItem *item = new CFileItem(path, false);
          item->SetLabel(name);
          item->SetLabelPreformated(true);
          items.Add(item);
        }
        ref_release(program);
      }
    }
    ref_release(list);
  }

  return true;
}
