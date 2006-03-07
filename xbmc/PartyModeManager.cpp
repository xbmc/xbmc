#include "stdafx.h"
#include "PartyModeManager.h"
#include "Application.h"
#include "playlistplayer.h"
#include "MusicDatabase.h"
#include "Util.h"
#include "FileItem.h"
#include "GUIWindowMusicPlayList.h"

#define QUEUE_DEPTH       10
#define TIMER             30000L    // 30 seconds
#define HISTORY_SIZE      25
#define DB_MINIMUM        HISTORY_SIZE * 2

CPartyModeManager g_partyModeManager;

CPartyModeManager::CPartyModeManager(void)
{
  m_bEnabled = false;
  m_iLastUserSong = -1;
  m_strCurrentFilter = "";
}

CPartyModeManager::~CPartyModeManager(void)
{
}

bool CPartyModeManager::Enable()
{
  CMusicDatabase musicdatabase;
  if (musicdatabase.Open())
  {
    if (musicdatabase.GetSongsCount((CStdString)"") < DB_MINIMUM)
    {
      musicdatabase.Close();
      OnError(16031, (CStdString)"Party mode needs atleast 50 songs in the database. Aborting.");
      return false;
    }
    if (!musicdatabase.InitialisePartyMode())
    {
      musicdatabase.Close();
      OnError(16032, (CStdString)"Party mode could not initialise database. Aborting.");
      return false;
    }
  }
  else
  {
    OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
    return false;
  }
  musicdatabase.Close();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode enabled!");

  // clear any previous state
  m_iLastUserSong = -1;
  m_strCurrentFilter = "";

  // setup the playlist
  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.ShufflePlay(PLAYLIST_MUSIC, false);
  g_playlistPlayer.Repeat(PLAYLIST_MUSIC, false);
  g_playlistPlayer.RepeatOne(PLAYLIST_MUSIC, false);

  // add initial songs
  if (!AddRandomSongs())
    return false;

  // start playing
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Play(0);

  // open now playing window
  if (m_gWindowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

  // done
  m_bEnabled = true;
  SendUpdateMessage();
  return true;
}

void CPartyModeManager::Disable()
{
  m_bEnabled = false;
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode disabled.");
}

void CPartyModeManager::OnSongChange()
{
  if (!IsEnabled())
    return;
  Process();
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
  Sleep(100);
  SendUpdateMessage();
}

bool CPartyModeManager::AddRandomSongs()
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iMissingSongs > 0)
  {
    // add songs to fill queue
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CFileItemList items;
      if (musicdatabase.PartyModeGetRandomSongs(items, iMissingSongs, HISTORY_SIZE, m_strCurrentFilter))
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

int CPartyModeManager::GetUniqueRandomSongsLeft()
{
  int iSongs = -1;
  if (IsEnabled())
  {
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
      iSongs = musicdatabase.PartyModeGetUniqueRandomSongsLeft(m_strCurrentFilter);
    musicdatabase.Close();
  }
  return iSongs;
}