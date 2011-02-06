/*
* LinuxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAS_SDL_2D

#include "LinuxRenderer.h"
#include "../../Application.h"
#include "../../Util.h"
#include "TextureManager.h"
#include "../ffmpeg/DllSwScale.h"

// http://www.martinreddy.net/gfx/faqs/colorconv.faq

YUVRANGE yuv_range_lim =  { 16, 235, 16, 240, 16, 240 };
YUVRANGE yuv_range_full = {  0, 255,  0, 255,  0, 255 };

YUVCOEF yuv_coef_bt601 = {
     0.0f,   1.403f,
  -0.344f,  -0.714f,
   1.773f,     0.0f,
};

YUVCOEF yuv_coef_bt709 = {
     0.0f,  1.5701f,
 -0.1870f, -0.4664f,
  1.8556f,     0.0f, /* page above have the 1.8556f as negative */
};

YUVCOEF yuv_coef_ebu = {
    0.0f,  1.140f,
 -0.396f, -0.581f,
  2.029f,    0.0f,
};

YUVCOEF yuv_coef_smtp240m = {
     0.0f,  1.5756f,
 -0.2253f, -0.5000f, /* page above have the 0.5000f as positive */
  1.8270f,     0.0f,
};


CLinuxRenderer::CLinuxRenderer()
{
  CLog::Log(LOGDEBUG,"Created LinuxRenderer...");

  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    m_pOSDYTexture[i] = NULL;
    m_pOSDATexture[i] = NULL;
  }

  for (int p=0; p<MAX_PLANES; p++) {
     m_image.plane[p]=NULL;
     m_image.stride[p]=0;
  }

#ifdef USE_SDL_OVERLAY
  m_overlay = NULL;
  m_screen = NULL;
#else
  m_backbuffer = NULL;
  m_screenbuffer = NULL;
#endif

  m_image.flags = 0;

  m_dllSwScale = new DllSwScale;
}

CLinuxRenderer::~CLinuxRenderer()
{
  UnInit();
  delete m_dllSwScale;
}

//********************************************************************************************************
void CLinuxRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_pOSDYTexture[index])
  {
    SDL_FreeSurface(m_pOSDYTexture[index]);
    m_pOSDYTexture[index] = NULL;
  }
  if (m_pOSDATexture[index])
  {
    SDL_FreeSurface(m_pOSDATexture[index]);
    m_pOSDATexture[index] = NULL;
    CLog::Log(LOGDEBUG, "Deleted OSD textures (%i)", index);
  }
  m_iOSDTextureHeight[index] = 0;
}

//***********************************************************************************************************
void CLinuxRenderer::CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride)
{
  for (int y = 0; y < h; ++y)
  {
    memcpy(dst, src, w);
    memcpy(dsta, srca, w);
    src += srcstride;
    srca += srcstride;
    dst += dststride;
    dsta += dststride;
  }
}

void CLinuxRenderer::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
  // OSD is drawn after draw_slice / put_image
  // this means that the buffer has already been handed off to the RGB converter
  // solution: have separate OSD textures

  // if it's down the bottom, use sub alpha blending
  //  m_SubsOnOSD = (y0 > (int)(m_sourceRect.bottom - m_sourceRect.top) * 4 / 5);

  //Sometimes happens when switching between fullscreen and small window
  if( w == 0 || h == 0 )
  {
    CLog::Log(LOGINFO, "Zero dimensions specified to DrawAlpha, skipping");
    return;
  }

  //use temporary rect for calculation to avoid messing with module-rect while other functions might be using it.
  DRAWRECT osdRect;
  RESOLUTION res = GetResolution();

  if (w > m_iOSDTextureWidth)
  {
    //delete osdtextures so they will be recreated with the correct width
    for (int i = 0; i < 2; ++i)
    {
      DeleteOSDTextures(i);
    }
    m_iOSDTextureWidth = w;
  }
  else
  {
    // clip to buffer
    if (w > m_iOSDTextureWidth) w = m_iOSDTextureWidth;
    if (h > g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top)
    {
      h = g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top;
    }
  }

  // scale to fit screen
  const CRect rv = g_graphicsContext.GetViewWindow();

  // Vobsubs are defined to be 720 wide.
  // NOTE: This will not work nicely if we are allowing mplayer to render text based subs
  //       as it'll want to render within the pixel width it is outputting.

  float xscale;
  float yscale;

  if(true /*isvobsub*/) // xbox_video.cpp is fixed to 720x576 osd, so this should be fine
  { // vobsubs are given to us unscaled
    // scale them up to the full output, assuming vobsubs have same
    // pixel aspect ratio as the movie, and are 720 pixels wide

    float pixelaspect = m_fSourceFrameRatio * m_sourceHeight / m_sourceWidth;
    xscale = rv.Width() / 720.0f;
    yscale = xscale * g_settings.m_ResInfo[res].fPixelRatio / pixelaspect;
  }
  else
  { // text subs/osd assume square pixels, but will render to full size of view window
    // if mplayer could be fixed to use monitorpixelaspect when rendering it's osd
    // this would give perfect output, however monitorpixelaspect currently doesn't work
    // that way
    xscale = 1.0f;
    yscale = 1.0f;
  }

  // horizontal centering, and align to bottom of subtitles line
  osdRect.left = rv.x1 + (rv.Width() - (float)w * xscale) / 2.0f;
  osdRect.right = osdRect.left + (float)w * xscale;
  float relbottom = ((float)(g_settings.m_ResInfo[res].iSubtitles - g_settings.m_ResInfo[res].Overscan.top)) / (g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top);
  osdRect.bottom = rv.y1 + rv.Height() * relbottom;
  osdRect.top = osdRect.bottom - (float)h * yscale;

  int iOSDBuffer = (m_iOSDRenderBuffer + 1) % m_NumOSDBuffers;

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
    m_pOSDYTexture[iOSDBuffer] = SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer],
		32, RMASK, GMASK, BMASK, AMASK);

    m_pOSDATexture[iOSDBuffer] = SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer],
		32, RMASK, GMASK, BMASK, AMASK);

    if (m_pOSDYTexture[iOSDBuffer] == NULL || m_pOSDATexture[iOSDBuffer] == NULL)
    {
      CLog::Log(LOGERROR, "Could not create OSD/Sub textures");
      DeleteOSDTextures(iOSDBuffer);
      return;
    }
    else
    {
      CLog::Log(LOGDEBUG, "Created OSD textures (%i)", iOSDBuffer);
    }
  }

  //We know the resources have been used at this point (or they are the second buffer, wich means they aren't in use anyways)
  //reset these so the gpu doesn't try to block on these
  if (SDL_LockSurface(m_pOSDYTexture[iOSDBuffer]) == 0 &&
      SDL_LockSurface(m_pOSDATexture[iOSDBuffer]) == 0)
  {
    //clear the textures
    memset(m_pOSDYTexture[iOSDBuffer]->pixels, 0, m_pOSDYTexture[iOSDBuffer]->pitch*m_iOSDTextureHeight[iOSDBuffer]);
    memset(m_pOSDATexture[iOSDBuffer]->pixels, 0, m_pOSDATexture[iOSDBuffer]->pitch*m_iOSDTextureHeight[iOSDBuffer]);

    //draw the osd/subs
    CopyAlpha(w, h, src, srca, stride, (BYTE*)m_pOSDYTexture[iOSDBuffer]->pixels, (BYTE*)m_pOSDATexture[iOSDBuffer]->pixels, m_pOSDYTexture[iOSDBuffer]->pitch);
  }
  SDL_UnlockSurface(m_pOSDYTexture[iOSDBuffer]);
  SDL_UnlockSurface(m_pOSDATexture[iOSDBuffer]);

  //set module variables to calculated values
  m_OSDRect = osdRect;
  m_OSDWidth = (float)w;
  m_OSDHeight = (float)h;
  m_OSDRendered = true;
}

//********************************************************************************************************
void CLinuxRenderer::RenderOSD()
{
  int iRenderBuffer = m_iOSDRenderBuffer;

  if (!m_pOSDYTexture[iRenderBuffer] || !m_pOSDATexture[iRenderBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;

  CSingleLock lock(g_graphicsContext);

  //copy all static vars to local vars because they might change during this function by mplayer callbacks
  DRAWRECT osdRect = m_OSDRect;

  // Render the image
  SDL_Rect rect;
  rect.x = osdRect.left;
  rect.y = osdRect.top;
  rect.w = osdRect.right - osdRect.left;
  rect.h = osdRect.bottom - osdRect.top;
  g_graphicsContext.BlitToScreen(m_pOSDYTexture[iRenderBuffer],NULL,&rect);

}

void CLinuxRenderer::ManageTextures()
{
  int neededbuffers = 0;
  //use 1 buffer in fullscreen mode and 2 buffers in windowed mode
  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (m_NumOSDBuffers != 1)
    {
      m_iOSDRenderBuffer = 0;
      m_NumOSDBuffers = 1;
      m_OSDWidth = m_OSDHeight = 0;
      //delete second osd textures
      DeleteOSDTextures(1);
    }
    neededbuffers = 1;
  }
  else
  {
    if (m_NumOSDBuffers != 2)
    {
      m_NumOSDBuffers = 2;
      m_iOSDRenderBuffer = 0;
      m_OSDWidth = m_OSDHeight = 0;
      // buffers will be created on demand in DrawAlpha()
    }
    neededbuffers = 2;
  }

}

bool CLinuxRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  CLog::Log(LOGDEBUG,"CLinuxRenderer::Configure - w: %d, h: %d, dw: %d, dh: %d, fps: %4.2f", width, height, d_width, d_height, fps);

  m_sourceWidth = width;
  m_sourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();

#ifdef USE_SDL_OVERLAY

  m_screen = g_graphicsContext.getScreenSurface()->SDL();

  if (m_overlay && (m_overlay->w != width || m_overlay->h != height)) {
     SDL_FreeYUVOverlay(m_overlay);
     m_overlay = NULL;
  }

  if (m_overlay == NULL && m_screen != NULL) {
     m_overlay =  SDL_CreateYUVOverlay(width, height, SDL_YV12_OVERLAY, m_screen);
     if (m_overlay == NULL) {
        CLog::Log(LOGERROR, "CLinuxRenderer::Configure - failed to create YUV overlay. w: %d, h: %d", width, height);
        return false;
     }
  }

  if (m_image.plane[0] == NULL) {
     m_image.stride[0] = m_overlay->pitches[0];
     m_image.stride[1] = m_overlay->pitches[1];
     m_image.stride[2] = m_overlay->pitches[2];

     m_image.plane[0] = m_overlay->pixels[0];
     m_image.plane[1] = m_overlay->pixels[1];
     m_image.plane[2] = m_overlay->pixels[2];
  }

#else
  if (m_backbuffer && (m_backbuffer->w != (int)width || m_backbuffer->h != (int)height)) {
     SDL_FreeSurface(m_backbuffer);
     m_backbuffer=NULL;

     for (int p=0;p<MAX_PLANES;p++) {
        if (m_image.plane[p])
           delete [] m_image.plane[p];
        m_image.plane[p]=NULL;
     }
  }

  if (m_screenbuffer && (m_screenbuffer->w != (int)width || m_screenbuffer->h != (int)height)) {
     SDL_FreeSurface(m_screenbuffer);
     m_screenbuffer=NULL;
  }

  if (m_backbuffer == NULL)
     m_backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, RMASK, GMASK, BMASK, AMASK);;

  if (m_screenbuffer == NULL)
     m_screenbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, RMASK, GMASK, BMASK, AMASK);;

  if (m_image.plane[0] == NULL) {
     m_image.stride[0] = m_sourceWidth;
     m_image.stride[1] = m_sourceWidth>>1;
     m_image.stride[2] = m_sourceWidth>>1;

     m_image.plane[0] = new BYTE[m_image.stride[0] * height];
     if (m_image.plane[1])
        delete [] m_image.plane[1];
     m_image.plane[1] = new BYTE[m_image.stride[1] * height];
     if (m_image.plane[2])
        delete [] m_image.plane[2];
     m_image.plane[2] = new BYTE[m_image.stride[2] * height];
  }

#endif

  m_image.width = width;
  m_image.height = height;
  m_image.cshift_x = 1;
  m_image.cshift_y = 1;

  CLog::Log(LOGDEBUG,"Allocated image. source w: %d, strides: %d,%d,%d", m_sourceWidth, m_image.stride[0], m_image.stride[1], m_image.stride[2]);

  return true;
}

int CLinuxRenderer::GetImage(YV12Image *image, int source, bool readonly)
{

  if (!image) return -1;

  CSingleLock lock(g_graphicsContext);

  if (!m_image.plane[0]) {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - image planes not allocated");
     return -1;
  }

  if (m_image.flags != 0) {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - request image but none to give");
     return -1;
  }

  if( source != AUTOSOURCE ) {
    CLog::Log(LOGERROR,"CLinuxRenderer::GetImage - currently supporting only single image. no source selection");
  }

  m_image.flags = readonly?IMAGE_FLAG_READING:IMAGE_FLAG_WRITING;

#ifdef USE_SDL_OVERLAY
  SDL_LockYUVOverlay(m_overlay);
#endif

  // copy the image - should be operator of YV12Image
  for (int p=0;p<MAX_PLANES;p++) {
     image->plane[p]=m_image.plane[p];
     image->stride[p] = m_image.stride[p];
  }
  image->width = m_image.width;
  image->height = m_image.height;
  image->flags = m_image.flags;
  image->cshift_x = m_image.cshift_x;
  image->cshift_y = m_image.cshift_y;

  return 0;
}

void CLinuxRenderer::ReleaseImage(int source, bool preserve)
{
  CSingleLock lock(g_graphicsContext);
  m_image.flags = 0;

#ifdef USE_SDL_OVERLAY
  if (m_overlay)
     SDL_UnlockYUVOverlay(m_overlay);
#else

  if (!m_backbuffer) {
     return;
  }

  // copy image to backbuffer

  SDL_LockSurface(m_backbuffer);

  // transform from YUV to RGB
  struct SwsContext *context = m_dllSwScale->sws_getContext(m_image.width, m_image.height, PIX_FMT_YUV420P, m_backbuffer->w, m_backbuffer->h, PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
  uint8_t *src[]  = { m_image.plane[0], m_image.plane[1], m_image.plane[2], 0 };
  int srcStride[] = { m_image.stride[0], m_image.stride[1], m_image.stride[2], 0 };
  uint8_t *dst[]  = { (uint8_t*)m_backbuffer->pixels, 0, 0, 0 };
  int dstStride[] = { m_backbuffer->pitch, 0, 0, 0 };

  m_dllSwScale->sws_scale(context, src, srcStride, 0, m_image.height, dst, dstStride);

  for (int n=0; n<720*90;n++) {
    *(((uint8_t*)m_backbuffer->pixels) + (720*10) + n) = 70;
  }

  m_dllSwScale->sws_freeContext(context);

  SDL_UnlockSurface(m_backbuffer);

  FlipPage(0);
#endif


}

void CLinuxRenderer::Reset()
{
}

void CLinuxRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();
}

void CLinuxRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{

  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();

// TODO: prepare for render in RenderUpdate

  Render(flags);


}

void CLinuxRenderer::FlipPage(int source)
{
// TODO: if image buffer changed (due to DrawSlice) than re-copy its content (convert from YUV).

  // copy back buffer to screen buffer
#ifndef USE_SDL_OVERLAY
  if (m_screenbuffer) {
     SDL_BlitSurface(m_backbuffer, NULL, m_screenbuffer, NULL);
  }
#endif

  return;
}


unsigned int CLinuxRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;

  YV12Image &im = m_image;
  // copy Y
  p = 0;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  w >>= im.cshift_x; h >>= im.cshift_y;
  x >>= im.cshift_x; y >>= im.cshift_y;

  // copy U
  p = 1;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  // copy V
  p = 2;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  return 0;
}

unsigned int CLinuxRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_resolution = PAL_4x3;

  m_iOSDRenderBuffer = 0;
  m_NumOSDBuffers = 0;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  if (!m_dllSwScale->Load())
        CLog::Log(LOGERROR,"CLinuxRendererGL::PreInit - failed to load rescale libraries!");

  #if (! defined USE_EXTERNAL_FFMPEG)
    m_dllSwScale->sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #elif (defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)
    m_dllSwScale->sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #endif

  return 0;
}

void CLinuxRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  for (int i=0; i< NUM_BUFFERS; i++) {
    DeleteOSDTextures(i);
  }

#ifdef USE_SDL_OVERLAY
  if (m_overlay)
     SDL_FreeYUVOverlay(m_overlay);

#else
  if (m_backbuffer) {
     SDL_FreeSurface(m_backbuffer);
     m_backbuffer=NULL;
  }

  if (m_screenbuffer) {
     SDL_FreeSurface(m_screenbuffer);
     m_screenbuffer=NULL;
  }
#endif

  for (int p=0; p<MAX_PLANES;p++) {
     if (m_image.plane[p]) {
#ifndef USE_SDL_OVERLAY
        delete [] m_image.plane[p];
#endif
        m_image.plane[p] = NULL;
        m_image.stride[p]=0;
     }
  }

}

void CLinuxRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;

  /* general stuff */
  RenderLowMem(flags);
  RenderOSD();
}

void CLinuxRenderer::RenderLowMem(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);

#if defined (USE_SDL_OVERLAY)

  SDL_Rect rect;
  rect.x = (int)m_sourceRect.x1;
  rect.y = (int)m_sourceRect.y1;
  rect.w = (int)m_sourceRect.Width();
  rect.h = (int)m_sourceRect.Height();

  int nRet = SDL_DisplayYUVOverlay(m_overlay, &rect);
#else

  SDL_Rect rect;
  rect.x = (int)m_sourceRect.x1;
  rect.y = (int)m_sourceRect.y1;
  rect.w = (int)m_sourceRect.Width();
  rect.h = (int)m_sourceRect.Height();
  g_graphicsContext.BlitToScreen(m_screenbuffer,NULL,&rect);

#endif
}

void CLinuxRenderer::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);
#ifndef USE_SDL_OVERLAY
  // we use backbuffer for the thumbnail cause screen buffer isnt used when opengl is used.
  if (m_backbuffer && texture) {
     SDL_BlitSurface(m_backbuffer, NULL, texture->GetPixels(), NULL);
  }
#endif
}

// called from GUI thread after playback has finished to release resources
void CLinuxRenderer::OnClose()
{
}

#endif // HAS_SDL_2D
