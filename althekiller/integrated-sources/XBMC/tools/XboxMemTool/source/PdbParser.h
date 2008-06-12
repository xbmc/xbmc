
#pragma once

#include "diacreate.h"

class CSymbolInfo
{
public:
  CSymbolInfo()
  {
    symbolName = L"";
    moduleName = "";
    sourceFileName = L"";
    
    startAddress = 0LL;
    endAddress = 0LL;

  }
  
  std::wstring symbolName;
  std::string moduleName;
  std::wstring sourceFileName;
  
  DWORD64 startAddress;
  DWORD64 endAddress;
};

class CPdbParser
{
public:
  CPdbParser(const char* pdbFile);
  ~CPdbParser();

  BOOL Initialize();
  void Uninitialize();
  
  void SetLoadAddress(DWORD loadAddress);
  DWORD GetSignature();
  
  bool GetSymbolInfoByVA(CSymbolInfo& info, DWORD64 addr);
  bool GetLineNumberByVA(DWORD& line, DWORD64 addr);
  
private:
  DWORD m_signature;
  std::string m_file;
  
  // dia stuff
  CComPtr<IDiaSession> m_pSession;
  CComPtr<IDiaSymbol> m_pGlobal;
};
