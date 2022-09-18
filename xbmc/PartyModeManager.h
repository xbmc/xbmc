/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;
namespace PLAYLIST
{
using Id = int;
class CPlayList;
}

typedef enum
{
  PARTYMODECONTEXT_UNKNOWN,
  PARTYMODECONTEXT_MUSIC,
  PARTYMODECONTEXT_VIDEO
} PartyModeContext;

class CPartyModeManager final
{
public:
  CPartyModeManager(void);

  bool Enable(PartyModeContext context=PARTYMODECONTEXT_MUSIC, const std::string& strXspPath = "");
  void Disable();
  void Play(int iPos);
  void OnSongChange(bool bUpdatePlayed = false);
  void AddUserSongs(PLAYLIST::CPlayList& tempList, bool bPlay = false);
  void AddUserSongs(CFileItemList& tempList, bool bPlay = false);
  bool IsEnabled(PartyModeContext context=PARTYMODECONTEXT_UNKNOWN) const;
  int GetSongsPlayed();
  int GetMatchingSongs();
  int GetMatchingSongsPicked();
  int GetMatchingSongsLeft();
  int GetRelaxedSongs();
  int GetRandomSongs();
  PartyModeContext GetType() const;

private:
  void Process();
  bool AddRandomSongs();
  void Add(CFileItemPtr &pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void OnError(int iError, const std::string& strLogMessage);
  void ClearState();
  void UpdateStats();
  void Announce();
  PLAYLIST::Id GetPlaylistId() const;

  // state
  bool m_bEnabled;
  bool m_bIsVideo;
  int m_iLastUserSong;
  std::string m_type;

  // statistics
  int m_iSongsPlayed;
  int m_iMatchingSongs;
  int m_iMatchingSongsPicked;
  int m_iMatchingSongsLeft;
  int m_iRelaxedSongs;
  int m_iRandomSongs;

  // history
  std::vector<std::pair<int, int>> m_songIDCache;
};

extern CPartyModeManager g_partyModeManager;
