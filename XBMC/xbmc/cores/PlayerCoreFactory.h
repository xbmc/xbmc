#pragma once
#include "iplayer.h"

class CPlayerCoreFactory
{
public:
	CPlayerCoreFactory();
	virtual ~CPlayerCoreFactory();
	IPlayer* CreatePlayer(const CStdString& strCore,IPlayerCallback& callback) const;

};