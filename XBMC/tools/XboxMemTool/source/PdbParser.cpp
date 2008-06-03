
#include "..\stdafx.h"
#include "PdbParser.h"

CPdbParser::CPdbParser(const char* file)
{
  m_file = file;
}

CPdbParser::~CPdbParser()
{
  Uninitialize();
}

BOOL CPdbParser::Initialize()
{
  HRESULT hr;
  
  CComPtr<IDiaDataSource> pSource;
  // Initialize The Component Object Module Library
  // Obtain Access To The Provider
  hr = CoCreateInstance( CLSID_DiaSource, NULL, CLSCTX_INPROC_SERVER, __uuidof( IDiaDataSource ), (void **)&pSource);

  if (FAILED(hr))
  { 
    g_log.Log(LOG_ERROR, "Could not CoCreate CLSID_DiaSource. Register msdia71.dll." );
    return FALSE;
  }

  wchar_t wszFilename[_MAX_PATH];
  mbstowcs(wszFilename, m_file.c_str(), sizeof(wszFilename) /sizeof( wszFilename[0]));

  if (FAILED(pSource->loadDataFromPdb(wszFilename))) 
  {
    {
      g_log.Log(LOG_ERROR, "error loading pdb file : %s", m_file.c_str());
      return FALSE;
    }
  }

  if(FAILED(pSource->openSession(&m_pSession))) 
  {
    g_log.Log(LOG_ERROR, "error opening Session");
    return FALSE;
  }
  
  if(FAILED(m_pSession->get_globalScope(&m_pGlobal)))
  {
    g_log.Log(LOG_ERROR, "error get_globalScope");
    return FALSE;
  }
  
  DWORD id = 0;
  m_pGlobal->get_symIndexId(&id);
  if(id == 0)
  {
    g_log.Log(LOG_ERROR, "error get_indexId");
    return FALSE;
  }

  m_pGlobal->get_signature(&m_signature);

  return TRUE;
}

void CPdbParser::Uninitialize()
{
  // dia stff
  m_pSession = NULL;
  m_pGlobal = NULL;
}

DWORD CPdbParser::GetSignature()
{
  return m_signature;
}

void CPdbParser::SetLoadAddress(DWORD loadAddress)
{
  m_pSession->put_loadAddress(loadAddress);
}

bool CPdbParser::GetSymbolInfoByVA(CSymbolInfo& info, DWORD64 addr)
{
  CComPtr<IDiaSymbol> pSymbol;
  
  // we only want function names
  enum SymTagEnum symTag = SymTagFunction;
  HRESULT res = m_pSession->findSymbolByVA(addr, symTag, &pSymbol);
  if (!SUCCEEDED(res) || pSymbol == NULL)
  {
    // ok, didn't work. SymTagFunction seems to fail for decorated functions names in 3th party libraries
    // For now fall back on SymTagNull but we could be more specific here.
    symTag = SymTagNull;
    res = m_pSession->findSymbolByVA(addr, symTag, &pSymbol);

  }
  
  if (SUCCEEDED(res) && pSymbol != NULL)
  {
    BSTR name = NULL;
    DWORD64 length = 0;
    DWORD64 startAddress = 0;

    HRESULT hRes;
    {
      if (symTag == SymTagNull)
      {
        hRes = pSymbol->get_undecoratedName(&name);
      }
      if (symTag != SymTagNull || hRes != S_OK)
      {
        hRes = pSymbol->get_name(&name);
      }

      if(hRes == S_OK && name != NULL)
      {
        info.symbolName = name;
      }
    }
    
    if(pSymbol->get_virtualAddress(&startAddress) == S_OK)
    {
      info.startAddress = startAddress;
      if(pSymbol->get_length(&length) == S_OK)
      {
        info.endAddress = startAddress + length;
      }
    }

    // some sourcefile information
    {
      IDiaEnumLineNumbers* pEnumLineNumbers = NULL;
      
      // 1 means findLinesByRVA should return all line numbers it can find in the byte range
      // from addr to addr + 1, which will always be one
      if (m_pSession->findLinesByVA(startAddress, 1, &pEnumLineNumbers) == S_OK && pEnumLineNumbers != NULL)
      {
        IDiaLineNumber* pDiaLine = NULL;
        IDiaSourceFile* pDiaSourceFile = NULL;
        
        if (pEnumLineNumbers->Item(0, &pDiaLine) == S_OK)
        {
          if (pDiaLine->get_sourceFile(&pDiaSourceFile) == S_OK && pDiaSourceFile != NULL)
          {
            name = NULL;
            if (pDiaSourceFile->get_fileName(&name) == S_OK && name != NULL)
            {
              info.sourceFileName = name;
            }
          }
        }
      }
    }

    pSymbol = 0;
    
    return true;
  }
  
  return false;
}

bool CPdbParser::GetLineNumberByVA(DWORD& line, DWORD64 addr)
{
  IDiaEnumLineNumbers* pEnumLineNumbers = NULL;
      
  // 1 means findLinesByRVA should return all line numbers it can find in the byte range
  // from addr to addr + 1, which will 'hopefully' always be one
  if (m_pSession->findLinesByVA(addr, 1, &pEnumLineNumbers) == S_OK && pEnumLineNumbers != NULL)
  {
    IDiaLineNumber* pDiaLine = NULL;
    
    if (pEnumLineNumbers->Item(0, &pDiaLine) == S_OK)
    {
      DWORD lineNumber = -1;
      if (pDiaLine->get_lineNumber(&lineNumber) == S_OK && line >= 0)
      {
        line = lineNumber;
        return true;
      }
    }
  }
  
  return false;
}
