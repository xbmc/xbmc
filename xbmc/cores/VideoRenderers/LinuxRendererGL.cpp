/*
* XBoxMediaCenter
* Linux OpenGL Renderer
* Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
*
* Based on XBoxRenderer by Frodo/jcmarshall
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
#ifndef HAS_SDL_2D
#include "stdafx.h"
#include <locale.h>
#include "LinuxRendererGL.h"
#include "../../Application.h"
#include "../../Util.h"
#include "../../Settings.h"
#include "../../XBVideoConfig.h"
#include "../../../guilib/Surface.h"
#include "../../../guilib/FrameBufferObject.h"

#ifdef HAS_SDL_OPENGL

using namespace Surface;
using namespace Shaders;

CLinuxRendererGL::CLinuxRendererGL()
{
  m_pBuffer = NULL;
  m_textureTarget = GL_TEXTURE_2D;
  m_fSourceFrameRatio = 1.0f;
  m_iResolution = PAL_4x3;
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    m_pOSDYTexture[i] = 0;
    m_pOSDATexture[i] = 0;

    // possiblly not needed?
    //m_eventTexturesDone[i] = CreateEvent(NULL,FALSE,TRUE,NULL);
    //m_eventOSDDone[i] = CreateEvent(NULL,TRUE,TRUE,NULL);
  }
  m_shaderProgram = 0;
  m_fragmentShader = 0;
  m_renderMethod = RENDER_GLSL;
  m_renderQuality = RQ_MULTIPASS;
  m_yTex = 0;
  m_uTex = 0;
  m_vTex = 0;

  m_iYV12RenderBuffer = 0;
  m_pOSDYBuffer = NULL;
  m_pOSDABuffer = NULL;
  m_currentField = FIELD_FULL;
  m_reloadShaders = 0;
  m_pYUVShader = NULL;
  m_pVideoFilterShader = NULL;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;

  memset(m_image, 0, sizeof(m_image));
  memset(m_YUVTexture, 0, sizeof(m_YUVTexture));

  m_rgbBuffer = NULL;
  m_rgbBufferSize = 0;
}

CLinuxRendererGL::~CLinuxRendererGL()
{
  UnInit();
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    //CloseHandle(m_eventTexturesDone[i]);
    //CloseHandle(m_eventOSDDone[i]);
  }
  if (m_pBuffer)
  {
    delete m_pBuffer;
  }
  if (m_rgbBuffer != NULL) {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }
  if (m_pOSDYBuffer)
  {
    free(m_pOSDYBuffer);
    m_pOSDYBuffer = NULL;
  }
  if (m_pOSDABuffer)
  {
    free(m_pOSDABuffer);
    m_pOSDABuffer = NULL;
  }
}

//********************************************************************************************************
void CLinuxRendererGL::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_pOSDYTexture[index])
  {
    g_graphicsContext.BeginPaint();
    if (glIsTexture(m_pOSDYTexture[index]))
      glDeleteTextures(1, &m_pOSDYTexture[index]);
    g_graphicsContext.EndPaint();
    m_pOSDYTexture[index] = 0;
  }
  if (m_pOSDATexture[index])
  {
    g_graphicsContext.BeginPaint();
    if (glIsTexture(m_pOSDATexture[index]))
      glDeleteTextures(1, &m_pOSDATexture[index]);
    g_graphicsContext.EndPaint();
    m_pOSDATexture[index] = 0;
    CLog::Log(LOGDEBUG, "Deleted OSD textures (%i)", index);
  }
  if (m_pOSDYBuffer)
  {
    free(m_pOSDYBuffer);
    m_pOSDYBuffer = NULL;
  }
  if (m_pOSDABuffer)
  {
    free(m_pOSDABuffer);
    m_pOSDABuffer = NULL;
  }
  m_iOSDTextureHeight[index] = 0;
}

void CLinuxRendererGL::Setup_Y8A8Render()
{

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
void CLinuxRendererGL::CalculateFrameAspectRatio(int desired_width, int desired_height)
{
  m_fSourceFrameRatio = (float)desired_width / desired_height;

  // Check whether mplayer has decided that the size of the video file should be changed
  // This indicates either a scaling has taken place (which we didn't ask for) or it has
  // found an aspect ratio parameter from the file, and is changing the frame size based
  // on that.
  if (m_iSourceWidth == desired_width && m_iSourceHeight == desired_height)
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
void CLinuxRendererGL::CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride)
{
  // DISABLED !!
  // As it is only used by mplayer
  return;

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

void CLinuxRendererGL::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
  // DISABLED !!
  // As it is only used by mplayer
  return;

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

  RECT rc = { 0, 0, w, h };

  int iOSDBuffer = (m_iOSDRenderBuffer + 1) % m_NumOSDBuffers;

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
    glGenTextures(1, &m_pOSDYTexture[iOSDBuffer]);
    glGenTextures(1, &m_pOSDATexture[iOSDBuffer]);  
    VerifyGLState();

    if (!m_pOSDYBuffer)
    {
      m_pOSDYBuffer = (GLubyte*)malloc(m_iOSDTextureWidth * m_iOSDTextureHeight[iOSDBuffer]);
    }
    if (!m_pOSDABuffer)
    {
      m_pOSDABuffer = (GLubyte*)malloc(m_iOSDTextureWidth * m_iOSDTextureHeight[iOSDBuffer]);
    }
    
    if (!(m_pOSDYTexture[iOSDBuffer] && m_pOSDATexture[iOSDBuffer] && m_pOSDYBuffer && m_pOSDABuffer)) 
      /*      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_L8, 0, &m_pOSDYTexture[iOSDBuffer]) ||
              D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_A8, 0, &m_pOSDATexture[iOSDBuffer])*/
    {
      CLog::Log(LOGERROR, "Could not create OSD/Sub textures");
      DeleteOSDTextures(iOSDBuffer);
      return;
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, m_pOSDYTexture[iOSDBuffer]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, NP2(m_iOSDTextureWidth), m_iOSDTextureHeight[iOSDBuffer], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
      glBindTexture(GL_TEXTURE_2D, m_pOSDATexture[iOSDBuffer]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, NP2(m_iOSDTextureWidth), m_iOSDTextureHeight[iOSDBuffer], 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
      CLog::Log(LOGDEBUG, "Created OSD textures (%i)", iOSDBuffer);
    }
    SetEvent(m_eventOSDDone[iOSDBuffer]);
  }

  //Don't do anything here that would require locking of grapichcontext
  //it shouldn't be needed, and locking here will slow down prepared rendering
  if( WaitForSingleObject(m_eventOSDDone[iOSDBuffer], 500) == WAIT_TIMEOUT )
  {
    //This should only happen if flippage wasn't called
    SetEvent(m_eventOSDDone[iOSDBuffer]);
  }

  //We know the resources have been used at this point (or they are the second buffer, wich means they aren't in use anyways)
  //reset these so the gpu doesn't try to block on these
  
  memset(m_pOSDYBuffer, 0, m_iOSDTextureWidth * m_iOSDTextureHeight[iOSDBuffer]);
  memset(m_pOSDABuffer, 0, m_iOSDTextureWidth * m_iOSDTextureHeight[iOSDBuffer]);
  CopyAlpha(w, h, src, srca, stride, m_pOSDYBuffer, m_pOSDABuffer, m_iOSDTextureWidth);
  glBindTexture(GL_TEXTURE_2D, m_pOSDYTexture[iOSDBuffer]);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pOSDYBuffer);
  glBindTexture(GL_TEXTURE_2D, m_pOSDATexture[iOSDBuffer]);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pOSDABuffer);

  //set module variables to calculated values
  m_OSDRect = osdRect;
  m_OSDWidth = (float)w;
  m_OSDHeight = (float)h;
  m_OSDRendered = true;
}

//********************************************************************************************************
void CLinuxRendererGL::RenderOSD()
{
  // DISABLED !!
  // As it is only used by mplayer
  return;

  int iRenderBuffer = m_iOSDRenderBuffer;
  
  if (!m_pOSDYTexture[iRenderBuffer] || !m_pOSDATexture[iRenderBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;
  if (!glIsTexture(m_pOSDYTexture[m_iOSDRenderBuffer]))
    return;

  ResetEvent(m_eventOSDDone[iRenderBuffer]);
  
  CSingleLock lock(g_graphicsContext);

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;
  DRAWRECT osdRect = m_OSDRect;
  //  if (!viewportRect.bottom && !viewportRect.right)
  //    return;

  // Set state to render the image

  Setup_Y8A8Render();

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // Render the image
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_pOSDYTexture[m_iOSDRenderBuffer]);
  glBegin(GL_QUADS);
  glColor3f(1.0, 1.0, 1.0);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(osdRect.left, osdRect.top);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(osdRect.right, osdRect.top);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(osdRect.right, osdRect.bottom);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(osdRect.left, osdRect.bottom);
  glEnd();  

}

//********************************************************************************************************
//Get resolution based on current mode.
RESOLUTION CLinuxRendererGL::GetResolution()
{
  if (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
  {
    return m_iResolution;
  }
  return g_graphicsContext.GetVideoResolution();
}

float CLinuxRendererGL::GetAspectRatio()
{
  float fWidth = (float)m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropLeft - g_stSettings.m_currentVideoSettings.m_CropRight;
  float fHeight = (float)m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropTop - g_stSettings.m_currentVideoSettings.m_CropBottom;
  return m_fSourceFrameRatio * fWidth / fHeight * m_iSourceHeight / m_iSourceWidth;
}

void CLinuxRendererGL::GetVideoRect(RECT &rectSrc, RECT &rectDest)
{
  rectSrc = rs;
  rectDest = rd;
}

void CLinuxRendererGL::CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fInputFrameRatio, float fZoomAmount)
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


void CLinuxRendererGL::ManageTextures()
{
  int neededbuffers = 0;
  m_NumYV12Buffers = 1;
  m_NumOSDBuffers = 1;
  m_iYV12RenderBuffer = 0;
  m_iOSDRenderBuffer = 0;
  return;
}

void CLinuxRendererGL::ManageDisplay()
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

void CLinuxRendererGL::ChooseBestResolution(float fps)
{
  bool bUsingPAL = g_videoConfig.HasPAL();    // current video standard:PAL or NTSC
  bool bCanDoWidescreen = g_videoConfig.HasWidescreen(); // can widescreen be enabled?
  bool bWideScreenMode = false;

  // If the resolution selection is on Auto the following rules apply :
  //
  // BIOS Settings     ||Display resolution
  // WS|480p|720p/1080i||4:3 Videos     |16:6 Videos
  // ------------------||------------------------------
  // - | X  |    X     || 480p 4:3      | 720p
  // - | X  |    -     || 480p 4:3      | 480p 4:3
  // - | -  |    X     || 720p          | 720p
  // - | -  |    -     || NTSC/PAL 4:3  |NTSC/PAL 4:3
  // X | X  |    X     || 720p          | 720p
  // X | X  |    -     || 480p 4:3      | 480p 16:9
  // X | -  |    X     || 720p          | 720p
  // X | -  |    -     || NTSC/PAL 4:3  |NTSC/PAL 16:9

  // Work out if the framerate suits PAL50 or PAL60
  bool bPal60 = false;
  if (bUsingPAL && g_guiSettings.GetInt("videoplayer.framerateconversions") == FRAME_RATE_USE_PAL60 && g_videoConfig.HasPAL60())
  {
    // yes we're in PAL
    // yes PAL60 is allowed
    // yes dashboard PAL60 settings is enabled
    // Calculate the framerate difference from a divisor of 120fps and 100fps
    // (twice 60fps and 50fps to allow for 2:3 IVTC pulldown)
    float fFrameDifference60 = fabs(120.0f / fps - floor(120.0f / fps + 0.5f));
    float fFrameDifference50 = fabs(100.0f / fps - floor(100.0f / fps + 0.5f));

    // Make a decision based on the framerate difference
    if (fFrameDifference60 < fFrameDifference50)
      bPal60 = true;
  }

  // If the display resolution was specified by the user then use it, unless
  // it's a PAL setting, whereby we use the above setting to autoswitch to PAL60
  // if appropriate
  RESOLUTION DisplayRes = (RESOLUTION) g_guiSettings.GetInt("videoplayer.displayresolution");
  if ( DisplayRes != AUTORES )
  {
    if (bPal60)
    {
      if (DisplayRes == PAL_16x9) DisplayRes = PAL60_16x9;
      if (DisplayRes == PAL_4x3) DisplayRes = PAL60_4x3;
    }
    CLog::Log(LOGNOTICE, "Display resolution USER : %s (%d)", g_settings.m_ResInfo[DisplayRes].strMode, DisplayRes);
    m_iResolution = DisplayRes;
    return;
  }

  // Work out if framesize suits 4:3 or 16:9
  // Uses the frame aspect ratio of 8/(3*sqrt(3)) (=1.53960) which is the optimal point
  // where the percentage of black bars to screen area in 4:3 and 16:9 is equal
  static const float fOptimalSwitchPoint = 8.0f / (3.0f*sqrt(3.0f));
  if (bCanDoWidescreen && m_fSourceFrameRatio > fOptimalSwitchPoint)
    bWideScreenMode = true;

  // We are allowed to switch video resolutions, so we must
  // now decide which is the best resolution for the video we have
  if (bUsingPAL)  // PAL resolutions
  {
    // Currently does not allow HDTV solutions, as it is my beleif
    // that the XBox hardware only allows HDTV resolutions for NTSC systems.
    // this may need revising as more knowledge is obtained.
    if (bPal60)
    {
      if (bWideScreenMode)
        m_iResolution = PAL60_16x9;
      else
        m_iResolution = PAL60_4x3;
    }
    else    // PAL50
    {
      if (bWideScreenMode)
        m_iResolution = PAL_16x9;
      else
        m_iResolution = PAL_4x3;
    }
  }
  else      // NTSC resolutions
  {
    if (bCanDoWidescreen)
    { // The TV set has a wide screen (16:9)
      // So we always choose the best HD widescreen resolution no matter what
      // the video aspect ratio is
      // If the TV has no HD support widescreen mode is chossen according to video AR

      if (g_videoConfig.Has1080i())     // Widescreen TV with 1080i res
      m_iResolution = HDTV_1080i;
      else if (g_videoConfig.Has720p()) // Widescreen TV with 720p res
      m_iResolution = HDTV_720p;
      else if (g_videoConfig.Has480p()) // Widescreen TV with 480p
      {
        if (bWideScreenMode) // Choose widescreen mode according to video AR
          m_iResolution = HDTV_480p_16x9;
        else
          m_iResolution = HDTV_480p_4x3;
    }
      else if (bWideScreenMode)         // Standard 16:9 TV set with no HD
        m_iResolution = NTSC_16x9;
      else
        m_iResolution = NTSC_4x3;
    }
    else
    { // The TV set has a 4:3 aspect ratio
      // So 4:3 video sources will best fit the screen with 4:3 resolution
      // We choose 16:9 resolution only for 16:9 video sources

      if (m_fSourceFrameRatio >= 16.0f / 9.0f)
    {
        // The video fits best into widescreen modes so they are
        // the first choices
        if (g_videoConfig.Has1080i())
          m_iResolution = HDTV_1080i;
        else if (g_videoConfig.Has720p())
          m_iResolution = HDTV_720p;
        else if (g_videoConfig.Has480p())
          m_iResolution = HDTV_480p_4x3;
        else
          m_iResolution = NTSC_4x3;
      }
      else
      {
        // The video fits best into 4:3 modes so 480p
        // is the first choice
        if (g_videoConfig.Has480p())
          m_iResolution = HDTV_480p_4x3;
        else if (g_videoConfig.Has1080i())
          m_iResolution = HDTV_1080i;
        else if (g_videoConfig.Has720p())
          m_iResolution = HDTV_720p;
        else
          m_iResolution = NTSC_4x3;
      }
    }
  }

  CLog::Log(LOGNOTICE, "Display resolution AUTO : %s (%d)", g_settings.m_ResInfo[m_iResolution].strMode, m_iResolution);
}

bool CLinuxRendererGL::ValidateRenderTarget()
{
  if (!m_pBuffer)
  {
    // try pbuffer first
    //m_pBuffer = new CSurface(256, 256, false, g_graphicsContext.getScreenSurface(), NULL, NULL, false, false, true);
    m_pBuffer = new CSurface(256, 256, false, g_graphicsContext.getScreenSurface(), g_graphicsContext.getScreenSurface(), NULL, false, false, false);
    if (m_pBuffer && !m_pBuffer->IsValid())
    {
      delete m_pBuffer;
      m_pBuffer = new CSurface(256, 256, false, g_graphicsContext.getScreenSurface(), g_graphicsContext.getScreenSurface(), NULL, false, false, false);
      if (m_pBuffer->IsValid())
        CLog::Log(LOGNOTICE, "GL: Created non-pbuffer OpenGL context");
    }
  }
  if (m_pBuffer && m_pBuffer->IsValid())
  {
    int maj, min;
    m_pBuffer->GetGLVersion(maj, min);
    if (maj<2 || !glewIsSupported("GL_ARB_texture_non_power_of_two"))
    {
      CLog::Log(LOGINFO, "GL: OpenGL version %d.%d detected", maj, min);
      if (!glewIsSupported("GL_ARB_texture_rectangle"))
      {
        CLog::Log(LOGERROR, "GL: GL_ARB_texture_rectangle not supported and OpenGL version is not 2.x");
        CLog::Log(LOGERROR, "GL: Reverting to POT textures");
        m_renderMethod |= RENDER_POT;
        return true;
      }
      CLog::Log(LOGINFO, "GL: NPOT textures are supported through GL_ARB_texture_rectangle extension");
      m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
      glEnable(GL_TEXTURE_RECTANGLE_ARB);
    } else {
      CLog::Log(LOGINFO, "GL: OpenGL version %d.%d detected", maj, min);
      CLog::Log(LOGINFO, "GL: NPOT textures are supported natively");
    }
  }
  else 
  {
    CLog::Log(LOGERROR, "GL: Could not create OpenGL context that is required for video playback");
    return false;
  }
  return true;
}

bool CLinuxRendererGL::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_fps = fps;
  m_iSourceWidth = width;
  m_iSourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(m_fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  // make sure we have a valid context that supports rendering
  if (!ValidateRenderTarget())
    return false;

  CreateYV12Texture(0);

  if (m_rgbBuffer != NULL) {
     delete [] m_rgbBuffer;
     m_rgbBuffer = NULL;
  }
 
  m_rgbBufferSize = width*height*4;
  m_rgbBuffer = new BYTE[m_rgbBufferSize];

  return true;
}

int CLinuxRendererGL::NextYV12Texture()
{
  return 0; //(m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CLinuxRendererGL::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;

  //CSingleLock lock(g_graphicsContext);
   
  source = 0;

  if (!m_image[source].plane[0]) 
  {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - image planes not allocated");
     return -1;
  }

  if (m_image[source].flags != 0) 
  {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - request image but none to give");
     return -1;
  }

  m_image[source].flags = readonly?IMAGE_FLAG_READING:IMAGE_FLAG_WRITING;

  // copy the image - should be operator of YV12Image
  for (int p=0;p<MAX_PLANES;p++) 
  {
     image->plane[p]=m_image[source].plane[p];
     image->stride[p] = m_image[source].stride[p];
  }
  image->width = m_image[source].width;
  image->height = m_image[source].height;
  image->flags = m_image[source].flags;
  image->cshift_x = m_image[source].cshift_x;
  image->cshift_y = m_image[source].cshift_y;
  image->texcoord_x = m_image[source].texcoord_x;
  image->texcoord_y = m_image[source].texcoord_y;

  return 0;
}

void CLinuxRendererGL::ReleaseImage(int source, bool preserve)
{ 
  // Eventual FIXME
  if (source!=0)
    source=0;

  m_image[source].flags = 0;

  YV12Image &im = m_image[source];
  YUVFIELDS &fields = m_YUVTexture[source];
  
  m_image[source].flags &= ~IMAGE_FLAG_INUSE;
  m_image[source].flags = 0;

  // if we don't have a shader, fallback to SW YUV2RGB for now
  
  g_graphicsContext.BeginPaint(m_pBuffer);

  if (m_renderMethod & RENDER_SW)
  {
    struct SwsContext *context = m_dllSwScale.sws_getContext(im.width, im.height, PIX_FMT_YUV420P, im.width, im.height, PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
    uint8_t *src[] = { im.plane[0], im.plane[1], im.plane[2] };
    int     srcStride[] = { im.stride[0], im.stride[1], im.stride[2] };
    uint8_t *dst[] = { m_rgbBuffer, 0, 0 };
    int     dstStride[] = { m_iSourceWidth*4, 0, 0 };
    int ret = m_dllSwScale.sws_scale(context, src, srcStride, 0, im.height, dst, dstStride);
    
    m_dllSwScale.sws_freeContext(context);
  }   

  static int imaging = -1;
  static GLfloat brightness = 0;
  static GLfloat contrast   = 0;

  brightness =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Brightness - 50.0)/100.0;
  contrast =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Contrast)/50.0;

  if (m_renderMethod & RENDER_SW) 
  {
    if (imaging==-1)
    {
      imaging = 0;
      if (glewIsSupported("GL_ARB_imaging"))
      {
        CLog::Log(LOGINFO, "GL: ARB Imaging extension supported");
        imaging = 1;
      }
      else 
      {
        int maj=0, min=0;
        g_graphicsContext.getScreenSurface()->GetGLVersion(maj, min);
        if (maj>=2)
        {
          imaging = 1;
        } else if (min>=2) {
          imaging = 1;
        }
      }
    }
    if (imaging)
    {
      glPixelTransferf(GL_RED_SCALE, contrast);
      glPixelTransferf(GL_GREEN_SCALE, contrast);
      glPixelTransferf(GL_BLUE_SCALE, contrast);
      glPixelTransferf(GL_RED_BIAS, brightness);
      glPixelTransferf(GL_GREEN_BIAS, brightness);
      glPixelTransferf(GL_BLUE_BIAS, brightness);
      VerifyGLState();
    }
  }

  glEnable(GL_TEXTURE_2D);
  VerifyGLState();
  glBindTexture(m_textureTarget, fields[0][0]);
  VerifyGLState();
  if (m_renderMethod & RENDER_SW)
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width, im.height, GL_BGRA, GL_UNSIGNED_BYTE, m_rgbBuffer);
  else
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width, im.height, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[0]);

  VerifyGLState();
  if (m_renderMethod & RENDER_GLSL)
  {    
    glBindTexture(m_textureTarget, fields[0][1]);
    VerifyGLState();
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width/2, im.height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[1]);
    VerifyGLState();
    glBindTexture(m_textureTarget, fields[0][2]);
    VerifyGLState();
    glTexSubImage2D(m_textureTarget, 0, 0, 0, im.width/2, im.height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, im.plane[2]);
    VerifyGLState();
  }
  
  if (imaging)
  {
    glPixelTransferf(GL_RED_SCALE, 1.0);
    glPixelTransferf(GL_GREEN_SCALE, 1.0);
    glPixelTransferf(GL_BLUE_SCALE, 1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    glPixelTransferf(GL_GREEN_BIAS, 0.0);
    glPixelTransferf(GL_BLUE_BIAS, 0.0);
    VerifyGLState();
  }
  g_graphicsContext.EndPaint(m_pBuffer);
}

void CLinuxRendererGL::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_image[i].flags = 0;
    /* reset texure locks, abit uggly, could result in tearing */
    //SetEvent(m_eventTexturesDone[i]); 
  }
}

void CLinuxRendererGL::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  //CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();
}

void CLinuxRendererGL::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  //if (!m_YUVTexture[m_iYV12RenderBuffer][FIELD_FULL][0]) return ;
  if (!m_YUVTexture[0][FIELD_FULL][0]) return ;

  //CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();

  g_graphicsContext.BeginPaint();

  if (clear) 
  {
    glClearColor(m_clearColour&0xff000000,
                 m_clearColour&0x00ff0000,
                 m_clearColour&0x0000ff00,
                 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0,0,0,0);
    if (alpha<255) 
    {
#ifdef _LINUX
#warning Alpha blending currently disabled
#endif
      //glDisable(GL_BLEND);
    } else {
      //glDisable(GL_BLEND);
    }
  }
  glDisable(GL_BLEND);
  Render(flags);
  VerifyGLState();
  glEnable(GL_BLEND);
  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::FlipPage(int source)
{  
  CLog::Log(LOGNOTICE, "Calling FlipPage");
  //if( source >= 0 && source < m_NumYV12Buffers )
  m_iYV12RenderBuffer = source;
  //else
  //m_iYV12RenderBuffer = NextYV12Texture();
  
  /* we always decode into to the next buffer */
  //++m_iOSDRenderBuffer %= m_NumOSDBuffers;
  
  /* if osd wasn't rendered this time around, previuse should not be */
  /* displayed on next frame */
  
  if( !m_OSDRendered )
    m_OSDWidth = m_OSDHeight = 0;
  
  m_OSDRendered = false;
  
  g_graphicsContext.BeginPaint();
  g_graphicsContext.Flip();
  g_graphicsContext.EndPaint();

  return;
}


unsigned int CLinuxRendererGL::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;
  
  int index = NextYV12Texture();
  if( index < 0 )
    return -1;

  YV12Image &im = m_image[index];
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

unsigned int CLinuxRendererGL::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_iResolution = PAL_4x3;

  m_iOSDRenderBuffer = 0;
  m_iYV12RenderBuffer = 0;
  m_NumOSDBuffers = 1;
  m_NumYV12Buffers = 1;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = 0 ; //(g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  // make sure we have a valid context that supports rendering
  if (!ValidateRenderTarget())
    return false;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load()) 
        CLog::Log(LOGERROR,"CLinuxRendererGL::PreInit - failed to load rescale libraries!");

  m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  LoadShaders();
  return true;
}

void CLinuxRendererGL::UpdateVideoFilter()
{
  if (m_scalingMethod == g_stSettings.m_currentVideoSettings.m_ScalingMethod)
    return;

  m_scalingMethod = g_stSettings.m_currentVideoSettings.m_ScalingMethod;
  switch (g_stSettings.m_currentVideoSettings.m_ScalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  case VS_SCALINGMETHOD_LINEAR:
    m_renderQuality = RQ_SINGLEPASS;
    //m_renderQuality = RQ_MULTIPASS;
    if (m_pVideoFilterShader)
    {
      m_pVideoFilterShader->Free();
      delete m_pVideoFilterShader;
      m_pVideoFilterShader = NULL;
    }
    break;
        
  case VS_SCALINGMETHOD_CUBIC:
    VerifyGLState();
    m_renderQuality = RQ_MULTIPASS;
    if (m_pVideoFilterShader)
    {
      m_pVideoFilterShader->Free();
      delete m_pVideoFilterShader;
      m_pVideoFilterShader = NULL;
    }
    VerifyGLState();
    m_pVideoFilterShader = new BicubicFilterShader(0.3, 0.3);
    if (m_pVideoFilterShader && m_pVideoFilterShader->CompileAndLink())
    {
      VerifyGLState();
      if (!m_pVideoFilterShader->CompileAndLink())
      {
        CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
        m_pVideoFilterShader->Free();
        delete m_pVideoFilterShader;
        m_pVideoFilterShader = NULL;
      }
    }
    break;

  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_SINC8:
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "GL: TODO: This scaler has not yet been implemented");
    m_renderQuality = RQ_SINGLEPASS;
    //m_renderQuality = RQ_MULTIPASS;
    if (m_pVideoFilterShader)
    {
      m_pVideoFilterShader->Free();
      delete m_pVideoFilterShader;
      m_pVideoFilterShader = NULL;
    }
    break;
  }
}

void CLinuxRendererGL::LoadShaders(int renderMethod)
{
  bool err = false;  
  if (glCreateProgram)
  {
    g_graphicsContext.BeginPaint(m_pBuffer);
    if (m_pYUVShader)
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }
    if (renderMethod & (FIELD_ODD|FIELD_EVEN))
      m_pYUVShader = new YUV2RGBBobShader(); // create bob deinterlacing shader
    else
      m_pYUVShader = new YUV2RGBProgressiveShader(); // create regular progressive scan shader

    if (m_pYUVShader && m_pYUVShader->CompileAndLink())
    {
      m_renderMethod = RENDER_GLSL;
      UpdateVideoFilter();
    }
    else
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
      err = true;
      CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
    }
    g_graphicsContext.EndPaint(m_pBuffer);
  }
  else if (err && glewIsSupported("GL_ARB_fragment_shader")) 
  {    
    // TODO
    CLog::Log(LOGNOTICE, "GL: ARB shaders support detected but unimplementated at this time, falling back to software YUV2RGB");
    m_renderMethod = RENDER_SW ;
  } 
  else 
  {
    m_renderMethod = RENDER_SW ;
    CLog::Log(LOGNOTICE, "GL: Shaders support not present, falling back to SW mode");
  }
  return;

  bool error = false;
  if (!m_shaderProgram && glCreateProgram)
  {

    const char* shaderv = 
      "void main()"
      "{"
      "gl_TexCoord[0] = gl_MultiTexCoord0;"
      "gl_TexCoord[1] = gl_MultiTexCoord1;"
      "gl_TexCoord[2] = gl_MultiTexCoord2;"
      "gl_Position = ftransform();"
      "}";

    string shaderf;

    if (renderMethod == FIELD_FULL)
    {
      shaderf = 
        "uniform sampler2D ytex;"
        "uniform sampler2D utex;"
        "uniform sampler2D vtex;"
        "uniform float brightness;"
        "uniform float contrast;"
        "void main()"
        "{"
        "vec4 yuv, rgb;"
        "mat4 yuvmat = { 1.0,      1.0,     1.0,     0.0, "
        "                0,       -0.39465, 2.03211, 0.0, "
        "                1.13983, -0.58060, 0.0,     0.0, "
        "                0.0,     0.0,      0.0,     0.0 }; "
        "yuv.rgba = vec4(texture2D(ytex, gl_TexCoord[0].xy).r,"
        "                0.436 * (texture2D(utex, gl_TexCoord[1].xy).r * 2.0 - 1.0),"
        "                0.615 * (texture2D(vtex, gl_TexCoord[2].xy).r * 2.0 - 1.0),"
        "                0.0);"
        "rgb = yuvmat * yuv;"
        "rgb = (rgb-vec4(0.5))*vec4(contrast) + vec4(0.5) + vec4(brightness) ;"
        "rgb.a = 1.0;"
        "gl_FragColor = rgb;"
        "}";
    }
    else if (renderMethod == FIELD_ODD || renderMethod == FIELD_EVEN)
    {
/*
      shaderf = 
        "uniform sampler2D ytex;"
        "uniform sampler2D utex;"
        "uniform sampler2D vtex;"
        "uniform float brightness;"
        "uniform float contrast;"
        "uniform float stepX, stepY;"
        "uniform int field;"
        "void main()"
        "{"
        "vec4 yuv, rgb, yuv1, yuv2;"
        "vec2 offsetY, offsetUV, offsetY1, offsetY2, offsetUV1, offsetUV2;"
        "float temp1 = mod(gl_TexCoord[0].y, 2*stepY);"

        "offsetY  = gl_TexCoord[0].xy ;"
        "offsetUV = gl_TexCoord[1].xy ;"
        "offsetY.y  -= (temp1 - stepY/2 + (float)field*stepY);"
        "offsetUV.y -= (temp1 - stepY/2 + (float)field*stepY)/2;"

        "offsetY1 = offsetY2 = offsetY;"
        "offsetUV1 = offsetUV2 = offsetUV;"
        "offsetY1.y -= stepY*2;"
        "offsetY2.y += stepY*2;"
        "offsetUV1.y -= stepY;"
        "offsetUV2.y += stepY;"
        "mat4 yuvmat = { 1.0,      1.0,     1.0,     0.0, "
        "                0,       -0.39465, 2.03211, 0.0, "
        "                1.13983, -0.58060, 0.0,     0.0, "
        "                0.0,     0.0,      0.0,     0.0 }; "
        "yuv1 = vec4(texture2D(ytex, offsetY1).r,"
        "            texture2D(utex, offsetUV1).r,"
        "            texture2D(vtex, offsetUV1).r,"
        "            0.0);"

        "yuv2 = vec4(texture2D(ytex, offsetY2).r,"
        "            texture2D(utex, offsetUV2).r,"
        "            texture2D(vtex, offsetUV2).r,"
        "            0.0);"

        "yuv = vec4(texture2D(ytex, offsetY).r,"
        "           texture2D(utex, offsetUV).r,"
        "           texture2D(vtex, offsetUV).r,"
        "           0.0);"
        "yuv.rgba = mix(yuv, yuv2, temp1/(stepY*2));"
        "yuv = vec4(yuv.r, 0.436 * (yuv.g * 2.0 - 1.0), "
        "           0.615 * (yuv.b * 2.0 - 1.0), 0);"        
        "rgb = yuvmat * yuv;"
        "rgb = (rgb-vec4(0.5))*vec4(contrast) + vec4(0.5) + vec4(brightness) ;"
        "rgb.a = 1.0;"
        "gl_FragColor = rgb;"
        "}";
*/

      shaderf = 
        "uniform sampler2D ytex;"
        "uniform sampler2D utex;"
        "uniform sampler2D vtex;"
        "uniform float brightness;"
        "uniform float contrast;"
        "uniform float stepX, stepY;"
        "uniform int field;"
        "void main()"
        "{"
        "vec4 yuv, rgb, yuv2;"
        "vec4 taps[4][4];"
        "vec2 offsetY, offsetUV, offsetY2, offsetUV2;"
        "float temp1 = mod(gl_TexCoord[0].y, 2*stepY);"

        "offsetY  = gl_TexCoord[0].xy ;"
        "offsetUV = gl_TexCoord[1].xy ;"
        "offsetY.y  -= (temp1 - stepY/2 + float(field)*stepY);"
        "offsetUV.y -= (temp1 - stepY/2 + float(field)*stepY)/2;"
        "mat4 yuvmat = { 1.0,      1.0,     1.0,     0.0, "
        "                0,       -0.39465, 2.03211, 0.0, "
        "                1.13983, -0.58060, 0.0,     0.0, "
        "                0.0,     0.0,      0.0,     0.0 }; "
        "yuv = vec4(texture2D(ytex, offsetY).r,"
        "           texture2D(utex, offsetUV).r,"
        "           texture2D(vtex, offsetUV).r,"
        "           0.0);"
        "yuv.gba = vec3(0.436 * (yuv.g * 2.0 - 1.0),"
        "           0.615 * (yuv.b * 2.0 - 1.0), 0);"        
        "rgb = yuvmat * yuv;"
        "rgb = (rgb-vec4(0.5))*vec4(contrast) + vec4(0.5) + vec4(brightness);"
        "rgb.a = 1.0;"
        "gl_FragColor = rgb;"
        "}";
    }

    const char* shaderfrect = 
      "uniform sampler2DRect ytex;"
      "uniform sampler2DRect utex;"
      "uniform sampler2DRect vtex;"
      "void main()"
      "{"
      "float y = texture2DRect(ytex, gl_TexCoord[0].xy).r ;"
      "float u = texture2DRect(utex, gl_TexCoord[1].xy).r ;"
      "float v = texture2DRect(vtex, gl_TexCoord[2].xy).r ;"
      "y = 1.1643*(y-0.0625);"
      "u = u - 0.5;"
      "v = v - 0.5;"
      "float r = clamp(y+1.5958*v, 0.0, 1.0);"
      "float g = clamp(y-0.39173*u-0.81290*v, 0.0, 1.0);"
      "float b = clamp(y+2.017*u, 0.0, 1.0);"
      "gl_FragColor = vec4(r, g, b, 1.0);"
      "}";

    GLint params[4]; 
    
    /* 
       Workaround for locale bug in nVidia's shader compiler.
       Save the current locale, set to a neutral while compiling and switch back afterwards.
    */
    char * currentLocale = setlocale(LC_NUMERIC, NULL);
    setlocale(LC_NUMERIC, "C");

    g_graphicsContext.BeginPaint(m_pBuffer);
    m_shaderProgram = glCreateProgram();
    m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (m_textureTarget==GL_TEXTURE_2D)
    {
      const char *ptr = shaderf.c_str();
      glShaderSource(m_fragmentShader, 1, &ptr, 0);
    } 
    else 
    {
      glShaderSource(m_fragmentShader, 1, &shaderfrect, 0);
    }
    glShaderSource(m_vertexShader, 1, &shaderv, 0);
    glCompileShader(m_fragmentShader);
    glCompileShader(m_vertexShader);
    glGetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, params);
    if (params[0]!=GL_TRUE) 
    {
      error = true;
      GLchar log[512];
      CLog::Log(LOGERROR, "GL: Error compiling shader");
      glGetShaderInfoLog(m_fragmentShader, 512, NULL, log);
      CLog::Log(LOGERROR, (const char*)log);
    }
    glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
    if (params[0]!=GL_TRUE) 
    {
      error = true;
      GLchar log[512];
      CLog::Log(LOGERROR, "Error compiling shader");
      glGetShaderInfoLog(m_vertexShader, 512, NULL, log);
      CLog::Log(LOGERROR, (const char*)log);
    }
    glAttachShader(m_shaderProgram, m_fragmentShader);
    glAttachShader(m_shaderProgram, m_vertexShader);
    VerifyGLState();
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, params);
    if (params[0]!=GL_TRUE) 
    {
      error = true;
      GLchar log[512];
      CLog::Log(LOGERROR, "Error linking shader");
      glGetProgramInfoLog(m_shaderProgram, 512, NULL, log);
      CLog::Log(LOGERROR, (const char*)log);
    }
    glValidateProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, params);
    if (params[0]!=GL_TRUE) 
    {
      error = true;
      GLchar log[512];
      CLog::Log(LOGERROR, "Error validating shader");
      glGetProgramInfoLog(m_shaderProgram, 512, NULL, log);
      CLog::Log(LOGERROR, (const char*)log);
    }
    m_yTex = glGetUniformLocation(m_shaderProgram, "ytex");
    VerifyGLState();
    m_uTex = glGetUniformLocation(m_shaderProgram, "utex");
    VerifyGLState();
    m_vTex = glGetUniformLocation(m_shaderProgram, "vtex");
    VerifyGLState();
    m_brightness = glGetUniformLocation(m_shaderProgram, "brightness");
    VerifyGLState();
    m_contrast = glGetUniformLocation(m_shaderProgram, "contrast");
    VerifyGLState();

    if ((renderMethod == FIELD_ODD) || (renderMethod == FIELD_EVEN))
    {
      m_stepX = glGetUniformLocation(m_shaderProgram, "stepX");
      m_stepY = glGetUniformLocation(m_shaderProgram, "stepY");
      m_shaderField = glGetUniformLocation(m_shaderProgram, "field");
      VerifyGLState();
    }

    g_graphicsContext.EndPaint(m_pBuffer);
    if (error)
    {
      CLog::Log(LOGNOTICE, "GL: Error loading loading GLSL shader, falling back to software rendering");
      m_renderMethod = RENDER_SW;
    }
    else
    {
      CLog::Log(LOGNOTICE, "GL: Successfully loaded GLSL shader");
    }
    m_renderMethod = RENDER_GLSL;    

    /* 
       Workaround for locale bug in nVidia's shader compiler.
       Revert to original locale
    */
    setlocale(LC_NUMERIC, currentLocale);
  } 
  else if (glewIsSupported("GL_ARB_fragment_shader")) 
  {    
    // TODO
    CLog::Log(LOGNOTICE, "GL: ARB shaders are supported but unimplementated at this time");
    m_renderMethod = RENDER_SW ;
  } 
  else 
  {
    m_renderMethod = RENDER_SW ;
    CLog::Log(LOGNOTICE, "GL: Shaders support not present, falling back to SW mode");
  }
}

void CLinuxRendererGL::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_fbo.Cleanup();

  // YV12 textures, subtitle and osd stuff
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteYV12Texture(i);
    DeleteOSDTextures(i);
  }
  
  if (m_shaderProgram)
  {
    glDeleteShader(m_vertexShader);
    VerifyGLState();
    glDeleteShader(m_fragmentShader);
    VerifyGLState();
    glDeleteProgram(m_shaderProgram);
    VerifyGLState();
    m_fragmentShader = 0;
    m_vertexShader = 0;
    m_shaderProgram = 0;
    m_yTex = 0;
    m_uTex = 0;
    m_vTex = 0;
  }
  if (m_pBuffer)
  {
    delete m_pBuffer;
    m_pBuffer = 0;
  } 

  if (m_rgbBuffer != NULL) 
  { 
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }

}

void CLinuxRendererGL::Render(DWORD flags)
{

  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_ODD)
  {
    if (m_currentField == FIELD_FULL)
    {
      m_reloadShaders = 1;
    }
    m_currentField = FIELD_ODD;
  } // even field
  else if (flags & RENDER_FLAG_EVEN)
  {
    if (m_currentField == FIELD_FULL)
    {
      m_reloadShaders = 1;
    }
    m_currentField = FIELD_EVEN;
  } // not interlaced
  else
  {
    if (m_currentField != FIELD_FULL)
    {
      m_reloadShaders = 1;
    }
    m_currentField = FIELD_FULL;
  }

  g_graphicsContext.BeginPaint();
  
  if( flags & RENDER_FLAG_NOOSD ) 
  {
    g_graphicsContext.EndPaint();
    return;
  }

  if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      RenderMultiPass(flags);
      //RenderLowMem(flags);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      RenderMultiPass(flags);
      VerifyGLState();
      break;

    case RQ_SOFTWARE:
      RenderSoftware(flags);
      VerifyGLState();
      break;
    }
  }
  else
  {
    RenderSoftware(flags);
    VerifyGLState();
  }

  /* general stuff */

  RenderOSD();

  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (g_application.NeedRenderFullScreen())
    { // render our subtitles and osd
      g_application.RenderFullScreen();
      VerifyGLState();
    }
    g_application.RenderMemoryStatus();
    VerifyGLState();
  }
  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::SetViewMode(int iViewMode)
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
  float fOffsetX1 = (float)g_settings.m_ResInfo[m_iResolution].Overscan.left;
  float fOffsetY1 = (float)g_settings.m_ResInfo[m_iResolution].Overscan.top;
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

void CLinuxRendererGL::AutoCrop(bool bCrop)
{
  if (!m_YUVTexture[0][FIELD_FULL][PLANE_Y]) return ;
  // FIXME: no cropping for now
  { // reset to defaults
    g_stSettings.m_currentVideoSettings.m_CropLeft = 0;
    g_stSettings.m_currentVideoSettings.m_CropRight = 0;
    g_stSettings.m_currentVideoSettings.m_CropTop = 0;
    g_stSettings.m_currentVideoSettings.m_CropBottom = 0;
  }
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
}

void CLinuxRendererGL::RenderMultiPass(DWORD flags)
{
  int index = 0; //m_iYV12RenderBuffer;
  YV12Image &im = m_image[index];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  g_graphicsContext.BeginPaint();

  glDisable(GL_DEPTH_TEST);
  VerifyGLState();

  //See RGB renderer for comment on this
#define CHROMAOFFSET_HORIZ 0.25f

  static GLfloat brightness = 0;
  static GLfloat contrast   = 0;

  brightness =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Brightness - 50.0)/100.0;
  contrast =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Contrast)/50.0;

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][0]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  VerifyGLState();

  // U
  glActiveTexture(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][1]);
  VerifyGLState();
    
  // V
  glActiveTexture(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][2]);
  VerifyGLState();
    
  glActiveTexture(GL_TEXTURE0);
  VerifyGLState();
        
  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    m_fbo.Cleanup();
    LoadShaders(m_currentField);
    VerifyGLState();
    SetTextureFilter(GL_LINEAR);
    VerifyGLState();
  }
  
  // make sure the yuv shader is loaded and ready to go
  if (!m_pYUVShader || (!m_pYUVShader->OK()))
  {
    CLog::Log(LOGERROR, "GL: YUV shader not active, cannot do multipass render");
    return;
  }

  // make sure FBO is valid and ready to go
  if (!m_fbo.IsValid())
  {
    m_fbo.Initialize();
    if (m_currentField != FIELD_FULL)
    {
      if (!m_fbo.CreateAndBindToTexture(GL_TEXTURE_2D, im.width, im.height/2, GL_RGBA))
      {
        CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
      }
    }
    else
    {
      if (!m_fbo.CreateAndBindToTexture(GL_TEXTURE_2D, im.width, im.height, GL_RGBA))
      {
        CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
      }
    }
  }
  
  m_fbo.BeginRender();
  VerifyGLState();
  
  m_pYUVShader->SetYTexture(0);
  m_pYUVShader->SetUTexture(1);
  m_pYUVShader->SetVTexture(2);
  VerifyGLState();
  m_pYUVShader->SetWidth(im.width);
  m_pYUVShader->SetHeight(im.height);
  VerifyGLState();

  glPushAttrib(GL_VIEWPORT_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();
  
  int imgheight;

  switch (m_currentField)
  {
  case FIELD_ODD:
    /*
    glUniform1f(m_stepY, 1/(float)im.height);
    glUniform1f(m_stepX, 1/(float)im.width);
    glUniform1i(m_shaderField, 1);
    */
    m_pYUVShader->SetField(1);
    imgheight = im.height/2;
    break;

  case FIELD_EVEN:
    /*
    glUniform1f(m_stepY, 1/(float)im.height);
    glUniform1f(m_stepX, 1/(float)im.width);
    glUniform1i(m_shaderField, 0);
    */
    m_pYUVShader->SetField(1);
    imgheight = im.height/2;
    break;

  default:
    imgheight = im.height;
    break;
  }

  gluOrtho2D(0, im.width, 0, imgheight);
  glViewport(0, 0, im.width, imgheight);
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();

  if (!m_pYUVShader->Enable())
  {
    CLog::Log(LOGERROR, "GL: Error enabling YUV shader");
  }

  // 1st Pass to video frame size

  glBegin(GL_QUADS);

  glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
  glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
  glMultiTexCoord2f(GL_TEXTURE2, 0, 0);
  glVertex2f((float)0, (float)0);
  
  glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
  glMultiTexCoord2f(GL_TEXTURE1, 1, 0);
  glMultiTexCoord2f(GL_TEXTURE2, 1, 0);
  glVertex2f((float)im.width, (float)0);
  
  glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
  glMultiTexCoord2f(GL_TEXTURE1, 1, 1);
  glMultiTexCoord2f(GL_TEXTURE2, 1, 1);
  glVertex2f((float)im.width, (float)imgheight);
  
  glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
  glMultiTexCoord2f(GL_TEXTURE1, 0, 1);
  glMultiTexCoord2f(GL_TEXTURE2, 0, 1);
  glVertex2f((float)0, (float)imgheight);

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();

  glPopMatrix(); // pop modelview
  glMatrixMode(GL_PROJECTION);
  glPopMatrix(); // pop projection
  glPopAttrib(); // pop viewport
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();

  m_fbo.EndRender();

  glActiveTexture(GL_TEXTURE1);
  glDisable(m_textureTarget);
  glActiveTexture(GL_TEXTURE2);
  glDisable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_fbo.Texture());
  VerifyGLState();
  
  // Use regular normalized texture coordinates

  // 2nd Pass to screen size with optional video filter

  if (m_pVideoFilterShader)
  {
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_NEAREST);
    m_pVideoFilterShader->SetSourceTexture(0);
    m_pVideoFilterShader->SetWidth(im.width);
    m_pVideoFilterShader->SetHeight(imgheight);
    m_pVideoFilterShader->Enable();
  }
  else
  {
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_LINEAR);
  }

  VerifyGLState();
  glBegin(GL_QUADS);

  glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
  glVertex4f((float)rd.left, (float)rd.top, 0, 1.0f );
  
  glMultiTexCoord2f(GL_TEXTURE0, im.texcoord_x, 0);
  glVertex4f((float)rd.right, (float)rd.top, 0, 1.0f);
  
  glMultiTexCoord2f(GL_TEXTURE0, im.texcoord_x, im.texcoord_y);
  glVertex4f((float)rd.right, (float)rd.bottom, 0, 1.0f);
  
  glMultiTexCoord2f(GL_TEXTURE0, 0, im.texcoord_y);
  glVertex4f((float)rd.left, (float)rd.bottom, 0, 1.0f);

  glEnd();

  VerifyGLState();

  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Disable();
  }

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::RenderLowMem(DWORD flags)
{
  //CSingleLock lock(g_graphicsContext);
  int index = 0; //m_iYV12RenderBuffer;
  YV12Image &im = m_image[index];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  g_graphicsContext.BeginPaint();

  glDisable(GL_DEPTH_TEST);

  //See RGB renderer for comment on this
#define CHROMAOFFSET_HORIZ 0.25f

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][0]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  static GLfloat brightness = 0;
  static GLfloat contrast   = 0;

  brightness =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Brightness - 50.0)/100.0;
  contrast =  ((GLfloat)g_stSettings.m_currentVideoSettings.m_Contrast)/50.0;

  if (m_renderMethod & RENDER_GLSL)
  {
    // U
    glActiveTexture(GL_TEXTURE1);
    glEnable(m_textureTarget);
    glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][1]);
    
    // V
    glActiveTexture(GL_TEXTURE2);
    glEnable(m_textureTarget);
    glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][2]);
    
    glActiveTexture(GL_TEXTURE0);
    VerifyGLState();
        
    if (m_reloadShaders)
    {
      m_reloadShaders = 0;
      if (m_shaderProgram)
      {
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
        glDeleteProgram(m_shaderProgram);
        VerifyGLState();
        m_shaderProgram = 0;
        LoadShaders(m_currentField);
        if (!m_shaderProgram)
        {
          CLog::Log(LOGERROR, "GL: Error loading during shaders during render method change");
          return;
        }
      }
      if (m_currentField==FIELD_FULL)
        SetTextureFilter(GL_LINEAR);
      else
        SetTextureFilter(GL_LINEAR);
    }
    
    glUseProgram(m_shaderProgram);
    VerifyGLState();
    glUniform1i(m_yTex, 0);
    VerifyGLState();
    glUniform1i(m_uTex, 1);
    VerifyGLState();
    glUniform1i(m_vTex, 2);
    VerifyGLState();
    glUniform1f(m_brightness, brightness);
    glUniform1f(m_contrast, contrast);

    switch (m_currentField)
    {
    case FIELD_ODD:
      glUniform1f(m_stepY, 1/(float)im.height);
      glUniform1f(m_stepX, 1/(float)im.width);
      glUniform1i(m_shaderField, 1);
      break;

    case FIELD_EVEN:
      glUniform1f(m_stepY, 1/(float)im.height);
      glUniform1f(m_stepX, 1/(float)im.width);
      glUniform1i(m_shaderField, 0);
      break;

    default:
      break;
    }

  }

  glBegin(GL_QUADS);
  
  if (m_textureTarget==GL_TEXTURE_2D)
  {
    // Use regular normalized texture coordinates

    glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
      glMultiTexCoord2f(GL_TEXTURE2, 0, 0);
    }
    glVertex4f((float)rd.left, (float)rd.top, 0, 1.0f );
    
    glMultiTexCoord2f(GL_TEXTURE0, im.texcoord_x, 0);
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, im.texcoord_x, 0);
      glMultiTexCoord2f(GL_TEXTURE2, im.texcoord_x, 0);
    }
    glVertex4f((float)rd.right, (float)rd.top, 0, 1.0f);
    
    glMultiTexCoord2f(GL_TEXTURE0, im.texcoord_x, im.texcoord_y);
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, im.texcoord_x, im.texcoord_y);
      glMultiTexCoord2f(GL_TEXTURE2, im.texcoord_x, im.texcoord_y);
    }
    glVertex4f((float)rd.right, (float)rd.bottom, 0, 1.0f);
    
    glMultiTexCoord2f(GL_TEXTURE0, 0, im.texcoord_y);
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, 0, im.texcoord_y);
      glMultiTexCoord2f(GL_TEXTURE2, 0, im.texcoord_y);
    }
    glVertex4f((float)rd.left, (float)rd.bottom, 0, 1.0f);

  }  
  else 
  {
    // Use supported rectangle texture extension (texture coordinates
    // are not normalized)

    glMultiTexCoord2f(GL_TEXTURE0, (float)rs.left, (float)rs.top );
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f);
      glMultiTexCoord2f(GL_TEXTURE2, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
    }
    glVertex4f((float)rd.left, (float)rd.top, 0, 1.0f );
    
    glMultiTexCoord2f(GL_TEXTURE0, (float)rs.right, (float)rs.top );
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
      glMultiTexCoord2f(GL_TEXTURE2, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.top / 2.0f );
    }
    glVertex4f((float)rd.right, (float)rd.top, 0, 1.0f);
    
    glMultiTexCoord2f(GL_TEXTURE0, (float)rs.right, (float)rs.bottom );
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
      glMultiTexCoord2f(GL_TEXTURE2, (float)rs.right / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
    }
    glVertex4f((float)rd.right, (float)rd.bottom, 0, 1.0f);
    
    glMultiTexCoord2f(GL_TEXTURE0, (float)rs.left, (float)rs.bottom );
    if (m_renderMethod & RENDER_GLSL)
    {
      glMultiTexCoord2f(GL_TEXTURE1, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
      glMultiTexCoord2f(GL_TEXTURE2, (float)rs.left / 2.0f + CHROMAOFFSET_HORIZ, (float)rs.bottom / 2.0f );
    }
    glVertex4f((float)rd.left, (float)rd.bottom, 0, 1.0f);
  }
  glEnd();

  VerifyGLState();

  if (m_renderMethod & RENDER_GLSL)
  {
    glUseProgram(0);
    VerifyGLState();
    glActiveTexture(GL_TEXTURE1);
    glDisable(m_textureTarget);
    glActiveTexture(GL_TEXTURE2);
    glDisable(m_textureTarget);
  }

  glActiveTexture(GL_TEXTURE0);
  glDisable(m_textureTarget);
  VerifyGLState();
  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::RenderSoftware(DWORD flags)
{
  int index = 0;
  YV12Image &im = m_image[index];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  g_graphicsContext.BeginPaint();

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, m_YUVTexture[index][FIELD_FULL][0]);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glBegin(GL_QUADS);
  
  if (m_textureTarget==GL_TEXTURE_2D)
  {
    // Use regular normalized texture coordinates

    glTexCoord2f(0, 0);
    glVertex4f((float)rd.left, (float)rd.top, 0, 1.0f );
    
    glTexCoord2f(im.texcoord_x, 0);
    glVertex4f((float)rd.right, (float)rd.top, 0, 1.0f);
    
    glTexCoord2f(im.texcoord_x, im.texcoord_y);
    glVertex4f((float)rd.right, (float)rd.bottom, 0, 1.0f);
    
    glTexCoord2f(0, im.texcoord_y);
    glVertex4f((float)rd.left, (float)rd.bottom, 0, 1.0f);

  }  
  else 
  {
    // Use supported rectangle texture extension (texture coordinates
    // are not normalized)

    glTexCoord2f((float)rs.left, (float)rs.top );
    glVertex4f((float)rd.left, (float)rd.top, 0, 1.0f );
    
    glTexCoord2f((float)rs.right, (float)rs.top );
    glVertex4f((float)rd.right, (float)rd.top, 0, 1.0f);
    
    glTexCoord2f((float)rs.right, (float)rs.bottom );
    glVertex4f((float)rd.right, (float)rd.bottom, 0, 1.0f);
    
    glTexCoord2f((float)rs.left, (float)rs.bottom );
    glVertex4f((float)rd.left, (float)rd.bottom, 0, 1.0f);
  }

  glEnd();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::CreateThumbnail(SDL_Surface * surface, unsigned int width, unsigned int height)
{
  //CSingleLock lock(g_graphicsContext);
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CLinuxRendererGL::DeleteYV12Texture(int index)
{
  YV12Image &im = m_image[index];
  YUVFIELDS &fields = m_YUVTexture[index];

  if( fields[FIELD_FULL][0] == 0 ) return;

  /* finish up all textures, and delete them */
  g_graphicsContext.BeginPaint(m_pBuffer);
  for(int f = 0;f<MAX_FIELDS;f++) 
  {
    for(int p = 0;p<MAX_PLANES;p++) 
    {
      if( fields[f][p] )
      {
        if (glIsTexture(fields[f][p]))
          glDeleteTextures(1, &fields[f][p]);
        fields[f][p] = 0;
      }
    }
  }
  g_graphicsContext.EndPaint(m_pBuffer);

  for(int p = 0;p<MAX_PLANES;p++) 
  {
    if (im.plane[p]) 
    {
      delete[] im.plane[p];
      im.plane[p] = NULL;
    }
  }
  CLog::Log(LOGDEBUG, "Deleted YV12 texture %i", index);
}

void CLinuxRendererGL::ClearYV12Texture(int index)
{
  YV12Image &im = m_image[index];

  //memset(im.plane[0], 0,   im.stride[0] * im.height);
  //memset(im.plane[1], 128, im.stride[1] * im.height>>im.cshift_y );
  //memset(im.plane[2], 128, im.stride[2] * im.height>>im.cshift_y );

}

bool CLinuxRendererGL::CreateYV12Texture(int index, bool clear)
{
  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;

  YV12Image &im = m_image[index];
  YUVFIELDS &fields = m_YUVTexture[index];

  if (clear)
  {
    DeleteYV12Texture(index);

    im.height = m_iSourceHeight;
    im.width = m_iSourceWidth;
    
    im.stride[0] = m_iSourceWidth;
    im.stride[1] = m_iSourceWidth/2;
    im.stride[2] = m_iSourceWidth/2;
    im.plane[0] = new BYTE[m_iSourceWidth * m_iSourceHeight];
    im.plane[1] = new BYTE[(m_iSourceWidth/2) * (m_iSourceHeight/2)];
    im.plane[2] = new BYTE[(m_iSourceWidth/2) * (m_iSourceHeight/2)];
    
    im.cshift_x = 1;
    im.cshift_y = 1;
    im.texcoord_x = 1.0;
    im.texcoord_y = 1.0;
  }

  g_graphicsContext.BeginPaint(m_pBuffer);

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++) 
  {
    for(p = 0;p<MAX_PLANES;p++) 
    {
      if (!glIsTexture(fields[f][p])) 
      {
        glGenTextures(1, &fields[f][p]);
        VerifyGLState();
      }
    }
  }

  // YUV 
  p = 0;
  glBindTexture(m_textureTarget, fields[0][0]);
  if (m_renderMethod & RENDER_SW)
  {
    // require Power Of Two textures?
    if (m_renderMethod & RENDER_POT)
    {
      static unsigned long np2x = 0, np2y = 0;
      np2x = NP2(im.width);
      np2y = NP2(im.height);
      CLog::Log(LOGNOTICE, "GL: Creating power of two texture of size %d x %d", np2x, np2y);
      glTexImage2D(m_textureTarget, 0, GL_RGBA, np2x, np2y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      im.texcoord_x = ((float)im.width / (float)np2x);
      im.texcoord_y = ((float)im.height / (float)np2y);
    }
    else
    {
      glTexImage2D(m_textureTarget, 0, GL_RGBA, im.width, im.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    }
  }
  else
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width, im.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
  VerifyGLState();

  if (m_renderMethod & RENDER_GLSL)
  {
    glBindTexture(m_textureTarget, fields[0][1]);
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width/2, im.height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    VerifyGLState();
    
    glBindTexture(m_textureTarget, fields[0][2]);
    glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, im.width/2, im.height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL); 
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    VerifyGLState();
  }

  g_graphicsContext.EndPaint(m_pBuffer);
  return true;
}

void CLinuxRendererGL::SetTextureFilter(GLenum method)
{
  YUVFIELDS &fields = m_YUVTexture[0];

  glBindTexture(m_textureTarget, fields[0][0]);  
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
  VerifyGLState();

  if (m_renderMethod & RENDER_GLSL)
  {
    glBindTexture(m_textureTarget, fields[0][1]);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
    VerifyGLState();
    
    glBindTexture(m_textureTarget, fields[0][2]);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
    VerifyGLState();
  }
}

void CLinuxRendererGL::TextureCallback(DWORD dwContext)
{
  SetEvent((HANDLE)dwContext);
}

#endif

#endif
