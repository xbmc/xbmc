/*!
	\file IMsgTargetCallback.h
	\brief 
	*/

#ifndef GUILIB_IMSGTARGETCALLBACK
#define GUILIB_IMSGTARGETCALLBACK

#pragma once
#include "guimessage.h"

/*!
	\ingroup winman
	\brief 
	*/
class IMsgTargetCallback
{
public:
  virtual bool   OnMessage(CGUIMessage& message)=0;
};

#endif
