
#include "../../../../stdafx.h"
#include "DVDAudioCodecFFmpeg.h"
#include "..\..\DVDPLayerDLL.h"

#include "..\..\ffmpeg\ffmpeg.h"

// DVD_AVCODEC_DLL

CDVDAudioCodecFFmpeg::CDVDAudioCodecFFmpeg() : CDVDAudioCodec()
{
  m_iBufferSize = 0;
  m_pCodecContext = NULL;
  m_bOpenedCodec = false;
  m_bDllLoaded = false;
}

CDVDAudioCodecFFmpeg::~CDVDAudioCodecFFmpeg()
{
  Dispose();
}

bool CDVDAudioCodecFFmpeg::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits)
{
  AVCodec* pCodec;
  m_bOpenedCodec = false;
      
  if (!m_bDllLoaded)
  {
    DllLoader* pDll = g_sectionLoader.LoadDLL(DVD_AVCODEC_DLL);
    if (!pDll)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecFFmpeg: Unable to load dll %s", DVD_AVCODEC_DLL);
      return false;
    }
    
    if (!dvdplayer_load_dll_avcodec(*pDll))
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecFFmpeg: Unable to resolve exports from %s", DVD_AVCODEC_DLL);
      Dispose();
      return false;
    }
    m_bDllLoaded = true;
  }
  
  // register all codecs, demux and protocols
  av_log_set_callback(dvdplayer_log);
  av_register_all();

  m_pCodecContext = avcodec_alloc_context();
  avcodec_get_context_defaults(m_pCodecContext);

  pCodec = avcodec_find_decoder(codecID);
  if (!pCodec)
  {
    CLog::DebugLog("CDVDAudioCodecFFmpeg::Open() Unable to find codec");
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
  
  // not used for any codec yet, set it to 0
  m_pCodecContext->extradata = NULL;
  m_pCodecContext->extradata_size = 0;
  
  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  if (avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::DebugLog("CDVDAudioCodecFFmpeg::Open() Unable to open codec");
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
    if (m_bOpenedCodec) avcodec_close(m_pCodecContext);
    m_bOpenedCodec = false;
    av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }

  if (m_bDllLoaded)
  {
    g_sectionLoader.UnloadDLL(DVD_AVCODEC_DLL);
    m_bDllLoaded = false;
  }
  
  m_iBufferSize = 0;
}

int CDVDAudioCodecFFmpeg::Decode(BYTE* pData, int iSize)
{
  int iBytesUsed;
  if (!m_pCodecContext) return -1;

  iBytesUsed = avcodec_decode_audio(m_pCodecContext, (int16_t *)m_buffer, &m_iBufferSize, pData, iSize);

  return iBytesUsed;
}

int CDVDAudioCodecFFmpeg::GetData(BYTE** dst)
{
  *dst = m_buffer;
  return m_iBufferSize;
}

void CDVDAudioCodecFFmpeg::Reset()
{
  if (m_pCodecContext) avcodec_flush_buffers(m_pCodecContext);
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
