
#pragma once

#include "DVDOverlayContainer.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"

class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;

class CDVDPlayerSubtitle
{
public:
  CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer);
  ~CDVDPlayerSubtitle();

  bool Init();
  void DeInit();
  
  void Process(__int64 pts);
  void Flush();
  void FindSubtitles(const char* strFilename);
  bool GetCurrentSubtitle(CStdString& strSubtitle, __int64 pts);
  int GetSubtitleCount();
  
private:
  CDVDOverlayContainer* m_pOverlayContainer;
  
  VecSubtitleFiles    m_vecSubtitleFiles;

  CDVDInputStream*    m_pInputStream;
  CDVDSubtitleStream* m_pSubtitleStream;
  CDVDSubtitleParser* m_pSubtitleFileParser;
};


//typedef struct SubtitleInfo
//{

//
//} SubtitleInfo;

