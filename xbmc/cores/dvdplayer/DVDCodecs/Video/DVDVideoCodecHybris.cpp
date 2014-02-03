/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DVDVideoCodecHybris.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDCodecUtils.h"

#define MAJOR_VERSION 12

#include "settings/Settings.h"
#if MAJOR_VERSION < 13
	#include "settings/GUISettings.h"
#else
	#include "settings/DisplaySettings.h"
	#include "settings/AdvancedSettings.h"
#endif
#include "utils/fastmemcpy.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <dirent.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CDVDVideoCodecHybris"

enum {
        OK          = 0,
        NO_ERROR    = 0,
};

CDVDVideoCodecHybris::CDVDVideoCodecHybris() : CDVDVideoCodec() {
  m_iDecodedWidth = 0;
  m_iDecodedHeight = 0;
  m_iConvertedWidth = 0;
  m_iConvertedHeight = 0;
  m_bDropPictures = false;
  m_codec = NULL;
  m_format = NULL;
  memset(&m_videoBuffer, 0, sizeof(m_videoBuffer));
  m_bVideoConvert = false;
  m_bitstream = NULL;
}

CDVDVideoCodecHybris::~CDVDVideoCodecHybris() {
  Dispose();
}

bool CDVDVideoCodecHybris::OpenDevices() {
  return false;
}

bool CDVDVideoCodecHybris::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) {
  int ret = 0;

  if (hints.software)
    return false;

  if(m_bitstream) {
      SAFE_DELETE(m_bitstream);
      m_bitstream = NULL;
  }

  m_drop = false;
  m_hints = hints;

  switch(hints.codec)
  {
    case CODEC_ID_MPEG4:
      m_name = "hyb-mpeg4";
      m_mimeType = "video/mp4v-es";
      break;
    case CODEC_ID_VP3:
    case CODEC_ID_VP6:
    case CODEC_ID_VP6F:
    case CODEC_ID_VP8:
      m_name = "hyb-vpX";
      m_mimeType = "video/x-vnd.on2.vp8";
      break;
    case CODEC_ID_H263:
      m_name = "hyb-h263";
      m_mimeType = "video/3gpp";
      break;
    case CODEC_ID_AVS:
    case CODEC_ID_CAVS:
    case CODEC_ID_H264:
      m_name = "hyb-h264";
      m_mimeType = "video/avc";
      m_bitstream = new CBitstreamConverter;
      if (!m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true))
      {
        SAFE_DELETE(m_bitstream);
        return false;
      }
      break;
    default:
      return false;
      break;
  }

  m_format = media_format_create_video_format("video/avc", hints.width, hints.height, 0, 0);
  if (m_format == NULL) {
    CLog::Log(LOGERROR, "%s::%s - Failed to create format object for %s", CLASSNAME, __func__, m_mimeType.c_str());
    return false;
  }

  m_codec = media_codec_create_by_codec_type(m_mimeType.c_str());
  if (m_codec == NULL) {
    CLog::Log(LOGERROR, "%s::%s - Failed to create codec for %s", CLASSNAME, __func__, m_mimeType.c_str());
    media_codec_release(m_codec);
    media_codec_delegate_destroy(m_codec);
    media_format_destroy(m_codec);
    return false;
  }

  if (hints.extrasize > 0) {
    size_t size = m_hints.extrasize;
    void *src_ptr = m_hints.extradata;
    if (m_bitstream)
    {
      size = m_bitstream->GetExtraSize();
      src_ptr = m_bitstream->GetExtraData();
    }
    media_format_set_byte_buffer(m_format, "csd-0", (uint8_t*)src_ptr, size);
  }

  if (media_codec_configure(m_codec, m_format, 0) != OK) {
    CLog::Log(LOGERROR, "%s::%s - Failed to configure codec for %s", CLASSNAME, __func__, m_mimeType.c_str());
    return false;
  }


  media_codec_start(m_codec);

  // setup a YUV420P DVDVideoPicture buffer.
  // first make sure all properties are reset.
  memset(&m_videoBuffer, 0x00, sizeof(DVDVideoPicture));

  m_videoBuffer.dts = DVD_NOPTS_VALUE;
  m_videoBuffer.pts = DVD_NOPTS_VALUE;
  m_videoBuffer.color_range = 0;
  m_videoBuffer.color_matrix = 4;
  m_videoBuffer.iFlags = DVP_FLAG_ALLOCATED;
  m_videoBuffer.iWidth = m_hints.width;
  m_videoBuffer.iHeight = m_hints.height;
  // these will get reset to crop values later
  m_videoBuffer.iDisplayWidth = m_hints.width;
  m_videoBuffer.iDisplayHeight = m_hints.height;

  return true;
}

void CDVDVideoCodecHybris::Dispose() {
  CLog::Log(LOGDEBUG, "%s::%s - Freeing memory allocated for buffers", CLASSNAME, __func__);

  while(!m_pts.empty())
    m_pts.pop();
  while(!m_dts.empty())
    m_dts.pop();

  media_codec_stop(m_codec);
  media_codec_release(m_codec);
  media_codec_delegate_destroy(m_codec);
  media_format_destroy(m_format);

  // clear m_videoBuffer bits
  free(m_videoBuffer.data[0]), m_videoBuffer.data[0] = NULL;
  free(m_videoBuffer.data[1]), m_videoBuffer.data[1] = NULL;
  free(m_videoBuffer.data[2]), m_videoBuffer.data[2] = NULL;
  m_videoBuffer.iFlags = 0;

  m_iDecodedWidth = 0;
  m_iDecodedHeight = 0;
  m_iConvertedWidth = 0;
  m_iConvertedHeight = 0;
  m_bDropPictures = false;
  m_bVideoConvert = false;

  if(m_bitstream) {
      SAFE_DELETE(m_bitstream);
      m_bitstream = NULL;
  }

}

void CDVDVideoCodecHybris::SetDropState(bool bDrop) {

  m_bDropPictures = bDrop;

}

int CDVDVideoCodecHybris::Decode(BYTE* pData, int iSize, double dts, double pts) {
 // Handle input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as DVDPlayerVideo has no concept of "try again".
  // we must return VC_BUFFER or VC_PICTURE, default to VC_BUFFER.
  int rtn = VC_BUFFER;

  MediaCodecBufferInfo bufferInfo;
  if (m_hints.ptsinvalid)
    pts = DVD_NOPTS_VALUE;

  // must check for an output picture 1st,
  // otherwise, mediacodec can stall on some devices.
  if (GetOutputPicture() > 0)
    rtn |= VC_PICTURE;

  if (pData)
  {
    if (m_bitstream)
    {
      m_bitstream->Convert(pData, iSize);
      iSize = m_bitstream->GetConvertSize();
      pData = m_bitstream->GetConvertBuffer();
    }

    // queue demux pkt in case we cannot get an input buffer
    amc_demux demux_pkt;
    demux_pkt.dts = dts;
    demux_pkt.pts = pts;
    demux_pkt.iSize = iSize;
    demux_pkt.pData = (uint8_t*)malloc(iSize);
    memcpy(demux_pkt.pData, pData, iSize);
    m_demux.push(demux_pkt);

    // try to fetch an input buffer
    int64_t timeout_us = 5000;
    size_t index;// = m_codec->dequeueInputBuffer(timeout_us);
    int ret = media_codec_dequeue_input_buffer(m_codec, &index, timeout_us);
    if (ret == OK)
    {
      // we have an input buffer, fill it.
      int size = media_codec_get_nth_input_buffer_capacity(m_codec, index);
      // fetch the front demux packet
      amc_demux &demux_pkt = m_demux.front();
      if (demux_pkt.iSize > size)
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecHybris::Decode, iSize(%d) > size(%d)", iSize, size);
        demux_pkt.iSize = size;
      }
      // fetch a pointer to the ByteBuffer backing store
      void *dst_ptr = media_codec_get_nth_input_buffer(m_codec, index);
      if (dst_ptr)
        memcpy(dst_ptr, demux_pkt.pData, demux_pkt.iSize);

      free(demux_pkt.pData);
      m_demux.pop();

      // Translate from dvdplayer dts/pts to MediaCodec pts,
      // pts WILL get re-ordered by MediaCodec if needed.
      // Do not try to pass pts as a unioned double/int64_t,
      // some android devices will diddle with presentationTimeUs
      // and you will get NaN back and DVDPlayerVideo will barf.
      int64_t presentationTimeUs = AV_NOPTS_VALUE;
      if (demux_pkt.pts != DVD_NOPTS_VALUE)
        presentationTimeUs = demux_pkt.pts;
      else if (demux_pkt.dts != DVD_NOPTS_VALUE)
        presentationTimeUs = demux_pkt.dts;
/*
CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: "
"pts(%f), ipts(%lld), iSize(%d), GetDataSize(%d), loop_cnt(%d)",
presentationTimeUs, pts_dtoi(presentationTimeUs), iSize, GetDataSize(), loop_cnt);
*/
//      m_codec->queueInputBuffer(index, offset, demux_pkt.iSize, presentationTimeUs, flags);
      bufferInfo.index = index;
      bufferInfo.offset = 0;
      bufferInfo.size = demux_pkt.iSize;
      bufferInfo.presentation_time_us = 0;//presentationTimeUs;
      bufferInfo.flags = 0;

      if(media_codec_queue_input_buffer(m_codec, &bufferInfo) != OK) {
        CLog::Log(LOGERROR, "CDVDVideoCodecHybris::Decode, failed to queue input buffer!");
      }
    }
  }

  return rtn;
}

void CDVDVideoCodecHybris::Reset() {

  // dump any pending demux packets
  while (!m_demux.empty())
  {
    amc_demux &demux_pkt = m_demux.front();
    free(demux_pkt.pData);
    m_demux.pop();
  }

  if (m_codec)
  {
    // now we can flush the actual MediaCodec object
    // TODO: what here?

    // Invalidate our local DVDVideoPicture bits
    m_videoBuffer.pts = DVD_NOPTS_VALUE;
  }
}

bool CDVDVideoCodecHybris::GetPicture(DVDVideoPicture* pDvdVideoPicture) {

  *pDvdVideoPicture = m_videoBuffer;
  m_videoBuffer.pts = DVD_NOPTS_VALUE;

  return true;
}

bool CDVDVideoCodecHybris::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}

int CDVDVideoCodecHybris::GetOutputPicture(void)
{
  int rtn = 0;
  int ret = 0;
  int64_t timeout_us = 5000;
  int index;// = m_codec->dequeueOutputBuffer(bufferInfo, timeout_us);
  MediaCodecBufferInfo bufferInfo;
  ret = media_codec_dequeue_output_buffer(m_codec, &bufferInfo, timeout_us);
  index = bufferInfo.index;
  if (ret == OK)
  {
    if (m_drop)
    {
      media_codec_release_output_buffer(m_codec, index, 0);
      return 0;
    }

    // some devices will return a valid index
    // before signaling INFO_OUTPUT_BUFFERS_CHANGED which
    // is used to setup m_output, D'uh. setup m_output here.
#if 0
    if (m_output.empty())
    {
      m_output = m_codec->getOutputBuffers();
      FlushInternal();
    }
#endif
    int flags = bufferInfo.flags;
#if 0
    if (flags & CJNIMediaCodec::BUFFER_FLAG_SYNC_FRAME)
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: BUFFER_FLAG_SYNC_FRAME");

    if (flags & CJNIMediaCodec::BUFFER_FLAG_CODEC_CONFIG)
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: BUFFER_FLAG_CODEC_CONFIG");
#endif
    if (flags & 4 /*CJNIMediaCodec::BUFFER_FLAG_END_OF_STREAM*/)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecHybris:: BUFFER_FLAG_END_OF_STREAM");
      media_codec_release_output_buffer(m_codec, index, 0);
      return 0;
    }

    {
      int size = bufferInfo.size;
      int offset = bufferInfo.offset;

      if (size && media_codec_get_nth_output_buffer_capacity(m_codec, index))
      {
        uint8_t *src_ptr = (uint8_t*)media_codec_get_nth_output_buffer(m_codec, index);
        src_ptr += offset;

        int loop_end = 0;
        if (m_videoBuffer.format == RENDER_FMT_NV12)
          loop_end = 2;
        else if (m_videoBuffer.format == RENDER_FMT_YUV420P)
          loop_end = 3;

        for (int i = 0; i < loop_end; i++)
        {
          uint8_t *src = src_ptr + m_src_offset[i];
          int src_stride = m_src_stride[i];
          uint8_t *dst = m_videoBuffer.data[i];
          int dst_stride = m_videoBuffer.iLineSize[i];

          int height = m_videoBuffer.iHeight;
          if (i > 0)
            height = (m_videoBuffer.iHeight + 1) / 2;

          for (int j = 0; j < height; j++, src += src_stride, dst += dst_stride)
            memcpy(dst, src, dst_stride);
        }
      }
      media_codec_release_output_buffer(m_codec, index, 0);
    }

    int64_t pts = bufferInfo.presentation_time_us;
    m_videoBuffer.dts = DVD_NOPTS_VALUE;
#if 0
    if (pts != AV_NOPTS_VALUE)
      m_videoBuffer.pts = pts;
#endif
/*
CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture "
"index(%d), pts(%f)", index, m_videoBuffer.pts);
*/
    rtn = 1;
  }
  else if (ret == -3 /* == CJNIMediaCodec::INFO_OUTPUT_BUFFERS_CHANGED*/)
  {
//    m_output = m_codec->getOutputBuffers();
  }
  else if (ret == -2 /* CJNIMediaCodec::INFO_OUTPUT_FORMAT_CHANGED*/)
  {
    OutputFormatChanged();
  }
  else if (ret == -1/* CJNIMediaCodec::INFO_TRY_AGAIN_LATER*/)
  {
    // normal dequeueOutputBuffer timeout, ignore it.
    rtn = -1;
  }
  else
  {
    // we should never get here
    CLog::Log(LOGERROR, "CDVDVideoCodecHybris::GetOutputPicture unknown index(%d)", index);
  }

  return rtn;
}

void CDVDVideoCodecHybris::OutputFormatChanged(void)
{
  MediaFormat f = media_codec_get_output_format(m_codec);

  int width = media_format_get_width(f);
  int height = media_format_get_height(f);
  int stride = media_format_get_stride(f);
  int slice_height = media_format_get_slice_height(f);
  int color_format= media_format_get_color_format(f);
  int crop_left = media_format_get_crop_left(f);
  int crop_top = media_format_get_crop_top(f);
  int crop_right = media_format_get_crop_right(f);
  int crop_bottom = media_format_get_crop_bottom(f);

  CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: "
    "width(%d), height(%d), stride(%d), slice-height(%d), color-format(%d)",
    width, height, stride, slice_height, color_format);
  CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: "
    "crop-left(%d), crop-top(%d), crop-right(%d), crop-bottom(%d)",
    crop_left, crop_top, crop_right, crop_bottom);

  //  Android device quirks and fixes
    if (stride <= 0)
        stride = width;
#if 0
    if (slice_height <= 0)
    {
      slice_height = height;
      if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar)
      {
        // NVidia Tegra 3 on Nexus 7 does not set slice_heights
        if (strstr(m_codecname.c_str(), "OMX.Nvidia.") != NULL)
        {
          slice_height = (((height) + 31) & ~31);
          CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: NVidia Tegra 3 quirk, slice_height(%d)", slice_height);
        }
      }
    }
    if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar)
    {
      slice_height -= crop_top / 2;
      // set crop top/left here, since the offset parameter already includes this.
      // if we would ignore the offset parameter in the BufferInfo, we could just keep
      // the original slice height and apply the top/left cropping instead.
      crop_top = 0;
      crop_left = 0;
    }
#endif
    // default picture format to none
    for (int i = 0; i < 4; i++)
      m_src_offset[i] = m_src_stride[i] = 0;
    // delete any existing buffers
    for (int i = 0; i < 4; i++)
      free(m_videoBuffer.data[i]);

    // setup picture format and data offset vectors
    if (color_format == 19/*CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar*/)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: COLOR_FormatYUV420Planar");

      // Y plane
      m_src_stride[0] = stride;
      m_src_offset[0] = crop_top * stride;
      m_src_offset[0]+= crop_left;

      // U plane
      m_src_stride[1] = (stride + 1) / 2;
      // skip over the Y plane
      m_src_offset[1] = slice_height * stride;
      // crop_top/crop_left divided by two
      // because one byte of the U/V planes
      // corresponds to two pixels horizontally/vertically
      m_src_offset[1]+= crop_top / 2 * m_src_stride[1];
      m_src_offset[1]+= crop_left / 2;

      // V plane
      m_src_stride[2] = (stride + 1) / 2;
      // skip over the Y plane
      m_src_offset[2] = slice_height * stride;
      // skip over the U plane
      m_src_offset[2]+= ((slice_height + 1) / 2) * ((stride + 1) / 2);
      // crop_top/crop_left divided by two
      // because one byte of the U/V planes
      // corresponds to two pixels horizontally/vertically
      m_src_offset[2]+= crop_top / 2 * m_src_stride[2];
      m_src_offset[2]+= crop_left / 2;

      m_videoBuffer.iLineSize[0] = width; // Y
      m_videoBuffer.iLineSize[1] = (width + 1) /2; // U
      m_videoBuffer.iLineSize[2] = (width + 1) /2; // V
      m_videoBuffer.iLineSize[3] = 0;

      unsigned int iPixels = width * height;
      unsigned int iChromaPixels = iPixels/4;
      m_videoBuffer.data[0] = (uint8_t*)malloc(16 + iPixels);
      m_videoBuffer.data[1] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videoBuffer.data[2] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videoBuffer.data[3] = NULL;
      m_videoBuffer.format = RENDER_FMT_YUV420P;
    }
#if 0
    else if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420SemiPlanar
          || color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_QCOM_FormatYUV420SemiPlanar
          || color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: COLOR_FormatYUV420SemiPlanar");

      // Y plane
      m_src_stride[0] = stride;
      m_src_offset[0] = crop_top * stride;
      m_src_offset[0]+= crop_left;

      // UV plane
      m_src_stride[1] = stride;
      // skip over the Y plane
      m_src_offset[1] = slice_height * stride;
      m_src_offset[1]+= crop_top * stride;
      m_src_offset[1]+= crop_left;

      m_videoBuffer.iLineSize[0] = width; // Y
      m_videoBuffer.iLineSize[1] = width; // UV
      m_videoBuffer.iLineSize[2] = 0;
      m_videoBuffer.iLineSize[3] = 0;

      unsigned int iPixels = width * height;
      unsigned int iChromaPixels = iPixels;
      m_videoBuffer.data[0] = (uint8_t*)malloc(16 + iPixels);
      m_videoBuffer.data[1] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videoBuffer.data[2] = NULL;
      m_videoBuffer.data[3] = NULL;
      m_videoBuffer.format = RENDER_FMT_NV12;
    }
#endif
    else
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec:: Fixme unknown color_format(%d)", color_format);
      return;
    }

  // picture display width/height include the cropping.
  m_videoBuffer.iDisplayWidth = crop_right + 1 - crop_left;
  m_videoBuffer.iDisplayHeight = crop_bottom + 1 - crop_top;

}


