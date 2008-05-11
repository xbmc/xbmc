#include "stdafx.h"
#include "AutoPtrHandle.h"
#include "DVDOverlayCodecSSA.h"
#include "DVDOverlaySSA.h"
#include "DVDStreamInfo.h"
#include "cores/ffmpeg/avcodec.h"

CDVDOverlayCodecSSA::CDVDOverlayCodecSSA() : CDVDOverlayCodec("SSA Subtitle Decoder")
{
  m_pOverlay = NULL;
}

CDVDOverlayCodecSSA::~CDVDOverlayCodecSSA() 
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
  
  if(m_libass)
    SAFE_RELEASE(m_libass);
}

bool CDVDOverlayCodecSSA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if(hints.codec != CODEC_ID_SSA)
    return false;
  
  m_libass = new CDVDSubtitlesLibass();
  m_libass->DecodeHeader((char *)hints.extradata, hints.extrasize);
  
  return true;
}

void CDVDOverlayCodecSSA::Dispose()
{ 
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}
    
int CDVDOverlayCodecSSA::Decode(BYTE* data, int size)
{
  //Libass can't do much without times
  return OC_ERROR;
}

int CDVDOverlayCodecSSA::Decode(BYTE* data, int size, double pts, double duration)
{ 

  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
  
  printf("[%s] at %.1f for %.1f\n", (char* )data, pts, duration);

  m_libass->DecodeDemuxPkt((char *)data, size, pts, duration);
  
  CDVDOverlaySSA* ssaOverlay = new CDVDOverlaySSA(m_libass);
  m_pOverlay = ssaOverlay;

  return OC_OVERLAY;
}
void CDVDOverlayCodecSSA::Reset()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

void CDVDOverlayCodecSSA::Flush()
{
  if(m_pOverlay)
      SAFE_RELEASE(m_pOverlay);
}

CDVDOverlay* CDVDOverlayCodecSSA::GetOverlay()
{
  if(m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;
    return overlay;
  }
  return NULL;
}

 
