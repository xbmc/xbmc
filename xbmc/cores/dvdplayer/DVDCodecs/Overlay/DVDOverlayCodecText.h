
#pragma once
#include "DVDOverlayCodec.h"

class CDVDOverlayText;

class CDVDOverlayCodecText : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecText();
  virtual ~CDVDOverlayCodecText();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* data, int size);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();
  
private:
  bool             m_bIsSSA;
  CDVDOverlayText* m_pOverlay;
};
