#pragma once
#include "DVDOverlayCodec.h"
#include "DVDSubtitles/DVDSubtitlesLibass.h"

class CDVDOverlaySSA;

class CDVDOverlayCodecSSA : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecSSA();
  virtual ~CDVDOverlayCodecSSA();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* data, int size, double pts, double duration);
  virtual int Decode(BYTE* data, int size);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();
  
private:
  CDVDSubtitlesLibass* m_libass;
  CDVDOverlaySSA* m_pOverlay;
};
