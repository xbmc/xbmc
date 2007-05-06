#include "stdafx.h"
#include "DTSCDDACodec.h"
#ifdef HAS_DTS_CODEC
#include "../../lib/libcdio/sector.h"

DTSCDDACodec::DTSCDDACodec() : DTSCodec()
{
  m_CodecName = "DTS CDDA";
}

DTSCDDACodec::~DTSCDDACodec()
{
}

__int64 DTSCDDACodec::Seek(__int64 iSeekTime)
{
  //  Calculate the next full second...
  int iSeekTimeFullSec = (int)(iSeekTime + (1000 - (iSeekTime % 1000))) / 1000;

  //  ...and the logical sector on the cd...
  lsn_t lsnSeek = iSeekTimeFullSec * CDIO_CD_FRAMES_PER_SEC;

  //  ... then seek to its position...
  int iNewOffset = (int)m_file.Seek(lsnSeek * CDIO_CD_FRAMESIZE_RAW, SEEK_SET);
  m_readBufferPos = 0;

  // ... and look if we really got there.
  int iNewSeekTime = (iNewOffset / CDIO_CD_FRAMESIZE_RAW) / CDIO_CD_FRAMES_PER_SEC;
  return iNewSeekTime * 1000; // ms
}

bool DTSCDDACodec::CalculateTotalTime()
{
  m_TotalTime  = (m_file.GetLength()/CDIO_CD_FRAMESIZE_RAW)/CDIO_CD_FRAMES_PER_SEC;
  m_Bitrate    = (int)((m_file.GetLength() * 8) / m_TotalTime); 
  m_TotalTime *= 1000; // ms
  return m_TotalTime > 0;
}

#endif

