/*
 *      Copyright (C) 2010-2013 Team XBMC
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

//#define DEBUG_VERBOSE 1

#include "system.h"
#include "system_gl.h"

#include "StageFrightVideo.h"

#include "android/activity/XBMCApp.h"
#include "guilib/GraphicContext.h"
#include "DVDClock.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include "settings/AdvancedSettings.h"

#include "xbmc/guilib/FrameBufferObject.h"
#include "windowing/WindowingFactory.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "windowing/egl/EGLWrapper.h"

#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <gui/SurfaceTexture.h>

#include <new>
#include <map>
#include <queue>
#include <list>

#define OMX_QCOM_COLOR_FormatYVU420SemiPlanar 0x7FA30C00
#define CLASSNAME "CStageFrightVideo"
#define INBUFCOUNT 16
#define NUMFBOTEX 4

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

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
  ERenderFormat format;
  EGLImageKHR eglimg;
  MediaBuffer* medbuf;
};

enum StageFrightQuirks
{
  QuirkNone = 0,
  QuirkSWRender = 0x01,
};

class CStageFrightDecodeThread;

class CStageFrightVideoPrivate : public MediaBufferObserver
{
public:
  CStageFrightVideoPrivate()
    : decode_thread(NULL), source(NULL), natwin(NULL)
    , eglDisplay(EGL_NO_DISPLAY), eglSurface(EGL_NO_SURFACE), eglContext(EGL_NO_CONTEXT)
    , eglInitialized(false)
    , framecount(0)
    , quirks(QuirkNone), cur_frame(NULL), prev_frame(NULL)
    , width(-1), height(-1)
    , texwidth(-1), texheight(-1)
    , client(NULL), decoder(NULL), decoder_component(NULL)
    , drop_state(false), resetting(false)
  {}

  virtual void signalBufferReturned(MediaBuffer *buffer)
  {
  }

  MediaBuffer* getBuffer(size_t size)
  {
    int i=0;
    for (; i<INBUFCOUNT; ++i)
      if (inbuf[i]->refcount() == 0 && inbuf[i]->size() >= size)
        break;
    if (i == INBUFCOUNT)
    {
      i = 0;
      for (; i<INBUFCOUNT; ++i)
        if (inbuf[i]->refcount() == 0)
          break;
      if (i == INBUFCOUNT)
        return NULL;
      inbuf[i]->setObserver(NULL);
      inbuf[i]->release();
      inbuf[i] = new MediaBuffer(size);
      inbuf[i]->setObserver(this);
    }

    inbuf[i]->add_ref();
    inbuf[i]->set_range(0, size);
    return inbuf[i];
  }

  bool inputBufferAvailable()
  {
    for (int i=0; i<INBUFCOUNT; ++i)
      if (inbuf[i]->refcount() == 0)
        return true;
        
    return false;
  }

  void loadOESShader(GLenum shaderType, const char* pSource, GLuint* outShader)
  {
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>loadOESShader\n");
  #endif

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
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>createOESProgram\n");
  #endif
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
    #if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, ">>OES_shader_setUp\n");
    #endif
      CheckGlError("OES_shader_setUp");
      createOESProgram(vsrc, fsrc, &mPgm);
    }

    mPositionHandle = glGetAttribLocation(mPgm, "vPosition");
    mTexSamplerHandle = glGetUniformLocation(mPgm, "texSampler");
    mTexMatrixHandle = glGetUniformLocation(mPgm, "texMatrix");
  }
  
  int NP2( unsigned x ) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
  }

  void InitializeEGL(int w, int h)
  {
    texwidth = w;
    texheight = h;
    if (!g_Windowing.IsExtSupported("GL_TEXTURE_NPOT"))
    {
      texwidth  = NP2(texwidth);
      texheight = NP2(texheight);
    }

    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLint contextAttributes[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    eglContext = eglCreateContext(eglDisplay, g_Windowing.GetEGLConfig(), EGL_NO_CONTEXT, contextAttributes);
    EGLint pbufferAttribs[] = {
      EGL_WIDTH, texwidth,
      EGL_HEIGHT, texheight,
      EGL_NONE
    };
    eglSurface = eglCreatePbufferSurface(eglDisplay, g_Windowing.GetEGLConfig(), pbufferAttribs);
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    CheckGlError("stf init");

    static const EGLint imageAttributes[] = {
      EGL_IMAGE_PRESERVED_KHR, EGL_FALSE,
      EGL_GL_TEXTURE_LEVEL_KHR, 0,
      EGL_NONE
    };

    for (int i=0; i<NUMFBOTEX; ++i)
    {
      glGenTextures(1, &(slots[i].texid));
      glBindTexture(GL_TEXTURE_2D,  slots[i].texid);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texwidth, texheight, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, 0);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // This is necessary for non-power-of-two textures
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      slots[i].eglimg = eglCreateImageKHR(eglDisplay, eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer)(slots[i].texid),imageAttributes);
      free_queue.push_back(std::pair<EGLImageKHR, int>(slots[i].eglimg, i));

    }
    glBindTexture(GL_TEXTURE_2D,  0);

    fbo.Initialize();
    OES_shader_setUp();

    eglInitialized = true;
#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: >>> Initialized EGL: w:%d; h:%d\n", CLASSNAME, texwidth, texheight);
#endif
  }
  
  void UninitializeEGL()
  {
    fbo.Cleanup();
    for (int i=0; i<NUMFBOTEX; ++i)
    {
      glDeleteTextures(1, &(slots[i].texid));
      eglDestroyImageKHR(eglDisplay, slots[i].eglimg);
    }

    if (eglContext != EGL_NO_CONTEXT)
      eglDestroyContext(eglDisplay, eglContext);
    eglContext = EGL_NO_CONTEXT;

    if (eglSurface != EGL_NO_SURFACE)
      eglDestroySurface(eglDisplay, eglSurface);
    eglSurface = EGL_NO_SURFACE;
    
    eglInitialized = false;
  }
  
  CStageFrightDecodeThread* decode_thread;

  sp<MediaSource> source;
  sp<ANativeWindow> natwin;
  
  MediaBuffer* inbuf[INBUFCOUNT];

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
  int64_t framecount;
  map<int64_t, Frame*> in_queue;
  map<int64_t, Frame*> out_queue;
  pthread_mutex_t in_mutex;
  pthread_cond_t in_condition;
  pthread_mutex_t out_mutex;
  pthread_cond_t out_condition;
  pthread_mutex_t free_mutex;

  int quirks;
  Frame *cur_frame;
  Frame *prev_frame;
  bool source_done;
  int x, y;
  int width, height;
  int texwidth, texheight;

  OMXClient *client;
  sp<MediaSource> decoder;
  const char *decoder_component;
  int videoColorFormat;
  int videoStride;
  int videoSliceHeight;

  bool drop_state;
  bool resetting;
#if defined(DEBUG_VERBOSE)
  unsigned int cycle_time;
#endif
};

/********************************************/

class CStageFrightDecodeThread : public CThread
{
protected:
  CStageFrightVideoPrivate *p;

public:
  CStageFrightDecodeThread(CStageFrightVideoPrivate *priv)
  : CThread("CStageFrightDecodeThread")
  , p(priv)
  {}
  
  void OnStartup()
  {
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: entering decode thread\n", CLASSNAME);
  #endif
  }
  
  void OnExit()
  {
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, "%s: exited decode thread\n", CLASSNAME);
  #endif
  }
  
  void Process()
  {
    Frame* frame;
    int32_t w, h, dw, dh;
    int decode_done = 0;
    int32_t keyframe = 0;
    int32_t unreadable = 0;
    MediaSource::ReadOptions readopt;
    // GLuint texid;

    //SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
    do
    {
      #if defined(DEBUG_VERBOSE)
      unsigned int time = XbmcThreads::SystemClockMillis();
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

      frame->eglimg = EGL_NO_IMAGE_KHR;
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
      }

      if (frame->status == OK)
      {
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

        outFormat->findInt32(kKeyWidth , &p->width);
        outFormat->findInt32(kKeyHeight, &p->height);

        cropLeft = cropTop = cropRight = cropBottom = 0;
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

#if defined(DEBUG_VERBOSE)
        CLog::Log(LOGDEBUG, ">>> new format col:%d, w:%d, h:%d, sw:%d, sh:%d, ctl:%d,%d; cbr:%d,%d\n", p->videoColorFormat, p->width, p->height, p->videoStride, p->videoSliceHeight, cropTop, cropLeft, cropBottom, cropRight);
#endif

        if (frame->medbuf)
          frame->medbuf->release();
        frame->medbuf = NULL;
        free(frame);
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

      if (frame->format == RENDER_FMT_EGLIMG)
      {
        if (!p->eglInitialized)
        {
          p->InitializeEGL(frame->width, frame->height);
        } 
        else if (p->texwidth != frame->width || p->texheight != frame->height)
        {
          p->UninitializeEGL();
          p->InitializeEGL(frame->width, frame->height);
        }

        if (p->free_queue.empty())
        {
          CLog::Log(LOGERROR, "%s::%s - Error: No free output buffers\n", CLASSNAME, __func__);
          if (frame->medbuf) {
            frame->medbuf->release();
          }
          free(frame);
          continue;;
        }  

        android::GraphicBuffer* graphicBuffer = static_cast<android::GraphicBuffer*>(frame->medbuf->graphicBuffer().get() );
        native_window_set_buffers_timestamp(p->natwin.get(), frame->pts * 1000);
        int err = p->natwin.get()->queueBuffer(p->natwin.get(), graphicBuffer);
        if (err == 0)
          frame->medbuf->meta_data()->setInt32(kKeyRendered, 1);
        frame->medbuf->release();
        frame->medbuf = NULL;
        g_xbmcapp.UpdateStagefrightTexture();
        // g_xbmcapp.GetSurfaceTexture()->updateTexImage();

        if (!p->drop_state)
        {
          // static const EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
          // EGLImageKHR img = eglCreateImageKHR(p->eglDisplay, EGL_NO_CONTEXT,
                                    // EGL_NATIVE_BUFFER_ANDROID,
                                    // (EGLClientBuffer)graphicBuffer->getNativeBuffer(),
                                    // eglImgAttrs);

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

          // glGenTextures(1, &texid);
          // glBindTexture(GL_TEXTURE_EXTERNAL_OES, texid);
          // glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, img);

          glBindTexture(GL_TEXTURE_EXTERNAL_OES, g_xbmcapp.GetAndroidTexture());
          
          GLfloat texMatrix[16];
          // const GLfloat texMatrix[] = {
            // 1, 0, 0, 0,
            // 0, -1, 0, 0,
            // 0, 0, 1, 0,
            // 0, 1, 0, 1
          // };
          g_xbmcapp.GetStagefrightTransformMatrix(texMatrix);
          // g_xbmcapp.GetSurfaceTexture()->getTransformMatrix(texMatrix);
          glUniformMatrix4fv(p->mTexMatrixHandle, 1, GL_FALSE, texMatrix);

          glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

          glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
          // glDeleteTextures(1, &texid);
          // eglDestroyImageKHR(p->eglDisplay, img);
          p->fbo.EndRender();

          glBindTexture(GL_TEXTURE_2D, 0);

          frame->eglimg = p->slots[cur_slot].eglimg;
        }
      }

    #if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: >>> pushed OUT frame; w:%d, h:%d, dw:%d, dh:%d, kf:%d, ur:%d, tm:%d\n", CLASSNAME, frame->width, frame->height, dw, dh, keyframe, unreadable, XbmcThreads::SystemClockMillis() - time);
    #endif

      pthread_mutex_lock(&p->out_mutex);
      p->cur_frame = frame;
      while (p->cur_frame)
        pthread_cond_wait(&p->out_condition, &p->out_mutex);
      pthread_mutex_unlock(&p->out_mutex);
    }
    while (!decode_done && !m_bStop);
    
    if (p->eglInitialized)
      p->UninitializeEGL();
    
  }
};

/***********************************************************/

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
    int64_t time_us = -1;
    MediaSource::ReadOptions::SeekMode mode;
    
    if (options && options->getSeekTo(&time_us, &mode))
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: reading source(%d): seek:%llu\n", CLASSNAME,p->in_queue.size(), time_us);
#endif
    }
    else
    {
#if defined(DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s: reading source(%d)\n", CLASSNAME,p->in_queue.size());
#endif
    }

    pthread_mutex_lock(&p->in_mutex);
    while (p->in_queue.empty() && p->decode_thread)
      pthread_cond_wait(&p->in_condition, &p->in_mutex);

    if (p->in_queue.empty())
    {
      pthread_mutex_unlock(&p->in_mutex);
      return VC_ERROR;
    }
    
    std::map<int64_t,Frame*>::iterator it = p->in_queue.begin();
    frame = it->second;
    ret = frame->status;
    *buffer = frame->medbuf;

    p->in_queue.erase(it);
    pthread_mutex_unlock(&p->in_mutex);

#if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> exiting reading source(%d); pts:%llu\n", p->in_queue.size(),frame->pts);
#endif

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
#if defined(DEBUG_VERBOSE)
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

  if (g_advancedSettings.m_stagefrightConfig.useSwRenderer)
    p->quirks |= QuirkSWRender;
    
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
    if (g_advancedSettings.m_stagefrightConfig.useAVCcodec == 0)
      return false;
    mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
    if ( *(char*)hints.extradata == 1 )
      p->meta->setData(kKeyAVCC, kTypeAVCC, hints.extradata, hints.extrasize);
    break;
  case CODEC_ID_MPEG4:
    if (g_advancedSettings.m_stagefrightConfig.useMP4codec == 0)
      return false;
    mimetype = MEDIA_MIMETYPE_VIDEO_MPEG4;
    break;
  case CODEC_ID_MPEG2VIDEO:
    if (g_advancedSettings.m_stagefrightConfig.useMPEG2codec == 0)
      return false;
    mimetype = MEDIA_MIMETYPE_VIDEO_MPEG2;
    break;
  case CODEC_ID_VP3:
  case CODEC_ID_VP6:
  case CODEC_ID_VP6F:
  case CODEC_ID_VP8:
    if (g_advancedSettings.m_stagefrightConfig.useVPXcodec == 0)
      return false;
    mimetype = MEDIA_MIMETYPE_VIDEO_VPX;
    break;
  case CODEC_ID_VC1:
  case CODEC_ID_WMV3:
    if (g_advancedSettings.m_stagefrightConfig.useVC1codec == 0)
      return false;
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
    delete p->client;
    p->client = NULL;
    CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Cannot connect OMX client");
    goto fail;
  }

  p->natwin = NULL;
  if ((p->quirks & QuirkSWRender) == 0)
  {
    g_xbmcapp.InitStagefrightSurface();
    p->natwin = g_xbmcapp.GetAndroidVideoWindow();
    native_window_api_connect(p->natwin.get(), NATIVE_WINDOW_API_MEDIA);

    if (!eglCreateImageKHR)
      eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglCreateImageKHR");
    if (!eglDestroyImageKHR)
      eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglDestroyImageKHR");
    if (!glEGLImageTargetTexture2DOES)
      glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) CEGLWrapper::GetProcAddress("glEGLImageTargetTexture2DOES");
  }

  p->decoder  = OMXCodec::Create(p->client->interface(), p->meta,
                                         false, p->source, NULL,
                                         OMXCodec::kHardwareCodecsOnly | (p->quirks & QuirkSWRender ? OMXCodec::kClientNeedsFramebuffer : 0),
                                         p->natwin
                                         );

  if (!(p->decoder != NULL && p->decoder->start() ==  OK))
  {
    p->decoder = NULL;
    goto fail;
  }

  outFormat = p->decoder->getFormat();

  if (!outFormat->findInt32(kKeyWidth, &p->width) || !outFormat->findInt32(kKeyHeight, &p->height)
        || !outFormat->findInt32(kKeyColorFormat, &p->videoColorFormat))
    goto fail;

  const char *component;
  if (outFormat->findCString(kKeyDecoderComponent, &component))
  {
    CLog::Log(LOGDEBUG, "%s::%s - component: %s\n", CLASSNAME, __func__, component);
    
    //Blacklist
    if (!strncmp(component, "OMX.Nvidia.mp4.decode", 21) && g_advancedSettings.m_stagefrightConfig.useMP4codec != 1)
    {
      // Has issues with some XVID encoded MP4. Only fails after actual decoding starts...
      CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Blacklisted component (MP4)");
      goto fail;
      
    }
    else if (!strncmp(component, "OMX.rk.", 7) && g_advancedSettings.m_stagefrightConfig.useAVCcodec != 1 && g_advancedSettings.m_stagefrightConfig.useMP4codec != 1) 
    {
      if (p->width % 32 != 0 || p->height % 16 != 0)
      {
        // Buggy. Hard crash on non MOD16 height videos and stride errors for non MOD32 width
        CLog::Log(LOGERROR, "%s::%s - %s\n", CLASSNAME, __func__,"Blacklisted component (MOD16)");
        goto fail;
        
      }
    }
  }

  cropLeft = cropTop = cropRight = cropBottom = 0;
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

  if (!outFormat->findInt32(kKeyStride, &p->videoStride))
    p->videoStride = p->width;
  if (!outFormat->findInt32(kKeySliceHeight, &p->videoSliceHeight))
    p->videoSliceHeight = p->height;
  
  for (int i=0; i<INBUFCOUNT; ++i)
  {
    p->inbuf[i] = new MediaBuffer(300000);
    p->inbuf[i]->setObserver(p);
  }
  
  pthread_mutex_init(&p->in_mutex, NULL);
  pthread_cond_init(&p->in_condition, NULL);
  pthread_mutex_init(&p->out_mutex, NULL);
  pthread_cond_init(&p->out_condition, NULL);
  pthread_mutex_init(&p->free_mutex, NULL);

  p->decode_thread = new CStageFrightDecodeThread(p);
  p->decode_thread->Create(true /*autodelete*/);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, ">>> format col:%d, w:%d, h:%d, sw:%d, sh:%d, ctl:%d,%d; cbr:%d,%d\n", p->videoColorFormat, p->width, p->height, p->videoStride, p->videoSliceHeight, cropTop, cropLeft, cropBottom, cropRight);
#endif

  return true;

fail:
  if (p->decoder != 0)
    p->decoder->stop();
  if (p->client)
  {
    p->client->disconnect();
    delete p->client;
  }
  if (p->decoder_component)
    free(&p->decoder_component);
  if ((p->quirks & QuirkSWRender) == 0)
    g_xbmcapp.UninitStagefrightSurface();
  return false;
}

/*** Decode ***/
int  CStageFrightVideo::Decode(BYTE *pData, int iSize, double dts, double pts)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::Decode - d:%p; s:%d; dts:%f; pts:%f\n", CLASSNAME, pData, iSize, dts, pts);
#endif

  Frame *frame;
  int demuxer_bytes = iSize;
  uint8_t *demuxer_content = pData;

  if (demuxer_content)
  {
    frame = (Frame*)malloc(sizeof(Frame));
    if (!frame)
      return VC_ERROR;

    frame->status  = OK;
    if (g_advancedSettings.m_stagefrightConfig.useInputDTS)
      frame->pts = (dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : ((pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : 0);
    else
      frame->pts = (pts != DVD_NOPTS_VALUE) ? pts_dtoi(pts) : ((dts != DVD_NOPTS_VALUE) ? pts_dtoi(dts) : 0);
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
    p->framecount++;
    p->in_queue.insert(std::pair<int64_t, Frame*>(p->framecount, frame));
    pthread_cond_signal(&p->in_condition);
    pthread_mutex_unlock(&p->in_mutex);
  }

  int ret = 0;
  if (p->inputBufferAvailable() && p->in_queue.size() < INBUFCOUNT)
    ret |= VC_BUFFER;
  else
    usleep(1000);
  if (p->cur_frame != NULL)
    ret |= VC_PICTURE;
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Decode: pushed IN frame (%d); tm:%d\n", CLASSNAME,p->in_queue.size(), XbmcThreads::SystemClockMillis() - time);
#endif

  return ret;
}

bool CStageFrightVideo::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
 #if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  if (p->prev_frame) {
    if (p->prev_frame->medbuf)
      p->prev_frame->medbuf->release();
    free(p->prev_frame);
    p->prev_frame = NULL;
  }
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::ClearPicture (%d)\n", CLASSNAME, XbmcThreads::SystemClockMillis() - time);
#endif

  return true;
}

bool CStageFrightVideo::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
#if defined(DEBUG_VERBOSE)
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGDEBUG, "%s::GetPicture\n", CLASSNAME);
  if (p->cycle_time != 0)
    CLog::Log(LOGDEBUG, ">>> cycle dur:%d\n", XbmcThreads::SystemClockMillis() - p->cycle_time);
  p->cycle_time = time;
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

  pDvdVideoPicture->format = frame->format;
  pDvdVideoPicture->dts = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts = frame->pts;
  pDvdVideoPicture->iWidth  = frame->width;
  pDvdVideoPicture->iHeight = frame->height;
  pDvdVideoPicture->iDisplayWidth = frame->width;
  pDvdVideoPicture->iDisplayHeight = frame->height;
  pDvdVideoPicture->iFlags  = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->stf = this;
  pDvdVideoPicture->eglimg = EGL_NO_IMAGE_KHR;

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

  if (pDvdVideoPicture->format == RENDER_FMT_EGLIMG)
  {
    pDvdVideoPicture->eglimg = frame->eglimg;
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> pic dts:%f, pts:%llu, img:%p, tm:%d\n", pDvdVideoPicture->dts, frame->pts, pDvdVideoPicture->eglimg, XbmcThreads::SystemClockMillis() - time);
  #endif
  } 
  else if (pDvdVideoPicture->format == RENDER_FMT_YUV420P)
  {
    pDvdVideoPicture->color_range  = 0;
    pDvdVideoPicture->color_matrix = 4;

    unsigned int luma_pixels = frame->width  * frame->height;
    unsigned int chroma_pixels = luma_pixels/4;
    BYTE* data = NULL;
    if (frame->medbuf && !p->drop_state)
      data = (BYTE*)((long)frame->medbuf->data() + frame->medbuf->range_offset());
    switch (p->videoColorFormat)
    {
      case OMX_COLOR_FormatYUV420Planar:
        pDvdVideoPicture->iLineSize[0] = frame->width;
        pDvdVideoPicture->iLineSize[1] = frame->width / 2;
        pDvdVideoPicture->iLineSize[2] = frame->width / 2;
        pDvdVideoPicture->iLineSize[3] = 0;
        pDvdVideoPicture->data[0] = data;
        pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + luma_pixels;
        pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + chroma_pixels;
        pDvdVideoPicture->data[3] = 0;
        break;
      case OMX_COLOR_FormatYUV420SemiPlanar:
      case OMX_QCOM_COLOR_FormatYVU420SemiPlanar:
        pDvdVideoPicture->iLineSize[0] = frame->width;
        pDvdVideoPicture->iLineSize[1] = frame->width;
        pDvdVideoPicture->iLineSize[2] = 0;
        pDvdVideoPicture->iLineSize[3] = 0;
        pDvdVideoPicture->data[0] = data;
        pDvdVideoPicture->data[1] = pDvdVideoPicture->data[0] + luma_pixels;
        pDvdVideoPicture->data[2] = pDvdVideoPicture->data[1] + chroma_pixels;
        pDvdVideoPicture->data[3] = 0;
        break;
      default:
        CLog::Log(LOGERROR, "%s::%s - Unsupported color format(%d)\n", CLASSNAME, __func__,p->videoColorFormat);
    }
  #if defined(DEBUG_VERBOSE)
    CLog::Log(LOGDEBUG, ">>> pic pts:%f, data:%p, col:%d, w:%d, h:%d, tm:%d\n", pDvdVideoPicture->pts, data, p->videoColorFormat, frame->width, frame->height, XbmcThreads::SystemClockMillis() - time);
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
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Close\n", CLASSNAME);
#endif

  Frame *frame;

  if (p->decode_thread && p->decode_thread->IsRunning())
    p->decode_thread->StopThread(false);
  p->decode_thread = NULL;
  pthread_cond_signal(&p->in_condition);

  // Give decoder_thread time to process EOS, if stuck on reading
  usleep(50000);

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning OUT\n");
#endif
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

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Stopping omxcodec\n");
#endif
  p->decoder->stop();
  p->client->disconnect();

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Cleaning IN(%d)\n", p->in_queue.size());
#endif
  std::map<int64_t,Frame*>::iterator it;
  while (!p->in_queue.empty())
  {
    it = p->in_queue.begin();
    frame = it->second;
    p->in_queue.erase(it);
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }
  
  if (p->decoder_component)
    free(&p->decoder_component);

  delete p->client;

  if ((p->quirks & QuirkSWRender) == 0)
    g_xbmcapp.UninitStagefrightSurface();

  pthread_mutex_destroy(&p->in_mutex);
  pthread_cond_destroy(&p->in_condition);
  pthread_mutex_destroy(&p->out_mutex);
  pthread_cond_destroy(&p->out_condition);
  pthread_mutex_destroy(&p->free_mutex);
  
  for (int i=0; i<INBUFCOUNT; ++i)
  {
    p->inbuf[i]->setObserver(NULL);
    p->inbuf[i]->release();
  }

  delete p;
}

void CStageFrightVideo::Reset(void)
{
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::Reset\n", CLASSNAME);
#endif
  Frame* frame;
  pthread_mutex_lock(&p->in_mutex);
  std::map<int64_t,Frame*>::iterator it;
  while (!p->in_queue.empty())
  {
    it = p->in_queue.begin();
    frame = it->second;
    p->in_queue.erase(it);
    if (frame->medbuf)
      frame->medbuf->release();
    free(frame);
  }
  p->resetting = true;
  p->framecount = 0;

  pthread_mutex_unlock(&p->in_mutex);
}

void CStageFrightVideo::SetDropState(bool bDrop)
{
  if (bDrop == p->drop_state)
    return;

#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::SetDropState (%d->%d)\n", CLASSNAME,p->drop_state,bDrop);
#endif

  p->drop_state = bDrop;
}

void CStageFrightVideo::SetSpeed(int iSpeed)
{
}

/***************/

void CStageFrightVideo::LockBuffer(EGLImageKHR eglimg)
{
 #if defined(DEBUG_VERBOSE)
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
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Locking %p: tm:%d\n", eglimg, XbmcThreads::SystemClockMillis() - time);
#endif

  p->busy_queue.push_back(std::pair<EGLImageKHR, int>(*it));
  p->free_queue.erase(it);
  pthread_mutex_unlock(&p->free_mutex);
}

void CStageFrightVideo::ReleaseBuffer(EGLImageKHR eglimg)
{
 #if defined(DEBUG_VERBOSE)
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
#if defined(DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "Unlocking %p: tm:%d\n", eglimg, XbmcThreads::SystemClockMillis() - time);
#endif

  p->free_queue.push_back(std::pair<EGLImageKHR, int>(*it));
  p->busy_queue.erase(it);
  pthread_mutex_unlock(&p->free_mutex);
}
