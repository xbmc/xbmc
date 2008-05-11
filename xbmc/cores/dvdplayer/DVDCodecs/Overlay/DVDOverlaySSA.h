#pragma once
#include "DVDOverlay.h"
#include "DVDSubtitles/DVDSubtitlesLibass.h"

class CDVDOverlaySSA : public CDVDOverlay
{
public:

  CDVDSubtitlesLibass* m_libass;

  CDVDOverlaySSA(CDVDSubtitlesLibass* libass) : CDVDOverlay(DVDOVERLAY_TYPE_SSA)
  {
    m_libass = libass;
    libass->Acquire();
  }
  
  ~CDVDOverlaySSA()
  {
    if(m_libass)
      SAFE_RELEASE(m_libass);
  }

};
