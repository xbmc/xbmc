#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "threads/Thread.h"
#include "xbmc/guilib/FrameBufferObject.h"
#include "cores/VideoRenderers/RenderFormats.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>

#if __cplusplus >= 201103L
#define char16_t LIBRARY_char16_t
#define char32_t LIBRARY_char32_t
#endif
#include <binder/ProcessState.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <gui/SurfaceTexture.h>
#if __cplusplus >= 201103L
#undef char16_t
#undef char32_t
#endif

#include "system_gl.h"

#include <map>
#include <list>

#define NUMFBOTEX 4
#define INBUFCOUNT 16

class CStageFrightDecodeThread;
class CJNISurface;
class CJNISurfaceTexture;
class CWinSystemEGL;
class CAdvancedSettings;
class CApplication;
class CApplicationMessenger;

struct stSlot
{
  GLuint texid;
  EGLImageKHR eglimg;
  int use_cnt;
};

struct Frame
{
  android::status_t status;
  int32_t width, height;
  int64_t pts;
  ERenderFormat format;
  EGLImageKHR eglimg;
  android::MediaBuffer* medbuf;
};

enum StageFrightQuirks
{
  QuirkNone = 0,
  QuirkSWRender = 0x01,
};

class CStageFrightVideoPrivate : public android::MediaBufferObserver
{
public:
  CStageFrightVideoPrivate();

  virtual void signalBufferReturned(android::MediaBuffer *buffer);

  android::MediaBuffer* getBuffer(size_t size);
  bool inputBufferAvailable();

  stSlot* getSlot(EGLImageKHR eglimg);
  stSlot* getFreeSlot();

  void loadOESShader(GLenum shaderType, const char* pSource, GLuint* outShader);
  void createOESProgram(const char* pVertexSource, const char* pFragmentSource, GLuint* outPgm);
  void OES_shader_setUp();
  void InitializeEGL(int w, int h);
  void ReleaseEGL();

public:
  CStageFrightDecodeThread* decode_thread;

  android::sp<android::MediaSource> source;

  android::MediaBuffer* inbuf[INBUFCOUNT];

  GLuint mPgm;
  GLint mPositionHandle;
  GLint mTexSamplerHandle;
  GLint mTexMatrixHandle;

  CApplication* m_g_application;
  CApplicationMessenger* m_g_applicationMessenger;
  CWinSystemEGL* m_g_Windowing;
  CAdvancedSettings* m_g_advancedSettings;

  CFrameBufferObject fbo;
  EGLDisplay eglDisplay;
  EGLSurface eglSurface;
  EGLContext eglContext;
  bool eglInitialized;

  stSlot texslots[NUMFBOTEX];

  android::sp<android::MetaData> meta;
  int64_t framecount;
  std::list<Frame*> in_queue;
  std::map<int64_t, Frame*> out_queue;
  CCriticalSection in_mutex;
  CCriticalSection out_mutex;
  CCriticalSection free_mutex;
  XbmcThreads::ConditionVariable in_condition;
  XbmcThreads::ConditionVariable out_condition;

  int quirks;
  Frame *cur_frame;
  Frame *prev_frame;
  bool source_done;
  int x, y;
  int width, height;
  int texwidth, texheight;

  android::OMXClient *client;
  android::sp<android::MediaSource> decoder;
  const char *decoder_component;
  int videoColorFormat;
  int videoStride;
  int videoSliceHeight;

  bool drop_state;
  bool resetting;
#if defined(DEBUG_VERBOSE)
  unsigned int cycle_time;
#endif

  unsigned int mVideoTextureId;
  CJNISurfaceTexture* mSurfTexture;
  CJNISurface* mSurface;
  android::sp<ANativeWindow> mVideoNativeWindow;

  static void  CallbackInitSurfaceTexture(void*);
  bool InitSurfaceTexture();
  void ReleaseSurfaceTexture();
  void UpdateSurfaceTexture();
  void GetSurfaceTextureTransformMatrix(float* transformMatrix);
};
