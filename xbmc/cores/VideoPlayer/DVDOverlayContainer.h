/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "threads/CriticalSection.h"

class CDVDInputStreamNavigator;
class CDVDDemuxSPU;

class CDVDOverlayContainer : public CCriticalSection
{
public:
  CDVDOverlayContainer();
  virtual ~CDVDOverlayContainer();

  void Add(CDVDOverlay* pPicture); // add a overlay to the fifo

  VecOverlays* GetOverlays(); // get the first overlay in this fifo
  bool ContainsOverlayType(DVDOverlayType type);

  void Clear(); // clear the fifo and delete all overlays
  void CleanUp(double pts); // validates all overlays against current pts
  int GetSize();

  void UpdateOverlayInfo(std::shared_ptr<CDVDInputStreamNavigator> pStream, CDVDDemuxSPU *pSpu, int iAction);
private:
  VecOverlaysIter Remove(VecOverlaysIter itOverlay); // removes a specific overlay

  VecOverlays m_overlays;
};
