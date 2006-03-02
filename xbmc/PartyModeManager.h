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
  void OnSongChange();
  void AddUserSongs(CPlayList& playlistTemp, bool bPlay = false);
  bool IsEnabled() { return m_bEnabled; };

private:
  void Process();
  bool AddRandomSongs();
  void Add(CFileItem *pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();

  bool m_bEnabled;
  int iLastUserSong;
  vector<long> m_vecHistory;
};

extern CPartyModeManager g_partyModeManager;