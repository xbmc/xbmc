#pragma once
#include "stdstring.h"
class IPlayer
{
public:
	IPlayer(){};
	virtual ~IPlayer(){};
	virtual bool	openfile(const CStdString& strFile){return false;};
	virtual bool  closefile(){return true;};
	virtual bool  IsPlaying() const {return false;} ;
};