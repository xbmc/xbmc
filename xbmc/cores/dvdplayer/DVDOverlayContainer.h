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

  void Remove(); // remove the first overlay in this fifo

  void Clear(); // clear the fifo and delete all overlays
  void CleanUp(double pts); // validates all overlays against current pts
  int GetSize();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, CDVDDemuxSPU *pSpu, int iAction);
private:
  VecOverlaysIter Remove(VecOverlaysIter itOverlay); // removes a specific overlay

  VecOverlays m_overlays;
};
