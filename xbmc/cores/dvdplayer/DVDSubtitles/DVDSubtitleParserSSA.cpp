/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "AutoPtrHandle.h"
#include "DVDSubtitleParserSSA.h"
#include "DVDOverlaySSA.h"
#include "DVDClock.h"
#include "Util.h"
#include "File.h"

using namespace std;

CDVDSubtitleParserSSA::CDVDSubtitleParserSSA(CDVDSubtitleStream* pStream, const string& strFile)
  : CDVDSubtitleParser(pStream, strFile)
{
  m_libass = new CDVDSubtitlesLibass();
}

CDVDSubtitleParserSSA::~CDVDSubtitleParserSSA()
{
  Dispose();
}

bool CDVDSubtitleParserSSA::Open(CDVDStreamInfo &hints)
{

  if(!m_libass->ReadFile(m_strFileName))
    return false;

  //Creating the overlays by going through the list of ass_events
  ass_event_t* assEvent = m_libass->GetEvents();
  int numEvents = m_libass->GetNrOfEvents();

  for(int i=0; i < numEvents; i++)
  {
    ass_event_t* curEvent =  (assEvent+i);
    if (curEvent)
    {
      CDVDOverlaySSA* ssaOverlay = new CDVDOverlaySSA(m_libass);
      ssaOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      ssaOverlay->iPTSStartTime = curEvent->Start * (DVD_TIME_BASE / 1000);
      ssaOverlay->iPTSStopTime  = (curEvent->Start + curEvent->Duration) * (DVD_TIME_BASE / 1000);

      ssaOverlay->replace = true;
      m_collection.Add(ssaOverlay);
    }
  }
  return true;
}

void CDVDSubtitleParserSSA::Dispose()
{
  if(m_libass)
  {
    SAFE_RELEASE(m_libass);
    CLog::Log(LOGINFO, "SSA Parser: Releasing reference to ASS Library");
  }

  m_collection.Clear();
}

void CDVDSubtitleParserSSA::Reset()
{
  m_collection.Reset();
}

CDVDOverlay* CDVDSubtitleParserSSA::Parse(double iPts)
{
  CDVDOverlay* pOverlay = m_collection.Get(iPts);
  return pOverlay;
}
