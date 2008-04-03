
#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitleLineCollection.h"

class CDVDSubtitleParserSami : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserSami(CDVDSubtitleStream* pStream, const std::string& strFile);
  virtual ~CDVDSubtitleParserSami();
  
  virtual bool Open(CDVDStreamInfo &hints);
  virtual void Dispose();
  virtual void Reset();
  
  virtual CDVDOverlay* Parse(double iPts);

private:
  int ParseFile();
  CDVDSubtitleLineCollection m_collection;
};
