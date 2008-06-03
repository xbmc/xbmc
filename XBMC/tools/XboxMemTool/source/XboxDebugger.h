
#pragma once

#include "Module.h"
#include "SnapShot.h"

class CXboxDebugger
{
public:
  CXboxDebugger(const char* target = NULL, const char* module = NULL);
  ~CXboxDebugger();
  
  BOOL Connect();
  void Disconnect();
  
  bool GetLoadedModules(std::vector<CModule>& modules);

private:
  std::string m_target;
  std::string m_module;
  
  BOOL m_isConnected;
};
