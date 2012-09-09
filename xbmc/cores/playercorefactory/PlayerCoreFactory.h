#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "../IPlayer.h"

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
class TiXmlElement;
class CPlayerCoreConfig;
class CPlayerSelectionRule;

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
  EPC_EXTPLAYER
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
  CPlayerCoreFactory();
  virtual ~CPlayerCoreFactory();

  IPlayer* CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const;

  static PLAYERCOREID GetPlayerCore(const CStdString& strCoreName);
  static CPlayerCoreConfig* GetPlayerConfig(const CStdString& strCoreName);
  static CStdString GetPlayerName(const PLAYERCOREID eCore);

  static IPlayer* CreatePlayer(const PLAYERCOREID eCore, IPlayerCallback& callback);
  static void GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores);   //Players supporting the specified file
  static void GetPlayers( VECPLAYERCORES &vecCores, bool audio, bool video ); //All audio players and/or video players
  static void GetPlayers( VECPLAYERCORES &vecCores );                         //All players

  static PLAYERCOREID GetDefaultPlayer( const CFileItem& item );

  static PLAYERCOREID SelectPlayerDialog(VECPLAYERCORES &vecCores, float posX = 0, float posY = 0);
  static PLAYERCOREID SelectPlayerDialog(float posX, float posY);

  static bool LoadConfiguration(TiXmlElement* pConfig, bool clear);

private:
  static std::vector<CPlayerCoreConfig *> s_vecCoreConfigs;
  static std::vector<CPlayerSelectionRule *> s_vecCoreSelectionRules;
};
