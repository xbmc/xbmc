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

#include "system.h"

#include "StageFrightVideo.h"

#include "android/activity/XBMCApp.h"
#include "guilib/GraphicContext.h"
#include "DVDClock.h"
#include "threads/Event.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <ui/GraphicBuffer.h>

#include <new>
#include <map>
#include <queue>

#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00
//#define STAGEFRIGHT_DEBUG_VERBOSE 1
#define CLASSNAME "CStageFrightVideo"
#define MINBUFIN 50

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
#define GETEXTENSION(type, ext) \
do \
{ \
    ext = (type) eglGetProcAddress(#ext); \
    if (!ext) \
    { \
        CLog::Log(LOGERROR, "CLinuxRendererGLES::%s - ERROR getting proc addr of " #ext "\n", __func__); \
    } \
} while (0);

#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#define EGL_IMAGE_PRESERVED_KHR   0x30D2

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

class CStageFrightVideoPrivate : public MediaBufferObserver
{
public:
  CStageFrightVideoPrivate()
    : source(NULL)
    , width(-1), height(-1)
    , cur_frame(NULL), prev_frame(NULL)
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

  sp<MetaData> meta;
  sp<MediaSource> source;
  List<Frame*> in_queue;
  std::map<EGLImageKHR, MediaBuffer*> out_map;
  pthread_mutex_t in_mutex;
  pthread_cond_t condition;
  
  Frame *cur_frame;
  Frame *prev_frame;
  bool source_done;
  int x, y;
  int width, height;

  OMXClient *client;
  sp<MediaSource> decoder;
  const char *decoder_component;
  int videoColorFormat;
  int videoStride;
  int videoSliceHeight;
  
  bool drop_state;
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int cycle_time;
#endif
};

/********************************************/

class CustomSource : public MediaSource
{
public:
  CustomSource(CStageFrightVideoPrivate *priv, sp<MetaData> meta)
  {
    p = priv;
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
    *buffer = NULL;

    pthread_mutex_lock(&p->in_mutex);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: reading source(%d)\n", CLASSNAME,p->in_queue.size());
#endif
    
    if (p->in_queue.empty())
    {
      pthread_mutex_unlock(&p->in_mutex);
      return ERROR_END_OF_STREAM;
    }

    frame = *p->in_queue.begin();
    ret = frame->status;

    if (ret == OK)
      *buffer = frame->medbuf->clone();

    p->in_queue.erase(p->in_queue.begin());
    pthread_mutex_unlock(&p->in_mutex);
    
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> exiting reading source(%d); pts:%llu\n", p->in_queue.size(),frame->pts);
#endif

    frame->medbuf->release();
    free(frame);

    return ret;
  }

private:
  sp<MetaData> source_meta;
  CStageFrightVideoPrivate *p;
};

/***********************************************************/

bool CStageFrightVideo::Open(CDVDStreamInfo &hints)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Open\n", CLASSNAME);
#endif

  CSingleLock lock(g_graphicsContext);

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

  p = new CStageFrightVideoPrivate;
  p->width     = hints.width;
  p->height    = hints.height;
 
  sp<MetaData> outFormat;
  int32_t cropLeft, cropTop, cropRight, cropBottom;

  p->meta = new MetaData;
  if (p->meta == NULL)
  {
    goto fail;
  }
  p->meta->setCString(kKeyMIMEType, mimetype);
  p->meta->setInt32(kKeyWidth, p->width);
  p->meta->setInt32(kKeyHeight, p->height);
  p->meta->setData(kKeyAVCC, kTypeAVCC, hints.extradata, hints.extrasize);

  android::ProcessState::self()->startThreadPool();

  p->source    = new CustomSource(p, p->meta);
  p->client    = new OMXClient;

  if (p->source == NULL || p->client == NULL)
  {
    goto fail;
  }

  if (p->client->connect() !=  OK)
  {
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot connect OMX client");
    goto fail;
  }  
  
  g_xbmcapp.InitStagefrightSurface();

  p->decoder  = OMXCodec::Create(p->client->interface(), p->meta,
                                         false, p->source, NULL,
                                         OMXCodec::kHardwareCodecsOnly,
                                         g_xbmcapp.GetAndroidVideoWindow());

  if (!(p->decoder != NULL && p->decoder->start() ==  OK))
  {
    p->client->disconnect();
    g_xbmcapp.UninitStagefrightSurface();
    goto fail;
  }

  outFormat = p->decoder->getFormat();
  if (!outFormat->findInt32(kKeyWidth, &p->width) || !outFormat->findInt32(kKeyHeight, &p->height) 
        || !outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat))
  {
    p->client->disconnect();
    g_xbmcapp.UninitStagefrightSurface();
    goto fail;
  }
  /*
  if (p->videoColorFormat != OMX_COLOR_FormatYUV420Planar &&
        p->videoColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
        p->videoColorFormat != OMX_QCOM_COLOR_FormatYVU420SemiPlanar)
  {
    CLog::Log(LOGERROR, "%s::%s - %s: %d\n", CLASSNAME, __func__,"Unsupported color format",p->videoColorFormat);
    p->client->disconnect();
    g_xbmcapp.UninitStagefrightSurface();
    goto fail;
  }
  */

  if (!outFormat->findInt32(kKeyStride, &p->videoStride))
    p->videoStride = p->width;
  if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
    p->videoSliceHeight = p->height;
  
  if (!outFormat->findRect(kKeyCropRect, &cropLeft, &cropTop, &cropRight, &cropBottom)) 
  {
    p->x = 0;   
    p->y = 0;   
  }
  else
  {
    p->x = cropLeft;   
    p->y = cropTop;
    p->width = cropRight - cropLeft + 1;
    p->height = cropBottom - cropTop + 1;
  }
  
  pthread_mutex_init(&p->in_mutex, NULL);
  pthread_cond_init(&p->condition, NULL);
  
  p->client->disconnect();

  if (!eglCreateImageKHR)
  {
    GETEXTENSION(PFNEGLCREATEIMAGEKHRPROC,  eglCreateImageKHR);
  }
  if (!eglDestroyImageKHR)
  {
    GETEXTENSION(PFNEGLDESTROYIMAGEKHRPROC, eglDestroyImageKHR);
  }

  return true;

fail:
  if (p->client)
    delete p->client;
  return false;
}

/*** Decode ***/
int  CStageFrightVideo::Decode(BYTE *pData, int iSize, double dts, double pts)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::Decode - d:%p; s:%d; dts:%f; pts:%f\n", CLASSNAME, pData, iSize, dts, pts);
  if (p->cycle_time != 0)
    CLog::Log(LOGDEBUG, ">>> cycle dur:%d\n", XbmcThreads::SystemClockMillis() - p->cycle_time);
  p->cycle_time = time;
#endif

  Frame *frame;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;
  int ret = VC_BUFFER;
  int32_t keyframe = 0;
  int32_t unreadable = 0;
    
  if (demuxer_content)
  {
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame)
      return VC_ERROR;

    frame->status  = OK;
    frame->pts = (dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : ((pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : 0);
    frame->medbuf = p->getBuffer(demuxer_bytes);
    if (!frame->medbuf)
    {
      free(frame);
      return VC_ERROR;
    }
    fast_memcpy(frame->medbuf->data(), demuxer_content, demuxer_bytes);
    frame->medbuf->meta_data()->clear();
    frame->medbuf->meta_data()->setInt64(kKeyTime, frame->pts);

    pthread_mutex_lock(&p->in_mutex);
    p->in_queue.push_back(frame);
    pthread_cond_signal(&p->condition);
    pthread_mutex_unlock(&p->in_mutex);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d); tm:%d\n", CLASSNAME,p->in_queue.size(), XbmcThreads::SystemClockMillis() - time);
#endif
  }
  
  if (p->in_queue.size() < MINBUFIN)
    return ret;

  /* Output */

	#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  time = XbmcThreads::SystemClockMillis();
	CLog::Log(LOGDEBUG, "%s: >>> Handling frame\n", CLASSNAME);
	#endif
  int32_t w, h, dw, dh;
	frame = (Frame*)malloc(sizeof(Frame));
	if (!frame) {
    p->cur_frame = NULL;
	  return VC_ERROR;
  }
  
	frame->medbuf = NULL;
	frame->status = p->decoder->read(&frame->medbuf);
	if (frame->status == INFO_FORMAT_CHANGED)
	{
	  sp<MetaData> outFormat = p->decoder->getFormat();

    outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat);
    if (!outFormat->findInt32(kKeyStride, &p->videoStride))
      p->videoStride = p->width;
    if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
      p->videoSliceHeight = p->height;
    
	  if (frame->medbuf)
      frame->medbuf->release();
	  free(frame);
    p->cur_frame = NULL;
    return ret;
	}
	if (frame->status == OK)
	{
    if (!frame->medbuf)
      return ret;
            
	  sp<MetaData> outFormat = p->decoder->getFormat();
	  outFormat->findInt32(kKeyWidth , &w);
	  outFormat->findInt32(kKeyHeight, &h);
    
 	  if (!outFormat->findInt32(kKeyDisplayWidth , &dw))
      dw = w;
	  if (!outFormat->findInt32(kKeyDisplayHeight, &dh))
      dh = h;
   
    if (!outFormat->findInt32(kKeyIsSyncFrame, &keyframe))
      keyframe = 0;
    if (!outFormat->findInt32(kKeyIsUnreadable, &unreadable))
      unreadable = 0;
      
	  frame->pts = 0;

	  // The OMX.SEC decoder doesn't signal the modified width/height
	  if (p->decoder_component && !strncmp(p->decoder_component, "OMX.SEC", 7) &&
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
    frame->medbuf->meta_data()->findInt64(kKeyTime, &(frame->pts));
    if (!frame->medbuf->graphicBuffer().get())  // hw buffers
    {
      if (frame->medbuf->range_length() == 0)
      {
        frame->medbuf->release();
        frame->medbuf = NULL;
        return ret;
      }
    }
	}
  else if (frame->status == INFO_FORMAT_CHANGED)
	{
    int32_t cropLeft, cropTop, cropRight, cropBottom;
	  sp<MetaData> outFormat = p->decoder->getFormat();

    if (!outFormat->findRect(kKeyCropRect, &cropLeft, &cropTop, &cropRight, &cropBottom)) 
    {
      p->x = 0;   
      p->y = 0;   
    }
    else
    {
      p->x = cropLeft;   
      p->y = cropTop;
      p->width = cropRight - cropLeft + 1;
      p->height = cropBottom - cropTop + 1;
    }
    outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat);
    if (!outFormat->findInt32(kKeyStride, &p->videoStride))
      p->videoStride = p->width;
    if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
      p->videoSliceHeight = p->height;
    
	  if (frame->medbuf)
      frame->medbuf->release();
	  free(frame);
    p->cur_frame = NULL;
    return ret;
	}
	else
	{
	  CLog::Log(LOGERROR, "%s - decoding error (%d)\n", CLASSNAME,frame->status);
	  if (frame->medbuf)
      frame->medbuf->release();
	  free(frame);
    p->cur_frame = NULL;
    return VC_ERROR;
	}
  
  p->cur_frame = frame;
	#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
	CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame; w:%d, h:%d, dw:%d, dh:%d, kf:%d, ur:%d, tm:%d\n", CLASSNAME, w, h, dw, dh, keyframe, unreadable, XbmcThreads::SystemClockMillis() - time);
	#endif

  if (p->cur_frame)
    ret |= VC_PICTURE;

  return ret;
}

bool CStageFrightVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
 #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  if (p->prev_frame) {
    if (p->prev_frame->medbuf)
    {
      if (pDvdVideoPicture->format == RENDER_FMT_ANDOES)
        ReleaseOutputBuffer(pDvdVideoPicture->eglimg);
      else
        p->prev_frame->medbuf->release();
    }
    free(p->prev_frame);
    p->prev_frame = NULL;
  }
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ClearPicture (%d)\n", CLASSNAME, XbmcThreads::SystemClockMillis() - time);
#endif

  return true;
}

bool CStageFrightVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::GetPicture\n", CLASSNAME);
#endif

  status_t status;

  if (!p->cur_frame)
    return false;
    
  Frame *frame = p->cur_frame;
  status  = frame->status;

  if (status != OK)
  {
    CLog::Log(LOGERROR, "%s::%s - Error getting picture from frame(%d)\n", CLASSNAME, __func__,status);
    if (frame->medbuf) {
      frame->medbuf->release();
    }
    free(frame);
    p->cur_frame = NULL;
    return false;
  }

  pDvdVideoPicture->format = RENDER_FMT_BYPASS;
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->iWidth  = frame->width;
  pDvdVideoPicture->iHeight = frame->height;
  pDvdVideoPicture->iDisplayWidth = frame->width;
  pDvdVideoPicture->iDisplayHeight = frame->height;
  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->pts = pts_itod(frame->pts);
  pDvdVideoPicture->eglimg = EGL_NO_IMAGE_KHR;
  
  if (frame->medbuf)
  {
    if (frame->medbuf->graphicBuffer() != 0)
    {
      pDvdVideoPicture->format = RENDER_FMT_ANDOES;
	  
      android::GraphicBuffer* gb = static_cast<android::GraphicBuffer*>( frame->medbuf->graphicBuffer().get() );
      EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
      pDvdVideoPicture->eglimg = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
                                  EGL_NATIVE_BUFFER_ANDROID,
                                  (EGLClientBuffer)gb->getNativeBuffer(),
                                  eglImgAttrs);                                  
      if (pDvdVideoPicture->eglimg == EGL_NO_IMAGE_KHR)
      {
        frame->medbuf->release();
        pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;
      }
      else
      {
        p->out_map.insert( std::pair<EGLImageKHR, MediaBuffer*>(pDvdVideoPicture->eglimg, frame->medbuf) );
        pDvdVideoPicture->stf = this;
      }
      
    #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> pic pts:%f, textured, tm:%d\n", pDvdVideoPicture->pts, XbmcThreads::SystemClockMillis() - time);
    #endif
    }
    else
    {
      pDvdVideoPicture->format = RENDER_FMT_YUV420P;
      pDvdVideoPicture->color_range  = 0;
      pDvdVideoPicture->color_matrix = 4;
    #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> pic pts:%f, data:%p, tm:%d\n", pDvdVideoPicture->pts, frame->medbuf, XbmcThreads::SystemClockMillis() - time);
    #endif

      unsigned int luma_pixels = frame->width * frame->height;
      unsigned int chroma_pixels = luma_pixels/4;
      BYTE* data = NULL;
      if (frame->medbuf && !p->drop_state)
        data = (BYTE*)(frame->medbuf->data() + frame->medbuf->range_offset());
      switch (p->videoColorFormat)
      {
        case OMX_COLOR_FormatYUV420Planar:
          pDvdVideoPicture->iLineSize[0] = p->videoStride;
          pDvdVideoPicture->iLineSize[1] = p->videoStride / 2;
          pDvdVideoPicture->iLineSize[2] = p->videoStride / 2;
          pDvdVideoPicture->iLineSize[3] = 0;
          pDvdVideoPicture->data[0] = data;
          pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + (p->videoStride  * p->videoSliceHeight);
          pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + (p->videoStride/2 * p->videoSliceHeight/2);
          pDvdVideoPicture->data[3] = 0;
          break;
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
          pDvdVideoPicture->iLineSize[0] = p->videoStride;
          pDvdVideoPicture->iLineSize[1] = p->videoStride;
          pDvdVideoPicture->iLineSize[2] = 0;
          pDvdVideoPicture->iLineSize[3] = 0;
          pDvdVideoPicture->data[0] = data;
          pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + (p->videoStride  * p->videoSliceHeight);
          pDvdVideoPicture->data[2] = 0;
          pDvdVideoPicture->data[3] = 0;
          break;
      }
    }
  }
  else
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  p->prev_frame = p->cur_frame;
  p->cur_frame = NULL;

  return true;
}

void CStageFrightVideo::Close()
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  Frame *frame;

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning IN(%d)\n", p->in_queue.size());
#endif
  while (!p->in_queue.empty())
  {
    frame = *p->in_queue.begin();
    p->in_queue.erase(p->in_queue.begin());
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning OUT(%d)\n",  p->out_map.size());
#endif
  std::map<EGLImageKHR, MediaBuffer*>::iterator it;
  while (!p->out_map.empty())
  {
    it = p->out_map.begin();
    it->second->release();
    p->out_map.erase(it);
  }

  if (p->cur_frame)
    free(p->cur_frame);
  if (p->prev_frame)
    free(p->prev_frame);

  p->decoder->stop();
  p->client->disconnect();

  if (p->decoder_component)
    free(&p->decoder_component);

  delete p->client;

  pthread_mutex_destroy(&p->in_mutex);
  pthread_cond_destroy(&p->condition);
  delete p;
  
  g_xbmcapp.UninitStagefrightSurface();
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
  
  p->drop_state = bDrop;
}

/***********************************************/

void CStageFrightVideo::LockOutputBuffer(EGLImageKHR eglimg)
{
 #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
 std::map<EGLImageKHR, MediaBuffer*>::iterator it;
  it = p->out_map.find(eglimg);
 
 if (it != p->out_map.end())
    it->second->add_ref();
  else
    CLog::Log(LOGERROR, "%s::LockOutputBuffer: EGL image not found\n", CLASSNAME);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::LockOutputBuffer (%d)\n", CLASSNAME, XbmcThreads::SystemClockMillis() - time);
#endif
}

void CStageFrightVideo::ReleaseOutputBuffer(EGLImageKHR eglimg)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  std::map<EGLImageKHR, MediaBuffer*>::iterator it;
  it = p->out_map.find(eglimg);

  if (it != p->out_map.end())
  {
    if(it->second->refcount() > 1)
    {
      it->second->release();
      return;
    }
      
    eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), it->first);
    it->second->release();
    p->out_map.erase(it);
  }
  else
    CLog::Log(LOGERROR, "%s::ReleaseOutputBuffer: EGL image not found\n", CLASSNAME);
    
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ReleaseOutputBuffer (%d): buf:%d\n", CLASSNAME, XbmcThreads::SystemClockMillis() - time, p->out_map.size());
#endif

}
