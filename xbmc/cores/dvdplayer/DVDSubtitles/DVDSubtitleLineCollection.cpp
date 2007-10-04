
#include "stdafx.h"
#include "DVDSubtitleLineCollection.h"


CDVDSubtitleLineCollection::CDVDSubtitleLineCollection()
{
  m_pHead = NULL;
  m_pCurrent = NULL;
  
  m_iSize = 0;
}

CDVDSubtitleLineCollection::~CDVDSubtitleLineCollection()
{
  Clear();
}

void CDVDSubtitleLineCollection::Add(CDVDOverlay* pOverlay)
{
  ListElement* pElement = new ListElement;
  pElement->pOverlay = pOverlay;
  pElement->pNext = NULL;

  if (!m_pHead)
  {
    m_pHead = pElement;
    m_pCurrent = m_pHead;
  }
  else
  {
    ListElement* pIt = m_pHead;

    while (pIt->pNext) pIt = pIt->pNext;
              
    pIt->pNext = pElement;
  }
  
  m_iSize++;
}

CDVDOverlay* CDVDSubtitleLineCollection::Get(double iPts)
{
  CDVDOverlay* pOverlay = NULL;
  
  if (m_pCurrent)
  {
    while (m_pCurrent && m_pCurrent->pOverlay->iPTSStartTime < iPts)
    {
      m_pCurrent = m_pCurrent->pNext;
    }
    
    pOverlay = m_pCurrent->pOverlay;

    // advance to the next overlay
    m_pCurrent = m_pCurrent->pNext;
  }
  return pOverlay;
}

void CDVDSubtitleLineCollection::Reset()
{
  m_pCurrent = m_pHead;
}

void CDVDSubtitleLineCollection::Clear()
{
  ListElement* pElement = NULL;
  
  while (m_pHead)
  {
    pElement = m_pHead;
    m_pHead = pElement->pNext;

    pElement->pOverlay->Release();
    delete pElement;
  }
  
  m_iSize = 0;
}
