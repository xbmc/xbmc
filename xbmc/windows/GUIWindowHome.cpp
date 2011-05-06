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

#include "GUIWindowHome.h"
#include "guilib/Key.h"
#include "utils/JobManager.h"
#include "utils/RecentlyAddedJob.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"

using namespace ANNOUNCEMENT;

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
{
  m_updateRA = (Audio | Video | Totals);
  
  CAnnouncementManager::AddAnnouncer(this);
}

CGUIWindowHome::~CGUIWindowHome(void)
{
  CAnnouncementManager::RemoveAnnouncer(this);
}

void CGUIWindowHome::OnInitWindow()
{  
  // for shared databases (ie mysql) always force an update on return to home
  // this is a temporary solution until remote announcements can be delivered
  if ( g_advancedSettings.m_databaseVideo.type.Equals("mysql") ||
       g_advancedSettings.m_databaseMusic.type.Equals("mysql") )
    m_updateRA = (Audio | Video | Totals);
  AddRecentlyAddedJobs( m_updateRA );

  CGUIWindow::OnInitWindow();
}

void CGUIWindowHome::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  int ra_flag = 0;

  CLog::Log(LOGDEBUG, "GOT ANNOUNCEMENT, type: %i, from %s, message %s",(int)flag, sender, message);

  if (flag & VideoLibrary)
  {
    if ((strcmp(message, "UpdateVideo") == 0) ||
        (strcmp(message, "RemoveVideo") == 0))
      ra_flag |= (Video | Totals);

    if (strcmp(message, "NewPlayCount") == 0)
      ra_flag |= Totals;
  }
  else if (flag & AudioLibrary)
  {
    if ((strcmp(message, "UpdateAudio") == 0) ||
        (strcmp(message, "RemoveAudio") == 0))
      ra_flag |= ( Audio | Totals );

    if (strcmp(message, "NewPlayCount") == 0)
      ra_flag |= Totals;
  }

  // add the job immediatedly if the home window is active
  // otherwise defer it to the next initialisation

  if (IsActive())
    AddRecentlyAddedJobs(ra_flag);
  else
    m_updateRA |= ra_flag;
}

void CGUIWindowHome::AddRecentlyAddedJobs(int flag)
{
  if (flag != 0)
    CJobManager::GetInstance().AddJob(new CRecentlyAddedJob(flag), NULL);
  m_updateRA = 0;
}

bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      if (IsActive())
        AddRecentlyAddedJobs(Video | Audio | Totals);
      else
        m_updateRA |= (Video | Audio | Totals);
      return true;
    }
    break;

  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}
