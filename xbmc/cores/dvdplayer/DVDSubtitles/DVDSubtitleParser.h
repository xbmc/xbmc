
#pragma once

#include "../DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleStream.h"

class CDVDSubtitleParser
{
public:
  CDVDSubtitleParser(CDVDSubtitleStream* pStream, const string& strFile)
  {
    m_pStream = pStream;
    m_strFileName = strFile;
  }
  
  virtual ~CDVDSubtitleParser()
  {
  }
  
  virtual bool Init() = 0;
  virtual void DeInit() = 0;
  virtual void Reset() = 0;
  
  virtual CDVDOverlay* Parse(double iPts) = 0;
  
protected:
  CDVDSubtitleStream* m_pStream;
  string m_strFileName;
};

