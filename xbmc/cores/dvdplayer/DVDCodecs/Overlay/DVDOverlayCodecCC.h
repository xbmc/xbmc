
#pragma once
#include "DVDOverlayCodec.h"

class CDVDOverlayText;

class CDVDOverlayCodecCC : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecCC();
  virtual ~CDVDOverlayCodecCC();
  virtual bool Open();
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double pts);
  virtual void Reset();
  virtual void Flush();
  virtual CDVDOverlay* GetOverlay();
  
private:
  CDVDOverlayText* m_pCurrentOverlay;
};
