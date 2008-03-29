
#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitleLineCollection.h"

class CDVDSubtitleParserMicroDVD : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserMicroDVD(CDVDSubtitleStream* pStream, const std::string& strFile);
  virtual ~CDVDSubtitleParserMicroDVD();
  
  virtual bool Open(CDVDStreamInfo &hints);
  virtual void Dispose();
  virtual void Reset();
  
  virtual CDVDOverlay* Parse(double iPts);

private:
  int ParseFile();
  double m_framerate;
  CDVDSubtitleLineCollection m_collection;
};
