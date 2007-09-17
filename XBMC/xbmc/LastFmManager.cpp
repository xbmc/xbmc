/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "LastFmManager.h"
#include "ApplicationRenderer.h"
#include "playlistplayer.h"
#include "util.h"
#include "playlistfactory.h"
#include "Picture.h"
#include "utils/http.h"
#include "utils/md5.h"
#include "FileSystem/File.h"
#include <sstream>

using namespace PLAYLIST;
using namespace XFILE;

#define XBMC_LASTFM_VERSION "0.2"
#ifdef _DEBUG
  #define XBMC_LASTFM_ID  "tst"
#else
  #define XBMC_LASTFM_ID  "xbmc"
#endif
#define XBMC_LASTFM_MINTRACKS 3

CLastFmManager* CLastFmManager::m_pInstance=NULL;

CLastFmManager::CLastFmManager()
{
  m_hWorkerEvent = CreateEvent(NULL, false, false, NULL);
}

CLastFmManager::~CLastFmManager()
{
  StopRadio(true);
  CloseHandle(m_hWorkerEvent);
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
  value = "";
  vector<CStdString> params;
  int iNumItems = StringUtils::SplitString(data, "\n", params);
  for (int i = 0; i < (int)params.size(); i++)
  {
    CStdString tmp = params[i];
    if (int pos = tmp.Find(key) >= 0)
    {
      tmp.Delete(pos - 1, key.GetLength() + 1);
      value = tmp;
      break;
    }
  }
  CLog::Log(LOGDEBUG, "Parameter %s -> %s", key.c_str(), value.c_str());
}

bool CLastFmManager::RadioHandShake()
{
  if (!m_RadioSession.IsEmpty()) return true; //already signed in

  if (dlgProgress)
  {
    dlgProgress->SetLine(2, 15251);//Connecting to Last.fm..
    dlgProgress->Progress();
  }

  m_RadioSession = "";

  CHTTP http;
  CStdString html;

  CStdString strPassword = g_guiSettings.GetString("lastfm.password");
  CStdString strUserName = g_guiSettings.GetString("lastfm.username");
  if (strUserName.IsEmpty() || strPassword.IsEmpty())
  {
    CLog::Log(LOGERROR, "Last.fm stream selected but no username or password set.");
    return false;
  }
  MD5_CTX md5state;
  unsigned char md5pword[16];
  MD5Init(&md5state);
  MD5Update(&md5state, (unsigned char *)strPassword.c_str(), (int)strPassword.size());
  MD5Final(md5pword, &md5state);
  char tmp[33];
  strncpy(tmp, "\0", sizeof(tmp));
  for (int j = 0;j < 16;j++) 
  {
    char a[3];
    sprintf(a, "%02x", md5pword[j]);
    tmp[2*j] = a[0];
    tmp[2*j+1] = a[1];
  }
  CStdString passwordmd5 = tmp;

  CStdString url;
  CUtil::URLEncode(strUserName);
  url.Format("http://ws.audioscrobbler.com/radio/handshake.php?version=%s&platform=%s&username=%s&passwordmd5=%s&debug=%i&partner=%s", XBMC_LASTFM_VERSION, XBMC_LASTFM_ID, strUserName, passwordmd5, 0, "");
  if (!http.Get(url, html))
  {
    CLog::Log(LOGERROR, "Connect to Last.fm radio failed.");
    return false;
  }
  //CLog::DebugLog("Handshake: %s", html.c_str());

  Parameter("session",    html, m_RadioSession);
  Parameter("base_url",   html, m_RadioBaseUrl);
  Parameter("base_path",  html, m_RadioBasePath);
  Parameter("subscriber", html, m_RadioSubscriber);
  Parameter("banned",     html, m_RadioBanned);

  if (m_RadioSession == "failed")
  {
    CLog::Log(LOGERROR, "Last.fm return failed response, possible bad username or password?");
    m_RadioSession = "";
  }
  return !m_RadioSession.IsEmpty();
}

void CLastFmManager::InitProgressDialog(const CStdString& strUrl)
{
  if (m_RadioSession.IsEmpty())
  {
    dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->SetHeading("Last.fm");
      dlgProgress->SetLine(0, 259);
      CStdString strUrlDec = strUrl;
      CUtil::UrlDecode(strUrlDec);
      dlgProgress->SetLine(1, strUrlDec);
      dlgProgress->SetLine(2, "");
      if (!dlgProgress->IsDialogRunning())
        dlgProgress->StartModal();
    }
  }
  else
  {
    g_ApplicationRenderer.SetBusy(true);
  }
}

void CLastFmManager::UpdateProgressDialog(const int iStringID)
{
  if (dlgProgress)
  {
    dlgProgress->SetLine(2, iStringID);
    dlgProgress->Progress();
  }
}

void CLastFmManager::CloseProgressDialog()
{
  if (dlgProgress)
  {
    dlgProgress->Close();
    dlgProgress = NULL;
  }
  else
  {
    g_ApplicationRenderer.SetBusy(false);
  }
}

bool CLastFmManager::ChangeStation(const CURL& stationUrl)
{
  DWORD start = timeGetTime();

  CStdString strUrl;
  stationUrl.GetURL(strUrl);

  InitProgressDialog(strUrl);

  StopRadio(false);
  if (!RadioHandShake())
  {
    CloseProgressDialog();
    return false;
  }

  UpdateProgressDialog(15252); // Selecting station...

  CHTTP http;
  CStdString url;
  CStdString html;
  url.Format("http://" + m_RadioBaseUrl + m_RadioBasePath + "/adjust.php?session=%s&url=%s&debug=%i", m_RadioSession, strUrl, 0);
  if (!http.Get(url, html)) 
  {
    CLog::Log(LOGERROR, "Connect to Last.fm to change station failed.");
    CloseProgressDialog();
    return false;
  }
  //CLog::DebugLog("ChangeStation: %s", html.c_str());

  CStdString strErrorCode;
  Parameter("error", html,  strErrorCode);
  if (strErrorCode != "")
  {
    CLog::Log(LOGERROR, "Last.fm returned an error (%s) response for change station request.", strErrorCode.c_str());
    CloseProgressDialog();
    return false;
  }

  UpdateProgressDialog(261); //Waiting for start....

  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, false);
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);
  RequestRadioTracks();
  CacheTrackThumb(XBMC_LASTFM_MINTRACKS);
  AddToPlaylist(XBMC_LASTFM_MINTRACKS);
  Create(); //start thread
  SetEvent(m_hWorkerEvent); //kickstart the thread

  CSingleLock lock(m_lockPlaylist);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if ((int)playlist.size())
  {
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play(0);
    CLog::Log(LOGDEBUG, "%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - start));
    CloseProgressDialog();
    return true;
  }
  CloseProgressDialog();
  return false;
}

bool CLastFmManager::RequestRadioTracks()
{
  DWORD start = timeGetTime();
  CStdString url;
  CStdString html;
  url.Format("http://" + m_RadioBaseUrl + m_RadioBasePath + "/xspf.php?sk=%s&discovery=0&desktop=", m_RadioSession);
  {
    CHTTP http;
    if (!http.Get(url, html)) 
    {
      m_RadioSession.empty();
      CLog::Log(LOGERROR, "LastFmManager: Connect to Last.fm to request tracks failed.");
      return false;
    }
  }
  //CLog::DebugLog("RequestRadioTracks: %s", html.c_str());


  //parse playlist
  TiXmlDocument xmlDoc;

  xmlDoc.Parse(html);
  if (xmlDoc.Error())
  {
    m_RadioSession.empty();
    CLog::Log(LOGERROR, "LastFmManager: Unable to parse tracklist Error: %s", xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement )
  {
    CLog::Log(LOGWARNING, "LastFmManager: No more tracks received");
    m_RadioSession.empty();
    return false;
  }

  TiXmlElement* pBodyElement = pRootElement->FirstChildElement("trackList");
  if (!pBodyElement )
  {
    CLog::Log(LOGWARNING, "LastFmManager: No more tracks received, no tracklist");
    m_RadioSession.empty();
    return false;
  }

  TiXmlElement* pTrackElement = pBodyElement->FirstChildElement("track");

  if (!pTrackElement)
  {
    CLog::Log(LOGWARNING, "LastFmManager: No more tracks received, empty tracklist");
    m_RadioSession.empty();
    return false;
  }
  while (pTrackElement)
  {
    CMusicInfoTag tag;

    CPlayList::CPlayListItem newItem("","");

    TiXmlElement* pElement = pTrackElement->FirstChildElement("location");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        CStdString url = child->Value();
        url.Replace("http:", "lastfm:");
        newItem.SetFileName(url);
      }
    }
    pElement = pTrackElement->FirstChildElement("title");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        newItem.SetDescription(child->Value());
        tag.SetTitle(child->Value());
      }
    }
    pElement = pTrackElement->FirstChildElement("creator");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        tag.SetArtist(child->Value());
      }
    }
    pElement = pTrackElement->FirstChildElement("album");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        tag.SetAlbum(child->Value());
      }
    }
   
    pElement = pTrackElement->FirstChildElement("duration");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        int iDuration = atoi(child->Value())/1000;
        newItem.SetDuration(iDuration);
        tag.SetDuration(iDuration);
      }
    }
    newItem.FillInDefaultIcon();
    pElement = pTrackElement->FirstChildElement("image");
    if (pElement)
    {
      TiXmlNode* child = pElement->FirstChild();
      if (child)
      {
        CStdString coverUrl = child->Value();
        if ((coverUrl != "") && (coverUrl.Find("noimage") == -1) && (coverUrl.Right(1) != "/"))
        {
          newItem.SetThumbnailImage(coverUrl);
        }
      }
    }
    newItem.SetMusicTag(tag);

    {
      CSingleLock lock(m_lockCache);
      m_RadioTrackQueue.Add(newItem);
    }
    pTrackElement = pTrackElement->NextSiblingElement();
  }
  //end parse
  CSingleLock lock(m_lockCache);
  int iNrCachedTracks = m_RadioTrackQueue.size();
  CLog::Log(LOGDEBUG, "%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - start));
  return iNrCachedTracks > 0;
}

void CLastFmManager::CacheTrackThumb(const int nrInitialTracksToAdd)
{
  DWORD start = timeGetTime();
  CSingleLock lock(m_lockCache);
  int iNrCachedTracks = m_RadioTrackQueue.size();
  CPicture pic;
  CHTTP http;
  for (int i = 0; i < nrInitialTracksToAdd && i < iNrCachedTracks; i++)
  {
    CPlayList::CPlayListItem& item = m_RadioTrackQueue[i];
    if (!item.GetMusicInfoTag()->Loaded())
    {
      //cache albumthumb, GetThumbnailImage contains the url to cache
      if (item.HasThumbnail())
      {
        CStdString coverUrl = item.GetThumbnailImage();
        CStdString crcFile;
        CStdString cachedFile;
        CStdString thumbFile;

        Crc32 crc;
        crc.ComputeFromLowerCase(coverUrl);
        crcFile.Format("%08x.tbn", crc);
        CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, crcFile, cachedFile);
        CUtil::AddFileToFolder(g_settings.GetLastFMThumbFolder(), crcFile, thumbFile);
        item.SetThumbnailImage("");
        try
        {
          //download to temp, then make a thumb
          if (CFile::Exists(thumbFile) || (http.Download(coverUrl, cachedFile) && pic.DoCreateThumbnail(cachedFile, thumbFile)))
          {
            if (CFile::Exists(cachedFile)) CFile::Delete(cachedFile);
            item.SetThumbnailImage(thumbFile);
          }
        }
        catch(...)
        {
          CLog::Log(LOGERROR, "LastFmManager: exception while caching %s to %s.", coverUrl, thumbFile);
        }
      }
      if (!item.HasThumbnail())
      {
        item.SetThumbnailImage("defaultAlbumCover.png");
      }
      item.GetMusicInfoTag()->SetLoaded();
    }
  }
  CLog::Log(LOGDEBUG, "%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - start));
}

void CLastFmManager::AddToPlaylist(const int nrTracks)
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  for (int i = 0; i < nrTracks; i++)
  {
    int iNrCachedTracks = m_RadioTrackQueue.size();
    if (iNrCachedTracks > 0)
    {
      CPlayList::CPlayListItem item = m_RadioTrackQueue[0];
      if (item.GetMusicInfoTag()->Loaded())
      {
        CSingleLock lock(m_lockCache);
        m_RadioTrackQueue.Remove(0);
        CSingleLock lock2(m_lockPlaylist);
        playlist.Add(item);
      }
      else 
      {
        break;
      }
    }
  }
}


void CLastFmManager::OnSongChange(bool bNewSongIsLastFm)
{
  if (m_RadioSession.IsEmpty())
    return;
  if (!bNewSongIsLastFm)
  {
    StopRadio(true);
    return;
  }
  
  DWORD start = timeGetTime();
  ReapSongs();
  MovePlaying();
  Update();
  SendUpdateMessage();
  
  CLog::Log(LOGDEBUG, "%s: Done (time: %i ms)", __FUNCTION__, (int)(timeGetTime() - start));
}

void CLastFmManager::Update()
{
  int iNrTracks = 0;
  {
    CSingleLock lock(m_lockPlaylist);
    CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
    iNrTracks = playlist.size();
  }
  if (iNrTracks < XBMC_LASTFM_MINTRACKS)
  {
    AddToPlaylist(XBMC_LASTFM_MINTRACKS - iNrTracks);
    int iNrCachedTracks = 0;
    {
      CSingleLock lock(m_lockCache);
      iNrCachedTracks = m_RadioTrackQueue.size();
    }
    if (iNrCachedTracks == 0)
    {
      //get more tracks
      if (ThreadHandle() != NULL)
      {
        SetEvent(m_hWorkerEvent);
      }
    }
  }
}

bool CLastFmManager::ReapSongs()
{
  CSingleLock lock(m_lockPlaylist);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);

  // reap any played songs
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  int i=0;
  while (i < playlist.size())
  {
    if (i < iCurrentSong)
    {
      playlist.Remove(i);
      iCurrentSong--;
    }
    else
      i++;
  }

  g_playlistPlayer.SetCurrentSong(iCurrentSong);
  return true;
}

bool CLastFmManager::MovePlaying()
{
  CSingleLock lock(m_lockPlaylist);
  // move current song to the top if its not there
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  if (iCurrentSong > 0)
  {
    CLog::Log(LOGINFO,"LastFmManager: Moving currently playing song from %i to 0", iCurrentSong);
    CPlayList &playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
    CPlayList playlistTemp;
    playlistTemp.Add(playlist[iCurrentSong]);
    playlist.Remove(iCurrentSong);
    for (int i=0; i<playlist.size(); i++)
      playlistTemp.Add(playlist[i]);
    playlist.Clear();
    for (int i=0; i<playlistTemp.size(); i++)
      playlist.Add(playlistTemp[i]);
  }
  g_playlistPlayer.SetCurrentSong(0);
  return true;
}

void CLastFmManager::SendUpdateMessage()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
}

void CLastFmManager::OnStartup()
{
  SetPriority(THREAD_PRIORITY_NORMAL);
}

void CLastFmManager::Process()
{
  while (!m_bStop)
  {
    WaitForSingleObject(m_hWorkerEvent, INFINITE);
    if (m_bStop)
      break;
    int iNrCachedTracks = m_RadioTrackQueue.size();
    if (iNrCachedTracks == 0)
    {
      RequestRadioTracks();
    }
    CSingleLock lock(m_lockCache);
    iNrCachedTracks = m_RadioTrackQueue.size();
    CacheTrackThumb(iNrCachedTracks);
  }
  CLog::Log(LOGINFO,"LastFM thread terminated");
}

void CLastFmManager::StopRadio(bool bKillSession /*= true*/)
{
  if (bKillSession) m_RadioSession = "";
  if (m_ThreadHandle)
  {
    m_bStop = true;
    SetEvent(m_hWorkerEvent);
    StopThread();
  }
  m_RadioTrackQueue.Clear();
  {
    CSingleLock lock(m_lockPlaylist);
    //all last.fm tracks are now invalid, remove them from the playlist
    CPlayList &playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
    for (int i = playlist.size() - 1; i >= 0; i--)
    {
      if (playlist[i].IsLastFM())
      {
        playlist.Remove(i);
      }
    }
  }
  SendUpdateMessage();
}

bool CLastFmManager::CanScrobble(const CFileItem &fileitem)
{
  return (!fileitem.IsInternetStream()) || (fileitem.IsLastFM() && g_guiSettings.GetBool("lastfm.recordtoprofile"));
}
