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

#include "CMythDirectory.h"
#include "CMythSession.h"
#include "Util.h"
#include "DllLibCMyth.h"
#include "VideoInfoTag.h"
#include "URL.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "FileItem.h"
#include "StringUtils.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "DirectoryCache.h"

extern "C"
{
#include "cmyth/include/cmyth/cmyth.h"
#include "cmyth/include/refmem/refmem.h"
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

DIR_CACHE_TYPE CCMythDirectory::GetCacheType(const CStdString& strPath) const
{
  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

  /*
   * Always cache "All Recordings", "Guide" (top folder only), "Movies", and "TV Shows" (including
   * sub folders).
   *
   * Entire directory cache for myth:// is invalidated when the root directory is requested to
   * ensure content is always up-to-date.
   */
  if (fileName == "recordings"
  ||  fileName == "guide"
  ||  fileName == "movies"
  ||  fileName.Left(7) == "tvshows")
    return DIR_CACHE_ALWAYS;

  return DIR_CACHE_ONCE;
}

void CCMythDirectory::Release()
{
  if (m_recorder)
  {
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if (m_session)
  {
    CCMythSession::ReleaseSession(m_session);
    m_session = NULL;
  }
  m_dll = NULL;
}

bool CCMythDirectory::GetGuide(const CStdString& base, CFileItemList &items)
{
  cmyth_database_t db = m_session->GetDatabase();
  if (!db)
    return false;

  cmyth_chanlist_t list = m_dll->mysql_get_chanlist(db);
  if (!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of channels with url %s", __FUNCTION__, base.c_str());
    return false;
  }
  CURL url(base);

  int count = m_dll->chanlist_get_count(list);
  for (int i = 0; i < count; i++)
  {
    cmyth_channel_t channel = m_dll->chanlist_get_item(list, i);
    if (channel)
    {
      CStdString name, path, icon;

      if (!m_dll->channel_visible(channel))
      {
        m_dll->ref_release(channel);
        continue;
      }
      int num = m_dll->channel_channum(channel);
      char* str;
      if ((str = m_dll->channel_name(channel)))
      {
        name.Format("%d - %s", num, str);
        m_dll->ref_release(str);
      }
      else
        name.Format("%d");

      icon = GetValue(m_dll->channel_icon(channel));

      if (num <= 0)
      {
        CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s' - Skipped", __FUNCTION__, name.c_str(), icon.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - Channel '%s' Icon '%s'", __FUNCTION__, name.c_str(), icon.c_str());
        path.Format("guide/%d/", num);
        url.SetFileName(path);
        CFileItemPtr item(new CFileItem(url.Get(), true));
        item->SetLabel(name);
        item->SetLabelPreformated(true);
        if (icon.length() > 0)
        {
          url.SetFileName("files/channels/" + CUtil::GetFileName(icon));
          item->SetThumbnailImage(url.Get());
        }
        items.Add(item);
      }
      m_dll->ref_release(channel);
    }
  }

  // Sort by name only. Labels are preformated.
  items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

  m_dll->ref_release(list);
  return true;
}

bool CCMythDirectory::GetGuideForChannel(const CStdString& base, CFileItemList &items, int channelNumber)
{
  cmyth_database_t database = m_session->GetDatabase();
  if (!database)
  {
    CLog::Log(LOGERROR, "%s - Could not get database", __FUNCTION__);
    return false;
  }

  time_t now;
  time(&now);
  // this sets how many seconds of EPG from now we should grab
  time_t end = now + (1 * 24 * 60 * 60);

  cmyth_program_t *program = NULL;

  int count = m_dll->mysql_get_guide(database, &program, now, end);
  CLog::Log(LOGDEBUG, "%s - %i entries of guide data", __FUNCTION__, count);
  if (count <= 0)
    return false;

  for (int i = 0; i < count; i++)
  {
    if (program[i].channum == channelNumber)
    {
      CStdString path;
      path.Format("%s%s", base.c_str(), program[i].title);

      CDateTime starttime(program[i].starttime);
      CDateTime endtime(program[i].endtime);

      CDateTime localstart;
      if (program[i].starttime)
      {
        tm *local = localtime(&program[i].starttime); // Conversion to local time
        /*
         * Microsoft implementation of localtime returns NULL if on or before epoch.
         * (http://msdn.microsoft.com/en-us/library/bf12f0hc(VS.80).aspx)
         */
        if (local)
          localstart = *local;
        else
          localstart = program[i].starttime; // Use the actual start time as close enough.
      }
      CStdString title;
      title.Format("%s - %s", localstart.GetAsLocalizedTime("HH:mm", false), program[i].title);

      CFileItemPtr item(new CFileItem(title, false));
      item->SetLabel(title);
      item->m_dateTime = localstart;

      CVideoInfoTag* tag = item->GetVideoInfoTag();

      tag->m_strAlbum       = program[i].callsign;
      tag->m_strShowTitle   = program[i].title;
      tag->m_strPlotOutline = program[i].subtitle;
      tag->m_strPlot        = program[i].description;
      tag->m_strGenre       = program[i].category;

      if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
        tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
      tag->m_strOriginalTitle = tag->m_strShowTitle;

      tag->m_strTitle = tag->m_strAlbum;
      if (tag->m_strShowTitle.length() > 0)
        tag->m_strTitle += " : " + tag->m_strShowTitle;

      CDateTimeSpan runtime = endtime - starttime;
      StringUtils::SecondsToTimeString( runtime.GetSeconds()
                                      + runtime.GetMinutes() * 60
                                      + runtime.GetHours() * 3600, tag->m_strRuntime);

      tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
      tag->m_iEpisode = 0;
      tag->m_strStatus = program[i].rec_status;
      items.Add(item);
    }
  }

  // Sort by date only.
  items.AddSortMethod(SORT_METHOD_DATE, 552 /* Date */, LABEL_MASKS("%L", "%J", "%L", ""));

  m_dll->ref_release(program);
  return true;
}

bool CCMythDirectory::GetRecordings(const CStdString& base, CFileItemList &items, enum FilterType type,
                                    const CStdString& filter)
{
  cmyth_conn_t control = m_session->GetControl();
  if (!control)
    return false;

  cmyth_proglist_t list = m_dll->proglist_get_all_recorded(control);
  if (!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
    return false;
  }

  int count = m_dll->proglist_get_count(list);
  for (int i = 0; i < count; i++)
  {
    cmyth_proginfo_t program = m_dll->proglist_get_item(list, i);
    if (program)
    {
      if (!IsVisible(program))
      {
        m_dll->ref_release(program);
        continue;
      }

      CURL url(base);
      /*
       * The base is the URL used to connect to the master server. The hostname in this may not
       * appropriate for all items as MythTV supports multiple backends (master + slaves).
       *
       * The appropriate host for playback is contained in the program information sent back from
       * the master. The same username and password are used in the URL as for the master.
       */
      url.SetHostName(GetValue(m_dll->proginfo_host(program)));

      CStdString path = CUtil::GetFileName(GetValue(m_dll->proginfo_pathname(program)));
      CStdString name = GetValue(m_dll->proginfo_title(program));

      switch (type)
      {
      case MOVIES:
        if (!IsMovie(program))
        {
          m_dll->ref_release(program);
          continue;
        }
        url.SetFileName("movies/" + path);
        break;
      case TV_SHOWS:
        if (filter != name)
        {
          m_dll->ref_release(program);
          continue;
        }
        url.SetFileName("tvshows/" + name + "/" + path);
        break;
      case ALL:
        url.SetFileName("recordings/" + path);
        break;
      }

      CFileItemPtr item(new CFileItem(url.Get(), false));
      m_session->UpdateItem(*item, program);
      /*
       * TODO: Refactor UpdateItem so it doesn't change the path of a program that is currently
       * recording if it wasn't opened through Live TV.
       */
      item->m_strPath = url.Get(); // Overwrite potentially incorrect change to path if recording

      url.SetFileName("files/" + path + ".png");
      item->SetThumbnailImage(url.Get());

      /*
       * Don't adjust the name for MOVIES as additional information in the name will affect any scraper lookup.
       */
      if (type != MOVIES)
      {
        if (m_dll->proginfo_rec_status(program) == RS_RECORDING)
        {
          name += " (Recording)";
          item->SetThumbnailImage("");
        }
        else
          name += " (" + item->m_dateTime.GetAsLocalizedDateTime() + ")";
      }

      item->SetLabel(name);
      /*
       * Set the label as preformated for MOVIES so any scraper lookup will use
       * the label rather than the filename. Don't set as preformated for other
       * filter types as this prevents the display of the title changing
       * depending on what the list is being sorted by.
       */
      if (type == MOVIES)
        item->SetLabelPreformated(true);

      items.Add(item);
      m_dll->ref_release(program);
    }
  }

  /*
   * Only add sort by name for the Movies and All Recordings directories. For TV Shows they all have
   * the same name, so only date is useful.
   */
  if (type != TV_SHOWS)
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551 /* Name */, LABEL_MASKS("%Z (%J)", "%Q", "%L", ""));
    else
      items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%Z (%J)", "%Q", "%L", ""));
  }
  items.AddSortMethod(SORT_METHOD_DATE, 552 /* Date */, LABEL_MASKS("%Z", "%J", "%L", "%J"));

  m_dll->ref_release(list);
  return true;
}

/**
 * \brief Gets a list of folders for recorded TV shows
 */
bool CCMythDirectory::GetTvShowFolders(const CStdString& base, CFileItemList &items)
{
  cmyth_conn_t control = m_session->GetControl();
  if (!control)
    return false;

  cmyth_proglist_t list = m_dll->proglist_get_all_recorded(control);
  if (!list)
  {
    CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
    return false;
  }

  int count = m_dll->proglist_get_count(list);
  for (int i = 0; i < count; i++)
  {
    cmyth_proginfo_t program = m_dll->proglist_get_item(list, i);
    if (program)
    {
      if (!IsVisible(program))
      {
        m_dll->ref_release(program);
        continue;
      }

      if (!IsTvShow(program))
      {
        m_dll->ref_release(program);
        continue;
      }

      CStdString title = GetValue(m_dll->proginfo_title(program));

      // Only add each TV show once
      if (items.Contains(base + "/" + title + "/"))
      {
        m_dll->ref_release(program);
        continue;
      }

      CFileItemPtr item(new CFileItem(base + "/" + title + "/", true));
      item->SetLabel(title);
      item->SetLabelPreformated(true);

      items.Add(item);
      m_dll->ref_release(program);
    }

  }

  // Sort by name only. Labels are preformated.
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

  m_dll->ref_release(list);
  return true;
}

bool CCMythDirectory::GetChannels(const CStdString& base, CFileItemList &items)
{
  cmyth_conn_t control = m_session->GetControl();
  if (!control)
    return false;

  vector<cmyth_proginfo_t> channels;
  for (unsigned i = 0; i < 16; i++)
  {
    cmyth_recorder_t recorder = m_dll->conn_get_recorder_from_num(control, i);
    if (!recorder)
      continue;

    cmyth_proginfo_t program;
    program = m_dll->recorder_get_cur_proginfo(recorder);
    program = m_dll->recorder_get_next_proginfo(recorder, program, BROWSE_DIRECTION_UP);
    if (!program)
    {
      m_dll->ref_release(m_recorder);
      continue;
    }

    long startchan = m_dll->proginfo_chan_id(program);
    long currchan  = -1;
    while (startchan != currchan)
    {
      unsigned j;
      for (j = 0; j < channels.size(); j++)
      {
        if (m_dll->proginfo_compare(program, channels[j]) == 0)
          break;
      }

      if (j == channels.size())
        channels.push_back(program);

      program = m_dll->recorder_get_next_proginfo(recorder, program, BROWSE_DIRECTION_UP);
      if (!program)
        break;

      currchan = m_dll->proginfo_chan_id(program);
    }
    m_dll->ref_release(recorder);
  }

  CURL url(base);
  /*
   * The content of the cmyth_proginfo_t struct retrieved and stored in channels[] above does not
   * contain the host so the URL cannot be modified to support both master and slave servers.
   */

  for (unsigned i = 0; i < channels.size(); i++)
  {
    cmyth_proginfo_t program = channels[i];
    CStdString num, progname, channame, icon, sign;

    num   = GetValue(m_dll->proginfo_chanstr (program));
    icon  = GetValue(m_dll->proginfo_chanicon(program));

    url.SetFileName("channels/" + num + ".ts");
    CFileItemPtr item(new CFileItem(url.Get(), false));
    m_session->UpdateItem(*item, program);

    item->SetLabel(GetValue(m_dll->proginfo_chansign(program)));

    if (icon.length() > 0)
    {
      url.SetFileName("files/channels/" + CUtil::GetFileName(icon));
      item->SetThumbnailImage(url.Get());
    }

    /* hack to get sorting working properly when sorting by show title */
    if (item->GetVideoInfoTag()->m_strShowTitle.IsEmpty())
      item->GetVideoInfoTag()->m_strShowTitle = " ";

    items.Add(item);
    m_dll->ref_release(program);
  }

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551 /* Name */, LABEL_MASKS("%K[ - %Z]", "%B", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%K[ - %Z]", "%B", "%L", ""));

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 20364 /* TV show */, LABEL_MASKS("%Z[ - %B]", "%K", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 20364 /* TV show */, LABEL_MASKS("%Z[ - %B]", "%K", "%L", ""));

  return true;
}

bool CCMythDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  m_session = CCMythSession::AquireSession(strPath);
  if (!m_session)
    return false;

  m_dll = m_session->GetLibrary();
  if (!m_dll)
    return false;

  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

  if (fileName == "")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel(g_localizeStrings.Get(22018)); // Live channels
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/guide/", true));
    item->SetLabel(g_localizeStrings.Get(22020)); // Guide
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/movies/", true));
    item->SetLabel(g_localizeStrings.Get(20342)); // Movies
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/recordings/", true));
    item->SetLabel(g_localizeStrings.Get(22015)); // All recordings
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/tvshows/", true));
    item->SetLabel(g_localizeStrings.Get(20343)); // TV shows
    item->SetLabelPreformated(true);
    items.Add(item);

    // Sort by name only. Labels are preformated.
    items.AddSortMethod(SORT_METHOD_LABEL, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));

    /*
     * Clear the directory cache so the cached sub-folders are guaranteed to be accurate.
     */
    g_directoryCache.ClearSubPaths(base);

    return true;
  }
  else if (fileName == "channels")
    return GetChannels(base, items);
  else if (fileName == "guide")
    return GetGuide(base, items);
  else if (fileName.Left(6) == "guide/")
    return GetGuideForChannel(base, items, atoi(fileName.Mid(6)));
  else if (fileName == "movies")
    return GetRecordings(base, items, MOVIES);
  else if (fileName == "recordings")
    return GetRecordings(base, items);
  else if (fileName == "tvshows")
    return GetTvShowFolders(base, items);
  else if (fileName.Left(8) == "tvshows/")
    return GetRecordings(base, items, TV_SHOWS, fileName.Mid(8));
  return false;
}

bool CCMythDirectory::IsVisible(const cmyth_proginfo_t program)
{
  CStdString group = GetValue(m_dll->proginfo_recgroup(program));
  /*
   * Ignore programs that were recorded using "LiveTV" or that have been deleted via the
   * "Auto Expire Instead of Delete Recording" option, which places the recording in the
   * "Deleted" recording group for x days rather than deleting straight away.
   */
  return !(group.Equals("LiveTV") || group.Equals("Deleted"));
}

bool CCMythDirectory::IsMovie(const cmyth_proginfo_t program)
{
  /*
   * The mythconverg.recordedprogram.programid field (if it exists) is a combination key where the first 2 characters map
   * to the category_type and the rest is the key. From MythTV/release-0-21-fixes/mythtv/libs/libmythtv/programinfo.cpp
   * "MV" = movie
   * "EP" = series
   * "SP" = sports
   * "SH" = tvshow
   *
   * Based on MythTV usage it appears that the programid is only filled in for Movies though. Shame, could have used
   * it for the other categories as well.
   *
   * mythconverg.recordedprogram.category_type contains the exact information that is needed. However, category_type
   * isn't available through the libcmyth API. Since there is a direct correlation between the programid starting
   * with "MV" and the category_type being "movie" that should work fine.
   */

  const int iMovieLength = g_advancedSettings.m_iMythMovieLength; // Minutes
  if (iMovieLength > 0) // Use hack to identify movie based on length (used if EPG is dubious).
    return GetValue(m_dll->proginfo_programid(program)).Left(2) == "MV"
        || m_dll->proginfo_length_sec(program) > iMovieLength * 60; // Minutes to seconds
  else
    return GetValue(m_dll->proginfo_programid(program)).Left(2) == "MV";
}

bool CCMythDirectory::IsTvShow(const cmyth_proginfo_t program)
{
  /*
   * There isn't enough information exposed by libcmyth to distinguish between an episode in a series and a
   * one off TV show. See comment in IsMovie for more information.
   *
   * Return anything that isn't a movie as per the program ID. This may result in a recording being
   * in both the Movies and TV Shows folders if the advanced setting to choose a movie based on
   * recording length is used, but means that at least all recorded TV Shows can be found in one
   * place.
   */
  return GetValue(m_dll->proginfo_programid(program)).Left(2) != "MV";
}

bool CCMythDirectory::SupportsFileOperations(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString filename = url.GetFileName();
  CUtil::RemoveSlashAtEnd(filename);
  /*
   * TV Shows directory has sub-folders so extra check is included so only files get the file
   * operations.
   */
  return filename.Left(11) == "recordings/" ||
         filename.Left(7)  == "movies/" ||
        (filename.Left(8)  == "tvshows/" && CUtil::GetExtension(filename) != "");
}

bool CCMythDirectory::IsLiveTV(const CStdString& strPath)
{
  CURL url(strPath);
  return url.GetFileName().Left(9) == "channels/";
}
