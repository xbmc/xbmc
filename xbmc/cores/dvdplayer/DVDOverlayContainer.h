
#pragma once

#include "DVDCodecs/Overlay/DVDOverlay.h"

class CDVDInputStreamNavigator;
class CDVDDemuxSPU;

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
  void CleanUp(double pts); // validates all overlays against current pts
  int GetSize();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, CDVDDemuxSPU *pSpu, int iAction);
private:
  CDVDOverlay* Remove(CDVDOverlay* pOverlay); // removes a specific overlay

  VecOverlays m_overlays;

  CRITICAL_SECTION m_critSection;
};
