
#include "stdafx.h"
#include "DVDAudioCodecFFmpeg.h"

CDVDAudioCodecFFmpeg::CDVDAudioCodecFFmpeg() : CDVDAudioCodec()
{
  m_iBufferSize = 0;
  m_iBuffered = 0;
  m_pCodecContext = NULL;
  m_bOpenedCodec = false;
}

CDVDAudioCodecFFmpeg::~CDVDAudioCodecFFmpeg()
{
  Dispose();
}

bool CDVDAudioCodecFFmpeg::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize)
{
  AVCodec* pCodec;
  m_bOpenedCodec = false;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load()) return false;
  
  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  m_dllAvCodec.avcodec_get_context_defaults(m_pCodecContext);

  pCodec = m_dllAvCodec.avcodec_find_decoder(codecID);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to find codec");
    return false;
  }

  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = 1;

  if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

  m_pCodecContext->channels = iChannels;
  m_pCodecContext->sample_rate = iSampleRate;
  //m_pCodecContext->bits_per_sample = 24;
  
  if( ExtraData && ExtraSize > 0 )
  {
    m_pCodecContext->extradata_size = ExtraSize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(ExtraSize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, ExtraData, ExtraSize);
  }
  
  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDAudioCodecFFmpeg::Open() Unable to open codec");
    Dispose();
    return false;
  }
  
  m_bOpenedCodec = true;
  return true;
}

void CDVDAudioCodecFFmpeg::Dispose()
{
  if (m_pCodecContext)
  {
    if (m_bOpenedCodec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    m_bOpenedCodec = false;
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  
  m_iBufferSize = 0;
  m_iBuffered = 0;
}

int CDVDAudioCodecFFmpeg::Decode(BYTE* pData, int iSize)
{
  int iBytesUsed;
  if (!m_pCodecContext) return -1;

  iBytesUsed = m_dllAvCodec.avcodec_decode_audio(m_pCodecContext, (int16_t *)m_buffer, &m_iBufferSize, pData, iSize);

  if(m_iBufferSize == 0)
    m_iBuffered += iBytesUsed;
  else
    m_iBuffered = 0;

  return iBytesUsed;
}

int CDVDAudioCodecFFmpeg::GetData(BYTE** dst)
{
  *dst = m_buffer;
  return m_iBufferSize;
}

void CDVDAudioCodecFFmpeg::Reset()
{
  if (m_pCodecContext) m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);
  m_iBufferSize = 0;
  m_iBuffered = 0;
}

int CDVDAudioCodecFFmpeg::GetChannels()
{
  if (m_pCodecContext) return m_pCodecContext->channels;
  return 0;
}

int CDVDAudioCodecFFmpeg::GetSampleRate()
{
  if (m_pCodecContext) return m_pCodecContext->sample_rate;
  return 0;
}

int CDVDAudioCodecFFmpeg::GetBitsPerSample()
{
  if (m_pCodecContext) return 16;
  return 0;
}
