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
