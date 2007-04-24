
#include "..\stdafx.h"
#include "SymbolLookupHelper.h"

CSymbolLookupHelper::CSymbolLookupHelper()
{
  loadAddress = 0;
  m_pDebugger = new CXboxDebugger();
  m_pDebugger->Connect();
}

CSymbolLookupHelper::~CSymbolLookupHelper()
{
  m_pDebugger->Disconnect();
  delete m_pDebugger;

  UnloadModules();
}

bool CSymbolLookupHelper::LoadModules(std::vector<std::string>& pdbFiles)
{
  bool result = true;
  
  UnloadModules();
  if (m_pDebugger->GetLoadedModules(m_modules))
  {
    int size = pdbFiles.size();
    for (int i = 0; i < size; i++)
    {
      std::string& file = pdbFiles[i];
      if (!RegisterPdbFile(file.c_str()))
      {
        // do nothing for now, let the other files load
        g_log.Log(LOG_ERROR, "Failed to register file : %s", file.c_str());
        result = false;
      }
    }
  }
  
  return result;
}

void CSymbolLookupHelper::UnloadModules()
{
  int size = m_modules.size();
  for (int i = 0; i < size; i++)
  {
    CModule& module = m_modules[i];
    CPdbParser* pParser = module.pParser;
    if (pParser != NULL)
    {
      pParser->Uninitialize();
      delete pParser;
    }
  }
  m_modules.clear();
}

bool CSymbolLookupHelper::RegisterPdbFile(const char* pdbFile)
{
  CPdbParser* pParser = new CPdbParser(pdbFile);
  if (pParser->Initialize())
  {
    // some checks if this is the correct file, and to which module it belongs
    DWORD signature = pParser->GetSignature();
    int size = m_modules.size();
    for (int i = 0; i < size; i++)
    {
      CModule& module = m_modules[i];
      
      if (signature == module.signature)
      {
        if (module.pParser != NULL)
        {
          pParser->Uninitialize();
          delete pParser;
        }
        
        module.pParser = pParser;
        pParser->SetLoadAddress(module.loadAddress);
        
        if (module.loadAddress < 0x100000)
        {
          loadAddress = module.loadAddress;
        }
        return true;
      }
    }
    
    g_log.Log(LOG_ERROR, "No corresponding module found on the xbox for pdb file : %s", pdbFile);
    
  }

  pParser->Uninitialize();
  delete pParser;
  
  return false;
}

void CSymbolLookupHelper::UnregisterPdbFile(const char* pdbFile)
{
}
  
bool CSymbolLookupHelper::GetSymbolInfoByVA(CSymbolInfo& info, DWORD64 addr)
{
  // first determine in which module the address is.
  int size = m_modules.size();
  for (int i = 0; i < size; i++)
  {
    CModule& module = m_modules[i];
    DWORD start = module.loadAddress;
    DWORD end = module.loadAddress + module.size;
    
    if (addr >= start && addr < end)
    {
      // this is the module, retrieve the symbol
      if (module.pParser)
      {
        info.moduleName = module.name;
        bool res = module.pParser->GetSymbolInfoByVA(info, addr);
        return res;
      }
    }
  }
  
  return false;
}

bool CSymbolLookupHelper::GetModuleNameByVA(std::string& name, DWORD64 addr)
{
  // first determine in which module the address is.
  int size = m_modules.size();
  for (int i = 0; i < size; i++)
  {
    CModule& module = m_modules[i];
    DWORD start = module.loadAddress;
    DWORD end = module.loadAddress + module.size;
    
    if (addr >= start && addr < end)
    {
      // this is the module
      name = module.name;
      return true;
    }
  }
  
  return false;
}

bool CSymbolLookupHelper::GetLineNumberByVA(DWORD& line, DWORD64 addr)
{
  // first determine in which module the address is.
  int size = m_modules.size();
  for (int i = 0; i < size; i++)
  {
    CModule& module = m_modules[i];
    DWORD start = module.loadAddress;
    DWORD end = module.loadAddress + module.size;
    
    if (addr >= start && addr < end)
    {
      // this is the module, retrieve the symbol
      if (module.pParser)
      {
        bool res = module.pParser->GetLineNumberByVA(line, addr);
        return res;
      }
    }
  }
  
  return false;
}

DWORD CSymbolLookupHelper::GetLoadAddress()
{
  return loadAddress;
}