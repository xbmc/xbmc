
#include "stdafx.h"
#include "DVDAudioCodecLibDts.h"
#include "../../DVDStreamInfo.h"

inline int16_t convert(register int32_t i)
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
  register int i;
  register int32_t * f = (int32_t *) _f;

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

CDVDAudioCodecLibDts::CDVDAudioCodecLibDts() : CDVDAudioCodec()
{
  m_pState = NULL;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_iDecodedDataSize = 0;
  m_iInputBufferSize = 0;
}

CDVDAudioCodecLibDts::~CDVDAudioCodecLibDts()
{
  Dispose();
}

bool CDVDAudioCodecLibDts::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
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
  m_iOutputChannels = 2;

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

#define DTS_HEADER_SIZE 14
int CDVDAudioCodecLibDts::Decode(BYTE* pData, int iSize)
{
  level_t level = 1.0f;
  sample_t bias = 384;
  
  int iFrameLen = 0;
  bool bError = false;
  
  int iBytesToCopy = iSize;
  int iBytesFree = sizeof(m_inputBuffer) - m_iInputBufferSize;
  if (iBytesToCopy > iBytesFree) iBytesToCopy = iBytesFree;
  
  m_iDecodedDataSize = 0;
  
  memcpy(m_inputBuffer + m_iInputBufferSize, pData, iBytesToCopy);
  m_iInputBufferSize += iBytesToCopy;
  BYTE* pInput = m_inputBuffer;
  
  while (m_iInputBufferSize > DTS_HEADER_SIZE)
  {
    bError = false;
    
    // look for sync
    if (m_iFrameSize <= 0)
    {
      m_iFrameSize = m_dll.dts_syncinfo(m_pState, pInput, &m_iFlags, &m_iSourceSampleRate, &m_iSourceBitrate, &iFrameLen);
      if (m_iFrameSize <= 0)
      {
        // no sync found, shift one byte
        m_iInputBufferSize--;
        pInput++;
      }
    }
    
    if (m_iFrameSize > 0)
    {
      if (m_iSourceChannels == 0) m_iSourceChannels = GetNrOfChannels(m_iFlags);

      if ((m_iInputBufferSize) >= m_iFrameSize)
      {
        // we have the entire frame, decode it
        int iFlags = m_iFlags;
        if (m_iOutputChannels == 1)      iFlags = DTS_MONO;
        else if (m_iOutputChannels == 2) iFlags = DTS_STEREO;
        else
        {
          m_iOutputChannels = m_iSourceChannels;
          iFlags |= DTS_ADJUST_LEVEL;
        }

        m_dll.dts_frame(m_pState, pInput, &iFlags, &level, bias);

        //m_dll.dts_dynrng(m_pState, NULL, NULL);
        
        int iNrOfBlocks = m_dll.dts_blocks_num(m_pState);
        
        short* pDecodedData = (short*)(m_decodedData + m_iDecodedDataSize);
        
        for (int i = 0; i < iNrOfBlocks; i++)
        {
          if (m_dll.dts_block(m_pState) != 0)
          {
            // error, start syncing again
            CLog::Log(LOGERROR, "CDVDAudioCodecLibDts::Decode : dts_block != 0");
            bError = true;
            break;
          }
          pDecodedData += i * 256 * m_iOutputChannels;
          convert2s16_multi(m_fSamples, pDecodedData, iFlags & (DTS_CHANNEL_MASK | DTS_LFE));
        }
        
        // need to check if there is enough data in the output buffer for the next run
        
        if (!bError)
        {
          m_iDecodedDataSize += iNrOfBlocks * m_iOutputChannels * 256 * sizeof(short);
          
          pInput += m_iFrameSize;
          m_iInputBufferSize -= m_iFrameSize;
        }
        m_iFrameSize = 0;        
      }
      else
      {
        // found header, but we do not have enough data for a frame.
        memmove(m_inputBuffer, pInput, m_iInputBufferSize);
        break;
      }
    }
  }
  
  return iBytesToCopy;
}

int CDVDAudioCodecLibDts::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_iDecodedDataSize;
}

void CDVDAudioCodecLibDts::SetDefault()
{
  m_iInputBufferSize = 0;
  m_iFrameSize = 0;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_iDecodedDataSize = 0;
}

void CDVDAudioCodecLibDts::Reset()
{
  return;
  if (m_pState) m_dll.dts_free(m_pState);

  SetDefault();

  m_pState = m_dll.dts_init(0);
  m_fSamples = m_dll.dts_samples(m_pState);
}
