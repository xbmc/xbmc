/*!
	\file ActionManager.h
	\brief 
	*/

#pragma once
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

/*!
	\ingroup actionkeys
	\brief 
	*/
class CActionManager
{
public:
  CActionManager(void);
  virtual ~CActionManager(void);
  void		CallScriptAction(CGUIMessage& message);
  void		SetScriptActionCallback(IMsgSenderCallback* pCallback);

protected:
  IMsgSenderCallback*     m_pScriptActionCallback;
};

/*!
	\ingroup actionkeys
	\brief 
	*/
extern CActionManager g_actionManager;
