
#pragma once

#include "../../dvd_config.h"

enum DVDOverlayType
{
  DVDOVERLAY_TYPE_NONE    = -1,
  DVDOVERLAY_TYPE_SPU     = 1,
  DVDOVERLAY_TYPE_TEXT    = 2
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
    
    m_references = 1;
#ifdef DVDDEBUG_OVERLAY_TRACKER
    m_bTrackerReference = 0;
#endif
    iGroupId = 0;
  }
  
  virtual ~CDVDOverlay()
  {
    // CLog::DebugLog("CDVDOverlay::CleanUp, remove, start : %d, stop : %d", (int)(iPTSStartTime / 1000), (int)(iPTSStopTime / 1000));
    assert(m_references == 0);
#ifdef DVDDEBUG_OVERLAY_TRACKER
    if (m_bTrackerReference != 0) CLog::DebugLog("CDVDOverlay::~, overlay has an invalid olverlaycontainer reference, value : %d", m_bTrackerReference);
    assert(m_bTrackerReference == 0);
#endif
  }

  /**
   * decrease the reference counter by one.   
   */
  long Acquire()
  {
    long count = InterlockedIncrement(&m_references);
    return count;
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

  long GetNrOfReferences()
  {
    return m_references;
  }
  
  bool IsOverlayType(DVDOverlayType type) { return (m_type == type); }
  
  double iPTSStartTime;
  double iPTSStopTime;
  bool bForced; // display, no matter what
  int iGroupId;
#ifdef DVDDEBUG_OVERLAY_TRACKER
  int m_bTrackerReference;
#endif
  
protected:
  DVDOverlayType m_type;

private:
  long m_references;
};

typedef std::vector<CDVDOverlay*> VecOverlays;
typedef std::vector<CDVDOverlay*>::iterator VecOverlaysIter;
