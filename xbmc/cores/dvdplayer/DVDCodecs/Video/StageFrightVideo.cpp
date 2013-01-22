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

//#define STAGEFRIGHT_DEBUG_VERBOSE 1

#include "system.h"
#include "system_gl.h"

#include "StageFrightVideo.h"

#include "android/activity/XBMCApp.h"
#include "guilib/GraphicContext.h"
#include "DVDClock.h"
#include "threads/Event.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"

#include "xbmc/guilib/FrameBufferObject.h"
#include "windowing/WindowingFactory.h"

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
#include <ui/PixelFormat.h>

#include <new>
#include <map>
#include <queue>
#include <list>

#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00
#define CLASSNAME "CStageFrightVideo"
#define MINBUFIN 10
#define MAXBUFIN 25
#define NUMFBOTEX 4

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
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

GLint glerror;
#define CheckEglError(x) while((glerror = eglGetError()) != EGL_SUCCESS) CLog::Log(LOGERROR, "EGL error in %s: %x",x, glerror);
#define CheckGlError(x)  while((glerror = glGetError()) != GL_NO_ERROR) CLog::Log(LOGERROR, "GL error in %s: %x",x, glerror);

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
  int duration;
  ERenderFormat format;
  MediaBuffer* medbuf;
};

class CStageFrightVideoPrivate : public MediaBufferObserver
{
public:
  CStageFrightVideoPrivate()
    : thread_exited(0), stop_decode(0)
    , source(NULL), natwin(NULL)
    , eglDisplay(EGL_NO_DISPLAY), eglSurface(EGL_NO_SURFACE), eglContext(EGL_NO_CONTEXT)
    , eglInitialized(false)
    , mDataSize(0), mTimeSize(0), mPrevPts(-1)
    , cur_frame(NULL), prev_frame(NULL)
    , width(-1), height(-1)
    , client(NULL), decoder(NULL), decoder_component(NULL)
    , drop_state(false), resetting(false)
  {}

  virtual void signalBufferReturned(MediaBuffer *buffer)
  {
    mDataSize -= buffer->size();
    buffer->setObserver(NULL);
    buffer->release();
  }

  MediaBuffer* getBuffer(size_t size)
  {
    MediaBuffer* buf = new MediaBuffer(size);
    buf->setObserver(this);
    buf->add_ref();
    mDataSize += size;
    return buf;
  }


  void loadOESShader(GLenum shaderType, const char* pSource, GLuint* outShader)
  {
    CLog::Log(LOGDEBUG, ">>loadOESShader\n");

    GLuint shader = glCreateShader(shaderType);
    CheckGlError("loadOESShader");
    if (shader) {
      glShaderSource(shader, 1, &pSource, NULL);
      glCompileShader(shader);
      GLint compiled = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
      if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
          char* buf = (char*) malloc(infoLen);
          if (buf) {
            glGetShaderInfoLog(shader, infoLen, NULL, buf);
            printf("Shader compile log:\n%s\n", buf);
            free(buf);
          }
        } else {
          char* buf = (char*) malloc(0x1000);
          if (buf) {
            glGetShaderInfoLog(shader, 0x1000, NULL, buf);
            printf("Shader compile log:\n%s\n", buf);
            free(buf);
          }
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
    *outShader = shader;
  }

  void createOESProgram(const char* pVertexSource, const char* pFragmentSource, GLuint* outPgm)
  {
    CLog::Log(LOGDEBUG, ">>createOESProgram\n");
    GLuint vertexShader, fragmentShader;
    {
      loadOESShader(GL_VERTEX_SHADER, pVertexSource, &vertexShader);
    }
    {
      loadOESShader(GL_FRAGMENT_SHADER, pFragmentSource, &fragmentShader);
    }

    GLuint program = glCreateProgram();
    if (program) {
      glAttachShader(program, vertexShader);
      glAttachShader(program, fragmentShader);
      glLinkProgram(program);
      GLint linkStatus = GL_FALSE;
      glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
      if (linkStatus != GL_TRUE) {
        GLint bufLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
        if (bufLength) {
          char* buf = (char*) malloc(bufLength);
          if (buf) {
            glGetProgramInfoLog(program, bufLength, NULL, buf);
            printf("Program link log:\n%s\n", buf);
            free(buf);
          }
        }
        glDeleteProgram(program);
        program = 0;
      }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    *outPgm = program;
  }

  void OES_shader_setUp()
  {

    const char vsrc[] =
    "attribute vec4 vPosition;\n"
    "varying vec2 texCoords;\n"
    "uniform mat4 texMatrix;\n"
    "void main() {\n"
    "  vec2 vTexCoords = 0.5 * (vPosition.xy + vec2(1.0, 1.0));\n"
    "  texCoords = (texMatrix * vec4(vTexCoords, 0.0, 1.0)).xy;\n"
    "  gl_Position = vPosition;\n"
    "}\n";

    const char fsrc[] =
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES texSampler;\n"
    "varying vec2 texCoords;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(texSampler, texCoords);\n"
    "}\n";

    {
      CLog::Log(LOGDEBUG, ">>OES_shader_setUp\n");
      CheckGlError("OES_shader_setUp");
      createOESProgram(vsrc, fsrc, &mPgm);
    }

    mPositionHandle = glGetAttribLocation(mPgm, "vPosition");
    mTexSamplerHandle = glGetUniformLocation(mPgm, "texSampler");
    mTexMatrixHandle = glGetUniformLocation(mPgm, "texMatrix");
  }
  pthread_t decode_thread_id;
  volatile sig_atomic_t thread_exited, stop_decode;

  sp<MediaSource> source;
  sp<ANativeWindow> natwin;

  GLuint mPgm;
  GLint mPositionHandle;
  GLint mTexSamplerHandle;
  GLint mTexMatrixHandle;

  CFrameBufferObject fbo;
  EGLDisplay eglDisplay;
  EGLSurface eglSurface;
  EGLContext eglContext;
  bool eglInitialized;

  struct tex_slot
  {
    GLuint texid;
    EGLImageKHR eglimg;
  };
  tex_slot slots[NUMFBOTEX];
  std::list< std::pair<EGLImageKHR, int> > free_queue;
  std::list< std::pair<EGLImageKHR, int> > busy_queue;

  sp<MetaData> meta;
  List<Frame*> in_queue;
  pthread_mutex_t in_mutex;
  pthread_cond_t in_condition;
  pthread_mutex_t out_mutex;
  pthread_cond_t out_condition;
  pthread_mutex_t free_mutex;

  int mDataSize;
  int64_t mTimeSize;
  int64_t mPrevPts;

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
  bool resetting;
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

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: reading source(%d)\n", CLASSNAME,p->in_queue.size());
#endif

    pthread_mutex_lock(&p->in_mutex);
    while (p->in_queue.empty() && !p->stop_decode)
      pthread_cond_wait(&p->in_condition, &p->in_mutex);

    if (p->in_queue.empty())
    {
      pthread_mutex_unlock(&p->in_mutex);
      return VC_ERROR;
    }
    
    frame = *p->in_queue.begin();
    ret = frame->status;

    if (ret == OK)
      *buffer = frame->medbuf->clone();

    p->in_queue.erase(p->in_queue.begin());
    p->mTimeSize -= frame->duration;
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

void* decode_thread(void *arg)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time;
  CLog::Log(LOGDEBUG, "%s: entering decode thread\n", CLASSNAME);
#endif

  CStageFrightVideoPrivate *p = (CStageFrightVideoPrivate*)arg;
  Frame* frame;
  int32_t w, h, dw, dh;
  int decode_done = 0;
  int32_t keyframe = 0;
  int32_t unreadable = 0;
  MediaSource::ReadOptions readopt;


  do
  {
    #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    time = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGDEBUG, "%s: >>> Handling frame\n", CLASSNAME);
    #endif
    p->cur_frame = NULL;
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame) 
    {
      frame->status = VC_ERROR;
      decode_done   = 1;
      continue;
    }

    frame->medbuf = NULL;
    if (p->resetting)
    {
      readopt.setSeekTo(0);
      p->resetting = false;
    }
    frame->status = p->decoder->read(&frame->medbuf, &readopt);
    readopt.clearSeekTo();
    if (frame->status == OK)
    {
      if (!frame->medbuf->graphicBuffer().get())  // hw buffers
      {
        if (frame->medbuf->range_length() == 0)
        {
          CLog::Log(LOGERROR, "%s - Invalid buffer\n", CLASSNAME);
          frame->status = VC_ERROR;
          decode_done   = 1;
          frame->medbuf->release();
          frame->medbuf = NULL;
        }
        else
          frame->format = RENDER_FMT_YUV420P;
      }
      else
        frame->format = RENDER_FMT_EGLIMG;

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
      frame->medbuf = NULL;
      continue;
    }
    else
    {
      CLog::Log(LOGERROR, "%s - decoding error (%d)\n", CLASSNAME,frame->status);
      decode_done   = 1;
      if (frame->medbuf)
        frame->medbuf->release();
      frame->medbuf = NULL;
    }

    #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame; w:%d, h:%d, dw:%d, dh:%d, kf:%d, ur:%d, tm:%d\n", CLASSNAME, w, h, dw, dh, keyframe, unreadable, XbmcThreads::SystemClockMillis() - time);
    #endif

    if (frame->status == OK && frame->format == RENDER_FMT_EGLIMG)
    {
      android::GraphicBuffer* graphicBuffer = static_cast<android::GraphicBuffer*>(frame->medbuf->graphicBuffer().get() );
      native_window_set_buffers_timestamp(p->natwin.get(), frame->pts * 1000);
      int err = p->natwin.get()->queueBuffer(p->natwin.get(), graphicBuffer);
      if (err == 0)
        frame->medbuf->meta_data()->setInt32(kKeyRendered, 1);
      frame->medbuf->release();
      frame->medbuf = NULL;
    }

    pthread_mutex_lock(&p->out_mutex);
    p->cur_frame = frame;
    while (p->cur_frame)
      pthread_cond_wait(&p->out_condition, &p->out_mutex);
    pthread_mutex_unlock(&p->out_mutex);
  }
  while (!decode_done && !p->stop_decode);

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time;
  CLog::Log(LOGDEBUG, "%s: exited decode thread\n", CLASSNAME);
#endif
  p->thread_exited = 1;

  return 0;
}

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

  const char* mimetype;
  switch (hints.codec)
  {
  case CODEC_ID_H264:
    mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
    if ( *(char*)hints.extradata == 1 )
      p->meta->setData(kKeyAVCC, kTypeAVCC, hints.extradata, hints.extrasize);
    break;
   // FIXME : Inconsistent on tegra3. Disable for now
  // case CODEC_ID_MPEG4:
    // mimetype = MEDIA_MIMETYPE_VIDEO_MPEG4;
    // break;
  case CODEC_ID_VP3:
  case CODEC_ID_VP6:
  case CODEC_ID_VP6F:
  case CODEC_ID_VP8:
    mimetype = MEDIA_MIMETYPE_VIDEO_VPX;
    break;
  case CODEC_ID_VC1:
  case CODEC_ID_WMV3:
    mimetype = MEDIA_MIMETYPE_VIDEO_WMV;
    break;
  default:
    return false;
    break;
  }

  p->meta->setCString(kKeyMIMEType, mimetype);
  p->meta->setInt32(kKeyWidth, p->width);
  p->meta->setInt32(kKeyHeight, p->height);

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

  p->natwin = g_xbmcapp.GetAndroidVideoWindow();
  p->decoder  = OMXCodec::Create(p->client->interface(), p->meta,
                                         false, p->source, NULL,
                                         OMXCodec::kHardwareCodecsOnly | OMXCodec::kClientNeedsFramebuffer,
                                         p->natwin);

  if (!(p->decoder != NULL && p->decoder->start() ==  OK))
  {
    p->client->disconnect();
    goto fail;
  }

  outFormat = p->decoder->getFormat();
  if (!outFormat->findInt32(kKeyWidth, &p->width) || !outFormat->findInt32(kKeyHeight, &p->height)
        || !outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat))
  {
    p->client->disconnect();
    goto fail;
  }

  if (!outFormat->findInt32(kKeyStride, &p->videoStride))
    p->videoStride = p->width;
  if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
    p->videoSliceHeight = p->height;

  const char *component;
  if (outFormat->findCString(kKeyDecoderComponent, &component))
    CLog::Log(LOGDEBUG, "%s::%s - component: %s\n", CLASSNAME, __func__, component);


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
  pthread_cond_init(&p->in_condition, NULL);
  pthread_mutex_init(&p->out_mutex, NULL);
  pthread_cond_init(&p->out_condition, NULL);
  pthread_mutex_init(&p->free_mutex, NULL);

  pthread_create(&(p->decode_thread_id), NULL, &decode_thread, p);

  p->client->disconnect();

  if (!eglCreateImageKHR)
  {
    GETEXTENSION(PFNEGLCREATEIMAGEKHRPROC,  eglCreateImageKHR);
  }
  if (!eglDestroyImageKHR)
  {
    GETEXTENSION(PFNEGLDESTROYIMAGEKHRPROC, eglDestroyImageKHR);
  }
  if (!glEGLImageTargetTexture2DOES)
  {
    GETEXTENSION(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC, glEGLImageTargetTexture2DOES);
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

  if (!p->eglInitialized)
  {
    p->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint contextAttributes[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    p->eglContext = eglCreateContext(p->eglDisplay, g_Windowing.GetEGLConfig(), EGL_NO_CONTEXT, contextAttributes);
    EGLint pbufferAttribs[] = {
      EGL_WIDTH, p->width,
      EGL_HEIGHT, p->height,
      EGL_NONE
    };
    p->eglSurface = eglCreatePbufferSurface(p->eglDisplay, g_Windowing.GetEGLConfig(), pbufferAttribs);
    eglMakeCurrent(p->eglDisplay, p->eglSurface, p->eglSurface, p->eglContext);
    CheckGlError("stf init");

    static const EGLint imageAttributes[] = {
      EGL_IMAGE_PRESERVED_KHR, EGL_FALSE,
      EGL_GL_TEXTURE_LEVEL_KHR, 0,
      EGL_NONE
    };

    for (int i=0; i<NUMFBOTEX; ++i)
    {
      glGenTextures(1, &(p->slots[i].texid));
      glBindTexture(GL_TEXTURE_2D,  p->slots[i].texid);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->width, p->height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, 0);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // This is necessary for non-power-of-two textures
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      p->slots[i].eglimg = eglCreateImageKHR(p->eglDisplay, p->eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(p->slots[i].texid),imageAttributes);
      p->free_queue.push_back(std::pair<EGLImageKHR, int>(p->slots[i].eglimg, i));

    }
    glBindTexture(GL_TEXTURE_2D,  0);

    p->fbo.Initialize();
    p->OES_shader_setUp();

    p->eglInitialized = true;
  }

  if (p->free_queue.empty())
    return VC_ERROR;

  if (demuxer_content)
  {
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame)
      return VC_ERROR;

    frame->status  = OK;
    //frame->pts = (dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : ((pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : 0);
    frame->pts = (pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : ((dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : 0);
    frame->duration = 0;
    frame->medbuf = p->getBuffer(demuxer_bytes);
    if (!frame->medbuf)
    {
      free(frame);
      return VC_ERROR;
    }
    fast_memcpy(frame->medbuf->data(), demuxer_content, demuxer_bytes);
    frame->medbuf->meta_data()->clear();
    frame->medbuf->meta_data()->setInt64(kKeyTime, frame->pts);

    if (p->mPrevPts >= 0)
    {
      frame->duration = frame->pts - p->mPrevPts;
      p->mTimeSize += frame->duration;
    }
    p->mPrevPts = frame->pts;
    
    while (true) {
      if (p->thread_exited) {
          p->source_done = true;
          break;
      }
      pthread_mutex_lock(&p->in_mutex);
      if (p->in_queue.size() >= MAXBUFIN && p->cur_frame == NULL) {
          pthread_mutex_unlock(&p->in_mutex);
          usleep(1000);
          continue;
      }
      p->in_queue.push_back(frame);
      pthread_cond_signal(&p->in_condition);
      pthread_mutex_unlock(&p->in_mutex);
      break;
    }
  }

  int ret = VC_BUFFER;
  pthread_mutex_lock(&p->in_mutex);
  if (p->cur_frame != NULL && p->in_queue.size() >= MINBUFIN)
    ret |= VC_PICTURE;
  pthread_mutex_unlock(&p->in_mutex);
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d); tm:%d\n", CLASSNAME,p->in_queue.size(), XbmcThreads::SystemClockMillis() - time);
#endif

  return ret;
}

bool CStageFrightVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
 #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  if (p->prev_frame) {
    if (p->prev_frame->medbuf)
      p->prev_frame->medbuf->release();
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

  pthread_mutex_lock(&p->out_mutex);
  if (!p->cur_frame)
  {
    CLog::Log(LOGERROR, "%s::%s - Error getting frame\n", CLASSNAME, __func__);
    pthread_cond_signal(&p->out_condition);
    pthread_mutex_unlock(&p->out_mutex);
    return false;
  }

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
    pthread_cond_signal(&p->out_condition);
    pthread_mutex_unlock(&p->out_mutex);
    return false;
  }

  pDvdVideoPicture->format = frame->format;
  pDvdVideoPicture->pts = frame->pts;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->iWidth  = frame->width;
  pDvdVideoPicture->iHeight = frame->height;
  pDvdVideoPicture->iDisplayWidth = frame->width;
  pDvdVideoPicture->iDisplayHeight = frame->height;
  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->stf = this;
  pDvdVideoPicture->eglimg = EGL_NO_IMAGE_KHR;

  if (pDvdVideoPicture->format == RENDER_FMT_EGLIMG)
  {
    g_xbmcapp.UpdateStagefrightTexture();

    if (!p->drop_state)
    {
      pthread_mutex_lock(&p->free_mutex);
      std::list<std::pair<EGLImageKHR, int> >::iterator it = p->free_queue.begin();
      int cur_slot = it->second;
      pthread_mutex_unlock(&p->free_mutex);
      p->fbo.BindToTexture(GL_TEXTURE_2D, p->slots[cur_slot].texid);
      p->fbo.BeginRender();

      glDisable(GL_DEPTH_TEST);
      //glClear(GL_COLOR_BUFFER_BIT);

      const GLfloat triangleVertices[] = {
      -1.0f, 1.0f,
      -1.0f, -1.0f,
      1.0f, -1.0f,
      1.0f, 1.0f,
      };

      glVertexAttribPointer(p->mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, triangleVertices);
      glEnableVertexAttribArray(p->mPositionHandle);

      glUseProgram(p->mPgm);
      glUniform1i(p->mTexSamplerHandle, 0);
      glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_xbmcapp.GetAndroidTexture());

      GLfloat texMatrix[16];
      g_xbmcapp.GetStagefrightTransformMatrix(texMatrix);
      glUniformMatrix4fv(p->mTexMatrixHandle, 1, GL_FALSE, texMatrix);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

      p->fbo.EndRender();

      pDvdVideoPicture->eglimg = p->slots[cur_slot].eglimg;
      glBindTexture(GL_TEXTURE_2D, 0);

    #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>> pic dts:%f, pts:%llu, img:%p, tm:%d\n", pDvdVideoPicture->dts, frame->pts, pDvdVideoPicture->eglimg, XbmcThreads::SystemClockMillis() - time);
    #endif
    }
  } else if (pDvdVideoPicture->format == RENDER_FMT_YUV420P)
  {
    pDvdVideoPicture->color_range  = 0;
    pDvdVideoPicture->color_matrix = 4;

    //unsigned int luma_pixels = frame->width * frame->height;
    //unsigned int chroma_pixels = luma_pixels/4;
    BYTE* data = NULL;
    if (frame->medbuf && !p->drop_state)
      data = (BYTE*)((long)frame->medbuf->data() + frame->medbuf->range_offset());
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
      default:
        CLog::Log(LOGERROR, "%s::%s - Unsupported color format(%d)\n", CLASSNAME, __func__,p->videoColorFormat);
    }
  #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> pic pts:%f, data:%p, col:%d, w:%d, h:%d, tm:%d\n", pDvdVideoPicture->pts, data, p->videoColorFormat, p->videoStride, p->videoSliceHeight, XbmcThreads::SystemClockMillis() - time);
  #endif
  }

  if (p->drop_state)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  p->prev_frame = p->cur_frame;
  p->cur_frame = NULL;
  pthread_cond_signal(&p->out_condition);
  pthread_mutex_unlock(&p->out_mutex);

  return true;
}

void CStageFrightVideo::Close()
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  Frame *frame;

  p->stop_decode = 1;
  pthread_cond_signal(&p->in_condition);

  // Give decoder_thread time to process EOS, if stuck on reading
  usleep(50000);

  pthread_mutex_lock(&p->out_mutex);
  if (p->cur_frame)
  {
    if (p->cur_frame->medbuf)
      p->cur_frame->medbuf->release();
    free(p->cur_frame);
    p->cur_frame = NULL;
  }
  pthread_cond_signal(&p->out_condition);
  pthread_mutex_unlock(&p->out_mutex);

  if (p->prev_frame)
  {
    if (p->prev_frame->medbuf)
      p->prev_frame->medbuf->release();
    free(p->prev_frame);
    p->prev_frame = NULL;
  }

  p->decoder->stop();
  p->client->disconnect();

  pthread_join(p->decode_thread_id, NULL);

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
  
  if (p->decoder_component)
    free(&p->decoder_component);

  delete p->client;

  g_xbmcapp.UninitStagefrightSurface();

  p->fbo.Cleanup();
  for (int i=0; i<NUMFBOTEX; ++i)
  {
    glDeleteTextures(1, &(p->slots[i].texid));
    eglDestroyImageKHR(p->eglDisplay, p->slots[i].eglimg);
  }

  if (p->eglContext != EGL_NO_CONTEXT)
    eglDestroyContext(p->eglDisplay, p->eglContext);
  p->eglContext = EGL_NO_CONTEXT;

  if (p->eglSurface != EGL_NO_SURFACE)
    eglDestroySurface(p->eglDisplay, p->eglSurface);
  p->eglSurface = EGL_NO_SURFACE;

  pthread_mutex_destroy(&p->in_mutex);
  pthread_cond_destroy(&p->in_condition);
  pthread_mutex_destroy(&p->out_mutex);
  pthread_cond_destroy(&p->out_condition);
  pthread_mutex_destroy(&p->free_mutex);
  delete p;
}

void CStageFrightVideo::Reset(void)
{
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Reset\n", CLASSNAME);
#endif
  Frame* frame;
  pthread_mutex_lock(&p->in_mutex);
  while (!p->in_queue.empty())
  {
    frame = *p->in_queue.begin();
    p->in_queue.erase(p->in_queue.begin());
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }
  p->mDataSize = 0;
  p->mTimeSize = 0;
  p->mPrevPts = -1;
  p->resetting = true;
  pthread_mutex_unlock(&p->in_mutex);
}

void CStageFrightVideo::SetDropState(bool bDrop)
{
  if (bDrop == p->drop_state)
    return;

#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::SetDropState (%d->%d)\n", CLASSNAME,p->drop_state,bDrop);
#endif

  p->drop_state = bDrop;
}

void CStageFrightVideo::SetSpeed(int iSpeed)
{
}

int CStageFrightVideo::GetDataSize(void)
{
  return p->mDataSize;
}

double CStageFrightVideo::GetTimeSize(void)
{
  return pts_itod(p->mTimeSize);
}

/***************/

void CStageFrightVideo::LockBuffer(EGLImageKHR eglimg)
{
 #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  pthread_mutex_lock(&p->free_mutex);
  std::list<std::pair<EGLImageKHR, int> >::iterator it = p->free_queue.begin();
  for(;it != p->free_queue.end(); ++it)
  {
    if ((*it).first == eglimg)
    break;
  }
  if (it == p->free_queue.end())
  {
    pthread_mutex_unlock(&p->free_mutex);
    return;
  }
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Locking %p: tm:%d\n", eglimg, XbmcThreads::SystemClockMillis() - time);
#endif

  p->busy_queue.push_back(std::pair<EGLImageKHR, int>(*it));
  p->free_queue.erase(it);
  pthread_mutex_unlock(&p->free_mutex);
}

void CStageFrightVideo::ReleaseBuffer(EGLImageKHR eglimg)
{
 #if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  pthread_mutex_lock(&p->free_mutex);
  std::list<std::pair<EGLImageKHR, int> >::iterator it = p->busy_queue.begin();
  for(;it != p->busy_queue.end(); ++it)
  {
    if ((*it).first == eglimg)
    break;
  }
  if (it == p->busy_queue.end())
  {
    pthread_mutex_unlock(&p->free_mutex);
    return;
  }
#if defined(STAGEFRIGHT_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Unlocking %p: tm:%d\n", eglimg, XbmcThreads::SystemClockMillis() - time);
#endif

  p->free_queue.push_back(std::pair<EGLImageKHR, int>(*it));
  p->busy_queue.erase(it);
  pthread_mutex_unlock(&p->free_mutex);
}
