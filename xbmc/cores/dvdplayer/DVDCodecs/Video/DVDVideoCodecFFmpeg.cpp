
#include "../../../../stdafx.h"
#include "DVDVideoCodecFFmpeg.h"
#include "..\..\DVDDemuxers\DVDDemux.h"

#include "..\..\ffmpeg\ffmpeg.h"

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  m_pCodecContext = NULL;
  m_pConvertFrame = NULL;
  m_pFrame = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CodecID codecID, int iWidth, int iHeight, void* ExtraData, unsigned int ExtraSize)
{
  AVCodec* pCodec;

  if (!m_dllAvFormat.Load() || !m_dllAvCodec.Load()) return false;
  
  // register all codecs, demux and protocols
  m_dllAvCodec.av_log_set_callback(dvdplayer_log);
  m_dllAvFormat.av_register_all();

  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  // avcodec_get_context_defaults(m_pCodecContext);

  pCodec = m_dllAvCodec.avcodec_find_decoder(codecID);
  if (!pCodec)
  {
    CLog::DebugLog("CDVDVideoCodecFFmpeg::Open() Unable to find codec");
    return false;
  }

  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  /* some decoders (eg. dv) do not know the pix_fmt until they decode the
   * first frame. setting to -1 avoid enabling DR1 for them.
   */
  m_pCodecContext->pix_fmt = (PixelFormat) - 1;

  if (pCodec->capabilities & CODEC_CAP_DR1)
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;

  // Hack to correct wrong frame rates that seem to be generated by some
  // codecs
  if (m_pCodecContext->time_base.den > 1000 && m_pCodecContext->time_base.num == 1)
    m_pCodecContext->time_base.num = 1000;

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->height = iHeight;
  m_pCodecContext->width = iWidth;

  if( ExtraData && ExtraSize > 0 )
  {
    m_pCodecContext->extradata_size = ExtraSize;
    m_pCodecContext->extradata = m_dllAvCodec.av_mallocz(ExtraSize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, ExtraData, ExtraSize);
  }

  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::DebugLog("CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }

  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  if (!m_pFrame) return false;

  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  if (m_pFrame) m_dllAvCodec.av_free(m_pFrame);
  m_pFrame = NULL;

  if (m_pConvertFrame)
  {
    delete[] m_pConvertFrame->data[0];
    m_dllAvCodec.av_free(m_pConvertFrame);
  }
  m_pConvertFrame = NULL;

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    m_dllAvCodec.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  
  m_dllAvCodec.Unload();
  m_dllAvFormat.Unload();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
    // i don't know exactly how high this should be set
    // couldn't find any good docs on it. think it varies
    // from codec to codec on what it does

    //  2 seem to be to high.. it causes video to be ruined on following images
    if( bDrop )
      m_pCodecContext->hurry_up = 1;
    else
      m_pCodecContext->hurry_up = 0;
  }
}

int CDVDVideoCodecFFmpeg::Decode(BYTE* pData, int iSize)
{
  int iGotPicture = 0;

  if (!m_pCodecContext) return VC_ERROR;
  try
  {
    int iLen = m_dllAvCodec.avcodec_decode_video(m_pCodecContext, m_pFrame, &iGotPicture, pData, iSize);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception thrown when attempting to decode frame, skipping");
    return VC_ERROR;
  }
  if (iGotPicture != 0)
  {
    if (m_pCodecContext->pix_fmt != PIX_FMT_YUV420P)
    {
      if (!m_pConvertFrame)
      {
        // Allocate an AVFrame structure
        m_pConvertFrame =  m_dllAvCodec.avcodec_alloc_frame();

        // Determine required buffer size and allocate buffer
        int numBytes =  m_dllAvCodec.avpicture_get_size(PIX_FMT_YUV420P, m_pCodecContext->width, m_pCodecContext->height);
        BYTE* buffer = new BYTE[numBytes];

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        m_dllAvCodec.avpicture_fill((AVPicture *)m_pConvertFrame, buffer, PIX_FMT_YUV420P, m_pCodecContext->width, m_pCodecContext->height);
      }

      // convert the picture
      m_dllAvCodec.img_convert((AVPicture*)m_pConvertFrame, PIX_FMT_YUV420P,
                  (AVPicture*)m_pFrame, m_pCodecContext->pix_fmt,
                  m_pCodecContext->width, m_pCodecContext->height);
    }
    else
    {
      // no need to convert, just free any existing convert buffers
      if (m_pConvertFrame)
      {
        delete[] m_pConvertFrame->data[0];
         m_dllAvCodec.av_free(m_pConvertFrame);
        m_pConvertFrame = NULL;
      }
    }
  }

  if (iGotPicture > 0) return (VC_PICTURE | VC_BUFFER);
  return VC_BUFFER;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  if (m_pConvertFrame)
  {
    delete[] m_pConvertFrame->data[0];
    m_dllAvCodec.av_free(m_pConvertFrame);
    m_pConvertFrame = NULL;
  }
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  GetVideoAspect(m_pCodecContext, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight);

  pDvdVideoPicture->iWidth = m_pCodecContext->width;
  pDvdVideoPicture->iHeight = m_pCodecContext->height;

  if (m_pConvertFrame)
  {
    // we have a converted frame, use this one
    for (int i = 0; i < 4; i++) pDvdVideoPicture->data[i] = m_pConvertFrame->data[i];
    for (int i = 0; i < 4; i++) pDvdVideoPicture->iLineSize[i] = m_pConvertFrame->linesize[i];
    pDvdVideoPicture->iRepeatPicture = m_pConvertFrame->repeat_pict;
    pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;    
    pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
    pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
    pDvdVideoPicture->iFlags |= m_pCodecContext->hurry_up ? DVP_FLAG_DROPPED : 0;
    return true;
  }
  else if (m_pFrame && m_pFrame->data[0])
  {
    // m_pFrame is already the correct format, just copy
    for (int i = 0; i < 4; i++)pDvdVideoPicture->data[i] = m_pFrame->data[i];
    for (int i = 0; i < 4; i++) pDvdVideoPicture->iLineSize[i] = m_pFrame->linesize[i];
    pDvdVideoPicture->iRepeatPicture = m_pFrame->repeat_pict;
    pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;    
    pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
    pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
    pDvdVideoPicture->iFlags |= m_pCodecContext->hurry_up ? DVP_FLAG_DROPPED : 0;
    return true;
  }

  return false;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecFFmpeg::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if (iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}
