
#pragma once

#include "DVDOverlayContainer.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxSPU.h"

class CDVDInputStream;
class CDVDSubtitleStream;
class CDVDSubtitleParser;
class CDVDInputStreamNavigator;

class CDVDPlayerSubtitle
{
public:
  CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer);
  ~CDVDPlayerSubtitle();

  void Process(double pts);
  void Flush();
  void FindSubtitles(const char* strFilename);
  bool GetCurrentSubtitle(CStdString& strSubtitle, double pts);
  int GetSubtitleCount();

  void UpdateOverlayInfo(CDVDInputStreamNavigator* pStream, int iAction) { m_pOverlayContainer->UpdateOverlayInfo(pStream, &m_dvdspus, iAction); }

  void SendMessage(CDVDMsg* pMsg);
  bool OpenStream(CDVDStreamInfo &hints, string& filename);
  void CloseStream(bool flush);

private:
  CDVDOverlayContainer* m_pOverlayContainer;  

  CDVDSubtitleStream* m_pSubtitleStream;
  CDVDSubtitleParser* m_pSubtitleFileParser;
  CDVDDemuxSPU        m_dvdspus;

  CDVDStreamInfo      m_streaminfo;
};


//typedef struct SubtitleInfo
//{

//
//} SubtitleInfo;

