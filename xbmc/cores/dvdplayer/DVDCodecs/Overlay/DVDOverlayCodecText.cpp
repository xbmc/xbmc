
#include "stdafx.h"
#include "DVDOverlayCodecText.h"
#include "DVDOverlayText.h"
#include "../../DVDStreamInfo.h"
#include "../DllAvCodec.h"
#include "AutoPtrHandle.h"

CDVDOverlayCodecText::CDVDOverlayCodecText() : CDVDOverlayCodec("Text Subtitle Decoder")
{
  m_pOverlay = NULL;
}

CDVDOverlayCodecText::~CDVDOverlayCodecText()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

bool CDVDOverlayCodecText::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if(hints.codec == CODEC_ID_TEXT)
    return true;
  else
    return false;
}

void CDVDOverlayCodecText::Dispose()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecText::Decode(BYTE* data, int size)
{  
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);

  m_pOverlay = new CDVDOverlayText();
  m_pOverlay->iPTSStartTime = 0;
  m_pOverlay->iPTSStopTime = 0;


  char *start, *end, *p;
  start = (char*)data;
  end   = (char*)data + size;
  p     = (char*)data;
  while(p<end)
  {
    if(*p == '{')
    {
      if(p>start)
        m_pOverlay->AddElement(new CDVDOverlayText::CElementText(start, p-start));

      start = p+1;

      while(*p != '}' && p<end)
        p++;

      char* override = (char*)malloc(p-start + 1);
      memcpy(override, start, p-start);
      override[p-start] = '\0';
      CLog::Log(LOGINFO, "%s - Skipped formatting tag %s", __FUNCTION__, override);
      free(override);

      start = p+1;
    }
    p++;
  }
  if(p>start)
    m_pOverlay->AddElement(new CDVDOverlayText::CElementText(start, p-start));

  return OC_OVERLAY;
}

void CDVDOverlayCodecText::Reset()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

void CDVDOverlayCodecText::Flush()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

CDVDOverlay* CDVDOverlayCodecText::GetOverlay()
{
  if(m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;    
    return overlay;
  }
  return NULL;
}
