#include "stdafx.h"
#include "CMythDirectory.h"
#include "Util.h"
#include "DllLibCMyth.h"

extern "C" {
#if defined(_XBOX) || defined(WIN32)
#include "lib/libcmyth/cmyth.h"
#include "lib/libcmyth/mvp_refmem.h"
#else
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
#endif
}

using namespace DIRECTORY;


bool CCMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);
  DllLibCMyth dll;
  if(!dll.Load())
    return false;

  dll.dbg_level(CMYTH_DBG_DETAIL);

  if(url.GetHostName().IsEmpty())
    return false;

  int port = url.GetPort();
  if(port == 0)
    port = 6543;

  cmyth_conn_t control = dll.conn_connect_ctrl((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!control)
  {
    CLog::Log(LOGERROR, "%s - unable to connect to server %s, port %d", __FUNCTION__, url.GetHostName().c_str(), port);
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
    cmyth_database_t db = dll.database_init((char*)url.GetHostName().c_str(), "mythconverg", (char*)user.c_str(), (char*)pass.c_str());
    if(!db)
    {
      CLog::Log(LOGERROR, "%s - unable to connect to db with url %s", __FUNCTION__, strPath.c_str());
      dll.ref_release(control);
      return false;      
    }
    cmyth_chanlist_t list = dll.mysql_get_chanlist(db);
    if(!list)
    {
      CLog::Log(LOGERROR, "%s - unable to get list of channels with url %s", __FUNCTION__, strPath.c_str());
      dll.ref_release(db); 
      dll.ref_release(control); 
      return false;
    }
    int count = dll.chanlist_get_count(list);
    for(int i = 0; i < count; i++)
    {
      cmyth_channel_t channel = dll.chanlist_get_item(list, i);
      if(channel)
      {
        CStdString name, path, icon;

        int num = dll.channel_channum(channel);
        char* str;
        if((str = dll.channel_name(channel)))
        {
          name.Format("%d - %s", num, str); 
          dll.ref_release(str);
        }
        else
          name.Format("%d");

        if((str = dll.channel_icon(channel)))
        {
          icon = str;
          dll.ref_release(str);
        }

        path.Format("%s/%d.ts", base.c_str(), num);

        if(num <= 0)
        {
          CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s' - Skipped", __FUNCTION__, name.c_str(), icon.c_str());
        }
        else
        {
          CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s'", __FUNCTION__, name.c_str(), icon.c_str());
          CFileItem *item = new CFileItem(path, false);
          item->SetLabel(name);
          item->SetLabelPreformated(true);
          items.Add(item);
        }
        dll.ref_release(channel);
      }
    }
    dll.ref_release(list);
    dll.ref_release(db);
  }
  else if(url.GetFileName() == "recordings/")
  {
    cmyth_proglist_t list = dll.proglist_get_all_recorded(control);
    if(!list)
    {
      CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
      dll.ref_release(control);
      return false;
    }
    int count = dll.proglist_get_count(list);
    for(int i=0; i<count; i++)
    {
      cmyth_proginfo_t program = dll.proglist_get_item(list, i);
      if(program)
      {
        char* str;
        if((str = dll.proginfo_recgroup(program)))
        {
          if(strcmp(str, "LiveTV") == 0)
          {
            dll.ref_release(str);
            dll.ref_release(program);
            continue;
          }
        }

        if((str = dll.proginfo_pathname(program)))
        {
          CStdString recording, name, path;

          recording = CUtil::GetFileName(str);
          dll.ref_release(str);

          if((str = dll.proginfo_title(program)))
          {
            name = str;
            dll.ref_release(str);
          }
          else
            name = recording;

          CLog::Log(LOGDEBUG, "%s - recording %s (%s) has status %d", __FUNCTION__, name.c_str(), recording.c_str(), dll.proginfo_rec_status(program));
          path.Format("%s/%s", base.c_str(), recording.c_str());

          CFileItem *item = new CFileItem(path, false);
          item->SetLabel(name);
          item->SetLabelPreformated(true);
          items.Add(item);
        }
        dll.ref_release(program);
      }
    }
    dll.ref_release(list);
  }

  return true;
}
