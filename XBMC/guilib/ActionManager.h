#pragma once
#include "gui3d.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

#include "stdstring.h"
using namespace std;

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

extern CActionManager g_actionManager;
