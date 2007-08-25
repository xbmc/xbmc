
#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitleLineCollection.h"

class CDVDSubtitleParserSubrip : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserSubrip(CDVDSubtitleStream* pStream, const string& strFile);
  virtual ~CDVDSubtitleParserSubrip();
  
  virtual bool Init();
  virtual void DeInit();
  virtual void Reset();
  
  virtual CDVDOverlay* Parse(__int64 iPts);

private:
  int ParseFile();
  
  CDVDSubtitleLineCollection m_collection;
};
