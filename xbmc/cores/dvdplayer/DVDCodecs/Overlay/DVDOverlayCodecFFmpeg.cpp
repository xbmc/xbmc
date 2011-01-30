/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDOverlayCodecFFmpeg.h"
#include "DVDOverlayText.h"
#include "DVDOverlaySpu.h"
#include "DVDOverlayImage.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "utils/Win32Exception.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"

CDVDOverlayCodecFFmpeg::CDVDOverlayCodecFFmpeg() : CDVDOverlayCodec("FFmpeg Subtitle Decoder")
{
  m_pCodecContext = NULL;
  m_SubtitleIndex = -1;
  m_width         = 0;
  m_height        = 0;
  memset(&m_Subtitle, 0, sizeof(m_Subtitle));
}

CDVDOverlayCodecFFmpeg::~CDVDOverlayCodecFFmpeg()
{
  Dispose();
}

bool CDVDOverlayCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load()) return false;

  m_dllAvCodec.avcodec_register_all();

  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->sub_id = hints.identifier;
  m_pCodecContext->codec_tag = hints.codec_tag;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);

    // start parsing of extra data - create a copy to be safe and make it zero-terminating to avoid access violations!
    unsigned int parse_extrasize = hints.extrasize;
    char* parse_extra = new char[parse_extrasize + 1];
    memcpy(parse_extra, hints.extradata, parse_extrasize);
    parse_extra[parse_extrasize] = '\0';

    // assume that the extra data is formatted as a concatenation of lines ('\n' terminated) 
    char *ptr = parse_extra;
    do // read line by line
    {
      if (!strncmp(ptr, "size:", 5))
      {
        int width = 0, height = 0;
        sscanf(ptr, "size: %dx%d", &width, &height);
        if (width > 0 && height > 0)
        {
          m_pCodecContext->width = width;
          m_pCodecContext->height = height;
          CLog::Log(LOGDEBUG,"%s - parsed extradata: size: %d x %d", __FUNCTION__,  width, height);
        }
      }
      /*        
      // leaving commented code: these items don't work yet... but they may be meaningful
      if (!strncmp(ptr, "palette:", 8))
        if (sscanf(ptr, "palette: %x, %x, %x, %x, %x, %x, %x, %x,"
                                " %x, %x, %x, %x, %x, %x, %x, %x", ...        
      if (!strncasecmp(ptr, "forced subs: on", 15))
        forced_subs_only = 1;
      */
      // if tried all possibilities, then read newline char and move to next line
      ptr = strchr(ptr, '\n');
      if (ptr != NULL) ptr++;
    } 
    while (ptr != NULL && ptr <= parse_extra + parse_extrasize);

    delete[] parse_extra;
  }

  AVCodec* pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"%s - Unable to find codec %d", __FUNCTION__, hints.codec);
    return false;
  }

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }

  return true;
}

void CDVDOverlayCodecFFmpeg::Dispose()
{
  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  FreeSubtitle(m_Subtitle);

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

void CDVDOverlayCodecFFmpeg::FreeSubtitle(AVSubtitle& sub)
{
  for(unsigned i=0;i<sub.num_rects;i++)
  {
#if LIBAVCODEC_VERSION_INT >= (52<<10)
    if(sub.rects[i])
    {
      m_dllAvUtil.av_free(sub.rects[i]->pict.data[0]);
      m_dllAvUtil.av_free(sub.rects[i]->pict.data[1]);
      m_dllAvUtil.av_freep(&sub.rects[i]);
    }
#else
    if(sub.rects[i].bitmap)
      m_dllAvUtil.av_freep(&sub.rects[i].bitmap);
    if(m_Subtitle.rects[i].rgba_palette)
      m_dllAvUtil.av_freep(&sub.rects[i].rgba_palette);
#endif
  }
  if(sub.rects)
    m_dllAvUtil.av_freep(&sub.rects);
  sub.num_rects = 0;
}

int CDVDOverlayCodecFFmpeg::Decode(BYTE* data, int size, double pts, double duration)
{
  if (!m_pCodecContext)
    return 1;

  int gotsub = 0, len = 0;

  FreeSubtitle(m_Subtitle);

  try
  {
    len = m_dllAvCodec.avcodec_decode_subtitle(m_pCodecContext, &m_Subtitle, &gotsub, data, size);
  }
  catch (win32_exception e)
  {
    e.writelog("avcodec_decode_subtitle");
    return OC_ERROR;
  }

  if (len < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_subtitle returned failure", __FUNCTION__);
    return OC_ERROR;
  }

  if (len != size)
    CLog::Log(LOGWARNING, "%s - avcodec_decode_subtitle didn't consume the full packet", __FUNCTION__);

  if (!gotsub)
    return OC_BUFFER;

  m_SubtitleIndex = 0;

  return OC_OVERLAY;
}

void CDVDOverlayCodecFFmpeg::Reset()
{
  Flush();
}

void CDVDOverlayCodecFFmpeg::Flush()
{
  FreeSubtitle(m_Subtitle);
  m_SubtitleIndex = -1;

  try {

  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  } catch (win32_exception e) {
    e.writelog(__FUNCTION__);
  }
}

CDVDOverlay* CDVDOverlayCodecFFmpeg::GetOverlay()
{
  if(m_SubtitleIndex<0)
    return NULL;

  if(m_Subtitle.num_rects == 0 && m_SubtitleIndex == 0)
  {
    // we must add an empty overlay to replace the previous one
    CDVDOverlay* o = new CDVDOverlay(DVDOVERLAY_TYPE_NONE);
    o->iPTSStartTime = 0;
    o->iPTSStopTime  = 0;
    o->replace  = true;
    m_SubtitleIndex++;
    return o;
  }

  if(m_Subtitle.format == 0)
  {
    if(m_SubtitleIndex >= (int)m_Subtitle.num_rects)
      return NULL;

#if LIBAVCODEC_VERSION_INT >= (52<<10)
    if(m_Subtitle.rects[m_SubtitleIndex] == NULL)
      return NULL;
    AVSubtitleRect& rect = *m_Subtitle.rects[m_SubtitleIndex];
#else
    AVSubtitleRect& rect = m_Subtitle.rects[m_SubtitleIndex];
#endif

    CDVDOverlayImage* overlay = new CDVDOverlayImage();

    overlay->iPTSStartTime = DVD_MSEC_TO_TIME(m_Subtitle.start_display_time);
    overlay->iPTSStopTime  = DVD_MSEC_TO_TIME(m_Subtitle.end_display_time);
    overlay->replace  = true;
    overlay->linesize = rect.w;
    overlay->data     = (BYTE*)malloc(rect.w * rect.h);
    overlay->palette  = (uint32_t*)malloc(rect.nb_colors*4);
    overlay->palette_colors = rect.nb_colors;
    overlay->x        = rect.x;
    overlay->y        = rect.y;
    overlay->width    = rect.w;
    overlay->height   = rect.h;

    int right  = overlay->x + overlay->width;
    int bottom = overlay->y + overlay->height;

    if(m_height == 0 && m_pCodecContext->height)
      m_height = m_pCodecContext->height;
    if(m_width  == 0 && m_pCodecContext->width)
      m_width  = m_pCodecContext->width;

    if(bottom > m_height)
    {
      if     (bottom <= 480)
        m_height      = 480;
      else if(bottom <= 576)
        m_height      = 576;
      else if(bottom <= 720)
        m_height      = 720;
      else if(bottom <= 1080)
        m_height      = 1080;
      else
        m_height      = bottom;
    }
    if(right > m_width)
    {
      if     (right <= 720)
        m_width      = 720;
      else if(right <= 1024)
        m_width      = 1024;
      else if(right <= 1280)
        m_width      = 1280;
      else if(right <= 1920)
        m_width      = 1920;
      else
        m_width      = right;
    }

    overlay->source_width  = m_width;
    overlay->source_height = m_height;

#if LIBAVCODEC_VERSION_INT >= (52<<10)
    BYTE* s = rect.pict.data[0];
    BYTE* t = overlay->data;
    for(int i=0;i<rect.h;i++)
    {
      memcpy(t, s, rect.w);
      s += rect.pict.linesize[0];
      t += overlay->linesize;
    }

    for(int i=0;i<rect.nb_colors;i++)
      overlay->palette[i] = Endian_SwapLE32(((uint32_t *)rect.pict.data[1])[i]);

    m_dllAvUtil.av_free(rect.pict.data[0]);
    m_dllAvUtil.av_free(rect.pict.data[1]);
    m_dllAvUtil.av_freep(&m_Subtitle.rects[m_SubtitleIndex]);
#else
    BYTE* s = rect.bitmap;
    BYTE* t = overlay->data;
    for(int i=0;i<rect.h;i++)
    {
      memcpy(t, s, rect.w);
      s += rect.linesize;
      t += overlay->linesize;
    }

    memcpy(overlay->palette, rect.rgba_palette, rect.nb_colors*4);

    m_dllAvUtil.av_freep(&rect.bitmap);
    m_dllAvUtil.av_freep(&rect.rgba_palette);
#endif
    m_SubtitleIndex++;

    return overlay;
  }

  return NULL;
}
