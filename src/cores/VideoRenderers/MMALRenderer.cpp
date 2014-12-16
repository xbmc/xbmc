/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Util.h"
#include "MMALRenderer.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "windowing/WindowingFactory.h"
#include "cores/dvdplayer/DVDCodecs/Video/MMALCodec.h"
#include "xbmc/Application.h"

#define CLASSNAME "CMMALRenderer"

#ifdef _DEBUG
#define MMAL_DEBUG_VERBOSE
#endif

static void vout_control_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  mmal_buffer_header_release(buffer);
}

void CMMALRenderer::vout_input_port_cb(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALVideoBuffer *omvb = (CMMALVideoBuffer *)buffer->user_data;

  #if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s port:%p buffer %p (%p), len %d cmd:%x", CLASSNAME, __func__, port, buffer, omvb, buffer->length, buffer->cmd);
  #endif

  if (m_format == RENDER_FMT_MMAL)
  {
    mmal_queue_put(m_release_queue, buffer);
  }
  else if (m_format == RENDER_FMT_YUV420P)
  {
    mmal_buffer_header_release(buffer);
  }
  else assert(0);
}

static void vout_input_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
  CMMALRenderer *mmal = reinterpret_cast<CMMALRenderer*>(port->userdata);
  mmal->vout_input_port_cb(port, buffer);
}

bool CMMALRenderer::init_vout(MMAL_ES_FORMAT_T *format)
{
  MMAL_STATUS_T status;

  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);

  /* Create video renderer */
  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER, &m_vout);
  if(status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create vout component (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_vout->control->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  status = mmal_port_enable(m_vout->control, vout_control_port_cb);
  if(status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable vout control port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }
  m_vout_input = m_vout->input[0];
  m_vout_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;
  mmal_format_full_copy(m_vout_input->format, format);
  status = mmal_port_format_commit(m_vout_input);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to commit vout input format (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  m_vout_input->buffer_num = m_vout_input->buffer_num_recommended;
  m_vout_input->buffer_size = m_vout_input->buffer_size_recommended;

  status = mmal_port_enable(m_vout_input, vout_input_port_cb_static);
  if(status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to vout enable input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_component_enable(m_vout);
  if(status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to enable vout component (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
    return false;
  }

  if (m_format == RENDER_FMT_YUV420P)
  {
    m_vout_input_pool = mmal_pool_create(m_vout_input->buffer_num, m_vout_input->buffer_size);
    if (!m_vout_input_pool)
    {
      CLog::Log(LOGERROR, "%s::%s Failed to create pool for decoder input port (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));
      return false;
    }
  }
  return true;
}

void CMMALRenderer::Process()
{
  MMAL_BUFFER_HEADER_T *buffer;
  while (buffer = mmal_queue_wait(m_release_queue), buffer)
  {
    CMMALVideoBuffer *omvb = (CMMALVideoBuffer *)buffer->user_data;
    omvb->Release();
  }
  m_sync.Set();
}

CMMALRenderer::CMMALRenderer()
: CThread("CMMALRenderer")
{
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
  m_vout = NULL;
  m_vout_input = NULL;
  m_vout_input_pool = NULL;
  memset(m_buffers, 0, sizeof m_buffers);
  m_release_queue = mmal_queue_create();
  Create();
}

CMMALRenderer::~CMMALRenderer()
{
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
  // shutdown thread
  mmal_queue_put(m_release_queue, NULL);
  m_sync.Wait();
  mmal_queue_destroy(m_release_queue);
  UnInit();
}

void CMMALRenderer::AddProcessor(CMMALVideoBuffer *buffer, int index)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %p (%p) %i", CLASSNAME, __func__, buffer, buffer->mmal_buffer, index);
#endif

  YUVBUFFER &buf = m_buffers[index];
  assert(!buf.MMALBuffer);
  memset(&buf, 0, sizeof buf);
  buf.MMALBuffer = buffer->Acquire();
}

bool CMMALRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  ReleaseBuffers();

  m_sourceWidth  = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;

  m_fps = fps;
  m_iFlags = flags;
  m_format = format;

  CLog::Log(LOGDEBUG, "%s::%s - %dx%d->%dx%d@%.2f flags:%x format:%d ext:%x orient:%d", CLASSNAME, __func__, width, height, d_width, d_height, fps, flags, format, extended_format, orientation);

  m_RenderUpdateCallBackFn = NULL;
  m_RenderUpdateCallBackCtx = NULL;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  m_destWidth = g_graphicsContext.GetResInfo(m_resolution).iWidth;
  m_destHeight = g_graphicsContext.GetResInfo(m_resolution).iHeight;
  SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
  ManageDisplay();

  if (m_format == RENDER_FMT_MMAL|| m_format == RENDER_FMT_YUV420P)
  {
    MMAL_ES_FORMAT_T *es_format = mmal_format_alloc();
    es_format->type = MMAL_ES_TYPE_VIDEO;
    es_format->es->video.crop.width = m_sourceWidth;
    es_format->es->video.crop.height = m_sourceHeight;

    if (m_format == RENDER_FMT_MMAL)
    {
      es_format->encoding = MMAL_ENCODING_OPAQUE;
      es_format->es->video.width = m_sourceWidth;
      es_format->es->video.height = m_sourceHeight;
    }
    else if (m_format == RENDER_FMT_YUV420P)
    {
      const int pitch = ALIGN_UP(m_sourceWidth, 32);
      const int aligned_height = ALIGN_UP(m_sourceHeight, 16);

      es_format->encoding = MMAL_ENCODING_I420;
      es_format->es->video.width = pitch;
      es_format->es->video.height = aligned_height;

      if (CONF_FLAGS_YUVCOEF_MASK(m_iFlags) == CONF_FLAGS_YUVCOEF_BT709)
        es_format->es->video.color_space = MMAL_COLOR_SPACE_ITUR_BT709;
      else if (CONF_FLAGS_YUVCOEF_MASK(m_iFlags) == CONF_FLAGS_YUVCOEF_BT601)
        es_format->es->video.color_space = MMAL_COLOR_SPACE_ITUR_BT601;
      else if (CONF_FLAGS_YUVCOEF_MASK(m_iFlags) == CONF_FLAGS_YUVCOEF_240M)
        es_format->es->video.color_space = MMAL_COLOR_SPACE_SMPTE240M;
    }
    if (m_bConfigured)
      UnInit();
    m_bConfigured = init_vout(es_format);
    mmal_format_free(es_format);
  }
  else
    m_bConfigured = true;

  return m_bConfigured;
}

int CMMALRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %p %d %d", CLASSNAME, __func__, image, source, readonly);
#endif
  if (!image) return -1;

  if( source < 0)
    return -1;

  if (m_format == RENDER_FMT_MMAL)
  {
  }
  else if (m_format == RENDER_FMT_YUV420P)
  {
    const int pitch = ALIGN_UP(m_sourceWidth, 32);
    const int aligned_height = ALIGN_UP(m_sourceHeight, 16);

    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_timedwait(m_vout_input_pool->queue, 500);
    if (!buffer)
    {
      CLog::Log(LOGERROR, "%s::%s - mmal_queue_get failed", CLASSNAME, __func__);
      return -1;
    }

    mmal_buffer_header_reset(buffer);

    buffer->length = 3 * pitch * aligned_height >> 1;
    assert(buffer->length <= buffer->alloc_size);

    image->width    = m_sourceWidth;
    image->height   = m_sourceHeight;
    image->flags    = 0;
    image->cshift_x = 1;
    image->cshift_y = 1;
    image->bpp      = 1;

    image->stride[0] = pitch;
    image->stride[1] = image->stride[2] = pitch>>image->cshift_x;

    image->planesize[0] = pitch * aligned_height;
    image->planesize[1] = image->planesize[2] = (pitch>>image->cshift_x)*(aligned_height>>image->cshift_y);

    image->plane[0] = (uint8_t *)buffer->data;
    image->plane[1] = image->plane[0] + image->planesize[0];
    image->plane[2] = image->plane[1] + image->planesize[1];

    CLog::Log(LOGDEBUG, "%s::%s - %p %d", CLASSNAME, __func__, buffer, source);
    YUVBUFFER &buf = m_buffers[source];
    memset(&buf, 0, sizeof buf);
    buf.mmal_buffer = buffer;
  }
  else assert(0);

  return source;
}

void CMMALRenderer::ReleaseBuffer(int idx)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %d", CLASSNAME, __func__, idx);
#endif
  YUVBUFFER &buf = m_buffers[idx];
  SAFE_RELEASE(buf.MMALBuffer);
}

void CMMALRenderer::ReleaseImage(int source, bool preserve)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %d %d", CLASSNAME, __func__, source, preserve);
#endif
}

void CMMALRenderer::Reset()
{
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
}

void CMMALRenderer::Flush()
{
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
}

void CMMALRenderer::Update()
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
#endif
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CMMALRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %d %x %d", CLASSNAME, __func__, clear, flags, alpha);
#endif

  if (!m_bConfigured) return;

  CSingleLock lock(g_graphicsContext);

  ManageDisplay();

  // if running bypass, then the player might need the src/dst rects
  // for sizing video playback on a layer other than the gles layer.
  if (m_RenderUpdateCallBackFn)
    (*m_RenderUpdateCallBackFn)(m_RenderUpdateCallBackCtx, m_sourceRect, m_destRect);

  SetVideoRect(m_sourceRect, m_destRect);

  CRect old = g_graphicsContext.GetScissors();

  g_graphicsContext.BeginPaint();
  g_graphicsContext.SetScissors(m_destRect);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  g_graphicsContext.SetScissors(old);
  g_graphicsContext.EndPaint();
}

void CMMALRenderer::FlipPage(int source)
{
#if defined(MMAL_DEBUG_VERBOSE)
  CLog::Log(LOGDEBUG, "%s::%s - %d", CLASSNAME, __func__, source);
#endif

  if (!m_bConfigured || m_format == RENDER_FMT_BYPASS)
    return;

  YUVBUFFER *buffer = &m_buffers[source];
  // we only want to upload frames once
  if (buffer->flipindex++)
    return;
  if (m_format == RENDER_FMT_MMAL)
  {
    CMMALVideoBuffer *omvb = buffer->MMALBuffer;
    if (omvb)
    {
#if defined(MMAL_DEBUG_VERBOSE)
      CLog::Log(LOGDEBUG, "%s::%s %p (%p)", CLASSNAME, __func__, omvb, omvb->mmal_buffer);
#endif
      omvb->Acquire();
      mmal_port_send_buffer(m_vout_input, omvb->mmal_buffer);
    } else assert(0);
  }
  else if (m_format == RENDER_FMT_YUV420P)
  {
    CLog::Log(LOGDEBUG, "%s::%s - %p %d", CLASSNAME, __func__, buffer->mmal_buffer, source);
    if (buffer->mmal_buffer)
      mmal_port_send_buffer(m_vout_input, buffer->mmal_buffer);
    else assert(0);
  }
  else assert(0);
}

unsigned int CMMALRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();

  m_iFlags = 0;

  m_resolution = CDisplaySettings::Get().GetCurrentResolution();
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);

  m_formats.clear();
  m_formats.push_back(RENDER_FMT_YUV420P);
  m_formats.push_back(RENDER_FMT_MMAL);
  m_formats.push_back(RENDER_FMT_BYPASS);

  m_NumYV12Buffers = NUM_BUFFERS;

  return 0;
}

void CMMALRenderer::ReleaseBuffers()
{
  for (int i=0; i<NUM_BUFFERS; i++)
    ReleaseBuffer(i);
}

void CMMALRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);
  CLog::Log(LOGDEBUG, "%s::%s", CLASSNAME, __func__);
  if (m_vout)
  {
    mmal_component_disable(m_vout);
    mmal_port_disable(m_vout->control);
  }

  if (m_vout_input)
  {
    mmal_port_flush(m_vout_input);
    mmal_port_disable(m_vout_input);
    m_vout_input = NULL;
  }

  if (m_vout_input_pool)
  {
    mmal_pool_destroy(m_vout_input_pool);
    m_vout_input_pool = NULL;
  }

  if (m_vout)
  {
    mmal_component_release(m_vout);
    m_vout = NULL;
  }
  ReleaseBuffers();

  m_RenderUpdateCallBackFn = NULL;
  m_RenderUpdateCallBackCtx = NULL;

  m_src_rect.SetRect(0, 0, 0, 0);
  m_dst_rect.SetRect(0, 0, 0, 0);
  m_video_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_display_stereo_mode = RENDER_STEREO_MODE_OFF;
  m_StereoInvert = false;

  m_bConfigured = false;
}

bool CMMALRenderer::RenderCapture(CRenderCapture* capture)
{
  if (!m_bConfigured)
    return false;

  CLog::Log(LOGDEBUG, "%s::%s - %p", CLASSNAME, __func__, capture);

  capture->BeginRender();
  capture->EndRender();

  return true;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************

bool CMMALRenderer::Supports(EDEINTERLACEMODE mode)
{
  if(mode == VS_DEINTERLACEMODE_OFF
  || mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE)
    return true;

  return false;
}

bool CMMALRenderer::Supports(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_MMAL_ADVANCED)
    return true;
  if (method == VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF)
    return true;
  if (method == VS_INTERLACEMETHOD_MMAL_BOB)
    return true;
  if (method == VS_INTERLACEMETHOD_MMAL_BOB_HALF)
    return true;

  return false;
}

bool CMMALRenderer::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_ROTATION        ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO)
    return true;

  return false;
}

bool CMMALRenderer::Supports(ESCALINGMETHOD method)
{
  return false;
}

EINTERLACEMETHOD CMMALRenderer::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_MMAL_ADVANCED;
}

void CMMALRenderer::SetVideoRect(const CRect& InSrcRect, const CRect& InDestRect)
{
  // we get called twice a frame for left/right. Can ignore the rights.
  if (g_graphicsContext.GetStereoView() == RENDER_STEREO_VIEW_RIGHT)
    return;

  if (!m_vout_input)
    return;

  CRect SrcRect = InSrcRect, DestRect = InDestRect;
  RENDER_STEREO_MODE video_stereo_mode = (m_iFlags & CONF_FLAGS_STEREO_MODE_SBS) ? RENDER_STEREO_MODE_SPLIT_VERTICAL :
                                         (m_iFlags & CONF_FLAGS_STEREO_MODE_TAB) ? RENDER_STEREO_MODE_SPLIT_HORIZONTAL : RENDER_STEREO_MODE_OFF;
  bool stereo_invert                   = (m_iFlags & CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT) ? true : false;
  RENDER_STEREO_MODE display_stereo_mode = g_graphicsContext.GetStereoMode();

  // fix up transposed video
  if (m_renderOrientation == 90 || m_renderOrientation == 270)
  {
    float diff = (DestRect.Height() - DestRect.Width()) * 0.5f;
    DestRect.x1 -= diff;
    DestRect.x2 += diff;
    DestRect.y1 += diff;
    DestRect.y2 -= diff;
  }

  // check if destination rect or video view mode has changed
  if (!(m_dst_rect != DestRect) && !(m_src_rect != SrcRect) && m_video_stereo_mode == video_stereo_mode && m_display_stereo_mode == display_stereo_mode && m_StereoInvert == stereo_invert)
    return;

  CLog::Log(LOGDEBUG, "%s::%s %d,%d,%d,%d -> %d,%d,%d,%d (o:%d v:%d d:%d i:%d)", CLASSNAME, __func__,
      (int)SrcRect.x1, (int)SrcRect.y1, (int)SrcRect.x2, (int)SrcRect.y2,
      (int)DestRect.x1, (int)DestRect.y1, (int)DestRect.x2, (int)DestRect.y2,
      m_renderOrientation, video_stereo_mode, display_stereo_mode, stereo_invert);

  m_src_rect = SrcRect;
  m_dst_rect = DestRect;
  m_video_stereo_mode = video_stereo_mode;
  m_display_stereo_mode = display_stereo_mode;
  m_StereoInvert = stereo_invert;

  // might need to scale up m_dst_rect to display size as video decodes
  // to separate video plane that is at display size.
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  CRect gui(0, 0, CDisplaySettings::Get().GetResolutionInfo(res).iWidth, CDisplaySettings::Get().GetResolutionInfo(res).iHeight);
  CRect display(0, 0, CDisplaySettings::Get().GetResolutionInfo(res).iScreenWidth, CDisplaySettings::Get().GetResolutionInfo(res).iScreenHeight);

  if (display_stereo_mode != RENDER_STEREO_MODE_OFF && display_stereo_mode != RENDER_STEREO_MODE_MONO)
  switch (video_stereo_mode)
  {
  case RENDER_STEREO_MODE_SPLIT_VERTICAL:
    // optimisation - use simpler display mode in common case of unscaled 3d with same display mode
    if (video_stereo_mode == display_stereo_mode && DestRect.x1 == 0.0f && DestRect.x2 * 2.0f == gui.Width() && !stereo_invert)
    {
      SrcRect.x2 *= 2.0f;
      DestRect.x2 *= 2.0f;
      video_stereo_mode = RENDER_STEREO_MODE_OFF;
      display_stereo_mode = RENDER_STEREO_MODE_OFF;
    }
    else if (display_stereo_mode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN || display_stereo_mode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
    {
      SrcRect.x2 *= 2.0f;
    }
    break;

  case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
    // optimisation - use simpler display mode in common case of unscaled 3d with same display mode
    if (video_stereo_mode == display_stereo_mode && DestRect.y1 == 0.0f && DestRect.y2 * 2.0f == gui.Height() && !stereo_invert)
    {
      SrcRect.y2 *= 2.0f;
      DestRect.y2 *= 2.0f;
      video_stereo_mode = RENDER_STEREO_MODE_OFF;
      display_stereo_mode = RENDER_STEREO_MODE_OFF;
    }
    else if (display_stereo_mode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN || display_stereo_mode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
    {
      SrcRect.y2 *= 2.0f;
    }
    break;

  default: break;
  }

  if (gui != display)
  {
    float xscale = display.Width()  / gui.Width();
    float yscale = display.Height() / gui.Height();
    DestRect.x1 *= xscale;
    DestRect.x2 *= xscale;
    DestRect.y1 *= yscale;
    DestRect.y2 *= yscale;
  }

  MMAL_DISPLAYREGION_T region;
  memset(&region, 0, sizeof region);

  region.set                 = MMAL_DISPLAY_SET_DEST_RECT|MMAL_DISPLAY_SET_SRC_RECT|MMAL_DISPLAY_SET_FULLSCREEN|MMAL_DISPLAY_SET_NOASPECT|MMAL_DISPLAY_SET_MODE;
  region.dest_rect.x         = lrintf(DestRect.x1);
  region.dest_rect.y         = lrintf(DestRect.y1);
  region.dest_rect.width     = lrintf(DestRect.Width());
  region.dest_rect.height    = lrintf(DestRect.Height());

  region.src_rect.x          = lrintf(SrcRect.x1);
  region.src_rect.y          = lrintf(SrcRect.y1);
  region.src_rect.width      = lrintf(SrcRect.Width());
  region.src_rect.height     = lrintf(SrcRect.Height());

  region.fullscreen = MMAL_FALSE;
  region.noaspect = MMAL_TRUE;

  if (m_renderOrientation)
  {
    region.set |= MMAL_DISPLAY_SET_TRANSFORM;
    if (m_renderOrientation == 90)
      region.transform = MMAL_DISPLAY_ROT90;
    else if (m_renderOrientation == 180)
      region.transform = MMAL_DISPLAY_ROT180;
    else if (m_renderOrientation == 270)
      region.transform = MMAL_DISPLAY_ROT270;
    else assert(0);
  }

  if (video_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL && display_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    region.mode = MMAL_DISPLAY_MODE_STEREO_TOP_TO_TOP;
  else if (video_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL && display_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    region.mode = MMAL_DISPLAY_MODE_STEREO_TOP_TO_LEFT;
  else if (video_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL && display_stereo_mode == RENDER_STEREO_MODE_SPLIT_HORIZONTAL)
    region.mode = MMAL_DISPLAY_MODE_STEREO_LEFT_TO_TOP;
  else if (video_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL && display_stereo_mode == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    region.mode = MMAL_DISPLAY_MODE_STEREO_LEFT_TO_LEFT;
  else
    region.mode = MMAL_DISPLAY_MODE_LETTERBOX;

  MMAL_STATUS_T status = mmal_util_set_display_region(m_vout_input, &region);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "%s::%s Failed to set display region (status=%x %s)", CLASSNAME, __func__, status, mmal_status_to_string(status));

  CLog::Log(LOGDEBUG, "%s::%s %d,%d,%d,%d -> %d,%d,%d,%d mode:%d", CLASSNAME, __func__,
      region.src_rect.x, region.src_rect.y, region.src_rect.width, region.src_rect.height,
      region.dest_rect.x, region.dest_rect.y, region.dest_rect.width, region.dest_rect.height, region.mode);
}
