#ifndef GUILIB_IMSGSENDERCALLBACK
#define GUILIB_IMSGSENDERCALLBACK

#pragma once
#include "guimessage.h"

class IMsgSenderCallback
{
public:
  virtual void   SendMessage(CGUIMessage& message)=0;
};

#endif