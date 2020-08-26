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

  /*!
   * \brief Makes the overlay collection sequential by ensuring only one overlay exists at a given time.
   * This is done by modifying individual overlay's start and stop pts or by removing any overlapping overlays if another
   * one exists on the same time frame.
   * It is useful for cases such as ASS subs in which libass renders all existing overlays provided the player pts, In such cases,
   * as long as the library is informed that an overlay exists on a specific time frame, all images will  be rendered at a single pass.
  */
  void MakeSequential();

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

