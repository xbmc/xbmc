/*!
\file IMsgSenderCallback.h
\brief 
*/

#ifndef GUILIB_IMSGSENDERCALLBACK
#define GUILIB_IMSGSENDERCALLBACK

#pragma once
#include "GUIMessage.h"

/*!
 \ingroup winman
 \brief 
 */
class IMsgSenderCallback
{
public:
  virtual bool SendMessage(CGUIMessage& message) = 0;
  virtual ~IMsgSenderCallback() {} 
};

#endif
