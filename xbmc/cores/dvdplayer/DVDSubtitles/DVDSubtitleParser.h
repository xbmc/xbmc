
#pragma once

#include "../DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleStream.h"

class CDVDSubtitleParser
{
public:
  CDVDSubtitleParser(CDVDSubtitleStream* pStream, const char* strFile)
  {
    m_pStream = pStream;
    m_strFileName = strdup(strFile);
  }
  
  virtual ~CDVDSubtitleParser()
  {
    //DeInit();
    if (m_strFileName) free(m_strFileName);
  }
  
  virtual bool Init() = 0;
  virtual void DeInit() = 0;
  virtual void Reset() = 0;
  
  virtual CDVDOverlay* Parse(__int64 iPts) = 0;
  
protected:
  CDVDSubtitleStream* m_pStream;
  char* m_strFileName;
};

