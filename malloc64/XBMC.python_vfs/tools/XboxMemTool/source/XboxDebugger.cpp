
#include "..\stdafx.h"
#include "XboxDebugger.h"
#include <xboxdbg.h>

CXboxDebugger::CXboxDebugger(const char* target, const char* module)
{
  if (target != NULL) m_target = target;
  else m_target = "";
  
  if (module != NULL) m_module = module;
  else m_module = "";

  m_isConnected = false;
}

CXboxDebugger::~CXboxDebugger()
{
  if (m_isConnected)
  {
    Disconnect();
  }
}

BOOL CXboxDebugger::Connect()
{
  if (m_target.size() > 0)
  {
    if (XBDM_NOERR != DmSetXboxName(m_target.c_str()))
    {
      m_isConnected = false;
      return false;
    }
  }
  
  m_isConnected = true;
  return true;
}

void CXboxDebugger::Disconnect()
{
  m_isConnected = false;
}

bool CXboxDebugger::GetLoadedModules(std::vector<CModule>& modules)
{
  PDM_WALK_MODULES pWalkMod = NULL;
  DMN_MODLOAD modLoad;
  
  HRESULT xbres = 0;
  
  g_log.Log(LOG_INFO, "CXboxDebugger::GetLoadedModules, Setting up connection with the default xbox");

  xbres = DmWalkLoadedModules(&pWalkMod, &modLoad);
  if (XBDM_NOERR != xbres)
  {
    g_log.Log(LOG_INFO, "CXboxDebugger::GetLoadedModules, failed setting up connection");
    return false;
  }
  
  while (XBDM_NOERR == xbres)
  {
    CModule module;
    module.loadAddress = (DWORD)modLoad.BaseAddress;
    module.name = modLoad.Name;
    module.size = modLoad.Size;
    module.signature = modLoad.TimeStamp;
    
    modules.push_back(module);
    
    xbres = DmWalkLoadedModules(&pWalkMod, &modLoad);
  }
  DmCloseLoadedModules(pWalkMod);
  
  return true;
}
