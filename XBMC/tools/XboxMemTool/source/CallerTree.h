
#pragma once

//#include "XboxMemoryTrace.h"
#include "SymbolLookupHelper.h"
#include "SnapShot.h"

class CTreeCall;
class CTreeFunction;

class CTreeFunction
{
public:
  CTreeFunction()
  {
    m_pTrack = NULL;
    m_pCallHead = NULL;
    m_startAddress = 0;
    m_endAddress = 0;
    
    functionName = L"";
    moduleName = "";
    sourceFileName = L"";
  }
  
  DWORD64 m_startAddress;
  DWORD64 m_endAddress;
  
  CTreeFunction* m_pTrack;
  std::wstring functionName;
  std::string moduleName;
  std::wstring sourceFileName;
  
private:
  CTreeCall* m_pCallHead; // start of CTreeCall child list
};

class CTreeCall
{
public:
  CTreeCall()
  {
    m_pFromCall = NULL;
    m_pFunction = NULL;
    m_pChildHead = NULL;
    m_pChildNext = NULL;
    address = 0;
    allocatedSize = 0;
    nrOfAllocations = 0;
    line = 0;
  }

  ~CTreeCall()
  {
    CTreeCall* m_pCall = m_pChildHead;
    while (m_pCall != NULL)
    {
      CTreeCall* temp = m_pCall;
      m_pCall = m_pCall->m_pChildNext;
      delete temp;
    }
  }
  
  DWORD GetFunctionOffset()
  {
    DWORD64 offset = 0;
    if (m_pFromCall != NULL && m_pFromCall->m_pFunction != NULL)
    {
      offset = m_pFromCall->address - m_pFromCall->m_pFunction->m_startAddress;
    }
    
    return (DWORD)offset;
  }
  
  CTreeCall* GetChildByFunction(CTreeFunction* pFunction)
  {
      CTreeCall* m_pCall = m_pChildHead;
      while (m_pCall != NULL && m_pCall->m_pFunction != pFunction)
      {
        m_pCall = m_pCall->m_pChildNext;
      }
      return m_pCall;
  }

  CTreeCall* GetChildByAddress(DWORD address)
  {
      CTreeCall* m_pCall = m_pChildHead;
      while (m_pCall != NULL && m_pCall->address != address)
      {
        m_pCall = m_pCall->m_pChildNext;
      }
      return m_pCall;
  }
  
  void AddChild(CTreeCall* pTreeCall)
  {
    if (pTreeCall == (CTreeCall*)0xcdcdcdcd)
    {
      OutputDebugString("abc");
    }
    if (m_pChildHead == NULL)
    {
      m_pChildHead = pTreeCall;
    }
    else
    {
      CTreeCall* m_pCall = m_pChildHead;
      while (m_pCall->m_pChildNext != NULL)
      {
        m_pCall = m_pCall->m_pChildNext;
      }
      m_pCall->m_pChildNext = pTreeCall;
    }
  }
  

  
  DWORD GetAllocatedSize()
  {
    ParseChilds();
    return allocatedSize;
  }

  DWORD GetNrOffAllocations()
  {
    ParseChilds();
    return nrOfAllocations;
  }
  
  DWORD address;
  DWORD allocatedSize;
  DWORD nrOfAllocations;
  DWORD line;
  
  CTreeCall* m_pFromCall;
  
  CTreeCall* m_pChildHead; // used by this object itself
  CTreeCall* m_pChildNext; // used by the parent of this object
  
  CTreeFunction* m_pFunction; // me not be NULL after parsing, every call, calls a function
  
private:

  
  void ParseChilds()
  {
    if (allocatedSize == 0 && nrOfAllocations == 0)
    {
      // first run
      CTreeCall* m_pCall = m_pChildHead;
      while (m_pCall != NULL)
      {
        allocatedSize += m_pCall->GetAllocatedSize();
        nrOfAllocations += m_pCall->GetNrOffAllocations();
        
        m_pCall = m_pCall->m_pChildNext;
      }
    }
  }
};

class CCallerTree
{
public:
  CCallerTree(CSymbolLookupHelper* pHelper);
  ~CCallerTree();
    
  void Clear();
  void ProcessSnapShot(CSnapShot* pSnapShot);
  
  CTreeFunction* m_pTrackFunction[0xFF + 1]; // 256 elements for faster lookup
  
  CTreeCall* m_pTopCall;
private:
  CTreeFunction* getOrCreateFunction(DWORD address);
  
  CTreeCall* m_pTrackCall;
  
  
  CSymbolLookupHelper* m_pSymbolLookupHelper;
};
