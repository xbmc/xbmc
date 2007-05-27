
#pragma once

#include "DVDCodecs/Overlay/DVDOverlay.h"

class CDVDOverlayContainer
{
public:
  CDVDOverlayContainer();
  virtual ~CDVDOverlayContainer();

  void Lock()   { EnterCriticalSection(&m_critSection); }
  void Unlock() { LeaveCriticalSection(&m_critSection); }

  void Add(CDVDOverlay* pPicture); // add a overlay to the fifo
  
  VecOverlays* GetOverlays(); // get the first overlay in this fifo
  bool ContainsOverlayType(DVDOverlayType type);
  
  void Remove(); // remove the first overlay in this fifo
  
  void Clear(); // clear the fifo and delete all overlays
  void CleanUp(__int64 pts); // validates all overlays against current pts
  int GetSize();

private:
  CDVDOverlay* Remove(CDVDOverlay* pOverlay); // removes a specific overlay
  
  VecOverlays m_overlays;
  
  CRITICAL_SECTION m_critSection;
};
