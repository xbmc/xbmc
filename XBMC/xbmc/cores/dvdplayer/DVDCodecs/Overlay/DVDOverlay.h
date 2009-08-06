#pragma once

#include "cores/VideoRenderers/OverlayRenderer.h"
#include <assert.h>
#include <vector>

enum DVDOverlayType
{
  DVDOVERLAY_TYPE_NONE    = -1,
  DVDOVERLAY_TYPE_SPU     = 1,
  DVDOVERLAY_TYPE_TEXT    = 2,
  DVDOVERLAY_TYPE_IMAGE   = 3,
  DVDOVERLAY_TYPE_SSA     = 4
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
    iGroupId = 0;
    m_overlay = NULL;
  }

  CDVDOverlay(const CDVDOverlay& src)
  {
    m_type        = src.m_type;
    iPTSStartTime = src.iPTSStartTime;
    iPTSStopTime  = src.iPTSStopTime;
    bForced       = src.bForced;
    replace       = src.replace;
    iGroupId      = src.iGroupId;
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
   * decrease the reference counter by one.
   */
  CDVDOverlay* Acquire()
  {
    InterlockedIncrement(&m_references);
    return this;
  }

  /**
   * increase the reference counter by one.
   */
  long Release()
  {
    long count = InterlockedDecrement(&m_references);
    if (count == 0) delete this;
    return count;
  }

  bool IsOverlayType(DVDOverlayType type) { return (m_type == type); }

  double iPTSStartTime;
  double iPTSStopTime;
  bool bForced; // display, no matter what
  bool replace; // replace by next nomatter what stoptime it has
  int iGroupId;
  OVERLAY::COverlay* m_overlay;
protected:
  DVDOverlayType m_type;

private:
  long m_references;
};

typedef std::vector<CDVDOverlay*> VecOverlays;
typedef std::vector<CDVDOverlay*>::iterator VecOverlaysIter;
