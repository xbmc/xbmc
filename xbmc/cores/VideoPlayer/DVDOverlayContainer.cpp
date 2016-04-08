/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDOverlayContainer.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "threads/SingleLock.h"

CDVDOverlayContainer::CDVDOverlayContainer()
{
}

CDVDOverlayContainer::~CDVDOverlayContainer()
{
  Clear();
}

void CDVDOverlayContainer::Add(CDVDOverlay* pOverlay)
{
  pOverlay->Acquire();

  CSingleLock lock(*this);

  // markup any non ending overlays, to finish
  // when this new one starts, there can be
  // multiple overlays queued at same start
  // point so only stop them when we get a
  // new startpoint
  for(int i = m_overlays.size();i>0;)
  {
    i--;
    if(m_overlays[i]->iPTSStopTime)
    {
      if(!m_overlays[i]->replace)
        break;
      if(m_overlays[i]->iPTSStopTime <= pOverlay->iPTSStartTime)
        break;
    }
    if(m_overlays[i]->iPTSStartTime != pOverlay->iPTSStartTime)
      m_overlays[i]->iPTSStopTime = pOverlay->iPTSStartTime;
  }

  m_overlays.push_back(pOverlay);

}

VecOverlays* CDVDOverlayContainer::GetOverlays()
{
  return &m_overlays;
}

VecOverlaysIter CDVDOverlayContainer::Remove(VecOverlaysIter itOverlay)
{
  VecOverlaysIter itNext;
  CDVDOverlay* pOverlay = *itOverlay;

  {
    CSingleLock lock(*this);
    itNext = m_overlays.erase(itOverlay);
  }

  pOverlay->Release();

  return itNext;
}

void CDVDOverlayContainer::CleanUp(double pts)
{
  CSingleLock lock(*this);

  VecOverlaysIter it = m_overlays.begin();
  while (it != m_overlays.end())
  {
    CDVDOverlay* pOverlay = *it;

    // never delete forced overlays, they are used in menu's
    // clear takes care of removing them
    // also if stoptime = 0, it means the next subtitles will use its starttime as the stoptime
    // which means we cannot delete overlays with stoptime 0
    if (!pOverlay->bForced && pOverlay->iPTSStopTime <= pts && pOverlay->iPTSStopTime != 0)
    {
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, removing %d", (int)(pts / 1000));
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, remove, start : %d, stop : %d", (int)(pOverlay->iPTSStartTime / 1000), (int)(pOverlay->iPTSStopTime / 1000));
      it = Remove(it);
      continue;
    }
    else if (pOverlay->bForced)
    {
      //Check for newer replacements
      VecOverlaysIter it2 = it;
      bool bNewer = false;
      while (!bNewer && ++it2 != m_overlays.end())
      {
        CDVDOverlay* pOverlay2 = *it2;
        if (pOverlay2->bForced && pOverlay2->iPTSStartTime <= pts) bNewer = true;
      }

      if (bNewer)
      {
        it = Remove(it);
        continue;
      }
    }
    ++it;
  }

}

void CDVDOverlayContainer::Clear()
{
  CSingleLock lock(*this);
  for (auto &overlay : m_overlays)
  {
    overlay->Release();
  }
  m_overlays.clear();
}

int CDVDOverlayContainer::GetSize()
{
  return m_overlays.size();
}

bool CDVDOverlayContainer::ContainsOverlayType(DVDOverlayType type)
{
  bool result = false;

  CSingleLock lock(*this);

  VecOverlaysIter it = m_overlays.begin();
  while (!result && it != m_overlays.end())
  {
    if (((CDVDOverlay*)*it)->IsOverlayType(type)) result = true;
    ++it;
  }

  return result;
}

/*
 * iAction should be LIBDVDNAV_BUTTON_NORMAL or LIBDVDNAV_BUTTON_CLICKED
 */
void CDVDOverlayContainer::UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, CDVDDemuxSPU *pSpu, int iAction)
{
  CSingleLock lock(*this);

  pStream->CheckButtons();

  //Update any forced overlays.
  for(VecOverlays::iterator it = m_overlays.begin(); it != m_overlays.end(); ++it )
  {
    if ((*it)->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)(*it);

      // make sure its a forced (menu) overlay
      // set menu spu color and alpha data if there is a valid menu overlay
      if (pOverlaySpu->bForced)
      {
        if (pOverlaySpu->Acquire()->Release() > 1)
        {
          pOverlaySpu = new CDVDOverlaySpu(*pOverlaySpu);
          (*it)->Release();
          (*it) = pOverlaySpu;
        }

        if(pStream->GetCurrentButtonInfo(pOverlaySpu, pSpu, iAction))
        {
          pOverlaySpu->m_textureid = 0;
        }

      }
    }
  }
}
