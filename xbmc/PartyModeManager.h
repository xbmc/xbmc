#pragma once
#include "FileItem.h"
#include "playlist.h"

using namespace PLAYLIST;

class CPartyModeManager
{
public:
  CPartyModeManager(void);
  virtual ~CPartyModeManager(void);

  bool Enable();
  void Disable();
  void Play(int iPos);
  void OnSongChange(bool bUpdatePlayed = false);
  void AddUserSongs(CPlayList& playlistTemp, bool bPlay = false);
  bool IsEnabled() { return m_bEnabled; };
  int GetSongsPlayed();
  int GetMatchingSongs();
  int GetMatchingSongsPicked();
  int GetMatchingSongsLeft();
  int GetRelaxedSongs();
  int GetRandomSongs();

private:
  void Process();
  bool AddRandomSongs(int iSongs = 0);
  void Add(CFileItem *pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void OnError(int iError, CStdString& strLogMessage);
  int GetSongCount(int iType);
  void ClearState();
  void UpdateStats();

  // state
  bool m_bEnabled;
  int m_iLastUserSong;
  int m_iHistory;
  CStdString m_strCurrentFilter;

  // statistics
  int m_iSongsPlayed;
  int m_iMatchingSongs;
  int m_iMatchingSongsPicked;
  int m_iMatchingSongsLeft;
  int m_iRelaxedSongs;
  int m_iRandomSongs;
};

extern CPartyModeManager g_partyModeManager;