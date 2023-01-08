/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../DVDCodecs/Overlay/DVDOverlay.h"

typedef struct stListElement
{
  std::shared_ptr<CDVDOverlay> pOverlay;
  struct stListElement* pNext;

} ListElement;

class CDVDSubtitleLineCollection
{
public:
  CDVDSubtitleLineCollection();
  virtual ~CDVDSubtitleLineCollection();

  //void Lock()   { EnterCriticalSection(&m_critSection); }
  //void Unlock() { LeaveCriticalSection(&m_critSection); }

  void Add(std::shared_ptr<CDVDOverlay> pSubtitle);
  void Sort();

  std::shared_ptr<CDVDOverlay> Get(double iPts = 0LL); // get the first overlay in this fifo

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

