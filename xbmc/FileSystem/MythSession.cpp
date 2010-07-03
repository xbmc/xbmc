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

#include "DllLibCMyth.h"
#include "MythSession.h"
#include "VideoInfoTag.h"
#include "AdvancedSettings.h"
#include "DateTime.h"
#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "StringUtils.h"
#include "utils/SingleLock.h"
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

CCriticalSection       CMythSession::m_section_session;
vector<CMythSession*>  CMythSession::m_sessions;

void CMythSession::CheckIdle()
{
  CSingleLock lock(m_section_session);

  vector<CMythSession*>::iterator it;
  for (it = m_sessions.begin(); it != m_sessions.end(); )
  {
    CMythSession* session = *it;
    if (session->m_timestamp + 5000 < CTimeUtils::GetTimeMS())
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
      return session;
    }
  }
  return new CMythSession(url);
}

void CMythSession::ReleaseSession(CMythSession* session)
{
  session->SetListener(NULL);
  session->m_timestamp = CTimeUtils::GetTimeMS();
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
    m_dll->ref_release(t);
  }
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
  tag->m_strTitle         = item.m_strTitle; // With subtitle included, if present, as above. Shown in the OSD
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
  tag->m_strGenre         = GetValue(m_dll->proginfo_category(program)); // e.g. Sports
  tag->m_strAlbum         = GetValue(m_dll->proginfo_chansign(program)); // e.g. TV3
  tag->m_strRuntime       = StringUtils::SecondsToTimeString(m_dll->proginfo_length_sec(program));
  tag->m_iSeason          = 0; // So XBMC treats the content as an episode and displays tag information.
  tag->m_iEpisode         = 0;

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
  CURL url(item.m_strPath);
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
      item.m_strPath = url.Get();
    }
    CStdString chanicon = GetValue(m_dll->proginfo_chanicon(program));
    if (!chanicon.IsEmpty())
    {
      url.SetFileName("files/channels/" + CUtil::GetFileName(chanicon)); // e.g. files/channels/tv3.jpg
      item.SetThumbnailImage(url.Get());
    }
  }
  else
  {
    /*
     * MythTV thumbnails aren't generated until a program has finished recording.
     */
    if (m_dll->proginfo_rec_status(program) == RS_RECORDED)
    {
      url.SetFileName("files/" + CUtil::GetFileName(GetValue(m_dll->proginfo_pathname(program))) + ".png");
      item.SetThumbnailImage(url.Get());
    }
  }
}

CMythSession::CMythSession(const CURL& url)
{
  m_control   = NULL;
  m_event     = NULL;
  m_database  = NULL;
  m_hostname  = url.GetHostName();
  m_username  = url.GetUserName() == "" ? MYTH_DEFAULT_USERNAME : url.GetUserName();
  m_password  = url.GetPassWord() == "" ? MYTH_DEFAULT_PASSWORD : url.GetPassWord();
  m_port      = url.HasPort() ? url.GetPort() : MYTH_DEFAULT_PORT;
  m_timestamp = CTimeUtils::GetTimeMS();
  m_dll = new DllLibCMyth;
  m_dll->Load();
  if (m_dll->IsLoaded())
  {
    if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
      m_dll->dbg_level(CMYTH_DBG_ALL);
    else if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG)
      m_dll->dbg_level(CMYTH_DBG_DETAIL);
    else
      m_dll->dbg_level(CMYTH_DBG_ERROR);
  }
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
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      CLog::Log(LOGDEBUG, "%s - MythTV event LIVETV_CHAIN_UPDATE: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_SIGNAL:
      CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
      break;
    case CMYTH_EVENT_ASK_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event ASK_RECORDING", __FUNCTION__);
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
}

cmyth_conn_t CMythSession::GetControl()
{
  if (!m_control)
  {
    if (!m_dll->IsLoaded())
      return false;

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
      return false;

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
