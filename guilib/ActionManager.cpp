#include "ActionManager.h"

CActionManager g_actionManager;

CActionManager::CActionManager(void)
{
}

CActionManager::~CActionManager(void)
{
}

void CActionManager::CallScriptAction(CGUIMessage& message)
{
  if (!m_pScriptActionCallback) return;
  m_pScriptActionCallback->SendMessage(message);
}

void CActionManager::SetScriptActionCallback(IMsgSenderCallback* pCallback)
{
  m_pScriptActionCallback=pCallback;
}
