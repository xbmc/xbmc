
#pragma once

#include "PdbParser.h"
#include "XboxDebugger.h"
#include "Module.h"

class CSymbolLookupHelper
{
public:
  CSymbolLookupHelper();
  ~CSymbolLookupHelper();

  bool LoadModules(std::vector<std::string>& pdbFiles);
  void UnloadModules();
  
  bool RegisterPdbFile(const char* pdbFile);
  void UnregisterPdbFile(const char* pdbFile);

  bool GetSymbolInfoByVA(CSymbolInfo& info, DWORD64 addr);
  bool GetModuleNameByVA(std::string& name, DWORD64 addr);
  bool GetLineNumberByVA(DWORD& line, DWORD64 addr);
  
  DWORD GetLoadAddress();
  
private:

  CXboxDebugger* m_pDebugger;
  
  std::vector<CModule> m_modules;
  DWORD loadAddress;
};
