#include "stdafx.h"
#include "CMythDirectory.h"
#include "CMythSession.h"
#include "Util.h"
#include "DllLibCMyth.h"
#include "VideoInfoTag.h"
#include "URL.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

using namespace DIRECTORY;
using namespace XFILE;
using namespace std;

CCMythDirectory::CCMythDirectory()
{
  m_session  = NULL;
  m_dll      = NULL;
  m_database = NULL;
  m_recorder = NULL;
}

CCMythDirectory::~CCMythDirectory()
{
  Release();
}

void CCMythDirectory::Release()
{
  if(m_recorder)
  {
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_session)
  {
    CCMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
  m_dll = NULL;
}

bool CCMythDirectory::GetRecordings(const CStdString& base, CFileItemList &items)
{
  cmyth_conn_t control = m_session->GetControl();
  if(!control)
    return false;

  CURL url(base);

  cmyth_proglist_t list = m_dll->proglist_get_all_recorded(control);
  if(!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
    return false;
  }
  int count = m_dll->proglist_get_count(list);
  for(int i=0; i<count; i++)
  {
    cmyth_proginfo_t program = m_dll->proglist_get_item(list, i);
    if(program)
    {
      if(GetValue(m_dll->proginfo_recgroup(program)).Equals("LiveTV"))
      {
        m_dll->ref_release(program);
        continue;
      }

      CStdString name, path;

      path = GetValue(m_dll->proginfo_pathname(program));
      path = CUtil::GetFileName(path);
      name = GetValue(m_dll->proginfo_title(program));

      CFileItem *item = new CFileItem("", false);
      m_session->UpdateItem(*item, program);

      url.SetFileName("recordings/" + path);
      url.GetURL(item->m_strPath);

      url.SetFileName("files/" + path +  ".png");
      url.GetURL(path);
      item->SetThumbnailImage(path);

      if(m_dll->proginfo_rec_status(program) != RS_RECORDING)
        name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      else
      {
        name += " (Recording)";
        item->SetThumbnailImage("");
      }

      item->SetLabel(name);

      items.Add(item);
      m_dll->ref_release(program);
    }

    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    else
      items.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    items.AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%Z (%J)", "%I", "%L", "%I"));
    items.AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%Z", "%J %Q", "%L", "%J"));

  }
  m_dll->ref_release(list);
  return true;
}


bool CCMythDirectory::GetChannelsDb(const CStdString& base, CFileItemList &items)
{
  cmyth_database_t db = m_session->GetDatabase();
  if(!db)
    return false;

  cmyth_chanlist_t list = m_dll->mysql_get_chanlist(db);
  if(!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of channels with url %s", __FUNCTION__, base.c_str());
    return false;
  }
  CURL url(base);

  int count = m_dll->chanlist_get_count(list);
  for(int i = 0; i < count; i++)
  {
    cmyth_channel_t channel = m_dll->chanlist_get_item(list, i);
    if(channel)
    {
      CStdString name, path, icon;

      int num = m_dll->channel_channum(channel);
      char* str;
      if((str = m_dll->channel_name(channel)))
      {
        name.Format("%d - %s", num, str); 
        m_dll->ref_release(str);
      }
      else
        name.Format("%d");

      icon = GetValue(m_dll->channel_icon(channel));

      if(num <= 0)
      {
        CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s' - Skipped", __FUNCTION__, name.c_str(), icon.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s'", __FUNCTION__, name.c_str(), icon.c_str());
        path.Format("channels/%d.ts", num);
        url.SetFileName(path);
        url.GetURL(path);
        CFileItem *item = new CFileItem(path, false);
        item->SetLabel(name);
        item->SetLabelPreformated(true);
        if(icon.length() > 0)
        {
          url.SetFileName("files/channels/" + CUtil::GetFileName(icon));
          url.GetURL(icon);
          item->SetThumbnailImage(icon);
        }
        items.Add(item);
      }
      m_dll->ref_release(channel);
    }
  }
  m_dll->ref_release(list);
  return true;
}

bool CCMythDirectory::GetChannels(const CStdString& base, CFileItemList &items)
{
  cmyth_conn_t control = m_session->GetControl();
  if(!control)
    return false;

  std::vector<cmyth_proginfo_t> channels;
  for(unsigned i=0;i<16;i++)
  {
    cmyth_recorder_t recorder = m_dll->conn_get_recorder_from_num(control, i);
    if(!recorder)
      continue;

    cmyth_proginfo_t program;
    program = m_dll->recorder_get_cur_proginfo(recorder);
    program = m_dll->recorder_get_next_proginfo(recorder, program, BROWSE_DIRECTION_UP);
    if(!program) {
      m_dll->ref_release(m_recorder);
      continue;
    }

    long startchan = m_dll->proginfo_chan_id(program);
    long currchan  = -1;
    while(startchan != currchan)
    {
      unsigned j;
      for(j=0;j<channels.size();j++)
      {
        if(m_dll->proginfo_compare(program, channels[j]) == 0)
          break;
      }

      if(j == channels.size())
        channels.push_back(program);

      program = m_dll->recorder_get_next_proginfo(recorder, program, BROWSE_DIRECTION_UP);
      if(!program)
        break;

      currchan = m_dll->proginfo_chan_id(program);
    }
    m_dll->ref_release(recorder);
  }

  CURL url(base);

  for(unsigned i=0;i<channels.size();i++)
  {
    cmyth_proginfo_t program = channels[i];
    CStdString num, progname, channame, icon, sign;

    num      = GetValue(m_dll->proginfo_chanstr (program));
    icon     = GetValue(m_dll->proginfo_chanicon(program));

    CFileItem *item = new CFileItem("", false);
    m_session->UpdateItem(*item, program);
    url.SetFileName("channels/" + num + ".ts");
    url.GetURL(item->m_strPath);
    item->SetLabel(GetValue(m_dll->proginfo_chansign(program)));

    if(icon.length() > 0)
    {
      url.SetFileName("files/channels/" + CUtil::GetFileName(icon));
      url.GetURL(icon);
      item->SetThumbnailImage(icon);
    }

    /* hack to get sorting working properly when sorting by show title */
    if(item->GetVideoInfoTag()->m_strShowTitle.IsEmpty())
      item->GetVideoInfoTag()->m_strShowTitle = " ";

    items.Add(item);
    m_dll->ref_release(program);
  }

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));


  return true;
}

bool CCMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  m_session = CCMythSession::AquireSession(strPath);
  if(!m_session)
    return false;

  m_dll = m_session->GetLibrary();
  if(!m_dll)
    return false;

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

    return true;
  }
  else if(url.GetFileName() == "channels/")
    return GetChannels(base, items);

  else if(url.GetFileName() == "channelsdb/")
    return GetChannelsDb(base, items);

  else if(url.GetFileName() == "recordings/")
    return GetRecordings(base, items);

  return false;
}

CDateTime CCMythDirectory::GetValue(cmyth_timestamp_t t) 
{ 
  return m_session->GetValue(t); 
}