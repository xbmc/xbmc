#pragma once
#include "FileItem.h"
#include "playlist.h"

class CPartyModeManager
{
public:
  CPartyModeManager(void);
  virtual ~CPartyModeManager(void);

  bool Enable();
  void Disable();
  void Play(int iPos);
  void OnSongChange(bool bUpdatePlayed = false);
  void AddUserSongs(PLAYLIST::CPlayList& tempList, bool bPlay = false);
  void AddUserSongs(CFileItemList& tempList, bool bPlay = false);
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
  bool AddInitialSongs(vector<long> &songIDs);
  void Add(CFileItem *pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void OnError(int iError, CStdString& strLogMessage);
  int GetSongCount(int iType);
  void ClearState();
  void UpdateStats();
  CStdString GetWhereClauseWithHistory() const;
  void AddToHistory(long songID);
  void GetRandomSelection(vector<long> &in, unsigned int number, vector<long> &out);

  // state
  bool m_bEnabled;
  int m_iLastUserSong;
  CStdString m_strCurrentFilter;
  CStdString m_type;

  // statistics
  int m_iSongsPlayed;
  int m_iMatchingSongs;
  int m_iMatchingSongsPicked;
  int m_iMatchingSongsLeft;
  int m_iRelaxedSongs;
  int m_iRandomSongs;

  // history
  unsigned int m_songsInHistory;
  vector<long> m_history;
};

extern CPartyModeManager g_partyModeManager;