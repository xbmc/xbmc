#include "PlayerCoreFactory.h"
#include "mplayer.h"

CPlayerCoreFactory::CPlayerCoreFactory()
{
}
CPlayerCoreFactory::~CPlayerCoreFactory()
{
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore) const
{
	CStdString strCoreLower=strCore;
	strCoreLower.ToLower();
	if (strCoreLower=="mplayer") return new CMPlayer();
	//if (strCoreLower=="xine") return new CXinePlayer();

	// default = mplayer
	return new CMPlayer();
}


