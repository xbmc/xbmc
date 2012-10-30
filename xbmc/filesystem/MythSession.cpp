/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "DllLibCMyth.h"
#include "MythSession.h"
#include "video/VideoInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "XBDateTime.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

extern "C"
{
#include "cmyth/include/cmyth/cmyth.h"
#include "cmyth/include/refmem/refmem.h"
}

using namespace XFILE;
using namespace std;

#define MYTH_DEFAULT_PORT     6543
#define MYTH_DEFAULT_USERNAME "mythtv"
#define MYTH_DEFAULT_PASSWORD "mythtv"
#define MYTH_DEFAULT_DATABASE "mythconverg"

#define MYTH_IDLE_TIMEOUT     5 * 60 // 5 minutes in seconds

CCriticalSection       CMythSession::m_section_session;
vector<CMythSession*>  CMythSession::m_sessions;

void CMythSession::CheckIdle()
{
  CSingleLock lock(m_section_session);

  vector<CMythSession*>::iterator it;
  for (it = m_sessions.begin(); it != m_sessions.end(); )
  {
    CMythSession* session = *it;
    if ((XbmcThreads::SystemClockMillis() - session->m_timestamp) > (MYTH_IDLE_TIMEOUT * 1000) )
    {
      CLog::Log(LOGINFO, "%s - closing idle connection to MythTV backend: %s", __FUNCTION__, session->m_hostname.c_str());
      delete session;
      it = m_sessions.erase(it);
    }
    else
    {
      it++;
    }
  }
}

CMythSession* CMythSession::AquireSession(const CURL& url)
{
  CSingleLock lock(m_section_session);

  vector<CMythSession*>::iterator it;
  for (it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    CMythSession* session = *it;
    if (session->CanSupport(url))
    {
      m_sessions.erase(it);
      CLog::Log(LOGDEBUG, "%s - Aquired existing MythTV session: %p", __FUNCTION__, session);
      return session;
    }
  }
  CMythSession* session = new CMythSession(url);
  CLog::Log(LOGINFO, "%s - Aquired new MythTV session for %s: %p", __FUNCTION__,
            url.GetWithoutUserDetails().c_str(), session);
  return session;
}

void CMythSession::ReleaseSession(CMythSession* session)
{
  CLog::Log(LOGDEBUG, "%s - Releasing MythTV session: %p", __FUNCTION__, session);
  session->SetListener(NULL);
  session->m_timestamp = XbmcThreads::SystemClockMillis();
  CSingleLock lock(m_section_session);
  m_sessions.push_back(session);
}

CDateTime CMythSession::GetValue(cmyth_timestamp_t t)
{
  CDateTime result;
  if (t)
  {
    time_t time = m_dll->timestamp_to_unixtime(t); // Returns NULL if error
    if (time)
      result = CTimeUtils::GetLocalTime(time);
    else
      result = CTimeUtils::GetLocalTime(0);
    m_dll->ref_release(t);
  }
  else // Return epoch so 0 and NULL behave the same.
    result = CTimeUtils::GetLocalTime(0);

  return result;
}

CStdString CMythSession::GetValue(char *str)
{
  CStdString result;
  if (str)
  {
    result = str;
    m_dll->ref_release(str);
    result.Trim();
  }
  return result;
}

void CMythSession::SetFileItemMetaData(CFileItem &item, cmyth_proginfo_t program)
{
  if (!program)
    return;

  /*
   * Set the FileItem meta-data.
   */
  CStdString title        = GetValue(m_dll->proginfo_title(program)); // e.g. Mythbusters
  CStdString subtitle     = GetValue(m_dll->proginfo_subtitle(program)); // e.g. The Pirate Special
  item.m_strTitle         = title;
  if (!subtitle.IsEmpty())
    item.m_strTitle      += " - \"" + subtitle + "\""; // e.g. Mythbusters - "The Pirate Special"
  item.m_dateTime         = GetValue(m_dll->proginfo_rec_start(program));
  item.m_dwSize           = m_dll->proginfo_length(program); // size in bytes

  /*
   * Set the VideoInfoTag meta-data so it matches the FileItem meta-data where possible.
   */
  CVideoInfoTag* tag      = item.GetVideoInfoTag();
  tag->m_strTitle         = subtitle; // The title is just supposed to be the episode title.
  tag->m_strShowTitle     = title;
  tag->m_strOriginalTitle = title;
  tag->m_strPlotOutline   = subtitle;
  tag->m_strPlot          = GetValue(m_dll->proginfo_description(program));
  /*
   * TODO: Strip out the subtitle from the description if it is present at the start? OR add the
   * subtitle to the start of the plot if not already as it used to? Seems strange, should be
   * handled by skin?
   *
  if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
    tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;
   */
  tag->m_genre            = StringUtils::Split(GetValue(m_dll->proginfo_category(program)), g_advancedSettings.m_videoItemSeparator); // e.g. Sports
  tag->m_strAlbum         = GetValue(m_dll->proginfo_chansign(program)); // e.g. TV3
  tag->m_strRuntime       = StringUtils::SecondsToTimeString(m_dll->proginfo_length_sec(program));
  
  SetSeasonAndEpisode(program, &tag->m_iSeason, &tag->m_iEpisode);

  /*
   * Original air date is used by the VideoInfoScanner to scrape the TV Show information into the
   * Video Library. If the original air date is empty the date returned will be the epoch.
   */
  CStdString originalairdate = GetValue(m_dll->proginfo_originalairdate(program)).GetAsDBDate();
  if (originalairdate != "1970-01-01"
  &&  originalairdate != "1969-12-31")
  tag->m_firstAired.SetFromDateString(originalairdate);

  /*
   * Video sort title is the raw title with the date appended on the end in a sortable format so
   * when the "All Recordings" listing is sorted by "Name" rather than "Date", all of the episodes
   * for a given show are still presented in date order (even though some may have a subtitle that
   * would cause it to be shown in a different position if it was indeed strictly sorting by
   * what is displayed in the list).
   */
  tag->m_strSortTitle = title + " " + item.m_dateTime.GetAsDBDateTime(); // e.g. Mythbusters 2009-12-13 12:23:14

  /*
   * Set further FileItem and VideoInfoTag meta-data based on whether it is LiveTV or not.
   */
  CURL url(item.GetPath());
  if (url.GetFileName().Left(9) == "channels/")
  {
    /*
     * Prepend the channel number onto the FileItem title for the listing so it's clear what is
     * playing on each channel without using up as much room as the channel name.
     */
    CStdString number = GetValue(m_dll->proginfo_chanstr(program));
    item.m_strTitle = number + " - " + item.m_strTitle;

    /*
     * Append the channel name onto the end of the tag title for the OSD so it's clear what LiveTV
     * channel is currently being watched to give some context for Next or Previous channel. Added
     * to the end so sorting by title will work, and it's not really as important as the title
     * within the OSD.
     */
    CStdString name = GetValue(m_dll->proginfo_chansign(program));
    if (!name.IsEmpty())
      tag->m_strTitle += " - " + name;

    /*
     * Set the sort title to be the channel number.
     */
    tag->m_strSortTitle = number;

    /*
     * Set the status so XBMC treats the content as LiveTV.
     */
    tag->m_strStatus = "livetv";

    /*
     * Update the path and channel icon for LiveTV in case the channel has changed through
     * NextChannel(), PreviousChannel() or SetChannel().
     */
    if (!number.IsEmpty())
    {
      url.SetFileName("channels/" + number + ".ts"); // e.g. channels/3.ts
      item.SetPath(url.Get());
    }
    CStdString chanicon = GetValue(m_dll->proginfo_chanicon(program));
    if (!chanicon.IsEmpty())
    {
      url.SetFileName("files/channels/" + URIUtils::GetFileName(chanicon)); // e.g. files/channels/tv3.jpg
      item.SetArt("thumb", url.Get());
    }
  }
  else
  {
    /*
     * MythTV thumbnails aren't generated until a program has finished recording.
     */
    if (m_dll->proginfo_rec_status(program) == RS_RECORDED)
    {
      url.SetFileName("files/" + URIUtils::GetFileName(GetValue(m_dll->proginfo_pathname(program))) + ".png");
      item.SetArt("thumb", url.Get());
    }
  }
}

void CMythSession::SetSeasonAndEpisode(const cmyth_proginfo_t &program, int *season, int *episode) {
  /*
   * A valid programid generated from an XMLTV source should look like:
   * [EP|MV|SH|SP][seriesid][episode][season]([partnumber][parttotal])
   * mythtv/trunk/programs/mytfilldatabaseline/xmltvparser.cpp - Line 522 onwards.
   * 
   * Season changed to a base36 character for XMLTV in Myth 0.24. http://svn.mythtv.org/trac/changeset/24724
   * 
   * A valid SchedulesDirect programid appears to have a similar format to the XMLTV programid but
   * doesn't have any obvious way to parse out the season and episode information. The number at the
   * end of the programid could possibly be the completely sequential number for the episode, but
   * even that doesn't seem to match up with TVDB. SchedulesDirect data does seem to have a valid
   * original air date though, so if we identify a SchedulesDirect programid, leave the season and
   * episode as 0. 
   */
  CStdString programid = GetValue(m_dll->proginfo_programid(program));
  CStdString seriesid = GetValue(m_dll->proginfo_seriesid(program));

  /*
   * Default the season and episode to 0 so XBMC treats the content as an episode and displays tag
   * information. If the season and episode can be parsed from the programid these will be
   * overwritten.
   */
  *season  = 0;
  *episode = 0;
  
  if (programid.IsEmpty() // Can't do anything if the program ID is empty
  ||  seriesid.IsEmpty()) // Can't figure out the end parsing if the series ID is empty  {
    return;
  
  CStdString category = programid.Left(2); // Valid for both XMLTV and SchedulesDirect sources
  if (category != "MV"  // Movie
  &&  category != "EP"  // Series
  &&  category != "SH"  // TV Show
  &&  category != "SP") // Sports
    return;
  
  if (programid.Mid(category.length(), seriesid.length()) != seriesid) // Series ID does not follow the category
    return;
  
  CStdString remainder = programid.Mid(category.length() + seriesid.length()); // Whatever is after series ID
  
  /*
   * All SchedulesDirect remainders appear to be 4 characters and start with a 0. If the assumption
   * is correct that the number somehow relates to the sequential episode number across all seasons
   * then we can ignore remainders that start with 0. It will be very unlikely for a sequential
   * episode number for a series to be > 999.
   */
  if (remainder.length() == 4     // All SchedulesDirect codes seem to be 4 characters
  &&  remainder.Left(0)  == "0")  // Padded with 0's for low number. No valid XMLTV remainder will start with 0.
    return;
  
  /*
   * If the remainder is more than 5 characters, it must include the optional part number and total
   * number of parts. Strip off the last 2 characters assuming that there are ridiculously few
   * cases where the number of parts for a single episode is > 9.
   */
  if (remainder.length() >= 5) // Must include optional part number and total number of parts
    remainder = remainder.Left(remainder.length() - 2); // Assumes part number and total are both < 10
  
  /*
   * Now for some heuristic black magic.
   */
  if (remainder.length() == 2)  // Single character season and episode.
  {
    *season = atoi(remainder.Right(1)); // TODO: Fix for base 36 in Myth 0.24. Assume season < 10
    *episode = atoi(remainder.Left(1));
  }
  else if (remainder.length() == 3) // Ambiguous in Myth 0.23. Single character season in Myth 0.24
  {
    /*
     * Following heuristics are intended to work with largest possible number of cases. It won't be
     * perfect, but way better than just assuming the season is < 10.
     */
    if (remainder.Right(1) == "0") // e.g. 610. Unlikely to have a season of 0 (specials) with more than 9 special episodes.
    {
      *season = atoi(remainder.Right(2));
      *episode = atoi(remainder.Left(1));
    }
    else if (remainder.Mid(2, 1) == "0") // e.g. 203. Can't have a season start with 0. Must be end of episode.
    {
      *season = atoi(remainder.Right(1)); // TODO: Fix for base 36 in Myth 0.24. Assume season < 10
      *episode = atoi(remainder.Left(2));
    }
    else if (atoi(remainder.Left(1)) > 3) // e.g. 412. Very unlikely to have more than 39 episodes per season if season > 9.
    {
      /*
       * TODO: See if a check for > 2 is better, e.g. is it still unlike to have more than 29 episodes
       * per season if season > 9?
       */
      *season = atoi(remainder.Right(2));
      *episode = atoi(remainder.Left(1));
    }
    else // e.g. 129. Assume season is < 10 or Myth 0.24 Base 36 season.
    {
      *season = atoi(remainder.Right(1)); // TODO: Fix for base 36 in Myth 0.24. Assume season < 10
      *episode = atoi(remainder.Left(2));
    }
  }
  else if (remainder.length() == 4) // Double digit season and episode in Myth 0.23 OR TODO: has part number and total number of parts
  {
    *season = atoi(remainder.Right(2));
    *episode = atoi(remainder.Left(2));
  }
  return;
}

CMythSession::CMythSession(const CURL& url) : CThread("CMythSession")
{
  m_control   = NULL;
  m_event     = NULL;
  m_database  = NULL;
  m_hostname  = url.GetHostName();
  m_username  = url.GetUserName() == "" ? MYTH_DEFAULT_USERNAME : url.GetUserName();
  m_password  = url.GetPassWord() == "" ? MYTH_DEFAULT_PASSWORD : url.GetPassWord();
  m_port      = url.HasPort() ? url.GetPort() : MYTH_DEFAULT_PORT;
  m_timestamp = XbmcThreads::SystemClockMillis();
  m_dll = new DllLibCMyth;
  m_dll->Load();
  if (m_dll->IsLoaded())
  {
    m_dll->set_dbg_msgcallback(&CMythSession::LogCMyth);
    if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
      m_dll->dbg_level(CMYTH_DBG_ALL);
    else if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG)
      m_dll->dbg_level(CMYTH_DBG_DETAIL);
    else
      m_dll->dbg_level(CMYTH_DBG_ERROR);
  }
  m_all_recorded = NULL;
}

CMythSession::~CMythSession()
{
  Disconnect();
  delete m_dll;
}

bool CMythSession::CanSupport(const CURL& url)
{
  if (m_hostname != url.GetHostName())
    return false;

  if (m_port != (url.HasPort() ? url.GetPort() : MYTH_DEFAULT_PORT))
    return false;

  if (m_username != (url.GetUserName() == "" ? MYTH_DEFAULT_USERNAME : url.GetUserName()))
    return false;

  if (m_password != (url.GetPassWord() == "" ? MYTH_DEFAULT_PASSWORD : url.GetPassWord()))
    return false;

  return true;
}

void CMythSession::Process()
{
  char buf[128];
  int  next;

  struct timeval to;

  while (!m_bStop)
  {
    /* check if there are any new events */
    to.tv_sec = 0;
    to.tv_usec = 100000;
    if (m_dll->event_select(m_event, &to) <= 0)
      continue;

    next = m_dll->event_get(m_event, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;

    switch (next)
    {
    case CMYTH_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - MythTV event UNKNOWN (error?)", __FUNCTION__);
      break;
    case CMYTH_EVENT_CLOSE:
      CLog::Log(LOGDEBUG, "%s - MythTV event EVENT_CLOSE", __FUNCTION__);
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE", __FUNCTION__);
      ResetAllRecordedPrograms();
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE_ADD: %s", __FUNCTION__, buf);
      ResetAllRecordedPrograms();
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE_UPDATE", __FUNCTION__);
      ResetAllRecordedPrograms();
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE_DELETE: %s", __FUNCTION__, buf);
      ResetAllRecordedPrograms();
      break;
    case CMYTH_EVENT_SCHEDULE_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event SCHEDULE_CHANGE", __FUNCTION__);
      break;
    case CMYTH_EVENT_DONE_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event DONE_RECORDING", __FUNCTION__);
      break;
    case CMYTH_EVENT_QUIT_LIVETV:
      CLog::Log(LOGDEBUG, "%s - MythTV event QUIT_LIVETV", __FUNCTION__);
      break;
    case CMYTH_EVENT_WATCH_LIVETV:
      CLog::Log(LOGDEBUG, "%s - MythTV event LIVETV_WATCH", __FUNCTION__);
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      CLog::Log(LOGDEBUG, "%s - MythTV event LIVETV_CHAIN_UPDATE: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_SIGNAL:
      CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
      break;
    case CMYTH_EVENT_ASK_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event ASK_RECORDING", __FUNCTION__);
      break;
    case CMYTH_EVENT_SYSTEM_EVENT:
      CLog::Log(LOGDEBUG, "%s - MythTV event SYSTEM_EVENT: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_UPDATE_FILE_SIZE:
      CLog::Log(LOGDEBUG, "%s - MythTV event UPDATE_FILE_SIZE: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_GENERATED_PIXMAP:
      CLog::Log(LOGDEBUG, "%s - MythTV event GENERATED_PIXMAP: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_CLEAR_SETTINGS_CACHE:
      CLog::Log(LOGDEBUG, "%s - MythTV event CLEAR_SETTINGS_CACHE", __FUNCTION__);
      break;
    }

    {
      CSingleLock lock(m_section);
      if (m_listener)
        m_listener->OnEvent(next, buf);
    }
  }
}

void CMythSession::Disconnect()
{
  if (!m_dll || !m_dll->IsLoaded())
    return;

  StopThread();

  if (m_control)
    m_dll->ref_release(m_control);
  if (m_event)
    m_dll->ref_release(m_event);
  if (m_database)
    m_dll->ref_release(m_database);
  if (m_all_recorded)
    m_dll->ref_release(m_all_recorded);
}

cmyth_conn_t CMythSession::GetControl()
{
  if (!m_control)
  {
    if (!m_dll->IsLoaded())
      return NULL;

    m_control = m_dll->conn_connect_ctrl((char*)m_hostname.c_str(), m_port, 16*1024, 4096);
    if (!m_control)
      CLog::Log(LOGERROR, "%s - unable to connect to server on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_control;
}

cmyth_database_t CMythSession::GetDatabase()
{
  if (!m_database)
  {
    if (!m_dll->IsLoaded())
      return NULL;

    m_database = m_dll->database_init((char*)m_hostname.c_str(), (char*)MYTH_DEFAULT_DATABASE,
                                      (char*)m_username.c_str(), (char*)m_password.c_str());
    if (!m_database)
      CLog::Log(LOGERROR, "%s - unable to connect to database on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_database;
}

bool CMythSession::SetListener(IEventListener *listener)
{
  if (!m_event && listener)
  {
    if (!m_dll->IsLoaded())
      return false;

    m_event = m_dll->conn_connect_event((char*)m_hostname.c_str(), m_port, 16*1024 , 4096);
    if (!m_event)
    {
      CLog::Log(LOGERROR, "%s - unable to connect to server on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
      return false;
    }
    /* start event handler thread */
    CThread::Create(false, THREAD_MINSTACKSIZE);
  }
  CSingleLock lock(m_section);
  m_listener = listener;
  return true;
}

DllLibCMyth* CMythSession::GetLibrary()
{
  if (m_dll->IsLoaded())
    return m_dll;
  return NULL;
}

/*
 * The caller must call m_dll->ref_release() when finished.
 */
cmyth_proglist_t CMythSession::GetAllRecordedPrograms()
{
  CSingleLock lock(m_section);
  if (!m_all_recorded)
  {
    if (m_all_recorded)
    {
      m_dll->ref_release(m_all_recorded);
      m_all_recorded = NULL;
    }
    cmyth_conn_t control = GetControl();
    if (!control)
      return NULL;

    m_all_recorded = m_dll->proglist_get_all_recorded(control);
  }
  /*
   * An extra reference is needed to prevent a race condition while resetting the proglist from
   * the Process() thread while it is being read.
   */
  m_dll->ref_hold(m_all_recorded);

  return m_all_recorded;
}

void CMythSession::ResetAllRecordedPrograms()
{
  CSingleLock lock(m_section);
  if (m_all_recorded)
  {
    m_dll->ref_release(m_all_recorded);
    m_all_recorded = NULL;
  }
  return;
}

void CMythSession::LogCMyth(int level, char *msg)
{
  int xbmc_lvl = -1;
  switch (level)
  {
    case CMYTH_DBG_NONE:  break;
    case CMYTH_DBG_ERROR: xbmc_lvl = LOGERROR;   break;
    case CMYTH_DBG_WARN:  xbmc_lvl = LOGWARNING; break;
    case CMYTH_DBG_INFO:  xbmc_lvl = LOGINFO;    break;
    default:              xbmc_lvl = LOGDEBUG;   break;
  }
  if (xbmc_lvl >= 0) {
    CLog::Log(xbmc_lvl, "%s", msg);
  }
}
