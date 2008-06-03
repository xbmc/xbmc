
#include "..\stdafx.h"
#include "SnapShot.h"

CSnapShot& CSnapShot::operator = (const CSnapShot &snapShot)
{
  // Check for self-assignment!
  if (this == &snapShot)      // Same object?
    return *this;

  this->m_loadAddress = snapShot.m_loadAddress;
  this->m_totalAllocations = snapShot.m_totalAllocations;
  this->m_totalSize = snapShot.m_totalSize;
  
  // copy the stack traces
  CStackTrace* pTemp = snapShot.m_pFirst;
  while (pTemp != NULL)
  {
    CStackTrace* pNew = new CStackTrace(pTemp);
    this->AppendStackTrace(pNew);
    
    pTemp = pTemp->m_pNext;
  }
  
  return *this;
}

CSnapShot& CSnapShot::operator -= (const CSnapShot &snapShot)
{
  //return *this;
  
  // every stack trace with the same addres that we find in snapShot
  // is removed from this one
  CStackTrace* pTemp = this->m_pFirst;
  CStackTrace* pPrevious = NULL;
  while (pTemp != NULL)
  {
    if (snapShot.ContainsStackTrace(pTemp))
    {
      CStackTrace* toRemove = pTemp;
      
      if (pTemp == this->m_pFirst)
      {
        this->m_pFirst = this->m_pFirst->m_pNext;
      }

      pTemp = pTemp->m_pNext;
      
      this->m_totalSize -= toRemove->allocatedSize;
      this->m_totalAllocations--;
      delete toRemove;
      
      if (pPrevious != NULL)
      {
        pPrevious->m_pNext = pTemp;
      }
    }
    else
    {
      pPrevious = pTemp;
      pTemp = pTemp->m_pNext;
    }
  }
  
  return *this;
}

const CSnapShot CSnapShot::operator - (const CSnapShot &snapShot) const
{
  CSnapShot result = *this;
  result -= snapShot;
  return result;
}
