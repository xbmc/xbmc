
#include "..\stdafx.h"
#include "CallerTree.h"

CCallerTree::CCallerTree(CSymbolLookupHelper* pHelper)
{
  m_pSymbolLookupHelper = pHelper;
  m_pTrackCall = NULL;

  for (int i = 0; i <= 0xFF; i++)
  {
    m_pTrackFunction[i] = NULL;
  }
  
  m_pTopCall = NULL;
}

CCallerTree::~CCallerTree()
{
  Clear();
}


  
void CCallerTree::Clear()
{/*
  CTreeCall* pCall = NULL;
  CTreeFunction* pFunction = NULL;
  
  std::list<CTreeCall*>::iterator callIter;
  for (callIter = m_calls.begin(); callIter != m_calls.end(); callIter++)
  {
    pCall = *callIter;
    delete pCall;
  }
  m_calls.clear();

  std::list<CTreeFunction*>::iterator functionIter;
  for (functionIter = m_functions.begin(); functionIter != m_functions.end(); functionIter++)
  {
    pFunction = *functionIter;
    delete pFunction;
  }
  m_functions.clear();*/
}

// this could use some optimisation
void CCallerTree::ProcessSnapShot(CSnapShot* pSnapShot)
{
  Clear();
  
  CTreeCall* pCall = NULL;
  
  // create an single entrance point for the tree
  m_pTopCall = new CTreeCall();
  m_pTopCall->address = 0;
        
  // first we create the entire call tree
  // calls represent the addresses given in a CTraceElement's stack strace
  for (CStackTrace* pStackTrace = pSnapShot->m_pFirst; pStackTrace != NULL; pStackTrace = pStackTrace->m_pNext)
  {
    int stackSize = pStackTrace->size();
    
    CTreeCall* pCurrentTreeCall = m_pTopCall; // first one, no usefull information
    
    for (int s = stackSize - 1; s >= 0; s--)
    {
      DWORD address = (*pStackTrace)[s].address;
      
      CTreeFunction* pFunctionCalled = NULL;
      DWORD addressCalled = 0;
      if (s > 0)
      {
        addressCalled = (*pStackTrace)[s - 1].address;
        pFunctionCalled = getOrCreateFunction(addressCalled);
      }

      if (s == (stackSize - 1))
      {
        // if s == 0, pCurrentTreeCall == m_pTopCall
        CTreeFunction* pFunction = getOrCreateFunction(address);

        CTreeCall* pCall = pCurrentTreeCall->GetChildByFunction(pFunction);
        if (pCall == NULL)
        {
          pCall = new CTreeCall();
          pCall->address = pFunction->m_startAddress;

          pCall->m_pFunction = pFunction;

          pCurrentTreeCall->AddChild(pCall);
        }
        pCurrentTreeCall = pCall;
      }
      
      // the calls are a bit more difficult because we want to create a tree of calls
      // creating a nice tree is done as follows
      // 1. the top of the tree correspondents to the last stack element (the main in an application)
      // 2. the second last element is the call made to a function from main
      // 3. look at the children in the top element for a corresponding address call
      // 4. if one is found, go back to 3 again for this element
      // 5. if none is found, create a new call
      // 6. all remaining calls in the stacktrace result in new TreeCAlls, even if some exist in the tree
      //    with the same address. This is because it represents a new memory allocation
      
      //CTreeCall* pCall = pCurrentTreeCall->GetChild(pFunctionCalled);
      CTreeCall* pCall = pCurrentTreeCall->GetChildByAddress(address);

      if (pCall == NULL)
      {
        pCall = new CTreeCall();
        pCall->address = address;

        pCall->m_pFunction = pFunctionCalled;
        pCall->m_pFromCall = pCurrentTreeCall;

        DWORD line = 0;
        if (m_pSymbolLookupHelper->GetLineNumberByVA(line, address))
        {
          pCall->line = line;
        }
        pCurrentTreeCall->AddChild(pCall);
      }

      // the last (or the first) element in the stacktrace holds the amount of
      // data allocated,
      if (s == 0)
      {
        pCall->allocatedSize += pStackTrace->allocatedSize;
        pCall->nrOfAllocations++;
      }
        
      // advance to the next element in the stack trace
      pCurrentTreeCall = pCall;
    }
  }
}

CTreeFunction* CCallerTree::getOrCreateFunction(DWORD address)
{
  // determine the slot to be used
  unsigned int slot = address & 0xFF;

  CTreeFunction* pFunction = NULL;
  if (m_pTrackFunction[slot] != NULL)
  {
    bool bFound = false;
    pFunction = m_pTrackFunction[slot];
    while (pFunction != NULL)
    {
      if (address >= pFunction->m_startAddress && address < pFunction->m_endAddress)
      {
        // found
        bFound = true;
        break;
      }
      pFunction = pFunction->m_pTrack;
    }
    
    // not found
    if (!bFound)
    {
      pFunction = NULL;
    }
  }
  
  // none found, create a new function
  if (pFunction == NULL)
  {
    pFunction = new CTreeFunction();

    // get some information from the pdb file
    CSymbolInfo info;
    if (m_pSymbolLookupHelper->GetSymbolInfoByVA(info, address))
    {
      pFunction->functionName = info.symbolName;
      pFunction->m_startAddress = info.startAddress;
      pFunction->m_endAddress = info.endAddress;
      pFunction->moduleName = info.moduleName;
      pFunction->sourceFileName = info.sourceFileName;
    }
    else
    {
      // set as much info as possible
      std::string moduleName;
      if (m_pSymbolLookupHelper->GetModuleNameByVA(moduleName, address))
      {
        pFunction->moduleName = moduleName;
      }
      pFunction->m_startAddress = address; // incosistent, but better then nothing
    }
    
    // save it to one big list
    if (m_pTrackFunction[slot] == NULL)
    {
      m_pTrackFunction[slot] = pFunction;
    }
    else
    {
      pFunction->m_pTrack = m_pTrackFunction[slot];
      m_pTrackFunction[slot] = pFunction;
    }
  }
  return pFunction;
}
