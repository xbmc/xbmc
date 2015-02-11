#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>

#include <memory>

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;
namespace PLAYLIST
{
  class CPlayList;
}

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
  bool AddRandomSongs(int iSongs = 0);
  bool AddInitialSongs(std::vector< std::pair<int,int> > &songIDs);
  void Add(CFileItemPtr &pItem);
  bool ReapSongs();
  bool MovePlaying();
  void SendUpdateMessage();
  void OnError(int iError, const std::string& strLogMessage);
  void ClearState();
  void UpdateStats();
  std::pair<std::string,std::string> GetWhereClauseWithHistory() const;
  void AddToHistory(int type, int songID);
  void GetRandomSelection(std::vector< std::pair<int,int> > &in, unsigned int number, std::vector< std::pair<int, int> > &out);
  void Announce();

  // state
  bool m_bEnabled;
  bool m_bIsVideo;
  int m_iLastUserSong;
  std::string m_strCurrentFilterMusic;
  std::string m_strCurrentFilterVideo;
  std::string m_type;

  // statistics
  int m_iSongsPlayed;
  int m_iMatchingSongs;
  int m_iMatchingSongsPicked;
  int m_iMatchingSongsLeft;
  int m_iRelaxedSongs;
  int m_iRandomSongs;

  // history
  unsigned int m_songsInHistory;
  std::vector< std::pair<int,int> > m_history;
};

extern CPartyModeManager g_partyModeManager;
