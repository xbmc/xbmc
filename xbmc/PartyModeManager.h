#pragma once
#include "FileItem.h"
#include "playlist.h"

typedef enum
{
  PARTYMODECONTEXT_UNKNOWN,
  PARTYMODECONTEXT_MUSIC,
  PARTYMODECONTEXT_VIDEO
} PartyModeContext;

class CPartyModeManager
{
public:
  CPartyModeManager(void);
  virtual ~CPartyModeManager(void);

  bool Enable(PartyModeContext context=PARTYMODECONTEXT_MUSIC, const CStdString& strXspPath = "");
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
  bool AddInitialSongs(vector<pair<int,long> > &songIDs);
  void Add(CFileItem *pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void OnError(int iError, CStdString& strLogMessage);
  int GetSongCount(int iType);
  void ClearState();
  void UpdateStats();
  std::pair<CStdString,CStdString> GetWhereClauseWithHistory() const;
  void AddToHistory(int type, long songID);
  void GetRandomSelection(vector<pair<int,long> > &in, unsigned int number, vector<pair<int, long> > &out);

  // state
  bool m_bEnabled;
  bool m_bIsVideo;
  int m_iLastUserSong;
  CStdString m_strCurrentFilterMusic;
  CStdString m_strCurrentFilterVideo;
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
  vector<pair<int,long> > m_history;
};

extern CPartyModeManager g_partyModeManager;