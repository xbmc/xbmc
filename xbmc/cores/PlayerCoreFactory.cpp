
#include "stdafx.h"
#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "cddaplayer.h"
#include "modplayer.h"
#include "SidPlayer.h"

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
	if (strCoreLower=="mplayer") return new CMPlayer(callback);
	if (strCoreLower=="cdda") return new CDDAPlayer(callback);
	if (strCoreLower=="mod") return new ModPlayer(callback);
	if (strCoreLower=="sid") return new SidPlayer(callback);
	//if (strCoreLower=="xine") return new CXinePlayer(callback);

	// default = mplayer
	return new CMPlayer(callback);
}


