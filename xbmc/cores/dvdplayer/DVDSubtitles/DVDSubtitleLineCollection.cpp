/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDSubtitleLineCollection.h"
#include "DVDClock.h"


CDVDSubtitleLineCollection::CDVDSubtitleLineCollection()
{
  m_pHead = NULL;
  m_pCurrent = NULL;
  m_pTail = NULL;

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
    m_pHead = m_pTail = pElement;
    m_pCurrent = m_pHead;
  }
  else
  {
    m_pTail->pNext = pElement;
    m_pTail = pElement;
  }

  m_iSize++;
}

void CDVDSubtitleLineCollection::Sort()
{
  if (!m_pHead || !m_pHead->pNext)
    return;
  
  for (ListElement* p1 = m_pHead; p1->pNext->pNext != NULL; p1 = p1->pNext)
  {
    for (ListElement* p2 = p1->pNext; p2->pNext != NULL; p2 = p2->pNext)
    {
      if (p1->pOverlay->iPTSStartTime > p2->pOverlay->iPTSStartTime)
      {
        CDVDOverlay* temp = p1->pOverlay;
        p1->pOverlay = p2->pOverlay;
        p2->pOverlay = temp;
      }
    }
  }
}

CDVDOverlay* CDVDSubtitleLineCollection::Get(double iPts)
{
  CDVDOverlay* pOverlay = NULL;

  if (m_pCurrent)
  {
    while (m_pCurrent && m_pCurrent->pOverlay->iPTSStopTime < iPts)
    {
      m_pCurrent = m_pCurrent->pNext;
    }

    if (m_pCurrent)
    {
      pOverlay = m_pCurrent->pOverlay;

      // advance to the next overlay
      m_pCurrent = m_pCurrent->pNext;
    }
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

  m_pTail    = NULL;
  m_pHead    = NULL;
  m_pCurrent = NULL;
  m_iSize    = 0;
}
