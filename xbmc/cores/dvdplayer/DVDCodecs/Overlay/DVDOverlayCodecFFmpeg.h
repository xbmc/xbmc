
#pragma once
#include "DVDOverlayCodec.h"
#include "../DllAvCodec.h"

class CDVDOverlaySpu;
class CDVDOverlayText;

class CDVDOverlayCodecFFmpeg : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecFFmpeg();
  virtual ~CDVDOverlayCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* data, int size, double pts);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();
  
private:
  void FreeSubtitle(AVSubtitle &sub);

  AVCodecContext* m_pCodecContext;
  AVSubtitle      m_Subtitle;
  int             m_SubtitleIndex;
  double          m_firstdts;
  double          m_currdts;

  DllAvCodec      m_dllAvCodec;
  DllAvUtil       m_dllAvUtil;

};
