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
#include "LastFmManager.h"
#include "Album.h"
#include "Artist.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "PlayListPlayer.h"
#include "playlists/PlayListFactory.h"
#include "utils/md5.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "GUIInfoManager.h"
#include "MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "URL.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "playlists/PlayList.h"
#include "utils/Crc32.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/XBMCTinyXML.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <sstream>

using namespace std;
using namespace MUSIC_INFO;
using namespace PLAYLIST;
using namespace XFILE;


CLastFmManager* CLastFmManager::m_pInstance=NULL;

CLastFmManager::CLastFmManager() : CThread("CLastFmManager")
{
}

CLastFmManager::~CLastFmManager()
{
  StopThread();
  CLog::Log(LOGINFO,"lastfm destroyed");
}

void CLastFmManager::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance=NULL;
  }
}

CLastFmManager* CLastFmManager::GetInstance()
{
  if (!m_pInstance)
    m_pInstance=new CLastFmManager;

  return m_pInstance;
}

void CLastFmManager::Parameter(const CStdString& key, const CStdString& data, CStdString& value)
{
}

bool CLastFmManager::RadioHandShake()
{
  return false;
}

void CLastFmManager::InitProgressDialog(const CStdString& strUrl)
{
}

void CLastFmManager::UpdateProgressDialog(const int iStringID)
{
}

void CLastFmManager::CloseProgressDialog()
{
}

bool CLastFmManager::ChangeStation(const CURL& stationUrl)
{
  return false;
}

bool CLastFmManager::RequestRadioTracks()
{
  return false;
}

void CLastFmManager::CacheTrackThumb(const int nrInitialTracksToAdd)
{
}

void CLastFmManager::AddToPlaylist(const int nrTracks)
{
}


void CLastFmManager::OnSongChange(CFileItem& newSong)
{
}

void CLastFmManager::Update()
{
}

bool CLastFmManager::ReapSongs()
{
  return false;
}

bool CLastFmManager::MovePlaying()
{
  return false;
}

void CLastFmManager::SendUpdateMessage()
{
}

void CLastFmManager::OnStartup()
{
}

void CLastFmManager::Process()
{
}

void CLastFmManager::StopRadio(bool bKillSession /*= true*/)
{
}

void CLastFmManager::CreateMD5Hash(const CStdString& bufferToHash, CStdString& hash)
{
}

bool CLastFmManager::CallXmlRpc(const CStdString& action, const CStdString& artist, const CStdString& title)
{
  return false;
}

bool CLastFmManager::Love(bool askConfirmation)
{
  return false;
}

bool CLastFmManager::Ban(bool askConfirmation)
{
  return false;
}

bool CLastFmManager::Love(const CMusicInfoTag& musicinfotag)
{
  return false;
}

bool CLastFmManager::Ban(const CMusicInfoTag& musicinfotag)
{
  return false;
}

bool CLastFmManager::Unlove(const CMusicInfoTag& musicinfotag, bool askConfirmation /*= true*/)
{
  return false;
}

bool CLastFmManager::Unban(const CMusicInfoTag& musicinfotag, bool askConfirmation /*= true*/)
{
  return false;
}

bool CLastFmManager::IsLastFmEnabled()
{
  return (
    !g_guiSettings.GetString("scrobbler.lastfmusername").IsEmpty() &&
    !g_guiSettings.GetString("scrobbler.lastfmpass").IsEmpty()
  );
}

bool CLastFmManager::CanLove()
{
  return false;
}

bool CLastFmManager::CanBan()
{
  return false;
}

bool CLastFmManager::CanScrobble(const CFileItem &fileitem)
{
  return (
    (!fileitem.IsInternetStream() && g_guiSettings.GetBool("scrobbler.lastfmsubmit"))
  );
}
