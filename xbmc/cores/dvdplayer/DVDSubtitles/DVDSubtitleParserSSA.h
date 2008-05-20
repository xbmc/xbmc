#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitleLineCollection.h"
#include "DVDSubtitlesLibass.h"
#include "DVDStreamInfo.h"


class CDVDSubtitleParserSSA : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserSSA(CDVDSubtitleStream* pStream, const std::string& strFile);
  virtual ~CDVDSubtitleParserSSA();

  virtual bool Open(CDVDStreamInfo &hints);
  virtual void Dispose();
  virtual void Reset();

  virtual CDVDOverlay* Parse(double iPts);

private:
  CDVDSubtitleLineCollection m_collection;
  CDVDSubtitlesLibass* m_libass;
};
