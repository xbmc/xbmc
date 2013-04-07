#pragma once
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

#include "threads/Thread.h"
#include "xbmc/guilib/FrameBufferObject.h"
#include "cores/VideoRenderers/RenderFormats.h"

#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

#include <binder/ProcessState.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <gui/SurfaceTexture.h>

#include "system_gl.h"

#include <map>
#include <list>

#define NUMFBOTEX 4
#define INBUFCOUNT 16

class CStageFrightDecodeThread;

using namespace android;

struct tex_slot
{
  GLuint texid;
  EGLImageKHR eglimg;
};

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

class CStageFrightVideoPrivate : public MediaBufferObserver
{
public:
  CStageFrightVideoPrivate();

  virtual void signalBufferReturned(MediaBuffer *buffer);

  MediaBuffer* getBuffer(size_t size);
  bool inputBufferAvailable();

  void loadOESShader(GLenum shaderType, const char* pSource, GLuint* outShader);
  void createOESProgram(const char* pVertexSource, const char* pFragmentSource, GLuint* outPgm);
  void OES_shader_setUp();
  void InitializeEGL(int w, int h);
  void UninitializeEGL();

public:
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

  tex_slot slots[NUMFBOTEX];
  std::list< std::pair<EGLImageKHR, int> > free_queue;
  std::list< std::pair<EGLImageKHR, int> > busy_queue;

  sp<MetaData> meta;
  int64_t framecount;
  std::map<int64_t, Frame*> in_queue;
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
