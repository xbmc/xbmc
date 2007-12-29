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
#include "PartyModeManager.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "Util.h"
#include "FileItem.h"
#include "GUIWindowMusicPlaylist.h"
#include "SmartPlaylist.h"
#include "GUIDialogProgress.h"

using namespace PLAYLIST;

#define QUEUE_DEPTH       10

CPartyModeManager g_partyModeManager;

CPartyModeManager::CPartyModeManager(void)
{
  m_bEnabled = false;
  m_strCurrentFilterMusic.Empty();
  m_strCurrentFilterVideo.Empty();
  ClearState();
}

CPartyModeManager::~CPartyModeManager(void)
{
}

bool CPartyModeManager::Enable(PartyModeContext context /*= PARTYMODECONTEXT_MUSIC*/, const CStdString& strXspPath /*= ""*/)
{
  // Filter using our PartyMode xml file
  CSmartPlaylist playlist;
  CStdString partyModePath;

  m_bIsVideo = context == PARTYMODECONTEXT_VIDEO;
  if (!strXspPath.IsEmpty()) //if a path to a smartplaylist is supplied use it
    partyModePath = strXspPath;
  else if (m_bIsVideo)
    partyModePath = g_settings.GetUserDataItem("PartyMode-Video.xsp");
  else
    partyModePath = g_settings.GetUserDataItem("PartyMode.xsp");
  if (playlist.Load(partyModePath))
  {
    m_type = playlist.GetType();
    if (context == PARTYMODECONTEXT_UNKNOWN)
    {
      //get it from the xsp file
      m_bIsVideo = (m_type.Equals("video") || m_type.Equals("mixed"));
    }
    if (m_type.Equals("mixed"))
      playlist.SetType("music");
    m_strCurrentFilterMusic = playlist.GetWhereClause();
    if (m_type.Equals("mixed"))
      playlist.SetType("video");
    m_strCurrentFilterVideo = playlist.GetWhereClause();
    playlist.SetType(m_type);
  }
  else
  {
    m_strCurrentFilterMusic.Empty();
    m_strCurrentFilterVideo.Empty();
    m_type = m_bIsVideo ? "video" : "music";
  }

  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  int iHeading = (m_bIsVideo ? 20250 : 20121);
  int iLine0 = (m_bIsVideo ? 20251 : 20123);
  pDialog->SetHeading(iHeading);
  pDialog->SetLine(0, iLine0);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");
  pDialog->StartModal();

  ClearState();
  DWORD time = timeGetTime();
  vector<pair<int,long> > songIDs;
  if (m_type.Equals("music") || m_type.Equals("mixed"))
  {
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CLog::Log(LOGINFO, "PARTY MODE MANAGER: Registering filter:[%s]", m_strCurrentFilterMusic.c_str());
      m_iMatchingSongs = (int)musicdatabase.GetSongIDs(m_strCurrentFilterMusic, songIDs);
      if (m_iMatchingSongs < 1 && m_type.Equals("music"))
      {
        pDialog->Close();
        musicdatabase.Close();
        OnError(16031, (CStdString)"Party mode found no matching songs. Aborting.");
        return false;
      }
    }
    else
    {
      pDialog->Close();
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    musicdatabase.Close();
  }
  if (m_type.Equals("video") || m_type.Equals("mixed"))
  {
    vector<pair<int,long> > songIDs2;
    CVideoDatabase database;
    if (database.Open())
    {
      CLog::Log(LOGINFO, "PARTY MODE MANAGER: Registering filter:[%s]", m_strCurrentFilterVideo.c_str());
      m_iMatchingSongs += (int)database.GetMusicVideoIDs(m_strCurrentFilterVideo, songIDs2);
      if (m_iMatchingSongs < 1)
      {
        pDialog->Close();
        database.Close();
        OnError(16031, (CStdString)"Party mode found no matching songs. Aborting.");
        return false;
      }
    }
    else
    {
      pDialog->Close();
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    database.Close();
    songIDs.insert(songIDs.end(),songIDs2.begin(),songIDs2.end());
  }

  // calculate history size
  if (m_iMatchingSongs < 50)
    m_songsInHistory = 0;
  else
    m_songsInHistory = (int)(m_iMatchingSongs/2);
  if (m_songsInHistory > 200)
    m_songsInHistory = 200;

  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Matching songs = %i, History size = %i", m_iMatchingSongs, m_songsInHistory);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode enabled!");

  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  g_playlistPlayer.ClearPlaylist(iPlaylist);
  g_playlistPlayer.SetShuffle(iPlaylist, false);
  g_playlistPlayer.SetRepeat(iPlaylist, PLAYLIST::REPEAT_NONE);

  pDialog->SetLine(0, (m_bIsVideo ? 20252 : 20124));
  pDialog->Progress();
  // add initial songs
  if (!AddInitialSongs(songIDs))
  {
    pDialog->Close();
    return false;
  }
  CLog::Log(LOGDEBUG, "%s time for song fetch: %lu", __FUNCTION__, timeGetTime() - time);

  // start playing
  g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
  Play(0);

  pDialog->Close();
  // open now playing window
  if (m_type.Equals("music"))
  {
    if (m_gWindowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
  }

  // done
  m_bEnabled = true;
  return true;
}

void CPartyModeManager::Disable()
{
  if (!IsEnabled())
    return;
  m_bEnabled = false;
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode disabled.");
}

void CPartyModeManager::OnSongChange(bool bUpdatePlayed /* = false */)
{
  if (!IsEnabled())
    return;
  Process();
  if (bUpdatePlayed)
    m_iSongsPlayed++;
}

void CPartyModeManager::AddUserSongs(CPlayList& tempList, bool bPlay /* = false */)
{
  if (!IsEnabled())
    return;

  // where do we add?
  int iAddAt = -1;
  if (m_iLastUserSong < 0 || bPlay)
    iAddAt = 1; // under the currently playing song
  else
    iAddAt = m_iLastUserSong + 1; // under the last user added song

  int iNewUserSongs = tempList.size();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding %i user selected songs at %i", iNewUserSongs, iAddAt);

  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;
  g_playlistPlayer.GetPlaylist(iPlaylist).Insert(tempList, iAddAt);

  // update last user added song location
  if (m_iLastUserSong < 0)
    m_iLastUserSong = 0;
  m_iLastUserSong += iNewUserSongs;

  if (bPlay)
    Play(1);
}

void CPartyModeManager::AddUserSongs(CFileItemList& tempList, bool bPlay /* = false */)
{
  if (!IsEnabled())
    return;

  // where do we add?
  int iAddAt = -1;
  if (m_iLastUserSong < 0 || bPlay)
    iAddAt = 1; // under the currently playing song
  else
    iAddAt = m_iLastUserSong + 1; // under the last user added song

  int iNewUserSongs = tempList.Size();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding %i user selected songs at %i", iNewUserSongs, iAddAt);

  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  g_playlistPlayer.GetPlaylist(iPlaylist).Insert(tempList, iAddAt);

  // update last user added song location
  if (m_iLastUserSong < 0)
    m_iLastUserSong = 0;
  m_iLastUserSong += iNewUserSongs;

  if (bPlay)
    Play(1);
}

void CPartyModeManager::Process()
{
  ReapSongs();
  MovePlaying();
  AddRandomSongs();
  UpdateStats();
  SendUpdateMessage();
}

bool CPartyModeManager::AddRandomSongs(int iSongs /* = 0 */)
{
  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iSongs <= 0)
    iSongs = iMissingSongs;
  // distribute between types if mixed
  int iSongsToAdd=iSongs;
  int iVidsToAdd=iSongs;
  if (m_type.Equals("mixed"))
  {
    if (iSongs == 1)
    {
      if (rand() % 10 < 7) // 70 % chance of grabbing a song
        iVidsToAdd = 0;
      else
        iSongsToAdd = 0;
    }
    if (iSongs > 1) // grab 70 % songs, 30 % mvids
    {
      iSongsToAdd = (int).7f*iSongs;
      iVidsToAdd = (int).3f*iSongs;
      while (iSongsToAdd+iVidsToAdd < iSongs) // correct any rounding by adding songs
        iSongsToAdd++;
    }
  }

  // add songs to fill queue
  if (m_type.Equals("music") || m_type.Equals("mixed"))
  {
    CMusicDatabase database;
    if (database.Open())
    {
      // Method:
      // 1. Grab a random entry from the database using a where clause
      // 2. Iterate on iSongs.

      // Note: At present, this method is faster than the alternative, which is to grab
      // all valid songids, then select a random number of them (as done in AddInitialSongs()).
      // The reason for this is simply the number of songs we are requesting - we generally
      // only want one here.  Any more than about 3 songs and it is more efficient
      // to use the technique in AddInitialSongs.  As it's unlikely that we'll require
      // more than 1 song at a time here, this method is faster.
      bool error(false);
      for (int i = 0; i < iSongsToAdd; i++)
      {
        pair<CStdString,CStdString> whereClause = GetWhereClauseWithHistory();
        CFileItem *item = new CFileItem;
        long songID;
        if (database.GetRandomSong(item, songID, whereClause.first))
        { // success
          Add(item);
          AddToHistory(1,songID);
        }
        else
        {
          delete item;
          error = true;
          break;
        }
      }

      if (error)
      {
        database.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    database.Close();
  }
  if (m_type.Equals("video") || m_type.Equals("mixed"))
  {
    CVideoDatabase database;
    if (database.Open())
    {
      // Method:
      // 1. Grab a random entry from the database using a where clause
      // 2. Iterate on iSongs.

      // Note: At present, this method is faster than the alternative, which is to grab
      // all valid songids, then select a random number of them (as done in AddInitialSongs()).
      // The reason for this is simply the number of songs we are requesting - we generally
      // only want one here.  Any more than about 3 songs and it is more efficient
      // to use the technique in AddInitialSongs.  As it's unlikely that we'll require
      // more than 1 song at a time here, this method is faster.
      bool error(false);
      for (int i = 0; i < iVidsToAdd; i++)
      {
        pair<CStdString,CStdString> whereClause = GetWhereClauseWithHistory();
        CFileItem *item = new CFileItem;
        long songID;
        if (database.GetRandomMusicVideo(item, songID, whereClause.second))
        { // success
          Add(item);
          AddToHistory(2,songID);
        }
        else
        {
          delete item;
          error = true;
          break;
        }
      }

      if (error)
      {
        database.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    database.Close();
  }
  return true;
}

void CPartyModeManager::Add(CFileItem *pItem)
{
  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  CPlayList::CPlayListItem playlistItem;
  CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);
  playlist.Add(playlistItem);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding randomly selected song at %i:[%s]", playlist.size() - 1, pItem->m_strPath.c_str());
  m_iMatchingSongsPicked++;
}

bool CPartyModeManager::ReapSongs()
{
  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  //CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);

  // reap any played songs
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  int i=0;
  while (i < g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    if (i < iCurrentSong)
    {
      g_playlistPlayer.GetPlaylist(iPlaylist).Remove(i);
      iCurrentSong--;
      if (i <= m_iLastUserSong)
        m_iLastUserSong--;
    }
    else
      i++;
  }

  g_playlistPlayer.SetCurrentSong(iCurrentSong);
  return true;
}

bool CPartyModeManager::MovePlaying()
{
  // move current song to the top if its not there
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  if (iCurrentSong > 0)
  {
    CLog::Log(LOGINFO,"PARTY MODE MANAGER: Moving currently playing song from %i to 0", iCurrentSong);
    CPlayList &playlist = g_playlistPlayer.GetPlaylist(iPlaylist);
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

void CPartyModeManager::SendUpdateMessage()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
}

void CPartyModeManager::Play(int iPos)
{
  // move current song to the top if its not there
  g_playlistPlayer.Play(iPos);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Playing song at %i", iPos);
  Process();
}

void CPartyModeManager::OnError(int iError, const CStdString&  strLogMessage)
{
  // open error dialog
  CGUIDialogOK::ShowAndGetInput(257, 16030, iError, 0);
  CLog::Log(LOGERROR, "PARTY MODE MANAGER: %s", strLogMessage.c_str());
  m_bEnabled = false;
  SendUpdateMessage();
}

int CPartyModeManager::GetSongsPlayed()
{
  if (!IsEnabled())
    return -1;
  return m_iSongsPlayed;
}

int CPartyModeManager::GetMatchingSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongs;
}

int CPartyModeManager::GetMatchingSongsPicked()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongsPicked;
}

int CPartyModeManager::GetMatchingSongsLeft()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongsLeft;
}

int CPartyModeManager::GetRelaxedSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iRelaxedSongs;
}

int CPartyModeManager::GetRandomSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iRandomSongs;
}

void CPartyModeManager::ClearState()
{
  m_iLastUserSong = -1;
  m_iSongsPlayed = 0;
  m_iMatchingSongs = 0;
  m_iMatchingSongsPicked = 0;
  m_iMatchingSongsLeft = 0;
  m_iRelaxedSongs = 0;
  m_iRandomSongs = 0;

  m_songsInHistory = 0;
  m_history.clear();
}

void CPartyModeManager::UpdateStats()
{
  m_iMatchingSongsLeft = m_iMatchingSongs - m_iMatchingSongsPicked;
  m_iRandomSongs = m_iMatchingSongsPicked;
  m_iRelaxedSongs = 0;  // unsupported at this stage
}

bool CPartyModeManager::AddInitialSongs(vector<pair<int,long> > &songIDs)
{
  int iPlaylist = PLAYLIST_MUSIC;
  if (m_bIsVideo)
    iPlaylist = PLAYLIST_VIDEO;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iMissingSongs > 0)
  {
    // generate iMissingSongs random ids from songIDs
    if (iMissingSongs > (int)songIDs.size())
      return false; // can't do it if we have less songs than we need

    vector<pair<int,long> > chosenSongIDs;
    GetRandomSelection(songIDs, iMissingSongs, chosenSongIDs);
    CStdString sqlWhereMusic = "where songview.idsong in (";
    CStdString sqlWhereVideo = "where musicvideo.idmvideo in (";

    for (vector<pair<int,long> >::iterator it = chosenSongIDs.begin(); it != chosenSongIDs.end(); it++)
    {
      CStdString song;
      song.Format("%i,", it->second);
      if (it->first == 1)
        sqlWhereMusic += song;
      if (it->first == 2)
        sqlWhereVideo += song;
    }
    // add songs to fill queue
    CFileItemList items;

    if (sqlWhereMusic.size() > 26)
    {
      sqlWhereMusic[sqlWhereMusic.size() - 1] = ')'; // replace the last comma with closing bracket
      CMusicDatabase database;
      database.Open();
      database.GetSongsByWhere("", sqlWhereMusic, items);
    }
    if (sqlWhereVideo.size() > 30)
    {
      sqlWhereVideo[sqlWhereVideo.size() - 1] = ')'; // replace the last comma with closing bracket
      CVideoDatabase database;
      database.Open();
      database.GetMusicVideosByWhere("videodb://3/2/", sqlWhereVideo, items);
    }

    m_history = chosenSongIDs;
    items.Randomize(); //randomizing the initial list or they will be in database order
    for (int i = 0; i < items.Size(); i++)
    {
      Add(items[i]);
      // TODO: Allow "relaxed restrictions" later?
    }
  }
  return true;
}

pair<CStdString,CStdString> CPartyModeManager::GetWhereClauseWithHistory() const
{
  CStdString historyWhereMusic;
  CStdString historyWhereVideo;
  // now add this on to the normal where clause
  if (m_history.size())
  {
    if (m_strCurrentFilterMusic.IsEmpty())
      historyWhereMusic = "where songview.idsong not in (";
    else
      historyWhereMusic = m_strCurrentFilterMusic + " and songview.idsong not in (";
    if (m_strCurrentFilterVideo.IsEmpty())
      historyWhereVideo = "where musicvideo.idmvideo not in (";
    else
      historyWhereVideo = m_strCurrentFilterVideo + " and musicvideo.idmvideo not in (";

    for (unsigned int i = 0; i < m_history.size(); i++)
    {
      CStdString number;
      number.Format("%i,", m_history[i].second);
      if (m_history[i].first == 1)
        historyWhereMusic += number;
      if (m_history[i].first == 2)
        historyWhereVideo += number;
    }
    historyWhereMusic.TrimRight(",");
    historyWhereMusic += ")";
    historyWhereVideo.TrimRight(",");
    historyWhereVideo += ")";
  }
  return make_pair(historyWhereMusic,historyWhereVideo);
}

void CPartyModeManager::AddToHistory(int type, long songID)
{
  while (m_history.size() >= m_songsInHistory && m_songsInHistory)
    m_history.erase(m_history.begin());
  m_history.push_back(make_pair<int,long>(type,songID));
}

void CPartyModeManager::GetRandomSelection(vector<pair<int,long> >& in, unsigned int number, vector<pair<int,long> >& out)
{
  // only works if we have < 32768 in the in vector
  srand(timeGetTime());
  for (unsigned int i = 0; i < number; i++)
  {
    int num = rand() % in.size();
    out.push_back(in[num]);
    in.erase(in.begin() + num);
  }
}
