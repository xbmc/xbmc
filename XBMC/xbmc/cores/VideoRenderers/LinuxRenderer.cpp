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

#include "stdafx.h"
#include "LinuxRenderer.h"
#include "../../Application.h"
#include "../../Util.h"
#include "../../XBVideoConfig.h"
#include "TextureManager.h"

#ifndef HAS_SDL_OPENGL

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

  m_fSourceFrameRatio = 1.0f;
  m_iResolution = PAL_4x3;
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

#ifdef HAS_SDL_OPENGL
  m_texture = NULL; 
#endif


#endif

  m_image.flags = 0;
 
}

CLinuxRenderer::~CLinuxRenderer()
{
  UnInit();
}

//********************************************************************************************************
void CLinuxRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_pOSDYTexture[index])
  {
#if defined (HAS_SDL_OPENGL)
    delete m_pOSDYTexture[index];
#else
    SDL_FreeSurface(m_pOSDYTexture[index]);
#endif
    m_pOSDYTexture[index] = NULL;
  }
  if (m_pOSDATexture[index])
  {
#if defined (HAS_SDL_OPENGL)
    delete m_pOSDATexture[index];
#else
    SDL_FreeSurface(m_pOSDATexture[index]);
#endif
    m_pOSDATexture[index] = NULL;
    CLog::Log(LOGDEBUG, "Deleted OSD textures (%i)", index);
  }
  m_iOSDTextureHeight[index] = 0;
}

//***************************************************************************************
// CalculateFrameAspectRatio()
//
// Considers the source frame size and output frame size (as suggested by mplayer)
// to determine if the pixels in the source are not square.  It calculates the aspect
// ratio of the output frame.  We consider the cases of VCD, SVCD and DVD separately,
// as these are intended to be viewed on a non-square pixel TV set, so the pixels are
// defined to be the same ratio as the intended display pixels.
// These formats are determined by frame size.
//***************************************************************************************
void CLinuxRenderer::CalculateFrameAspectRatio(int desired_width, int desired_height)
{
  m_fSourceFrameRatio = (float)desired_width / desired_height;

  // Check whether mplayer has decided that the size of the video file should be changed
  // This indicates either a scaling has taken place (which we didn't ask for) or it has
  // found an aspect ratio parameter from the file, and is changing the frame size based
  // on that.
  if ((int)m_iSourceWidth == desired_width && (int)m_iSourceHeight == desired_height)
    return ;

  // mplayer is scaling in one or both directions.  We must alter our Source Pixel Ratio
  float fImageFrameRatio = (float)m_iSourceWidth / m_iSourceHeight;

  // OK, most sources will be correct now, except those that are intended
  // to be displayed on non-square pixel based output devices (ie PAL or NTSC TVs)
  // This includes VCD, SVCD, and DVD (and possibly others that we are not doing yet)
  // For this, we can base the pixel ratio on the pixel ratios of PAL and NTSC,
  // though we will need to adjust for anamorphic sources (ie those whose
  // output frame ratio is not 4:3) and for SVCDs which have 2/3rds the
  // horizontal resolution of the default NTSC or PAL frame sizes

  // The following are the defined standard ratios for PAL and NTSC pixels
  float fPALPixelRatio = 128.0f / 117.0f;
  float fNTSCPixelRatio = 4320.0f / 4739.0f;

  // Calculate the correction needed for anamorphic sources
  float fNon4by3Correction = m_fSourceFrameRatio / (4.0f / 3.0f);

  // Finally, check for a VCD, SVCD or DVD frame size as these need special aspect ratios
  if (m_iSourceWidth == 352)
  { // VCD?
    if (m_iSourceHeight == 240) // NTSC
      m_fSourceFrameRatio = fImageFrameRatio * fNTSCPixelRatio;
    if (m_iSourceHeight == 288) // PAL
      m_fSourceFrameRatio = fImageFrameRatio * fPALPixelRatio;
  }
  if (m_iSourceWidth == 480)
  { // SVCD?
    if (m_iSourceHeight == 480) // NTSC
      m_fSourceFrameRatio = fImageFrameRatio * 3.0f / 2.0f * fNTSCPixelRatio * fNon4by3Correction;
    if (m_iSourceHeight == 576) // PAL
      m_fSourceFrameRatio = fImageFrameRatio * 3.0f / 2.0f * fPALPixelRatio * fNon4by3Correction;
  }
  if (m_iSourceWidth == 720)
  { // DVD?
    if (m_iSourceHeight == 480) // NTSC
      m_fSourceFrameRatio = fImageFrameRatio * fNTSCPixelRatio * fNon4by3Correction;
    if (m_iSourceHeight == 576) // PAL
      m_fSourceFrameRatio = fImageFrameRatio * fPALPixelRatio * fNon4by3Correction;
  }
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
  //  m_SubsOnOSD = (y0 > (int)(rs.bottom - rs.top) * 4 / 5);

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
  const RECT& rv = g_graphicsContext.GetViewWindow();

  // Vobsubs are defined to be 720 wide.
  // NOTE: This will not work nicely if we are allowing mplayer to render text based subs
  //       as it'll want to render within the pixel width it is outputting.

  float xscale;
  float yscale;

  if(true /*isvobsub*/) // xbox_video.cpp is fixed to 720x576 osd, so this should be fine
  { // vobsubs are given to us unscaled
    // scale them up to the full output, assuming vobsubs have same 
    // pixel aspect ratio as the movie, and are 720 pixels wide

    float pixelaspect = m_fSourceFrameRatio * m_iSourceHeight / m_iSourceWidth;
    xscale = (rv.right - rv.left) / 720.0f;
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
  osdRect.left = (float)rv.left + (float)(rv.right - rv.left - (float)w * xscale) / 2.0f;
  osdRect.right = osdRect.left + (float)w * xscale;
  float relbottom = ((float)(g_settings.m_ResInfo[res].iSubtitles - g_settings.m_ResInfo[res].Overscan.top)) / (g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top);
  osdRect.bottom = (float)rv.top + (float)(rv.bottom - rv.top) * relbottom;
  osdRect.top = osdRect.bottom - (float)h * yscale;

  int iOSDBuffer = (m_iOSDRenderBuffer + 1) % m_NumOSDBuffers;

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
#if defined(HAS_SDL_OPENGL)
    m_pOSDYTexture[iOSDBuffer] = new CGLTexture(SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 32, RMASK, GMASK, BMASK, AMASK),false,true);

    m_pOSDATexture[iOSDBuffer] = new CGLTexture(SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 32, RMASK, GMASK, BMASK, AMASK),false,true);

    if (m_pOSDYTexture[iOSDBuffer] == NULL || m_pOSDATexture[iOSDBuffer] == NULL) 
#else
    m_pOSDYTexture[iOSDBuffer] = SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 
		32, RMASK, GMASK, BMASK, AMASK);

    m_pOSDATexture[iOSDBuffer] = SDL_CreateRGBSurface(SDL_HWSURFACE, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 
		32, RMASK, GMASK, BMASK, AMASK);

    if (m_pOSDYTexture[iOSDBuffer] == NULL || m_pOSDATexture[iOSDBuffer] == NULL) 
#endif
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
#if defined(HAS_SDL_OPENGL)

  int textureBytesSize = m_pOSDYTexture[iOSDBuffer]->textureWidth * m_pOSDYTexture[iOSDBuffer]->textureHeight * 4; 
  unsigned char *dst = new unsigned char[textureBytesSize];
  unsigned char *dsta = new unsigned char[textureBytesSize];

  //clear the textures
  memset(dst, 0, textureBytesSize);
  memset(dsta, 0, textureBytesSize);
  
   //draw the osd/subs
  int dstPitch = m_pOSDYTexture[iOSDBuffer]->textureWidth * 4;
  CopyAlpha(w, h, src, srca, stride, dst, dsta, dstPitch);

  m_pOSDYTexture[iOSDBuffer]->Update(m_pOSDYTexture[iOSDBuffer]->textureWidth,
				     m_pOSDYTexture[iOSDBuffer]->textureHeight,
				     dstPitch,
				     dst,
				     false);

  m_pOSDATexture[iOSDBuffer]->Update(m_pOSDATexture[iOSDBuffer]->textureWidth,
				     m_pOSDATexture[iOSDBuffer]->textureHeight,
				     dstPitch,
				     dst,
				     false);
  delete [] dst;
  delete [] dsta;

#else
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
#endif

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

  // Set state to render the image
#if defined (HAS_SDL_OPENGL)
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;

  CGLTexture *pTex = m_pOSDYTexture[iRenderBuffer];
  pTex->LoadToGPU();
  glBindTexture(GL_TEXTURE_2D, pTex->id);
  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

  pTex = m_pOSDATexture[iRenderBuffer];
  pTex->LoadToGPU();
  glBindTexture(GL_TEXTURE_2D, pTex->id);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#endif

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // Render the image
#if defined(HAS_SDL_OPENGL)
  pTex = m_pOSDYTexture[iRenderBuffer];
  glBindTexture(GL_TEXTURE_2D, pTex->id);
  glBegin(GL_QUADS);

  float u1 = 0; 
  float u2 = (float)pTex->imageWidth / pTex->textureWidth;
  float v1 = 0;
  float v2 = (float)pTex->imageHeight / pTex->textureHeight;

  glTexCoord2f( u1, v1 );
  glVertex2f( osdRect.left, osdRect.top );

  glTexCoord2f( u2, v1 );
  glVertex2f( osdRect.right, osdRect.top );

  glTexCoord2f( u2, v2 );
  glVertex2f( osdRect.right, osdRect.bottom );

  glTexCoord2f( u1, v2 );
  glVertex2f( osdRect.left, osdRect.bottom );
 
  glEnd();

#else
  SDL_Rect rect;
  rect.x = osdRect.left;
  rect.y = osdRect.top;
  rect.w = osdRect.right - osdRect.left;
  rect.h = osdRect.bottom - osdRect.top; 
  g_graphicsContext.BlitToScreen(m_pOSDYTexture[iRenderBuffer],NULL,&rect);
#endif

}

//********************************************************************************************************
//Get resolution based on current mode.
RESOLUTION CLinuxRenderer::GetResolution()
{
  if (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
  {
    return m_iResolution;
  }
  return g_graphicsContext.GetVideoResolution();
}

float CLinuxRenderer::GetAspectRatio()
{
  float fWidth = (float)m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropLeft - g_stSettings.m_currentVideoSettings.m_CropRight;
  float fHeight = (float)m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropTop - g_stSettings.m_currentVideoSettings.m_CropBottom;
  return m_fSourceFrameRatio * fWidth / fHeight * m_iSourceHeight / m_iSourceWidth;
}

void CLinuxRenderer::GetVideoRect(RECT &rectSrc, RECT &rectDest)
{
  rectSrc = rs;
  rectDest = rd;
}

void CLinuxRenderer::CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fInputFrameRatio, float fZoomAmount)
{
  // scale up image as much as possible
  // and keep the aspect ratio (introduces with black bars)
  // calculate the correct output frame ratio (using the users pixel ratio setting
  // and the output pixel ratio setting)

  float fOutputFrameRatio = fInputFrameRatio / g_settings.m_ResInfo[GetResolution()].fPixelRatio;

  // maximize the movie width
  float fNewWidth = fScreenWidth;
  float fNewHeight = fNewWidth / fOutputFrameRatio;

  if (fNewHeight > fScreenHeight)
  {
    fNewHeight = fScreenHeight;
    fNewWidth = fNewHeight * fOutputFrameRatio;
  }

  // Scale the movie up by set zoom amount
  fNewWidth *= fZoomAmount;
  fNewHeight *= fZoomAmount;

  // Centre the movie
  float fPosY = (fScreenHeight - fNewHeight) / 2;
  float fPosX = (fScreenWidth - fNewWidth) / 2;

  rd.left = (int)(fPosX + fOffsetX1);
  rd.right = (int)(rd.left + fNewWidth + 0.5f);
  rd.top = (int)(fPosY + fOffsetY1);
  rd.bottom = (int)(rd.top + fNewHeight + 0.5f);
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

void CLinuxRenderer::ManageDisplay()
{
  const RECT& rv = g_graphicsContext.GetViewWindow();
  float fScreenWidth = (float)rv.right - rv.left;
  float fScreenHeight = (float)rv.bottom - rv.top;
  float fOffsetX1 = (float)rv.left;
  float fOffsetY1 = (float)rv.top;

  // source rect
  rs.left = g_stSettings.m_currentVideoSettings.m_CropLeft;
  rs.top = g_stSettings.m_currentVideoSettings.m_CropTop;
  rs.right = m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropRight;
  rs.bottom = m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropBottom;

  CalcNormalDisplayRect(fOffsetX1, fOffsetY1, fScreenWidth, fScreenHeight, GetAspectRatio() * g_stSettings.m_fPixelRatio, g_stSettings.m_fZoomAmount);
}

void CLinuxRenderer::ChooseBestResolution(float fps)
{
  // If the display resolution was specified by the user then use it
  RESOLUTION DisplayRes = (RESOLUTION) g_guiSettings.GetInt("videoplayer.displayresolution");
  if ( DisplayRes != AUTORES )
  {
    CLog::Log(LOGNOTICE, "Display resolution USER : %s (%d)", g_settings.m_ResInfo[DisplayRes].strMode, DisplayRes);
    m_iResolution = DisplayRes;
    return;
  }
  m_iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGNOTICE, "Display resolution AUTO : %s (%d)", g_settings.m_ResInfo[m_iResolution].strMode, m_iResolution);
  return;
}

bool CLinuxRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{

  CLog::Log(LOGDEBUG,"CLinuxRenderer::Configure - w: %d, h: %d, dw: %d, dh: %d, fps: %4.2f", width, height, d_width, d_height, fps);

  m_fps = fps;
  m_iSourceWidth = width;
  m_iSourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(m_fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

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

#ifdef HAS_SDL_OPENGL
     if (m_texture)
        delete m_texture;

     m_texture=NULL;
#endif
  }

  if (m_backbuffer == NULL)  
     m_backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, RMASK, GMASK, BMASK, AMASK);;

  if (m_screenbuffer == NULL)  
     m_screenbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, RMASK, GMASK, BMASK, AMASK);;

  if (m_image.plane[0] == NULL) {
     m_image.stride[0] = m_iSourceWidth;
     m_image.stride[1] = m_iSourceWidth>>1;
     m_image.stride[2] = m_iSourceWidth>>1;
   
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

  CLog::Log(LOGDEBUG,"Allocated image. source w: %d, strides: %d,%d,%d", m_iSourceWidth, m_image.stride[0], m_image.stride[1], m_image.stride[2]);

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
  struct SwsContext *context = m_dllSwScale.sws_getContext(m_image.width, m_image.height, PIX_FMT_YUV420P, m_backbuffer->w, m_backbuffer->h, PIX_FMT_ARGB, SWS_BILINEAR, NULL, NULL, NULL);
  uint8_t *src[] = { m_image.plane[0], m_image.plane[1], m_image.plane[2] };
  int     srcStride[] = { m_image.stride[0], m_image.stride[1], m_image.stride[2] };
  uint8_t *dst[] = { (uint8_t*)m_backbuffer->pixels, 0, 0 };
  int     dstStride[] = { m_backbuffer->pitch, 0, 0 };
  
  m_dllSwScale.sws_scale(context, src, srcStride, 0, m_image.height, dst, dstStride);

for (int n=0; n<720*90;n++) {
   *(((uint8_t*)m_backbuffer->pixels) + (720*10) + n) = 70;
}

  m_dllSwScale.sws_freeContext(context);

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

#ifndef HAS_SDL
  if (clear)
    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
  {
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_CONSTANTALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVCONSTANTALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_BLENDCOLOR, alpha );
  }
  else
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
#else 
// TODO: prepare for render in RenderUpdate
#endif

  Render(flags);


}

void CLinuxRenderer::FlipPage(int source)
{  
// TODO: if image buffer changed (due to DrawSlice) than re-copy its content (convert from YUV). 

  // copy back buffer to screen buffer
#ifndef USE_SDL_OVERLAY
#ifdef HAS_SDL_OPENGL
  if (!m_texture)
     m_texture = new CGLTexture(m_backbuffer,false,false);
  else
     m_texture->Update(m_backbuffer,false,false);

#else
  if (m_screenbuffer) {
     SDL_BlitSurface(m_backbuffer, NULL, m_screenbuffer, NULL);
  }
#endif
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
  m_iResolution = PAL_4x3;

  m_iOSDRenderBuffer = 0;
  m_NumOSDBuffers = 0;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load())
        CLog::Log(LOGERROR,"CLinuxRendererGL::PreInit - failed to load rescale libraries!");

  m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);

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

#ifdef HAS_SDL_OPENGL
  if (m_texture)
    delete m_texture;

  m_texture = NULL;
#endif

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

  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (g_application.NeedRenderFullScreen())
    { // render our subtitles and osd
      g_application.RenderFullScreen();
    }

    g_application.RenderMemoryStatus();
  }
}

void CLinuxRenderer::SetViewMode(int iViewMode)
{
  if (iViewMode < VIEW_MODE_NORMAL || iViewMode > VIEW_MODE_CUSTOM) iViewMode = VIEW_MODE_NORMAL;
  g_stSettings.m_currentVideoSettings.m_ViewMode = iViewMode;

  if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_NORMAL)
  { // normal mode...
    g_stSettings.m_fPixelRatio = 1.0;
    g_stSettings.m_fZoomAmount = 1.0;
    return ;
  }
  if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_CUSTOM)
  {
    g_stSettings.m_fZoomAmount = g_stSettings.m_currentVideoSettings.m_CustomZoomAmount;
    g_stSettings.m_fPixelRatio = g_stSettings.m_currentVideoSettings.m_CustomPixelRatio;
    return ;
  }

  // get our calibrated full screen resolution
  float fScreenWidth = (float)(g_settings.m_ResInfo[m_iResolution].Overscan.right - g_settings.m_ResInfo[m_iResolution].Overscan.left);
  float fScreenHeight = (float)(g_settings.m_ResInfo[m_iResolution].Overscan.bottom - g_settings.m_ResInfo[m_iResolution].Overscan.top);
  // and the source frame ratio
  float fSourceFrameRatio = GetAspectRatio();

  if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_ZOOM)
  { // zoom image so no black bars
    g_stSettings.m_fPixelRatio = 1.0;
    // calculate the desired output ratio
    float fOutputFrameRatio = fSourceFrameRatio * g_stSettings.m_fPixelRatio / g_settings.m_ResInfo[m_iResolution].fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full height.
    float fNewHeight = fScreenHeight;
    float fNewWidth = fNewHeight * fOutputFrameRatio;
    g_stSettings.m_fZoomAmount = fNewWidth / fScreenWidth;
    if (fNewWidth < fScreenWidth)
    { // zoom to full width
      fNewWidth = fScreenWidth;
      fNewHeight = fNewWidth / fOutputFrameRatio;
      g_stSettings.m_fZoomAmount = fNewHeight / fScreenHeight;
    }
  }
  else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_4x3)
  { // stretch image to 4:3 ratio
    g_stSettings.m_fZoomAmount = 1.0;
    if (m_iResolution == PAL_4x3 || m_iResolution == PAL60_4x3 || m_iResolution == NTSC_4x3 || m_iResolution == HDTV_480p_4x3)
    { // stretch to the limits of the 4:3 screen.
      // incorrect behaviour, but it's what the users want, so...
      g_stSettings.m_fPixelRatio = (fScreenWidth / fScreenHeight) * g_settings.m_ResInfo[m_iResolution].fPixelRatio / fSourceFrameRatio;
    }
    else
    {
      // now we need to set g_stSettings.m_fPixelRatio so that
      // fOutputFrameRatio = 4:3.
      g_stSettings.m_fPixelRatio = (4.0f / 3.0f) / fSourceFrameRatio;
    }
  }
  else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_14x9)
  { // stretch image to 14:9 ratio
    // now we need to set g_stSettings.m_fPixelRatio so that
    // fOutputFrameRatio = 14:9.
    g_stSettings.m_fPixelRatio = (14.0f / 9.0f) / fSourceFrameRatio;
    // calculate the desired output ratio
    float fOutputFrameRatio = fSourceFrameRatio * g_stSettings.m_fPixelRatio / g_settings.m_ResInfo[m_iResolution].fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full height.
    float fNewHeight = fScreenHeight;
    float fNewWidth = fNewHeight * fOutputFrameRatio;
    g_stSettings.m_fZoomAmount = fNewWidth / fScreenWidth;
    if (fNewWidth < fScreenWidth)
    { // zoom to full width
      fNewWidth = fScreenWidth;
      fNewHeight = fNewWidth / fOutputFrameRatio;
      g_stSettings.m_fZoomAmount = fNewHeight / fScreenHeight;
    }
  }
  else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_16x9)
  { // stretch image to 16:9 ratio
    g_stSettings.m_fZoomAmount = 1.0;
    if (m_iResolution == PAL_4x3 || m_iResolution == PAL60_4x3 || m_iResolution == NTSC_4x3 || m_iResolution == HDTV_480p_4x3)
    { // now we need to set g_stSettings.m_fPixelRatio so that
      // fOutputFrameRatio = 16:9.
      g_stSettings.m_fPixelRatio = (16.0f / 9.0f) / fSourceFrameRatio;
    }
    else
    { // stretch to the limits of the 16:9 screen.
      // incorrect behaviour, but it's what the users want, so...
      g_stSettings.m_fPixelRatio = (fScreenWidth / fScreenHeight) * g_settings.m_ResInfo[m_iResolution].fPixelRatio / fSourceFrameRatio;
    }
  }
  else // if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_ORIGINAL)
  { // zoom image so that the height is the original size
    g_stSettings.m_fPixelRatio = 1.0;
    // get the size of the media file
    // calculate the desired output ratio
    float fOutputFrameRatio = fSourceFrameRatio * g_stSettings.m_fPixelRatio / g_settings.m_ResInfo[m_iResolution].fPixelRatio;
    // now calculate the correct zoom amount.  First zoom to full width.
    float fNewWidth = fScreenWidth;
    float fNewHeight = fNewWidth / fOutputFrameRatio;
    if (fNewHeight > fScreenHeight)
    { // zoom to full height
      fNewHeight = fScreenHeight;
      fNewWidth = fNewHeight * fOutputFrameRatio;
    }
    // now work out the zoom amount so that no zoom is done
    g_stSettings.m_fZoomAmount = (m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropTop - g_stSettings.m_currentVideoSettings.m_CropBottom) / fNewHeight;
  }
}

void CLinuxRenderer::AutoCrop(bool bCrop)
{
// TODO: AutoCrop not implemented
}

void CLinuxRenderer::RenderLowMem(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

#ifndef HAS_SDL
  for (int i = 0; i < 3; ++i)
  {
    m_pD3DDevice->SetTexture(i, m_YUVTexture[index][FIELD_FULL][i]);
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  }

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_YV12VERTEX );
  m_pD3DDevice->SetPixelShader( m_hLowMemShader );

  //See RGB renderer for comment on this
  #define CHROMAOFFSET_HORIZ 0.25f

  // Render the image
  m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f); // fix texel align
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.top );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.top );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.bottom );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.bottom, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.bottom );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.bottom, 0, 1.0f );
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTexture(2, NULL);

  m_pD3DDevice->SetPixelShader( NULL );
  m_pD3DDevice->SetScissors(0, FALSE, NULL );

  //Okey, when the gpu is done with the textures here, they are free to be modified again
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone[index]);
#elif defined (USE_SDL_OVERLAY)

  SDL_Rect rect;
  rect.x = rs.left;
  rect.y = rs.top;
  rect.w = rs.right - rs.left;
  rect.h = rs.bottom - rs.top;

  int nRet = SDL_DisplayYUVOverlay(m_overlay, &rect);

#elif defined (HAS_SDL_OPENGL)
  g_graphicsContext.BeginPaint();

  CGLTexture *pTex = m_texture;
  if (pTex) {
     pTex->LoadToGPU();
     glBindTexture(GL_TEXTURE_2D, pTex->id);
     glEnable(GL_TEXTURE_2D);
     //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

     //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
     //glEnable(GL_BLEND);          // Turn Blending On
     glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

     //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
     //glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
     //glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
     //glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
     //glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
     //glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

     glBegin(GL_QUADS);

     float u1 = 0;
     float u2 = (float)pTex->imageWidth / pTex->textureWidth;
     float v1 = 0;
     float v2 = (float)pTex->imageHeight / pTex->textureHeight;

     glTexCoord2f( u1, v1 );
     glVertex2f( rs.left, rs.top );

     glTexCoord2f( u2, v1 );
     glVertex2f( rs.right, rs.top );

     glTexCoord2f( u2, v2 );
     glVertex2f( rs.right, rs.bottom );

     glTexCoord2f( u1, v2 );
     glVertex2f( rs.left, rs.bottom );

     glEnd();
  }

  g_graphicsContext.BeginPaint();
#else
  SDL_Rect rect;
  rect.x = rs.left;
  rect.y = rs.top;
  rect.w = rs.right - rs.left;
  rect.h = rs.bottom - rs.top; 
  g_graphicsContext.BlitToScreen(m_screenbuffer,NULL,&rect);
#endif

}

void CLinuxRenderer::CreateThumbnail(SDL_Surface * surface, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);
#ifndef USE_SDL_OVERLAY
  // we use backbuffer for the thumbnail cause screen buffer isnt used when opengl is used.
  if (m_backbuffer && surface) {
     SDL_BlitSurface(m_backbuffer, NULL, surface, NULL);
  }
#endif
}

// called from GUI thread after playback has finished to release resources
void CLinuxRenderer::OnClose()
{
}

#endif // HAS_SDL_OPENGL
