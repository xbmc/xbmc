#include "stdafx.h"
#include "CMythDirectory.h"
#include "CMythSession.h"
#include "Util.h"
#include "DllLibCMyth.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

using namespace DIRECTORY;
using namespace XFILE;
using namespace std;

bool CCMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  CCMythSession* session = CCMythSession::AquireSession(strPath);
  if(!session)
    return false;

  DllLibCMyth* dll = session->GetLibrary();
  if(!dll)
  {
    CCMythSession::ReleaseSession(session);
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
    cmyth_database_t db = session->GetDatabase();
    if(!db)
    {
      CCMythSession::ReleaseSession(session);
      return false;
    }

    cmyth_chanlist_t list = dll->mysql_get_chanlist(db);
    if(!list)
    {
      CLog::Log(LOGERROR, "%s - unable to get list of channels with url %s", __FUNCTION__, strPath.c_str());
      CCMythSession::ReleaseSession(session);
      return false;
    }
    int count = dll->chanlist_get_count(list);
    for(int i = 0; i < count; i++)
    {
      cmyth_channel_t channel = dll->chanlist_get_item(list, i);
      if(channel)
      {
        CStdString name, path, icon;

        int num = dll->channel_channum(channel);
        char* str;
        if((str = dll->channel_name(channel)))
        {
          name.Format("%d - %s", num, str); 
          dll->ref_release(str);
        }
        else
          name.Format("%d");

        if((str = dll->channel_icon(channel)))
        {
          icon = str;
          dll->ref_release(str);
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
        dll->ref_release(channel);
      }
    }
    dll->ref_release(list);
  }
  else if(url.GetFileName() == "recordings/")
  {
    cmyth_conn_t control = session->GetControl();
    if(!control)
    {
      CCMythSession::ReleaseSession(session);
      return false;
    }

    cmyth_proglist_t list = dll->proglist_get_all_recorded(control);
    if(!list)
    {
      CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
      CCMythSession::ReleaseSession(session);
      return false;
    }
    int count = dll->proglist_get_count(list);
    for(int i=0; i<count; i++)
    {
      cmyth_proginfo_t program = dll->proglist_get_item(list, i);
      if(program)
      {
        char* str;
        if((str = dll->proginfo_recgroup(program)))
        {
          if(strcmp(str, "LiveTV") == 0)
          {
            dll->ref_release(str);
            dll->ref_release(program);
            continue;
          }
        }

        if((str = dll->proginfo_pathname(program)))
        {
          CStdString recording, name, path;

          recording = CUtil::GetFileName(str);
          dll->ref_release(str);

          if((str = dll->proginfo_title(program)))
          {
            name = str;
            dll->ref_release(str);
          }
          else
            name = recording;

          CLog::Log(LOGDEBUG, "%s - recording %s (%s) has status %d", __FUNCTION__, name.c_str(), recording.c_str(), dll->proginfo_rec_status(program));
          path.Format("%s/%s", base.c_str(), recording.c_str());

          CFileItem *item = new CFileItem(path, false);
          item->SetLabel(name);

          /* fill video info tag */
          session->ProgramToTag(program, item->GetVideoInfoTag());


          if(dll->proginfo_rec_status(program) == RS_RECORDING)
            item->SetLabel2("(Recording)");
          else
            item->SetLabel2(item->GetVideoInfoTag()->m_strRuntime);

          item->SetLabelPreformated(true);
          items.Add(item);
        }
        dll->ref_release(program);
      }
    }
    dll->ref_release(list);
  }
  CCMythSession::ReleaseSession(session);
  return true;
}
