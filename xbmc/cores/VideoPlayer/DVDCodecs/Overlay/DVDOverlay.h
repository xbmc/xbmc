/*
 *  Copyright (C) 2006-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <assert.h>
#include <vector>
#include <atomic>

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
  explicit CDVDOverlay(DVDOverlayType type)
  {
    m_type = type;

    iPTSStartTime = 0LL;
    iPTSStopTime = 0LL;
    bForced = false;
    replace = false;
    m_references = 1;
    m_textureid = 0;
  }

  CDVDOverlay(const CDVDOverlay& src)
  {
    m_type        = src.m_type;
    iPTSStartTime = src.iPTSStartTime;
    iPTSStopTime  = src.iPTSStopTime;
    bForced       = src.bForced;
    replace       = src.replace;
    m_references  = 1;
    m_textureid = 0;
  }

  virtual ~CDVDOverlay()
  {
    assert(m_references == 0);
  }

  /**
  * increase the reference counter by one.
  */
  CDVDOverlay* Acquire()
  {
    m_references++;
    return this;
  }

  /**
  * decrease the reference counter by one.
  */
  int Release()
  {
    m_references--;
    int ret = m_references;
    if (m_references == 0)
      delete this;
    return ret;
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
   * return a copy to VideoPlayerSubtitle in order to have hw resources cleared
   * after rendering
   */
  virtual CDVDOverlay* Clone() { return Acquire(); }

  double iPTSStartTime;
  double iPTSStopTime;
  bool bForced; // display, no matter what
  bool replace; // replace by next nomatter what stoptime it has
  unsigned long m_textureid;
protected:
  DVDOverlayType m_type;

private:
  std::atomic_int m_references;
};

typedef std::vector<CDVDOverlay*> VecOverlays;
typedef std::vector<CDVDOverlay*>::iterator VecOverlaysIter;


class CDVDOverlayGroup : public CDVDOverlay
{

public:
  ~CDVDOverlayGroup() override
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
