#pragma once

/*
 *      Copyright (C) 2006-2013 Team XBMC
 *      http://xbmc.org
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

#include "cores/VideoRenderers/OverlayRenderer.h"
#include "threads/Atomics.h"
#include <assert.h>
#include <vector>

enum DVDOverlayType
{
  DVDOVERLAY_TYPE_NONE    = -1,
  DVDOVERLAY_TYPE_SPU     = 1,
  DVDOVERLAY_TYPE_TEXT    = 2,
  DVDOVERLAY_TYPE_IMAGE   = 3,
  DVDOVERLAY_TYPE_SSA     = 4,
  DVDOVERLAY_TYPE_GROUP   = 5,
};

class CDVDOverlay
{
public:
  CDVDOverlay(DVDOverlayType type)
  {
    m_type = type;

    iPTSStartTime = 0LL;
    iPTSStopTime = 0LL;
    bForced = false;
    replace = false;

    m_references = 1;
    m_overlay = NULL;
  }

  CDVDOverlay(const CDVDOverlay& src)
  {
    m_type        = src.m_type;
    iPTSStartTime = src.iPTSStartTime;
    iPTSStopTime  = src.iPTSStopTime;
    bForced       = src.bForced;
    replace       = src.replace;
    if(src.m_overlay)
      m_overlay   = src.m_overlay->Acquire();
    else
      m_overlay   = NULL;
    m_references  = 1;
  }

  virtual ~CDVDOverlay()
  {
    assert(m_references == 0);
    if(m_overlay)
      m_overlay->Release();
  }

  /**
  * increase the reference counter by one.
  */
  CDVDOverlay* Acquire()
  {
    AtomicIncrement(&m_references);
    return this;
  }

  /**
  * decrease the reference counter by one.
  */
  long Release()
  {
    long count = AtomicDecrement(&m_references);
    if (count == 0) delete this;
    return count;
  }

  /**
   * static release function for use with shared ptr for example
   */
  static void Release(CDVDOverlay* ov)
  {
    ov->Release();
  }

  bool IsOverlayType(DVDOverlayType type) { return (m_type == type); }

  /**
   * return a copy to DVDPlayerSubtitle in order to have hw resources cleared
   * after rendering
   */
  virtual CDVDOverlay* Clone() { return Acquire(); }

  double iPTSStartTime;
  double iPTSStopTime;
  bool bForced; // display, no matter what
  bool replace; // replace by next nomatter what stoptime it has
  OVERLAY::COverlay* m_overlay;
protected:
  DVDOverlayType m_type;

private:
  long m_references;
};

typedef std::vector<CDVDOverlay*> VecOverlays;
typedef std::vector<CDVDOverlay*>::iterator VecOverlaysIter;


class CDVDOverlayGroup : public CDVDOverlay
{

public:
  virtual ~CDVDOverlayGroup()
  {
    for(VecOverlaysIter it = m_overlays.begin(); it != m_overlays.end(); ++it)
      (*it)->Release();
    m_overlays.clear();
  }

  CDVDOverlayGroup()
    : CDVDOverlay(DVDOVERLAY_TYPE_GROUP)
  {
  }

  CDVDOverlayGroup(CDVDOverlayGroup& src)
    : CDVDOverlay(src)
  {
    for(VecOverlaysIter it = src.m_overlays.begin(); it != src.m_overlays.end(); ++it)
      m_overlays.push_back((*it)->Clone());
  }
  VecOverlays m_overlays;
};
