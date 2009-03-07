/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "CMythDirectory.h"
#include "CMythSession.h"
#include "Util.h"
#include "DllLibCMyth.h"
#include "VideoInfoTag.h"
#include "URL.h"
#include "GUISettings.h"
#include "FileItem.h"

extern "C" {
#include "lib/libcmyth/cmyth.h"
#include "lib/libcmyth/mvp_refmem.h"
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
  vector<CStdString> tokens;
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

        if(!m_dll->channel_visible(channel))
        {
          m_dll->ref_release(channel);
          continue;
        }
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
          CFileItemPtr item(new CFileItem(path, true));
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

bool CCMythDirectory::GetGuideForChannel(const CStdString& base, int ChanNum, CFileItemList &items)
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
    if (prog[i].channum == ChanNum)
    {
      CStdString path;
      path.Format("%s%s", base.c_str(), prog[i].title);

      CDateTime starttime(prog[i].starttime);
      CDateTime endtime(prog[i].endtime);

      CStdString title;
      title.Format("%s - \"%s\"", starttime.GetAsLocalizedDateTime(), prog[i].title);

      CFileItemPtr item(new CFileItem(title, false));
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

bool CCMythDirectory::GetRecordings(const CStdString& base, CFileItemList &items, enum FilterType type, const CStdString& filter)
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
      name = GetValue(m_dll->proginfo_title(program));
      /* If filtering by title, then just compare against the given filter */
      if ( type == bytitle )
      {
        if ( filter != name )
        {
          m_dll->ref_release(program);
          continue;
        }
      }
      /* Filtering by recording group requires another call to the DLL */
      if ( type == bygroup )
      {
        if (! ( GetValue(m_dll->proginfo_recgroup(program)).Equals(filter)))
        {
          m_dll->ref_release(program);
          continue;
        }
      }
      path = GetValue(m_dll->proginfo_pathname(program));
      path = CUtil::GetFileName(path);

      CFileItemPtr item(new CFileItem("", false));
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

/**
 * \brief Gets a list of groups of recordings, based on what filter is requested
 *
 */
bool CCMythDirectory::GetRecordingGroups(const CStdString& base, CFileItemList &items, enum FilterType type)
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

      CStdString itemName = "";
      /* Only two possibilities so far */
      if ( type == bytitle )
      {
        itemName = GetValue(m_dll->proginfo_title(program));
      }
      if ( type == bygroup )
      {
        itemName = GetValue(m_dll->proginfo_recgroup(program));
      }
      if (itemName == "" )
      {
        m_dll->ref_release(program);
        continue;
      }
      /* Don't want to add repeats of a group */
      if (items.Contains(base + "/" + itemName + "/"))
      {
        m_dll->ref_release(program);
        continue;
      }
      CFileItemPtr item(new CFileItem(base + "/" + itemName + "/", true));
      item->SetLabel(itemName);
      item->SetLabelPreformated(true);
      items.Add(item);
      m_dll->ref_release(program);
    }

    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
    else
      items.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%Z (%J)", "%I", "%L", ""));
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
        CFileItemPtr item(new CFileItem(path, false));
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

  vector<cmyth_proginfo_t> channels;
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

    CFileItemPtr item(new CFileItem("", false));
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
  //CLog::Log(LOGINFO,"CMythDirectory::GetDirectory(%s)",base.c_str());

  m_session = CCMythSession::AquireSession(strPath);
  if(!m_session)
    return false;

  m_dll = m_session->GetLibrary();
  if(!m_dll)
    return false;

  if(url.GetFileName().IsEmpty())
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel(g_localizeStrings.Get(22018));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings-all/", true));
    item->SetLabel(g_localizeStrings.Get(22015));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings-by-title/", true));
    item->SetLabel(g_localizeStrings.Get(22019));
    item->SetLabelPreformated(true);
    items.Add(item);

    /* I have not personally tested recording groups so I can't be sure
     * they are foolproof yet, hence they are not accessible for now */
    /*
    item.reset(new CFileItem(base + "/recordings-by-group/", true));
    item->SetLabel("Recordings by group");
    item->SetLabelPreformated(true);
    items.Add(item);
    */
    item.reset(new CFileItem(base + "/guide/", true));
    item->SetLabel(g_localizeStrings.Get(22020));
    item->SetLabelPreformated(true);
    items.Add(item);

    return true;
  }
  else if(url.GetFileName() == "channels/")
    return GetChannels(base, items);

  else if(url.GetFileName() == "channelsdb/")
    return GetChannelsDb(base, items);

/*  This section should never be reached, as we cannot get to the recordings folder by itself
 *  It was removed as it extra remote clicks to browse, but is in if this decision is to be
 *  reversed. *
 */
  else if(url.GetFileName() == "recordings/")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/by-title/",true));
    item->SetLabel(g_localizeStrings.Get(22016));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/by-group/",true));
    item->SetLabel(g_localizeStrings.Get(22017));
    item->SetLabelPreformated(true);
    items.Add(item);

    return true;

  }

  else if(url.GetFileName().Left(5) == "guide")
    return GetGuide(base, items);

/* There are two possible URLS which can get here.  Either /recordings/by-XXXXX/YYYY (which
 * cannot be reached any more) or /recordings-by-XXXXX/YYYY.
 * First we check for the 10 first characters being "recordings"
 */

  else if(url.GetFileName().Left(10) == "recordings") {
    /* Trim away the "recordings" part, and also the - or / seperator */
    CStdString filterPart = url.GetFileName().Right(url.GetFileName().length() - 11);
    if ( filterPart == "all/" )
      return GetRecordings(base, items, all,"");
    if ( filterPart.Left(9) == "by-title/" ) {
      /* We have "by-title" so check whether the URL ends in "by-title" or not */
      if ( filterPart == "by-title/" )
        return GetRecordingGroups(base, items, bytitle);
      /* A show is specified.  "by-title/" is 9 characters so chop them off */
      CStdString filter = filterPart.Right( filterPart.length() - 9);
      /* Remove trailing slash */
      filter = filter.Left(filter.length() - 1);
      return GetRecordings(base, items, bytitle, filter);
    }
    if ( filterPart.Left(9) == "by-group/") {
      /* We are filtering by group */
      if ( filterPart == "by-group" )
        return GetRecordingGroups(base, items, bygroup);
      /* Extract relevant section */
      CStdString filter = filterPart.Right( filterPart.length() - 9);
      filter = filter.Left(filter.length() - 1);
      return GetRecordings(base, items, bygroup, filter);
    }
  }
  return false;
}

CDateTime CCMythDirectory::GetValue(cmyth_timestamp_t t)
{
  return m_session->GetValue(t);
}
