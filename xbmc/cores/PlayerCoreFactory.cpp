
#include "stdafx.h"
#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "cddaplayer.h"
#include "modplayer.h"
#include "SidPlayer.h"
#include "dvdplayer\DVDPlayer.h"

CPlayerCoreFactory::CPlayerCoreFactory()
{
}
CPlayerCoreFactory::~CPlayerCoreFactory()
{
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore,IPlayerCallback& callback) const
{
	CStdString strCoreLower=strCore;
	strCoreLower.ToLower();
	if (strCoreLower == "dvdplayer") return new CDVDPlayer(callback);
	if (strCoreLower == "mplayer") return new CMPlayer(callback);
	if (strCoreLower == "cdda") return new CCDDAPlayer(callback);
	if (strCoreLower == "mod") return new ModPlayer(callback);
	if (strCoreLower == "sid") return new SidPlayer(callback);
	//if (strCoreLower=="xine") return new CXinePlayer(callback);

	// default = mplayer
	return new CMPlayer(callback);
}


