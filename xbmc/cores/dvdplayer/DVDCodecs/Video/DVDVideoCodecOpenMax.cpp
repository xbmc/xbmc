/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBOPENMAX)
#include "DVDClock.h"
#include "GUISettings.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecOpenMax.h"
#include "OpenMaxVideo.h"
#include "utils/log.h"
#include "Codecs/DllAvCodec.h"

// davilla, is this correct? I would have assumed CDVDVideoCodecOpenMax here
#define CLASSNAME "COpenMaxVideo"

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecOpenMax::CDVDVideoCodecOpenMax() : CDVDVideoCodec()
{
  m_codec_ctx = NULL;
  m_filter_ctx = NULL;
  m_omx_decoder = NULL;
  m_pFormatName = "omx-xxxx";

  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecOpenMax::~CDVDVideoCodecOpenMax()
{
  Dispose();
}

bool CDVDVideoCodecOpenMax::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // we always qualify even if DVDFactoryCodec does this too.
  if (g_guiSettings.GetBool("videoplayer.useomx") && !hints.software)
  {
    switch (hints.codec)
    {
      case CODEC_ID_H264:
      {
        m_pFormatName = "omx-h264";
        if (hints.extrasize < 7 || hints.extradata == NULL)
        {
          CLog::Log(LOGNOTICE,
            "%s::%s - avcC data too small or missing", CLASSNAME, __func__);
          return false;
        }
        // valid avcC data (bitstream) always starts with the value 1 (version)
        if ( *(char*)hints.extradata == 1 )
        {
          int nalsize = (((uint8_t*)hints.extradata)[4] & 0x3) + 1;
          if (!nalsize || nalsize == 3 || nalsize > 4)
          {
            CLog::Log(LOGNOTICE, "%s::%s - Invalid nal size (%d)", CLASSNAME, __func__, nalsize);
            return false;
          }
          CreateBitStreamFilter(hints, "h264_mp4toannexb");
        }
      }
      break;
      case CODEC_ID_MPEG4:
        m_pFormatName = "omx-mpeg4";
        CreateBitStreamFilter(hints, "mpeg4video_es");
      break;
      case CODEC_ID_MPEG2VIDEO:
        m_pFormatName = "omx-mpeg2";
      break;
      case CODEC_ID_WMV3:
        m_pFormatName = "omx-wmv3";
        CreateBitStreamFilter(hints, "vc1_asftorcv");
      break;
      case CODEC_ID_VC1:
        m_pFormatName = "omx-vc1";
        CreateBitStreamFilter(hints, "vc1_asftoannexg");
      break;
      default:
        return false;
      break;
    }

    m_omx_decoder = new COpenMaxVideo;
    if (!m_omx_decoder->Open(hints))
    {
      CLog::Log(LOGERROR,
        "%s::%s - failed to open, codec(%d), profile(%d), level(%d)", 
        CLASSNAME, __func__, hints.codec, hints.profile, hints.level);
      return false;
    }

    // allocate a YV12 DVDVideoPicture buffer.
    // first make sure all properties are reset.
    memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));

    m_videobuffer.dts = DVD_NOPTS_VALUE;
    m_videobuffer.pts = DVD_NOPTS_VALUE;
    m_videobuffer.format = DVDVideoPicture::FMT_OMXEGL;
    m_videobuffer.color_range  = 0;
    m_videobuffer.color_matrix = 4;
    m_videobuffer.iFlags  = DVP_FLAG_ALLOCATED;
    m_videobuffer.iWidth  = hints.width;
    m_videobuffer.iHeight = hints.height;
    m_videobuffer.iDisplayWidth  = hints.width;
    m_videobuffer.iDisplayHeight = hints.height;
    return true;
  }

  return false;
}

void CDVDVideoCodecOpenMax::Dispose()
{
  if (m_omx_decoder)
  {
    m_omx_decoder->Close();
    delete m_omx_decoder;
    m_omx_decoder = NULL;
  }
  if (m_videobuffer.iFlags & DVP_FLAG_ALLOCATED)
    m_videobuffer.iFlags = 0;

  DeleteBitStreamFilter();
}

void CDVDVideoCodecOpenMax::SetDropState(bool bDrop)
{
  m_omx_decoder->SetDropState(bDrop);
}

int CDVDVideoCodecOpenMax::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  if (pData)
  {
    int rtn;
    int demuxer_bytes = iSize;
    uint8_t *demuxer_content = pData;
    bool free_demuxer_content  = false;

    if (m_filter_ctx)
    {
      // convert demuxer packet from bitstream (AVC1) to bytestream (AnnexB)
      int bytestream_size = 0;
      uint8_t *bytestream_buff = NULL;

      m_dllAvCodec->av_bitstream_filter_filter(m_filter_ctx, m_codec_ctx, 
        NULL, &bytestream_buff, &bytestream_size, demuxer_content, demuxer_bytes, 0);
      if (bytestream_buff && (bytestream_size > 0))
      {
        free_demuxer_content = bytestream_buff != demuxer_content;
        demuxer_bytes = bytestream_size;
        demuxer_content  = bytestream_buff;
      }
    }

    rtn = m_omx_decoder->Decode(demuxer_content, demuxer_bytes, dts, pts);

    if (free_demuxer_content)
      m_dllAvUtil->av_free(demuxer_content);

    return rtn;
  }
  
  return VC_BUFFER;
}

void CDVDVideoCodecOpenMax::Reset(void)
{
  m_omx_decoder->Reset();
}

bool CDVDVideoCodecOpenMax::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  bool  ret;

  ret = m_omx_decoder->GetPicture(&m_videobuffer);
  *pDvdVideoPicture = m_videobuffer;

  return ret;
}

bool CDVDVideoCodecOpenMax::CreateBitStreamFilter(CDVDStreamInfo &hints, const char *bitstream_type)
{
  m_dllAvUtil = new DllAvUtil;
  m_dllAvCodec = new DllAvCodec;
  if (!m_dllAvUtil->Load() || !m_dllAvCodec->Load())
    return false;

  m_codec_ctx = m_dllAvCodec->avcodec_alloc_context();
  m_dllAvCodec->avcodec_get_context_defaults(m_codec_ctx);
  m_codec_ctx->codec_id = hints.codec;
  m_codec_ctx->codec_type = CODEC_TYPE_VIDEO;
  m_codec_ctx->coded_width = hints.width; 
  m_codec_ctx->coded_height = hints.height;
  m_codec_ctx->extradata = (uint8_t*)m_dllAvUtil->av_malloc(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
  m_codec_ctx->extradata_size = hints.extrasize;
  memcpy(m_codec_ctx->extradata, hints.extradata, hints.extrasize);
  memset(m_codec_ctx->extradata + hints.extrasize, 0, FF_INPUT_BUFFER_PADDING_SIZE);
  AVCodec *avcodec = m_dllAvCodec->avcodec_find_decoder(m_codec_ctx->codec_id);
  m_dllAvCodec->avcodec_open(m_codec_ctx, avcodec);

  m_filter_ctx = m_dllAvCodec->av_bitstream_filter_init(bitstream_type);
  if (!m_filter_ctx)
    return false;

  // and test extradata
  static const uint8_t testnal[] = { 0,0,0,2,0,0 };
  const uint8_t *test = testnal;
  int testsize  = 6;
  int outbuf_size = 0;
  uint8_t *outbuf = NULL;
  int res = m_dllAvCodec->av_bitstream_filter_filter(m_filter_ctx, m_codec_ctx, NULL, &outbuf, &outbuf_size, test, testsize, 0);
  delete outbuf;

  return res > 0;
}

void CDVDVideoCodecOpenMax::DeleteBitStreamFilter(void)
{
  if (m_filter_ctx)
  {
    m_dllAvCodec->av_bitstream_filter_close(m_filter_ctx);
    m_filter_ctx = NULL;
    if (m_codec_ctx)
    {
      if (m_codec_ctx->codec)
        m_dllAvCodec->avcodec_close(m_codec_ctx);
      if (m_codec_ctx->extradata)
        m_dllAvUtil->av_free(m_codec_ctx->extradata);
      m_dllAvUtil->av_free(m_codec_ctx);
      m_codec_ctx = NULL;
    }
  }
  if (m_dllAvUtil)
  {
    delete m_dllAvUtil;
    m_dllAvUtil = NULL;
  }
  if (m_dllAvCodec)
  {
    delete m_dllAvCodec;
    m_dllAvCodec = NULL;
  }
}

#endif
