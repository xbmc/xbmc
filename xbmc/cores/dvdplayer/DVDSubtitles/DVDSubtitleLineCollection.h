#pragma once

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

#include "../DVDCodecs/Overlay/DVDOverlay.h"

typedef struct stListElement
{
  CDVDOverlay* pOverlay;
  struct stListElement* pNext;

} ListElement;

class CDVDSubtitleLineCollection
{
public:
  CDVDSubtitleLineCollection();
  virtual ~CDVDSubtitleLineCollection();

  //void Lock()   { EnterCriticalSection(&m_critSection); }
  //void Unlock() { LeaveCriticalSection(&m_critSection); }

  void Add(CDVDOverlay* pSubtitle);
  void Sort();

  CDVDOverlay* Get(double iPts = 0LL); // get the first overlay in this fifo

  void Reset();

  void Remove();
  void Clear();
  int GetSize() { return m_iSize; }

private:
  ListElement* m_pHead;
  ListElement* m_pCurrent;
  ListElement* m_pTail;

  int m_iSize;
  //CRITICAL_SECTION m_critSection;
};

