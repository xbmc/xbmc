
#pragma once

class CStackTraceElement
{
public:
  CStackTraceElement()
  {
    m_pNext = NULL;
    address = 0;
  }

  CStackTraceElement(CStackTraceElement* pStackTraceElement)
  {
    m_pNext = NULL;
    this->address = pStackTraceElement->address;
  }
  
  DWORD address;
  CStackTraceElement* m_pNext;
};
  
class CStackTrace
{
public:
  CStackTrace()
  {
    m_pNext = NULL;
    m_pFirst = NULL;
    m_pLast = NULL;
    
    nrOfElements = 0;
  }

  CStackTrace(CStackTrace* pStackTrace)
  {
    this->m_pNext = NULL;
    this->m_pFirst = NULL;
    this->m_pLast = NULL;
    this->nrOfElements = 0;
    this->allocatedSize = pStackTrace->allocatedSize;
    this->address = pStackTrace->address;
    
    // copy over all elements
    CStackTraceElement* pTemp = pStackTrace->m_pFirst;
    while (pTemp != NULL)
    {
      CStackTraceElement* pNew = new CStackTraceElement(pTemp);
      this->AppendElement(pNew);
      pTemp = pTemp->m_pNext;
    }
  }
  
  ~CStackTrace()
  {
    while (m_pFirst != NULL)
    {
      CStackTraceElement* pTemp = m_pFirst;
      m_pFirst = m_pFirst->m_pNext;
      delete pTemp;
    }
  }
  
  void AppendElement(CStackTraceElement* pElement)
  {
    nrOfElements++;
    
    if (m_pFirst == NULL)
    {
      m_pFirst = pElement;
      m_pLast = pElement;
    }
    else
    {
      m_pLast->m_pNext = pElement;
      m_pLast = pElement;
    }
  }

  void AppendAddress(DWORD address)
  {
    CStackTraceElement* pStackTraceElement = new CStackTraceElement();
    pStackTraceElement->address = address;
    AppendElement(pStackTraceElement);
  }
  
  int size()
  {
    return nrOfElements;
  }
  
  CStackTraceElement& operator[] (int row)
  {
    CStackTraceElement* pTemp = m_pFirst;
    while (row > 0)
    {
      pTemp = pTemp->m_pNext;
      row--;
    }
    return *pTemp;
  }
  
  bool operator == (const CStackTrace &stackTrace) const
  {
    if (this->allocatedSize != stackTrace.allocatedSize) return false;
    if (this->address != stackTrace.address) return false;
    
    CStackTraceElement* pThis = this->m_pFirst;
    CStackTraceElement* pOther = stackTrace.m_pFirst;
    while (pThis != NULL && pOther != NULL)
    {
      if (pThis->address != pOther->address)
      {
        return false;
      }
      pThis = pThis->m_pNext;
      pOther = pOther->m_pNext;
    }
    
    // return true if both are NULL
    return (pThis == NULL && pOther == NULL);
    
  }
  
  CStackTrace* m_pNext; // used in its parent
  
  CStackTraceElement* m_pFirst;
  CStackTraceElement* m_pLast;
  
  DWORD allocatedSize;
  DWORD address; // address location of the allocated memory

private:
  int nrOfElements;
};


class CSnapShot
{
public:
  CSnapShot()
  {
    m_pFirst = NULL;
    m_pLast = NULL;
    m_totalSize = 0;
    m_totalAllocations = 0;
  }
  
  ~CSnapShot()
  {
    while (m_pFirst != NULL)
    {
      CStackTrace* pTemp = m_pFirst;
      m_pFirst = m_pFirst->m_pNext;
      delete pTemp;
    }
  }

  void AppendStackTrace(CStackTrace* pStackTrace)
  {
    if (m_pFirst == NULL)
    {
      m_pFirst = pStackTrace;
      m_pLast = pStackTrace;
    }
    else
    {
      m_pLast->m_pNext = pStackTrace;
      m_pLast = pStackTrace;
    }
  }

  bool ContainsStackTrace(CStackTrace* pStackTrace) const
  {
    CStackTrace* pTemp = m_pFirst;
    while (pTemp != NULL)
    {
      if (*pTemp == *pStackTrace)
      {
        return true;
      }
      pTemp = pTemp->m_pNext;
    }
    
    return false;
  }
  
  CSnapShot& operator = (const CSnapShot &snapShot);
  CSnapShot& operator -= (const CSnapShot &snapShot);
  const CSnapShot operator - (const CSnapShot &snapShot) const;

  CStackTrace* m_pFirst;
  CStackTrace* m_pLast;
  
  DWORD m_loadAddress;
  int m_totalSize;
  int m_totalAllocations;
};
