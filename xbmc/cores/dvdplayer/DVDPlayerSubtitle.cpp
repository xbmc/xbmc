
#include "stdafx.h"
#include "DVDPlayerSubtitle.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDSubtitles/DVDSubtitleParser.h"

CDVDPlayerSubtitle::CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer)
{
  m_pOverlayContainer = pOverlayContainer;
  
  m_pInputStream = NULL;
  m_pSubtitleFileParser = NULL;
  m_pSubtitleStream = NULL;
  m_vecSubtitleFiles.clear();
}

CDVDPlayerSubtitle::~CDVDPlayerSubtitle()
{
  DeInit();
}

bool CDVDPlayerSubtitle::Init()
{
  return true;
}

void CDVDPlayerSubtitle::DeInit()
{
  if (m_pSubtitleFileParser)
  {
    // there could be some overlays left in the overlaycontainer that will get deleted by the subtitleparser
    // just to be sure we empty that container
    
    m_pOverlayContainer->Lock();
    if (m_pOverlayContainer->GetSize() > 0)
    {
      CLog::Log(LOGWARNING, "CDVDPlayerSubtitle::DeInit, Overlays left in overlaycontainer, they will get deleted");
      m_pOverlayContainer->Clear();
    }
    m_pOverlayContainer->Unlock();
    
    m_pSubtitleFileParser->DeInit();
    delete m_pSubtitleFileParser;
    m_pSubtitleFileParser = NULL;
  }
  
  if (m_pSubtitleStream)
  {
    m_pSubtitleStream->Close();
    delete m_pSubtitleStream;
    m_pSubtitleStream = NULL;
  }
  
  if (m_pInputStream)
  {
    m_pInputStream->Close();
    delete m_pInputStream;
    m_pInputStream = NULL;
  }
  
  m_vecSubtitleFiles.clear();
}
  
void CDVDPlayerSubtitle::Flush()
{
  if (m_pSubtitleFileParser) m_pSubtitleFileParser->Reset();
}

void CDVDPlayerSubtitle::FindSubtitles(const char* strFilename)
{
  // find subtitles
  CDVDFactorySubtitle::GetSubtitles(m_vecSubtitleFiles, strFilename);
}

void CDVDPlayerSubtitle::Process(__int64 pts)
{
  if (m_vecSubtitleFiles.size() > 0)
  {
    // we have a number of possible valid subtitles
    // for now pick the first one
    if (!m_pSubtitleFileParser)
    {
      bool bError = false;
      
      m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, (m_vecSubtitleFiles[0]).c_str(), "");
      if (!m_pInputStream)
      {
        bError = true;
        CLog::Log(LOGERROR, "CDVDPlayerSubtitle::Process, Unable to create input stream");
      }
      else
      {
        m_pSubtitleStream = new CDVDSubtitleStream(m_pInputStream);
        if (!m_pSubtitleStream)
        {
          bError = true;
          CLog::Log(LOGERROR, "CDVDPlayerSubtitle::Process, Unable to create subtitle stream");
        }
        else
        {
          m_pSubtitleFileParser = CDVDFactorySubtitle::CreateParser(m_pSubtitleStream, (m_vecSubtitleFiles[0]).c_str());
          if (!m_pSubtitleFileParser)
          {
            bError = true;
            CLog::Log(LOGERROR, "CDVDPlayerSubtitle::Process, Unable to create subtitle parser");
          }
          else
          {
            if (!m_pSubtitleFileParser->Init())
            {
              bError = true;
              CLog::Log(LOGERROR, "CDVDPlayerSubtitle::Process, Unable to init subtitle parser");
            }
          }
        }
      }
      
      if (bError)
      {
        // clear subtitles vector to prevent process from trying it again
        m_vecSubtitleFiles.clear();
        DeInit();
      }
    }
    
    if (m_pSubtitleFileParser && m_pOverlayContainer->GetSize() < 5 && pts != DVD_NOPTS_VALUE)
    {
      CDVDOverlay* pOverlay = m_pSubtitleFileParser->Parse(pts);
      if (pOverlay)
      {
        m_pOverlayContainer->Add(pOverlay);
      }
    }
  }
}

bool CDVDPlayerSubtitle::GetCurrentSubtitle(CStdString& strSubtitle, __int64 pts)
{
  strSubtitle = "";
  bool bGotSubtitle = false;
  
  Process(pts); // TODO: move to separate thread?

  m_pOverlayContainer->Lock();
  VecOverlays* pOverlays = m_pOverlayContainer->GetOverlays();
  if (pOverlays && pOverlays->size() > 0)
  {
    vector<CDVDOverlay*>::reverse_iterator it = pOverlays->rbegin();
    while (!bGotSubtitle && it != pOverlays->rend())
    {
      CDVDOverlay* pOverlay = *it;

      if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_TEXT) &&
          (pOverlay->iPTSStartTime <= pts && (pOverlay->iPTSStopTime >= pts || pOverlay->iPTSStopTime == 0LL)))
      {
        CDVDOverlayText* pOverlayText = (CDVDOverlayText*)pOverlay;
        
        CDVDOverlayText::CElement* e = pOverlayText->m_pHead;
        while (e)
        {
          if (e->IsElementType(CDVDOverlayText::ELEMENT_TYPE_TEXT))
          {
            CDVDOverlayText::CElementText* t = (CDVDOverlayText::CElementText*)e;
            strSubtitle += t->m_text; // FIXME: currently cast to char * instead of wchar* since StdString is spewing ASSERTs
            if (e->pNext) strSubtitle += "\n";
          }
          e = e->pNext;
        }
        
        bGotSubtitle = true;
      }
      it++;
    }
  }
  m_pOverlayContainer->Unlock();
  
  return bGotSubtitle;
}

int CDVDPlayerSubtitle::GetSubtitleCount()
{
  return m_vecSubtitleFiles.size();
}
