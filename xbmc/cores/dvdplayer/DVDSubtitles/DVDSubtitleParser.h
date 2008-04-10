
#pragma once

#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDSubtitleStream.h"

class CDVDStreamInfo;

class CDVDSubtitleParser
{
public:
  CDVDSubtitleParser(CDVDSubtitleStream* pStream, const std::string& strFile)
  {
    m_pStream = pStream;
    m_strFileName = strFile;
  }
  
  virtual ~CDVDSubtitleParser()
  {
  }
  
  virtual bool Open(CDVDStreamInfo &hints) = 0;
  virtual void Dispose() = 0;
  virtual void Reset() = 0;
  
  virtual CDVDOverlay* Parse(double iPts) = 0;
  
protected:
  CDVDSubtitleStream* m_pStream;
  std::string m_strFileName;
};

