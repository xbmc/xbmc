#pragma once
#include "stdstring.h"

class IPlayerCallback
{
public:
	virtual void OnPlayBackEnded()=0;
	virtual void OnPlayBackStarted()=0;
};

class IPlayer
{
public:
	IPlayer(IPlayerCallback& callback):m_callback(callback){};
	virtual ~IPlayer(){};
	virtual bool	openfile(const CStdString& strFile){return false;};
	virtual bool  closefile(){return true;};
	virtual bool  IsPlaying() const {return false;} ;
protected:
	IPlayerCallback& m_callback;
};