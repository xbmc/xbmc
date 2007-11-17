
#include "stdafx.h"
#include "DVDOverlayCodecCC.h"
#include "DVDOverlayText.h"
#include "../../DVDClock.h"

extern "C" {
#include "libspucc/cc_decoder.h"
}
CDVDOverlayCodecCC::CDVDOverlayCodecCC() : CDVDOverlayCodec("Closed Caption")
{
  m_pCurrentOverlay = NULL;
  Reset();
}

CDVDOverlayCodecCC::~CDVDOverlayCodecCC()
{
}
static cc_decoder_t* m_cc_decoder = NULL;
bool CDVDOverlayCodecCC::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  Reset();
  cc_decoder_init();
  m_cc_decoder = cc_decoder_open();
  return true;
}

void CDVDOverlayCodecCC::Dispose()
{
  Flush();
  if (m_cc_decoder) cc_decoder_close(m_cc_decoder);
  m_cc_decoder = NULL;
}
/*
int CDVDOverlayCodecCC::DecodeFieldData(BYTE* pData, int iSize)
{
}
*/

int CDVDOverlayCodecCC::Decode(BYTE* pData, int iSize)
{
  // minimum amount of data is even more for cc
  decode_cc(m_cc_decoder, pData, iSize);

  if (iSize >= 2)
  {
    Flush();
    m_pCurrentOverlay = new CDVDOverlayText();
    
    cc_buffer_t* data = &m_cc_decoder->on_buf->channel[0];
    for (int r = 0; r < CC_ROWS; r++)
    {
      if (data->rows[r].num_chars > 0)
      {
        char row_text[CC_COLUMNS + 1];
        row_text[0] = 0;
        for (int c = 0; c < data->rows[r].num_chars; c++)
        {
          row_text[c] = data->rows[r].cells[c].c;
        }
        row_text[data->rows[r].num_chars] = '\n';
        row_text[data->rows[r].num_chars + 1] = 0;
        CDVDOverlayText::CElementText* pText = new CDVDOverlayText::CElementText(row_text);
        m_pCurrentOverlay->AddElement(pText);
      }
    }
    
    return OC_OVERLAY;
    
    /*
    
    m_pCurrentOverlay->iPTSStartTime = pts;
    m_pCurrentOverlay->iPTSStopTime = 0LL;

    char test[64];
    sprintf(test, "cc data : %I64d", pts);
    CDVDOverlayText::CElementText* pText = new CDVDOverlayText::CElementText(test);
    m_pCurrentOverlay->AddElement(pText);
    return OC_OVERLAY;*/
  }
  return OC_BUFFER;
}

void CDVDOverlayCodecCC::Reset()
{
  Flush();
}

void CDVDOverlayCodecCC::Flush()
{
  if (m_pCurrentOverlay)
  {
    // end time is not always known and may be 0.
    // In that case the overlay container does not remove the overlay.
    // We set it here to be sure
    if (m_pCurrentOverlay->iPTSStopTime == 0LL)
    {
      m_pCurrentOverlay->iPTSStopTime = m_pCurrentOverlay->iPTSStartTime + 1;
    }
    m_pCurrentOverlay->Release();
    m_pCurrentOverlay = NULL;
  }
}

CDVDOverlay* CDVDOverlayCodecCC::GetOverlay()
{
  CDVDOverlay* overlay = m_pCurrentOverlay;
  m_pCurrentOverlay = NULL;
  return overlay;

}
