
#pragma once
#include "DVDOverlayCodec.h"
#include "../../../ffmpeg/DllAvCodec.h"

class CDVDOverlaySpu;
class CDVDOverlayText;

class CDVDOverlayCodecFFmpeg : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecFFmpeg();
  virtual ~CDVDOverlayCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* data, int size);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();
  
private:
  void FreeSubtitle(AVSubtitle &sub);

  AVCodecContext* m_pCodecContext;
  AVSubtitle      m_Subtitle;
  int             m_SubtitleIndex;

  DllAvCodec      m_dllAvCodec;
  DllAvUtil       m_dllAvUtil;

};
