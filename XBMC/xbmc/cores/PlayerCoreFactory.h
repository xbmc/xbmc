#pragma once
#include "IPlayer.h"

enum EPLAYERCORES
{
  EPC_NONE,
  EPC_DVDPLAYER,
  EPC_MPLAYER,
  EPC_PAPLAYER,
  //EPC_SIDPLAYER,
  EPC_MODPLAYER
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
