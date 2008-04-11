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

bool CCMythDirectory::GetGuide(const CStdString& base, CFileItemList &items)
{
  CURL url(base);
  CStdString strPath = url.GetFileName();
  std::vector<CStdString> tokens;
  CStdString Delimiter = "/";
  CUtil::Tokenize(strPath, tokens, "/");

  if (tokens.size() > 1)
    return GetGuideForChannel(base, atoi(tokens[1].c_str()), items);
  else
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
          path.Format("guide/%d/", num);
          url.SetFileName(path);
          url.GetURL(path);
          CFileItem *item = new CFileItem(path, true);
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
}

bool CCMythDirectory::GetGuideForChannel(const CStdString& base, int ChanID, CFileItemList &items)
{
  cmyth_database_t db = m_session->GetDatabase();
  if(!db)
  {
    CLog::Log(LOGERROR, "%s - Could not get database", __FUNCTION__);
    return false;
  }

  time_t now;
  time(&now);
  // this sets how many seconds of EPG from now we should grabb
  time_t end = now + (1 * 24 * 60 * 60);

  cmyth_program_t *prog = NULL;

  int count = m_dll->mysql_get_guide(db, &prog, now, end);
  CLog::Log(LOGDEBUG, "%s - %i entries of guide data", __FUNCTION__, count);
  if (count <= 0)
    return false;

  for (int i = 0; i < count; i++)
  {
    if (prog[i].channum == ChanID)
    {
      CStdString path;
      path.Format("%s%s", base.c_str(), prog[i].title);

      CDateTime starttime(prog[i].starttime);
      CDateTime endtime(prog[i].endtime);

      CStdString title;
      title.Format("%s - \"%s\"", starttime.GetAsLocalizedDateTime(), prog[i].title);

      CFileItem *item = new CFileItem(title, false);
      item->SetLabel(title);
      item->m_dateTime = starttime;
      item->SetLabelPreformated(true);

      CVideoInfoTag* tag = item->GetVideoInfoTag();

      tag->m_strAlbum       = GetValue(prog[i].callsign);
      tag->m_strShowTitle   = GetValue(prog[i].title);
      tag->m_strPlotOutline = GetValue(prog[i].subtitle);
      tag->m_strPlot        = GetValue(prog[i].description);
      tag->m_strGenre       = GetValue(prog[i].category);

      if(tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
          tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
      tag->m_strOriginalTitle = tag->m_strShowTitle;

      tag->m_strTitle = tag->m_strAlbum;
      if(tag->m_strShowTitle.length() > 0)
        tag->m_strTitle += " : " + tag->m_strShowTitle;

      CDateTimeSpan span(endtime.GetDay() - starttime.GetDay(),
                         endtime.GetHour() - starttime.GetHour(),
                         endtime.GetMinute() - starttime.GetMinute(),
                         endtime.GetSecond() - starttime.GetSecond());

      StringUtils::SecondsToTimeString( span.GetSeconds()
                                      + span.GetMinutes() * 60 
                                      + span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);

      tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
      tag->m_iEpisode = 0;
      tag->m_strStatus = prog[i].rec_status;
      items.Add(item);
    }
  }
  m_dll->ref_release(prog);
  return true;
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

    item = new CFileItem(base + "/guide/", true);
    item->SetLabel("Guide");
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

  else if(url.GetFileName().Left(5) == "guide")
    return GetGuide(base, items);

  return false;
}
