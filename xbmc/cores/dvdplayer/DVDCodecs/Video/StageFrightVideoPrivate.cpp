/*
 *      Copyright (C) 2013 Team XBMC
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

#include "StageFrightVideoPrivate.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "windowing/egl/EGLWrapper.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"

GLint glerror;
#define CheckEglError(x) while((glerror = eglGetError()) != EGL_SUCCESS) CLog::Log(LOGERROR, "EGL error in %s: %x",x, glerror);
#define CheckGlError(x)  while((glerror = glGetError()) != GL_NO_ERROR) CLog::Log(LOGERROR, "GL error in %s: %x",x, glerror);

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

int NP2( unsigned x ) {
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

CStageFrightVideoPrivate::CStageFrightVideoPrivate()
    : decode_thread(NULL), source(NULL), natwin(NULL)
    , eglDisplay(EGL_NO_DISPLAY), eglSurface(EGL_NO_SURFACE), eglContext(EGL_NO_CONTEXT)
    , eglInitialized(false)
    , framecount(0)
    , quirks(QuirkNone), cur_frame(NULL), prev_frame(NULL)
    , width(-1), height(-1)
    , texwidth(-1), texheight(-1)
    , client(NULL), decoder(NULL), decoder_component(NULL)
    , drop_state(false), resetting(false)
{
  if (!eglCreateImageKHR)
    eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglCreateImageKHR");
  if (!eglDestroyImageKHR)
    eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglDestroyImageKHR");
  if (!glEGLImageTargetTexture2DOES)
    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) CEGLWrapper::GetProcAddress("glEGLImageTargetTexture2DOES");
}

void CStageFrightVideoPrivate::signalBufferReturned(MediaBuffer *buffer)
{
}

MediaBuffer* CStageFrightVideoPrivate::getBuffer(size_t size)
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

bool CStageFrightVideoPrivate::inputBufferAvailable()
{
  for (int i=0; i<INBUFCOUNT; ++i)
    if (inbuf[i]->refcount() == 0)
      return true;
      
  return false;
}

void CStageFrightVideoPrivate::loadOESShader(GLenum shaderType, const char* pSource, GLuint* outShader)
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

void CStageFrightVideoPrivate::createOESProgram(const char* pVertexSource, const char* pFragmentSource, GLuint* outPgm)
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

void CStageFrightVideoPrivate::OES_shader_setUp()
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

void CStageFrightVideoPrivate::InitializeEGL(int w, int h)
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

void CStageFrightVideoPrivate::UninitializeEGL()
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
