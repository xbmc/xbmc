/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
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

#include "lastfmscrobbler.h"
#include "threads/Atomics.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"

long CLastfmScrobbler::m_instanceLock = 0;
CLastfmScrobbler *CLastfmScrobbler::m_pInstance = NULL;

CLastfmScrobbler::CLastfmScrobbler()
  : CScrobbler(LASTFM_SCROBBLER_HANDSHAKE_URL, LASTFM_SCROBBLER_LOG_PREFIX)
{
}

CLastfmScrobbler::~CLastfmScrobbler()
{
  Term();
}

CLastfmScrobbler *CLastfmScrobbler::GetInstance()
{
  if (!m_pInstance) // Avoid spinning aimlessly
  {
    CAtomicSpinLock lock(m_instanceLock);
    if (!m_pInstance)
    {
      m_pInstance = new CLastfmScrobbler;
    }
  }
  return m_pInstance;
}

void CLastfmScrobbler::RemoveInstance()
{
  if (m_pInstance)
  {
    CAtomicSpinLock lock(m_instanceLock);
    delete m_pInstance;
    m_pInstance = NULL;
  }
}

void CLastfmScrobbler::LoadCredentials()
{
  SetUsername(g_guiSettings.GetString("scrobbler.lastfmusername"));
  SetPassword(g_guiSettings.GetString("scrobbler.lastfmpass"));
}

CStdString CLastfmScrobbler::GetJournalFileName()
{
  CStdString strFileName = g_settings.GetProfileUserDataFolder();
  return URIUtils::AddFileToFolder(strFileName, "LastfmScrobbler.xml");
}

void CLastfmScrobbler::NotifyUser(int error)
{
  CStdString strText;
  CStdString strAudioScrobbler;
  switch (error)
  {
    case SCROBBLER_USER_ERROR_BADAUTH:
      strText = g_localizeStrings.Get(15206);
      m_bBadAuth = true;
      strAudioScrobbler = g_localizeStrings.Get(15200);  // AudioScrobbler
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, strAudioScrobbler, strText, 10000);
      break;
    case SCROBBLER_USER_ERROR_BANNED:
      strText = g_localizeStrings.Get(15205);
      m_bBanned = true;
      strAudioScrobbler = g_localizeStrings.Get(15200);  // AudioScrobbler
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, strAudioScrobbler, strText, 10000);
      break;
    default:
      break;
  }
}

bool CLastfmScrobbler::CanScrobble()
{
  return (!g_guiSettings.GetString("scrobbler.lastfmusername").IsEmpty()  &&
          !g_guiSettings.GetString("scrobbler.lastfmpass").IsEmpty()  &&
         (g_guiSettings.GetBool("scrobbler.lastfmsubmit") ||
          g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")));
}

