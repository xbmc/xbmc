
#include "../../../../stdafx.h"
#include "DVDAudioCodecLibDts.h"
#include "..\..\DVDPLayerDLL.h"

static inline int16_t convert(int32_t i)
{
#ifdef LIBDTS_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void CDVDAudioCodecLibDts::convert2s16_1(convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[i] = convert (f[i]);
    }
}

void CDVDAudioCodecLibDts::convert2s16_2(convert_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
  {
    s16[2*i] = convert (f[i]);
    s16[2*i+1] = convert (f[i+256]);
  }
}

void CDVDAudioCodecLibDts::convert2s16_3(convert_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
  {
    s16[3*i] = convert (f[i]);
    s16[3*i+1] = convert (f[i+256]);
    s16[3*i+2] = convert (f[i+512]);
  }
}

void CDVDAudioCodecLibDts::convert2s16_4(convert_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
  {
    s16[4*i] = convert (f[i]);
    s16[4*i+1] = convert (f[i+256]);
    s16[4*i+2] = convert (f[i+512]);
    s16[4*i+3] = convert (f[i+768]);
  }
}

void CDVDAudioCodecLibDts::convert2s16_5(convert_t * _f, int16_t * s16)
{
  int i;
  int32_t * f = (int32_t *) _f;

  for (i = 0; i < 256; i++)
  {
    s16[5*i] = convert (f[i]);
    s16[5*i+1] = convert (f[i+256]);
    s16[5*i+2] = convert (f[i+512]);
    s16[5*i+3] = convert (f[i+768]);
    s16[5*i+4] = convert (f[i+1024]);
  }
}

void CDVDAudioCodecLibDts::convert2s16_multi(convert_t * _f, int16_t * s16, int flags)
{
  int i;
  int32_t * f = (int32_t *) _f;

  switch (flags)
  {
    case DTS_MONO:
      for (i = 0; i < 256; i++)
      {
        s16[5*i] = s16[5*i+1] = s16[5*i+2] = s16[5*i+3] = 0;
        s16[5*i+4] = convert (f[i]);
      }
      break;
    case DTS_CHANNEL:
    case DTS_STEREO:
    case DTS_DOLBY:
      convert2s16_2 (_f, s16);
      break;
    case DTS_3F:
      for (i = 0; i < 256; i++)
      {
        s16[5*i] = convert (f[i]);
        s16[5*i+1] = convert (f[i+512]);
        s16[5*i+2] = s16[5*i+3] = 0;
        s16[5*i+4] = convert (f[i+256]);
      }
      break;
    case DTS_2F2R:
      convert2s16_4 (_f, s16);
      break;
    case DTS_3F2R:
      convert2s16_5 (_f, s16);
      break;
    case DTS_MONO | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i+256]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case DTS_CHANNEL | DTS_LFE:
    case DTS_STEREO | DTS_LFE:
    case DTS_DOLBY | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case DTS_3F | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+768]);
        s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i+512]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case DTS_2F2R | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = convert (f[i+768]);
        s16[6*i+3] = convert (f[i+1024]);
        s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i]);
      }
      break;
    case DTS_3F2R | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+768]);
        s16[6*i+2] = convert (f[i+1024]);
        s16[6*i+3] = convert (f[i+1280]);
        s16[6*i+4] = convert (f[i+512]);
        s16[6*i+5] = convert (f[i]);
      }
      break;
  }
}

void CDVDAudioCodecLibDts::s16_swap(int16_t * s16, int channels)
{
  int i;
  uint16_t * u16 = (uint16_t *) s16;

  for (i = 0; i < 256 * channels; i++)
    u16[i] = (u16[i] >> 8) | (u16[i] << 8);
}

void CDVDAudioCodecLibDts::s32_swap(int32_t * s32, int channels)
{
  int i;
  uint32_t * u32 = (uint32_t *) s32;

  for (i = 0; i < 256 * channels; i++)
    u32[i] = (u32[i] << 24) | ((u32[i] << 8)&0xFF0000) |
    ((u32[i] >> 8)&0xFF00) | (u32[i] >> 24);
}

CDVDAudioCodecLibDts::CDVDAudioCodecLibDts() : CDVDAudioCodec()
{
  m_pState = NULL;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
  m_pInputBuffer = NULL;
}

CDVDAudioCodecLibDts::~CDVDAudioCodecLibDts()
{
  Dispose();
}

bool CDVDAudioCodecLibDts::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits)
{
  if (!m_dll.Load())
    return false;

  SetDefault();

  m_pState = m_dll.dts_init(0);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.dts_samples(m_pState);

  // set desired output
  m_iOutputChannels = iChannels;

  return true;
}

void CDVDAudioCodecLibDts::Dispose()
{
  if (m_pState) m_dll.dts_free(m_pState);
  m_pState = NULL;
}

int CDVDAudioCodecLibDts::GetNrOfChannels(int iFlags)
{
  int iChannels = 0;
  
  switch (iFlags & DTS_CHANNEL_MASK)
  {
    case DTS_MONO:           iChannels =  1; break;
    case DTS_CHANNEL:        iChannels =  1; break; // ???
    case DTS_STEREO:         iChannels =  2; break;
    case DTS_STEREO_SUMDIFF: iChannels =  2; break;
    case DTS_STEREO_TOTAL:   iChannels =  2; break;
    case DTS_3F:             iChannels =  3; break;
    case DTS_2F1R:           iChannels =  3; break;
    case DTS_3F1R:           iChannels =  4; break;
    case DTS_2F2R:           iChannels =  4; break;
    case DTS_3F2R:           iChannels =  5; break;
    case DTS_4F2R:           iChannels =  6; break;
    default: iChannels = 0; break;
  }
  if (iFlags & DTS_LFE) iChannels++;
  
  return iChannels;
}


#define HEADER_SIZE 14
int CDVDAudioCodecLibDts::Decode(BYTE* pData, int iSize)
{
  level_t level = 1.0f;
  sample_t bias = 384;
  
  int iLen = 0;
  m_decodedDataSize = 0;
  BYTE* pOldDataPointer = pData;
  while (iSize > 0)
  {
    if (m_iFrameSize == 0)
    {
      // no header seen : find one. We need at least 7 bytes to parse it
      int i = 0;
      while (i <= (iSize - HEADER_SIZE))
      {
        // it's possible that m_inputBuffer already contains 6 bits from our previous run
        // so use m_pInputBuffer to copy the rest of the data. We must rest it after dts_syncinfo though!!
        for (int u = 0; u < HEADER_SIZE; u++) m_pInputBuffer[u] = pData[u];

        iLen = m_dll.dts_syncinfo(m_pState, m_inputBuffer, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iFrameSize);
        if (iLen > 0)
        {
          if (m_iSourceChannels == 0)
          {
            m_iSourceChannels = GetNrOfChannels(m_iFlags);
          }
          m_iFrameSize = iLen;
          pData += HEADER_SIZE;
          m_pInputBuffer += HEADER_SIZE;
          iSize -= HEADER_SIZE;
          break;
        }

        // reset the buffer pointer if needed
        if ((m_pInputBuffer - m_inputBuffer) > 0) m_pInputBuffer = m_inputBuffer;

        // no sync found, shift one byte
        i++;
        pData++;
        //m_pInputBuffer++;
        iSize--;
      }
      if (m_iFrameSize == 0 && iSize < HEADER_SIZE)
      {
        // we are at the end of our stream and don't have enough data for our header anymore.
        // copy it to our buffer for later use;
        for (int i = 0; i < iSize; i++) m_pInputBuffer[i] = pData[i];
        m_pInputBuffer += iSize;
        pData += iSize;
        iSize = 0;
        break;
      }
    }
    else if (m_pInputBuffer - m_inputBuffer < m_iFrameSize)
    {
      // we are working on a frame that is m_iFrameSize big, but we don't have all data yet
      // just copy more data to it
      iLen = m_iFrameSize - (m_pInputBuffer - m_inputBuffer);
      if (iSize < iLen) iLen = iSize;
      fast_memcpy(m_pInputBuffer, pData, iLen);
      m_pInputBuffer += iLen;
      pData += iLen;
      iSize -= iLen;
    }
    else
    {
      // we have a frame to decode
      
      int iFlags = m_iFlags;
      if (m_iOutputChannels == 1)      iFlags = DTS_MONO;
      else if (m_iOutputChannels == 2) iFlags = DTS_STEREO;
      else
      {
        m_iOutputChannels = m_iSourceChannels;
        iFlags |= DTS_ADJUST_LEVEL;
      }

      m_dll.dts_frame(m_pState, m_inputBuffer, &iFlags, &level, bias);

      // [dts_dynrng (state, ...); this is only optional]
      int iNrOfBlocks = m_dll.dts_blocks_num(m_pState);
      for (int i = 0; i < iNrOfBlocks; i++)
      {
        if (m_dll.dts_block(m_pState) != 0)
        {
          OutputDebugString("Error!!!!!!!!!!");
          m_pInputBuffer = m_inputBuffer;
          m_iFrameSize = 0;
          m_decodedDataSize = 0;
          return -1;
        }
        
        convert2s16_multi(m_fSamples, ((short*)m_decodedData) + (i * 256 * m_iOutputChannels), iFlags & (DTS_CHANNEL_MASK | DTS_LFE));
      }

      m_pInputBuffer = m_inputBuffer;
      m_iFrameSize = 0;
      m_decodedDataSize = iNrOfBlocks * m_iOutputChannels * 256 * sizeof(short);
      return (pData - pOldDataPointer);
    }
  }
  return (pData - pOldDataPointer);
}

int CDVDAudioCodecLibDts::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_decodedDataSize;
}

void CDVDAudioCodecLibDts::SetDefault()
{
  m_pInputBuffer = m_inputBuffer;
  m_iFrameSize = 0;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
}

void CDVDAudioCodecLibDts::Reset()
{
  if (m_pState) m_dll.dts_free(m_pState);

  SetDefault();

  m_pState = m_dll.dts_init(0);
  m_fSamples = m_dll.dts_samples(m_pState);
}

