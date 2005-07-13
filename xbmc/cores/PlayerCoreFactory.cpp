
#include "../stdafx.h"
#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "modplayer.h"
#include "SidPlayer.h"
#include "dvdplayer\DVDPlayer.h"
#include "paplayer\paplayer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPlayerCoreFactory::CPlayerCoreFactory()
{}
CPlayerCoreFactory::~CPlayerCoreFactory()
{}

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore, IPlayerCallback& callback) const
{
  CStdString strCoreLower = strCore;
  strCoreLower.ToLower();
  if (strCoreLower == "dvdplayer") return new CDVDPlayer(callback);
  if (strCoreLower == "mplayer") return new CMPlayer(callback);
  if (strCoreLower == "mod") return new ModPlayer(callback);
  if (strCoreLower == "sid") return new SidPlayer(callback);
  //if (strCoreLower=="xine") return new CXinePlayer(callback);
  if (strCoreLower == "paplayer" ) return new PAPlayer(callback); // added by dataratt

  // default = mplayer
  return new CMPlayer(callback);
}


