#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IPlayer.h"

// do not remove mplayer - will break scripts
enum EPLAYERCORES
{
  EPC_NONE,
  EPC_DVDPLAYER,
  EPC_MPLAYER,
  EPC_PAPLAYER,
  EPC_EXTPLAYER
};

typedef std::vector<EPLAYERCORES> VECPLAYERCORES;

class CPlayerCoreFactory
{
public:
  CPlayerCoreFactory();
  virtual ~CPlayerCoreFactory();

  IPlayer* CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const;

  static EPLAYERCORES GetPlayerCore(const CStdString& strCore);
  static CStdString GetPlayerName(const EPLAYERCORES eCore);

  static IPlayer* CreatePlayer(const EPLAYERCORES eCore, IPlayerCallback& callback);
  static void GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores); //Players supporting the specified file
  static void GetPlayers( VECPLAYERCORES &vecCores );                       //All players

  static EPLAYERCORES GetDefaultPlayer( const CFileItem& item );

  static EPLAYERCORES SelectPlayerDialog(VECPLAYERCORES &vecCores, float posX = 0, float posY = 0);
  static EPLAYERCORES SelectPlayerDialog(float posX, float posY);
  
};
