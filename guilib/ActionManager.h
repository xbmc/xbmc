/*!
	\file ActionManager.h
	\brief 
	*/

#pragma once
#include "gui3d.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

#include "stdstring.h"
using namespace std;

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
