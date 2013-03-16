#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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

#include "cores/IPlayerCallback.h"
#include "threads/CriticalSection.h"
#include "utils/StdString.h"

#include <vector>

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
class TiXmlElement;
class CFileItem;
class CPlayerCoreConfig;
class CPlayerSelectionRule;
class IPlayer;

// do not remove mplayer - will break scripts
enum EPLAYERCORES
{
  EPC_NONE,
  EPC_DVDPLAYER,
  EPC_MPLAYER,
  EPC_PAPLAYER,
#if defined(HAS_AMLPLAYER)
  EPC_AMLPLAYER,
#endif
#if defined(HAS_OMXPLAYER)
  EPC_OMXPLAYER,
#endif
  EPC_EXTPLAYER,
  EPC_UPNPPLAYER,
};

typedef unsigned int PLAYERCOREID;
typedef std::vector<PLAYERCOREID> VECPLAYERCORES;
const PLAYERCOREID PCID_NONE = EPC_NONE;
const PLAYERCOREID PCID_DVDPLAYER = EPC_DVDPLAYER;
const PLAYERCOREID PCID_MPLAYER = EPC_MPLAYER;
const PLAYERCOREID PCID_PAPLAYER = EPC_PAPLAYER;
#if defined(HAS_AMLPLAYER)
const PLAYERCOREID PCID_AMLPLAYER = EPC_AMLPLAYER;
#endif
#if defined(HAS_OMXPLAYER)
const PLAYERCOREID PCID_OMXPLAYER = EPC_OMXPLAYER;
#endif

class CPlayerCoreFactory
{
public:
  static CPlayerCoreFactory& Get();

  PLAYERCOREID GetPlayerCore(const CStdString& strCoreName) const;
  CPlayerCoreConfig* GetPlayerConfig(const CStdString& strCoreName) const;
  CStdString GetPlayerName(const PLAYERCOREID eCore) const;

  IPlayer* CreatePlayer(const PLAYERCOREID eCore, IPlayerCallback& callback) const;
  IPlayer* CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const;
  void GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores) const;   //Players supporting the specified file
  void GetPlayers( VECPLAYERCORES &vecCores, bool audio, bool video ) const; //All audio players and/or video players
  void GetPlayers( VECPLAYERCORES &vecCores ) const;                         //All players

  void GetRemotePlayers( VECPLAYERCORES &vecCores ) const;                   //All remote players we can attach to

  PLAYERCOREID GetDefaultPlayer( const CFileItem& item ) const;

  PLAYERCOREID SelectPlayerDialog(VECPLAYERCORES &vecCores, float posX = 0, float posY = 0) const;
  PLAYERCOREID SelectPlayerDialog(float posX, float posY) const;

  bool LoadConfiguration(const std::string &file, bool clear);

  void OnPlayerDiscovered(const CStdString& id, const CStdString& name, EPLAYERCORES core);
  void OnPlayerRemoved(const CStdString& id);

protected:
  CPlayerCoreFactory();
  CPlayerCoreFactory(const CPlayerCoreFactory&);
  CPlayerCoreFactory const& operator=(CPlayerCoreFactory const&);
  virtual ~CPlayerCoreFactory();

private:
  std::vector<CPlayerCoreConfig *> m_vecCoreConfigs;
  std::vector<CPlayerSelectionRule *> m_vecCoreSelectionRules;
  CCriticalSection m_section;
};
