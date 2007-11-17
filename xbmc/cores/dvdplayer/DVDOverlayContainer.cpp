
#include "stdafx.h"
#include "DVDOverlayContainer.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"


CDVDOverlayContainer::CDVDOverlayContainer()
{
  m_overlays.clear();
  InitializeCriticalSection(&m_critSection);
}

CDVDOverlayContainer::~CDVDOverlayContainer()
{
  Clear();

  DeleteCriticalSection(&m_critSection);
}

void CDVDOverlayContainer::Add(CDVDOverlay* pOverlay)
{
  pOverlay->Acquire();

#ifdef DVDDEBUG_OVERLAY_TRACKER
  pOverlay->m_bTrackerReference++;
#endif

  EnterCriticalSection(&m_critSection);

  // markup any non ending overlays, to finish
  // when this new one starts, there can be
  // multiple overlays queued at same start
  // point so only stop them when we get a
  // new startpoint
  for(int i = m_overlays.size();i>0;)
  {
    i--;
    if(m_overlays[i]->iPTSStopTime != 0)
      break;

    if(m_overlays[i]->iPTSStartTime != pOverlay->iPTSStartTime)
      m_overlays[i]->iPTSStopTime = pOverlay->iPTSStartTime;
  }

  m_overlays.push_back(pOverlay);
  
  LeaveCriticalSection(&m_critSection);
}

VecOverlays* CDVDOverlayContainer::GetOverlays()
{
  return &m_overlays;
}

CDVDOverlay* CDVDOverlayContainer::Remove(CDVDOverlay* pOverlay)
{
  CDVDOverlay* pNext = NULL;
  
  EnterCriticalSection(&m_critSection);
  
  VecOverlaysIter it = m_overlays.begin();
  while (it != m_overlays.end())
  {
    if (*it == pOverlay)
    {
      it = m_overlays.erase(it);
      if (it != m_overlays.end()) pNext = *it;
    }
    else it++;
  }
    
  LeaveCriticalSection(&m_critSection);
  
#ifdef DVDDEBUG_OVERLAY_TRACKER
  pOverlay->m_bTrackerReference--;
#endif

  pOverlay->Release();

  return pNext;
}

void CDVDOverlayContainer::CleanUp(double pts)
{
  CDVDOverlay* pOverlay = NULL;
  
  EnterCriticalSection(&m_critSection);
  
  VecOverlaysIter it = m_overlays.begin();
  while (it != m_overlays.end())
  {
    pOverlay = *it;
    
    // never delete forced overlays, they are used in menu's
    // clear takes care of removing them
    // also if stoptime = 0, it means the next subtitles will use its starttime as the stoptime
    // which means we cannot delete overlays with stoptime 0
    if (!pOverlay->bForced && pOverlay->iPTSStopTime < pts && pOverlay->iPTSStopTime != 0)
    {
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, removing %d", (int)(pts / 1000));
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, remove, start : %d, stop : %d", (int)(pOverlay->iPTSStartTime / 1000), (int)(pOverlay->iPTSStopTime / 1000));
      pOverlay = Remove(pOverlay);
      continue;
    }
    else if (pOverlay->bForced)
    {
      //Check for newer replacements
      VecOverlaysIter it2 = it;
      CDVDOverlay* pOverlay2;
      bool bNewer = false;
      while (!bNewer && ++it2 != m_overlays.end())
      {
        pOverlay2 = *it2;
        if (pOverlay2->bForced && pOverlay2->iPTSStartTime <= pts) bNewer = true;
      }

      if (bNewer)
      {
        pOverlay = Remove(pOverlay);
        continue;
      }
    }
    it++;
  }
  
  LeaveCriticalSection(&m_critSection);
}

void CDVDOverlayContainer::Remove()
{
  if (m_overlays.size() > 0)
  {
    CDVDOverlay* pOverlay;

    EnterCriticalSection(&m_critSection);
    
    pOverlay = m_overlays.front();
    m_overlays.erase(m_overlays.begin());
    
    LeaveCriticalSection(&m_critSection);

#ifdef DVDDEBUG_OVERLAY_TRACKER
    pOverlay->m_bTrackerReference--;
#endif

    pOverlay->Release();
  }
}

void CDVDOverlayContainer::Clear()
{
  CLog::Log(LOGDEBUG,"clear");
  while (m_overlays.size() > 0) Remove();
}

int CDVDOverlayContainer::GetSize()
{
  return m_overlays.size();
}

bool CDVDOverlayContainer::ContainsOverlayType(DVDOverlayType type)
{
  bool result = false;
  
  EnterCriticalSection(&m_critSection);
  
  VecOverlaysIter it = m_overlays.begin();
  while (!result && it != m_overlays.end())
  {
    if (((CDVDOverlay*)*it)->IsOverlayType(type)) result = true;
    it++;
  }
  
  LeaveCriticalSection(&m_critSection);
  
  return result;
}

/*
 * iAction should be LIBDVDNAV_BUTTON_NORMAL or LIBDVDNAV_BUTTON_CLICKED
 */
void CDVDOverlayContainer::UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, CDVDDemuxSPU *pSpu, int iAction)
{
  EnterCriticalSection(&m_critSection);

  //Update any forced overlays.
  for(VecOverlays::iterator it = m_overlays.begin(); it != m_overlays.end(); it++ )     
  {
    if ((*it)->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)(*it);

      // make sure its a forced (menu) overlay
      // set menu spu color and alpha data if there is a valid menu overlay
      if (pOverlaySpu->bForced && pStream->GetCurrentGroupId() == pOverlaySpu->iGroupId)
        pStream->GetCurrentButtonInfo(pOverlaySpu, pSpu, iAction);
    }
  }
  LeaveCriticalSection(&m_critSection);
}
