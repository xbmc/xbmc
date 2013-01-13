/*
 *      Copyright (C) 2010-2012 Team XBMC
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
/***************************************************************************/
/*
 * Interface to the Android Stagefright library for
 * H/W accelerated H.264 decoding
 *
 * Copyright (C) 2011 Mohamed Naufal
 * Copyright (C) 2011 Martin Storsjö
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "system.h"

#include "StageFrightVideo.h"

#include "DVDClock.h"
#include "threads/Event.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <utils/List.h>
#include <utils/RefBase.h>

#include <new>
#include <map>
#include <queue>

#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00
#define STAGEFRIGHT_DEBUG_VERBOSE 1
#define CLASSNAME "CStageFrightVideo"
#define MAXBUFIN 10

const char *MEDIA_MIMETYPE_VIDEO_WMV  = "video/x-ms-wmv";

using namespace android;

static int64_t pts_dtoi(double pts)
{
  return (int64_t)(pts);
}

static double pts_itod(int64_t pts)
{
  return (double)pts;
}

struct Frame
{
  status_t status;
  int32_t width, height;
  int64_t pts;
  MediaBuffer* medbuf;
};

class StagefrightContext : public MediaBufferObserver
{
public:
  StagefrightContext()
    : source(NULL)
    , width(-1), height(-1)
    , end_frame(NULL)
    , source_done(false), thread_started(0), thread_exited(0), stop_decode(0)
    , dummy_medbuf(NULL)
    , client(NULL), decoder(NULL), decoder_component(NULL)
    , drop_state(false)
  {}

  virtual void signalBufferReturned(MediaBuffer *buffer)
  {
    buffer->setObserver(NULL);
    buffer->release();
  }

  MediaBuffer* getBuffer(size_t size)
  {
    MediaBuffer* buf = new MediaBuffer(size);
    buf->setObserver(this);
    buf->add_ref();
    return buf;
  }

  sp<MediaSource> *source;
  List<Frame*> in_queue;
  std::map<int64_t, Frame*> out_queue;
  pthread_mutex_t in_mutex, out_mutex;
  pthread_cond_t condition;
  pthread_t decode_thread_id;

  int32_t width, height;

  Frame *end_frame;
  bool source_done;
  volatile sig_atomic_t thread_started, thread_exited, stop_decode;

  MediaBuffer* dummy_medbuf;

  OMXClient *client;
  sp<MediaSource> *decoder;
  const char *decoder_component;
  
  bool drop_state;
};

class CustomSource : public MediaSource
{
public:
  CustomSource(StagefrightContext *ctx, sp<MetaData> meta)
  {
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: creating source\n", CLASSNAME);
#endif
    s = ctx;
    source_meta = meta;
  }

  virtual sp<MetaData> getFormat()
  {
    return source_meta;
  }

  virtual status_t start(MetaData *params)
  {
    return OK;
  }

  virtual status_t stop()
  {
    return OK;
  }

  virtual status_t read(MediaBuffer **buffer,
                        const MediaSource::ReadOptions *options)
  {
    Frame *frame;
    status_t ret;

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: reading source(%d)\n", CLASSNAME,s->in_queue.size());
#endif

    *buffer = NULL;
    if (s->thread_exited)
      return ERROR_END_OF_STREAM;

    pthread_mutex_lock(&s->in_mutex);

    while (s->in_queue.empty())
      pthread_cond_wait(&s->condition, &s->in_mutex);

    frame = *s->in_queue.begin();
    ret = frame->status;

    if (ret == OK)
      *buffer = frame->medbuf->clone();

    s->in_queue.erase(s->in_queue.begin());
    pthread_mutex_unlock(&s->in_mutex);
    
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> exiting reading source(%d); pts:%llu\n", s->in_queue.size(),frame->pts);
#endif

    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);

    return ret;
  }

private:
  sp<MetaData> source_meta;
  StagefrightContext *s;
};

void* decode_thread(void *arg)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s: entering decode thread\n", CLASSNAME);
#endif

  StagefrightContext *s = (StagefrightContext*)arg;
  Frame* frame;
  int32_t w, h;
  int decode_done = 0;

  do
  {
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: >>> Handling frame\n", CLASSNAME);
#endif
    frame = (Frame*)malloc(sizeof(Frame));
    MediaBuffer* buffer = NULL;
    frame->medbuf = NULL;
    if (!frame)
    {
      frame         = s->end_frame;
      frame->status = VC_ERROR;
      decode_done   = 1;
      s->end_frame  = NULL;
      goto push_frame;
    }
    frame->status = (*s->decoder)->read(&buffer);
    if (frame->status == INFO_FORMAT_CHANGED)
    {
      if (buffer)
        buffer->release();
      free(frame);
      continue;
    }
    else if (frame->status == OK)
    {
      sp<MetaData> outFormat = (*s->decoder)->getFormat();
      outFormat->findInt32(kKeyWidth , &w);
      outFormat->findInt32(kKeyHeight, &h);
      frame->pts = 0;

      // The OMX.SEC decoder doesn't signal the modified width/height
      if (s->decoder_component && !strncmp(s->decoder_component, "OMX.SEC", 7) &&
          (w & 15 || h & 15))
      {
        if (((w + 15)&~15) * ((h + 15)&~15) * 3/2 == frame->medbuf->range_length())
        {
          w = (w + 15)&~15;
          h = (h + 15)&~15;
        }
      }
      frame->width = w;
      frame->height = h;
      if (buffer)
      {
        buffer->meta_data()->findInt64(kKeyTime, &(frame->pts));
        if (!s->drop_state)
        {
          frame->medbuf = new MediaBuffer(buffer->range_length());
          memcpy(frame->medbuf->data(), buffer->data() + buffer->range_offset(), buffer->range_length());
        }
        buffer->release();
      }
    }
    else
    {
      CLog::Log(LOGERROR, "%s - decoding error (%d)\n", CLASSNAME,frame->status);
      if (buffer)
        buffer->release();
      free(frame);
      continue;
    }
push_frame:
    pthread_mutex_lock(&s->out_mutex);
    s->out_queue[frame->pts] = frame;
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame (%d)\n", CLASSNAME, s->out_queue.size());
#endif
    pthread_mutex_unlock(&s->out_mutex);
  }
  while (!decode_done && !s->stop_decode);

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s: exiting decode thread(%d)\n", CLASSNAME,s->out_queue.size());
#endif

  s->thread_exited = 1;

  return 0;
}

bool CStageFrightVideo::Open(CDVDStreamInfo &hints)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Open\n", CLASSNAME);
#endif

  // stagefright crashes with null size. Trap this...
  if (!hints.width || !hints.height)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"null size, cannot handle");
    return false;
  }

  const char* mimetype;
  switch (hints.codec)
  {
  case CODEC_ID_H264:
    mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
    break;
  case CODEC_ID_MPEG4:
    mimetype = MEDIA_MIMETYPE_VIDEO_MPEG4;
    break;
  case CODEC_ID_MPEG2VIDEO:
    mimetype = MEDIA_MIMETYPE_VIDEO_MPEG2;
    break;
  case CODEC_ID_VP8:
    mimetype = MEDIA_MIMETYPE_VIDEO_VPX;
    break;
  case CODEC_ID_VC1:
    mimetype = MEDIA_MIMETYPE_VIDEO_WMV;
    break;
  default:
    return false;
    break;
  }

  sp<MetaData> meta, outFormat;
  int32_t colorFormat = 0;

  m_context = new StagefrightContext;
  m_context->width     = hints.width;
  m_context->height    = hints.height;
 
  meta = new MetaData;
  if (meta == NULL)
  {
    goto fail;
  }
  meta->setCString(kKeyMIMEType, mimetype);
  meta->setInt32(kKeyWidth, m_context->width);
  meta->setInt32(kKeyHeight, m_context->height);
  meta->setInt32(kKeyIsSyncFrame,0);
  meta->setData(kKeyAVCC, kTypeAVCC, hints.extradata, hints.extrasize);

  android::ProcessState::self()->startThreadPool();

  m_context->source    = new sp<MediaSource>();
  *m_context->source   = new CustomSource(m_context, meta);
  m_context->client    = new OMXClient;
  m_context->end_frame = (Frame*)malloc(sizeof(Frame));
  m_context->end_frame->medbuf = NULL;

  if (m_context->source == NULL || !m_context->client || !m_context->end_frame)
  {
    goto fail;
  }

  if (m_context->client->connect() !=  OK)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot connect OMX client");
    goto fail;
  }
  
  m_context->decoder  = new sp<MediaSource>();
  *m_context->decoder = OMXCodec::Create(m_context->client->interface(), meta,
                                         false, *m_context->source, NULL,
                                         OMXCodec::kClientNeedsFramebuffer | OMXCodec::kHardwareCodecsOnly);
  if (!((*m_context->decoder != NULL) && ((*m_context->decoder)->start() ==  OK)))
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot start decoder");
    m_context->client->disconnect();
    goto fail;
  }

  outFormat = (*m_context->decoder)->getFormat();
  outFormat->findInt32(kKeyColorFormat, &colorFormat);
  if (colorFormat != OMX_COLOR_FormatYUV420Planar)
  {
    CLog::Log(LOGERROR, "%s::%s - %s: %d\n", CLASSNAME, __func__,"Unsupported color format",colorFormat);
    m_context->client->disconnect();
    goto fail;
  }

  outFormat->findCString(kKeyDecoderComponent, &m_context->decoder_component);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - Decoder: %s\n", CLASSNAME, __func__,m_context->decoder_component);
#endif

  pthread_mutex_init(&m_context->in_mutex, NULL);
  pthread_mutex_init(&m_context->out_mutex, NULL);
  pthread_cond_init(&m_context->condition, NULL);
  
  pthread_create(&(m_context->decode_thread_id), NULL, &decode_thread, m_context);
  m_context->thread_started = 1;

  return true;

fail:
  free(m_context->end_frame);
  delete m_context->client;
  return false;
}

int  CStageFrightVideo::Decode(BYTE *pData, int iSize, double dts, double pts)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Decode - d:%p; s:%d; dts:%f; pts:%f\n", CLASSNAME, pData, iSize, dts, pts);
#endif

  Frame *frame;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;

  if (m_context->thread_exited)
    m_context->source_done = true;

  if (!m_context->source_done && demuxer_content)
  {
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame)
    {
      return VC_ERROR;
    }
    frame->status  = OK;
    frame->pts = (dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : ((pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : 0);
    frame->medbuf = m_context->getBuffer(demuxer_bytes);
    if (!frame->medbuf)
    {
      free(frame);
      return VC_ERROR;
    }
    fast_memcpy(frame->medbuf->data(), demuxer_content, demuxer_bytes);
    frame->medbuf->meta_data()->clear();
    frame->medbuf->meta_data()->setInt64(kKeyTime, frame->pts);

    if(!m_context->dummy_medbuf)
    {
      m_context->dummy_medbuf = m_context->getBuffer(frame->medbuf->size());
      m_context->dummy_medbuf->meta_data()->setInt64(kKeyTime, frame->pts);
      if (m_context->dummy_medbuf)
        fast_memcpy(m_context->dummy_medbuf->data(), frame->medbuf->data(), frame->medbuf->size());
    }

    while (true) {
      if (m_context->thread_exited) {
          m_context->source_done = true;
          break;
      }
      pthread_mutex_lock(&m_context->in_mutex);
      if (m_context->in_queue.size() >= MAXBUFIN && m_context->out_queue.empty()) {
          pthread_mutex_unlock(&m_context->in_mutex);
          usleep(5000);
          continue;
      }
      m_context->in_queue.push_back(frame);
      pthread_cond_signal(&m_context->condition);
      pthread_mutex_unlock(&m_context->in_mutex);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d)\n", CLASSNAME,m_context->in_queue.size());
#endif
      if (m_context->out_queue.empty())
        usleep(5000);
      break;
    }
  }

  int ret = VC_BUFFER;
  pthread_mutex_lock(&m_context->out_mutex);
  if (m_context->out_queue.size() > 0)
    ret |= VC_PICTURE;
  pthread_mutex_unlock(&m_context->out_mutex);

  return ret;
}

bool CStageFrightVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::GetPicture(%d)\n", CLASSNAME,m_context->out_queue.size());
#endif

  bool ret = true;
  status_t status;

  pthread_mutex_lock(&m_context->out_mutex);
  if (m_context->out_queue.empty())
    ret = false;
  pthread_mutex_unlock(&m_context->out_mutex);

  if (!ret)
    return false;
    
  Frame *frame;
  frame = m_context->out_queue.begin()->second;
  pthread_mutex_lock(&m_context->out_mutex);
  m_context->out_queue.erase(m_context->out_queue.begin());
  pthread_mutex_unlock(&m_context->out_mutex);

  status  = frame->status;

  if (status != OK)
  {
    CLog::Log(LOGERROR, "%s::%s - Error getting picture from frame(%d)\n", CLASSNAME, __func__,status);
    if (frame->medbuf) {
      frame->medbuf->release();
    }
    free(frame);
    return false;
  }

  pDvdVideoPicture->format = RENDER_FMT_YUV420P;
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->color_range  = 0;
  pDvdVideoPicture->color_matrix = 4;
  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iWidth  = frame->width;
  pDvdVideoPicture->iHeight = frame->height;
  pDvdVideoPicture->iDisplayWidth = frame->width;
  pDvdVideoPicture->iDisplayHeight = frame->height;
  pDvdVideoPicture->pts = pts_itod(frame->pts);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> pic pts:%f, data:%p\n", pDvdVideoPicture->pts, frame->medbuf);
#endif

  unsigned int luma_pixels = frame->width * frame->height;
  unsigned int chroma_pixels = luma_pixels/4;
  BYTE* data = NULL;
  if (frame->medbuf)
    data = (BYTE*)(frame->medbuf->data() + frame->medbuf->range_offset());
  pDvdVideoPicture->iLineSize[0] = frame->width;
  pDvdVideoPicture->iLineSize[1] = frame->width / 2;
  pDvdVideoPicture->iLineSize[2] = frame->width / 2;
  pDvdVideoPicture->iLineSize[3] = 0;
  pDvdVideoPicture->data[0] = data;
  pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + (frame->width * frame->height);
  pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + (frame->width/2 * frame->height/2);
  pDvdVideoPicture->data[3] = 0;

  pDvdVideoPicture->stdcontext = (void *)frame;

  pDvdVideoPicture->iFlags |= data == NULL ? DVP_FLAG_DROPPED : 0;

  return true;
}

bool CStageFrightVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ClearPicture(%d)\n", CLASSNAME,m_context->out_queue.size());
#endif
  if (pDvdVideoPicture->stdcontext) {
    Frame* frame = static_cast<Frame*>(pDvdVideoPicture->stdcontext);
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
    pDvdVideoPicture->stdcontext = NULL;
  }

  return true;
}

void CStageFrightVideo::Close()
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  Frame *frame;

  if (m_context->thread_started)
  {
    if (!m_context->thread_exited)
    {
      m_context->stop_decode = 1;

      // Make sure decode_thread() doesn't get stuck
      pthread_mutex_lock(&m_context->out_mutex);
      while (!m_context->out_queue.empty())
      {
        frame = m_context->out_queue.begin()->second;
        m_context->out_queue.erase(m_context->out_queue.begin());
        if (frame->medbuf) {
          frame->medbuf->release();
        }
        free(frame);
      }
      pthread_mutex_unlock(&m_context->out_mutex);

      // Feed a dummy frame prior to signalling EOF.
      // This is required to terminate the decoder(OMX.SEC)
      // when only one frame is read during stream info detection.
      if (m_context->dummy_medbuf && (frame = (Frame*)malloc(sizeof(Frame))))
      {
        frame->status = OK;
        frame->medbuf = m_context->dummy_medbuf;
        pthread_mutex_lock(&m_context->in_mutex);
        m_context->in_queue.push_back(frame);
        pthread_cond_signal(&m_context->condition);
        pthread_mutex_unlock(&m_context->in_mutex);
      }

      pthread_mutex_lock(&m_context->in_mutex);
      m_context->end_frame->status = ERROR_END_OF_STREAM;
      m_context->in_queue.push_back(m_context->end_frame);
      pthread_cond_signal(&m_context->condition);
      pthread_mutex_unlock(&m_context->in_mutex);
    }

    pthread_join(m_context->decode_thread_id, NULL);

    m_context->thread_started = 0;
  }

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning IN(%d)\n", m_context->in_queue.size());
#endif
  while (!m_context->in_queue.empty())
  {
    frame = *m_context->in_queue.begin();
    m_context->in_queue.erase(m_context->in_queue.begin());
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning OUT(%d)\n", m_context->out_queue.size());
#endif
  while (!m_context->out_queue.empty())
  {
    frame = m_context->out_queue.begin()->second;
    m_context->out_queue.erase(m_context->out_queue.begin());
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }

  (*m_context->decoder)->stop();
  m_context->client->disconnect();

  if (m_context->decoder_component)
    free(&m_context->decoder_component);

  delete m_context->client;
  delete m_context->decoder;
  delete m_context->source;

  pthread_mutex_destroy(&m_context->in_mutex);
  pthread_mutex_destroy(&m_context->out_mutex);
  pthread_cond_destroy(&m_context->condition);

  delete m_context;
}

void CStageFrightVideo::Reset(void)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Reset\n", CLASSNAME);
#endif
  ::Sleep(100);
}

void CStageFrightVideo::SetDropState(bool bDrop)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::SetDropState (%d)\n", CLASSNAME,bDrop);
#endif
  
  m_context->drop_state = bDrop;

  if (bDrop)
  {
    Frame *frame;
    // blow all but the last ready video frame
    pthread_mutex_lock(&m_context->out_mutex);
    while (m_context->out_queue.size() > 1)
    {
      frame = m_context->out_queue.begin()->second;
      m_context->out_queue.erase(m_context->out_queue.begin());
      if (frame->medbuf)
        frame->medbuf->release();
      free(frame);
    }
    pthread_mutex_unlock(&m_context->out_mutex);

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::%s - bDrop(%d)\n",
              CLASSNAME, __func__, bDrop);
#endif
  }
}


