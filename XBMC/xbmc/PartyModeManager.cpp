#include "stdafx.h"
#include "PartyModeManager.h"
#include "Application.h"
#include "playlistplayer.h"
#include "MusicDatabase.h"
#include "Util.h"
#include "FileItem.h"
#include "GUIWindowMusicPlayList.h"
#include "SmartPlaylist.h"
#include "GUIDialogProgress.h"

#define QUEUE_DEPTH       10
#define NEW_PARTY_MODE_METHOD

CPartyModeManager g_partyModeManager;

CPartyModeManager::CPartyModeManager(void)
{
  m_bEnabled = false;
  m_strCurrentFilter = "";
  ClearState();
}

CPartyModeManager::~CPartyModeManager(void)
{
}
//#define NEW_PARTY_MODE_METHOD 1
bool CPartyModeManager::Enable()
{
  // Filter using our PartyMode xml file
  CSmartPlaylist playlist;
  CStdString partyModePath;

  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDialog->SetHeading("Party party");
  pDialog->SetLine(0,"Filtering songs");
  pDialog->StartModal();
  partyModePath = g_settings.GetUserDataItem("partymode.xml");
  if (playlist.Load(partyModePath))
    m_strCurrentFilter = playlist.GetWhereClause();
  else
    m_strCurrentFilter.Empty();
  CLog::Log(LOGINFO, "PARTY MODE MANAGER: Registering filter:[%s]", m_strCurrentFilter.c_str());

  ClearState();
  CMusicDatabase musicdatabase;
  DWORD time = timeGetTime();
#ifdef NEW_PARTY_MODE_METHOD
  vector<long> songIDs;
#endif
  if (musicdatabase.Open())
  {
#ifdef NEW_PARTY_MODE_METHOD
    m_iMatchingSongs = (int)musicdatabase.GetSongIDs(m_strCurrentFilter, songIDs);
#else
    m_iMatchingSongs = musicdatabase.GetSongsCount(m_strCurrentFilter);
#endif
    if (m_iMatchingSongs < 1)
    {
      pDialog->Close();
      musicdatabase.Close();
      OnError(16031, (CStdString)"Party mode found no matching songs. Aborting.");
      return false;
    }
#ifndef NEW_PARTY_MODE_METHOD
    if (!musicdatabase.InitialisePartyMode())
    {
      pDialog->Close();
      musicdatabase.Close();
      OnError(16032, (CStdString)"Party mode could not initialise database. Aborting.");
      return false;
    }
#endif
  }
  else
  {
    pDialog->Close();
    OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
    return false;
  }
  musicdatabase.Close();

  // calculate history size
  if (m_iMatchingSongs < 50)
    m_iHistory = 0;
  else
    m_iHistory = (int)(m_iMatchingSongs/2);
  if (m_iHistory > 200)
    m_iHistory = 200;

  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Matching songs = %i, History size = %i", m_iMatchingSongs, m_iHistory);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode enabled!");

  // setup the playlist
  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, false);
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);

  pDialog->SetLine(0,"Adding songs");
  pDialog->Progress();
  // add initial songs
#ifdef NEW_PARTY_MODE_METHOD
  if (!AddInitialSongs(songIDs))
    return false;
#else
  if (!AddRandomSongs())
    return false;
#endif
  CLog::Log(LOGDEBUG, __FUNCTION__" time for song fetch: %i", timeGetTime() - time);
  // start playing
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  Play(0);

  pDialog->Close();
  // open now playing window
  if (m_gWindowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

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

void CPartyModeManager::AddUserSongs(CPlayList& playlistTemp, bool bPlay /* = false */)
{
  if (!IsEnabled())
    return;

  // where do we add?
  int iAddAt = -1;
  if (m_iLastUserSong < 0 || bPlay)
    iAddAt = 1; // under the currently playing song
  else
    iAddAt = m_iLastUserSong + 1; // under the last user added song

  int iNewUserSongs = playlistTemp.size();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding %i user selected songs at %i", iNewUserSongs, iAddAt);

  // get songs starting at the AddAt location move them to the temp playlist
  // TODO: find a better way to do this
  // maybe something like playlist.Add(CPlayList& playlistTemp, int iPos)?
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  while (playlist.size() > iAddAt)
  {
    playlistTemp.Add(playlist[iAddAt]);
    playlist.Remove(iAddAt);
  }

  // now add temp playlist to back real playlist
  for (int i=0; i<playlistTemp.size(); i++)
  {
    playlist.Add(playlistTemp[i]);
  }

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
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iSongs <= 0)
    iSongs = iMissingSongs;
  if (iSongs > 0)
  {
    // add songs to fill queue
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CFileItemList items;
      if (musicdatabase.PartyModeGetRandomSongs(items, iSongs, m_iHistory, m_strCurrentFilter))
      {
        for (int i = 0; i < items.Size(); i++)
          Add(items[i]);
      }
      else
      {
        musicdatabase.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    musicdatabase.Close();
  }
  return true;
}

void CPartyModeManager::Add(CFileItem *pItem)
{
  CPlayList::CPlayListItem playlistItem;
  CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  playlist.Add(playlistItem);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding randomly selected song at %i:[%s]", playlist.size() - 1, pItem->m_strPath.c_str());
}

bool CPartyModeManager::ReapSongs()
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);

  // reap any played songs
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  vector<int> vecPlayed;
  for (int i=0; i<playlist.size(); i++)
  {
    // get played song list
    if (playlist[i].WasPlayed() && i != iCurrentSong)
      vecPlayed.push_back(i);
  }
  // dont remove them while traversing the playlist!
  for (int i=0; i<(int)vecPlayed.size(); i++)
  {
    int iSong = vecPlayed[i];
    CLog::Log(LOGINFO,"PARTY MODE MANAGER: Reaping played song at %i", iSong);
    g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(iSong);
    if (iSong < iCurrentSong) iCurrentSong--;
    if (iSong <= m_iLastUserSong) m_iLastUserSong--;
  }
  g_playlistPlayer.SetCurrentSong(iCurrentSong);
  return true;
}

bool CPartyModeManager::MovePlaying()
{
  // move current song to the top if its not there
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  if (iCurrentSong > 0)
  {
    CLog::Log(LOGINFO,"PARTY MODE MANAGER: Moving currently playing song from %i to 0", iCurrentSong);
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

void CPartyModeManager::OnError(int iError, CStdString& strLogMessage)
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
  m_iHistory = -1;
  m_iSongsPlayed = 0;
  m_iMatchingSongs = 0;
  m_iMatchingSongsPicked = 0;
  m_iMatchingSongsLeft = 0;
  m_iRelaxedSongs = 0;
  m_iRandomSongs = 0;
}

void CPartyModeManager::UpdateStats()
{
  // get database statistics
  CMusicDatabase musicdatabase;
  if (musicdatabase.Open())
  {
    m_iMatchingSongsPicked = musicdatabase.PartyModeGetMatchingSongCount();
    if (m_iMatchingSongs > m_iMatchingSongsPicked)
      m_iMatchingSongsLeft = m_iMatchingSongs - m_iMatchingSongsPicked;
    else
      m_iMatchingSongsLeft = 0;
    m_iRelaxedSongs = musicdatabase.PartyModeGetRelaxedSongCount();
    m_iRandomSongs = musicdatabase.PartyModeGetRandomSongCount();
  }
  musicdatabase.Close();
}

bool CPartyModeManager::AddInitialSongs(vector<long> &songIDs)
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iMissingSongs > 0)
  {
    // generate iMissingSongs random ids from songIDs
    if (iMissingSongs > (int)songIDs.size())
      return false; // can't do it if we have less songs than we need

    // TODO: rand() only generates between 0 and 32767
    // so we'll need to modify this in future
    srand(timeGetTime());
    vector<long> chosenSongIDs;
    for (int i = 0; i < iMissingSongs; i++)
    {
      int num = rand() % songIDs.size();
      chosenSongIDs.push_back(songIDs[num]);
      songIDs.erase(songIDs.begin() + num);
    }
    // construct the where clause
    CStdString sqlWhere = "where songview.idsong in (";
    for (vector<long>::iterator it = chosenSongIDs.begin(); it != chosenSongIDs.end(); it++)
    {
      CStdString song;
      song.Format("%i,", *it);
      sqlWhere += song;
    }
    // replace the last comma with closing bracket
    sqlWhere[sqlWhere.size() - 1] = ')';

    // add songs to fill queue
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CFileItemList items;
      musicdatabase.BeginTransaction();
      if (musicdatabase.GetSongsByWhere(sqlWhere, items))
      {
        musicdatabase.InitialisePartyMode();
        for (int i = 0; i < items.Size(); i++)
        {
          Add(items[i]);
          // TODO: Allow "relaxed restrictions" later?
          musicdatabase.UpdatePartyMode(chosenSongIDs[i], false);
        }
      }
      else
      {
        musicdatabase.CommitTransaction();
        musicdatabase.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
      musicdatabase.CommitTransaction();
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    musicdatabase.Close();
  }
  return true;
}

