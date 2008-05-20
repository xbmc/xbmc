#include "stdafx.h"
#include "DVDOverlayCodecSSA.h"
#include "DVDOverlaySSA.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"

CDVDOverlayCodecSSA::CDVDOverlayCodecSSA() : CDVDOverlayCodec("SSA Subtitle Decoder")
{
  m_pOverlay = NULL;
  m_libass = NULL;
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
  return m_libass->DecodeHeader((char *)hints.extradata, hints.extrasize);
}

void CDVDOverlayCodecSSA::Dispose()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecSSA::Decode(BYTE* data, int size, double pts, double duration)
{

  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);

  if(m_libass->DecodeDemuxPkt((char *)data, size, pts, duration))
    m_pOverlay = new CDVDOverlaySSA(m_libass);

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

