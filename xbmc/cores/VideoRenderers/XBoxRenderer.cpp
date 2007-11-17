/*
* XBoxMediaCenter
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
#include "XBoxRenderer.h"
#include "../../Application.h"
#include "../../Util.h"
#include "../../XBVideoConfig.h"

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


#ifndef HAS_SDL
CXBoxRenderer::CXBoxRenderer(LPDIRECT3DDEVICE8 pDevice)
#else
CXBoxRenderer::CXBoxRenderer()
#endif
{
#ifndef _LINUX
  m_pD3DDevice = pDevice;
#endif
  m_fSourceFrameRatio = 1.0f;
  m_iResolution = PAL_4x3;
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    m_pOSDYTexture[i] = NULL;
    m_pOSDATexture[i] = NULL;
    m_eventTexturesDone[i] = CreateEvent(NULL,FALSE,TRUE,NULL);
    m_eventOSDDone[i] = CreateEvent(NULL,TRUE,TRUE,NULL);
  }
  m_hLowMemShader = 0;

  m_iYV12RenderBuffer = 0;

  memset(m_image, 0, sizeof(m_image));

  memset(m_YUVTexture, 0, sizeof(m_YUVTexture));
  m_yuvcoef = yuv_coef_bt709;
  m_yuvrange = yuv_range_lim;
}

CXBoxRenderer::~CXBoxRenderer()
{
  UnInit();
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    CloseHandle(m_eventTexturesDone[i]);
    CloseHandle(m_eventOSDDone[i]);
  }
}

//********************************************************************************************************
void CXBoxRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_pOSDYTexture[index])
  {
#ifndef _LINUX
    m_pOSDYTexture[index]->Release();
#else
    SDL_FreeSurface(m_pOSDYTexture[index]);
#endif
    m_pOSDYTexture[index] = NULL;
  }
  if (m_pOSDATexture[index])
  {
#ifndef _LINUX
    m_pOSDATexture[index]->Release();
#else
    SDL_FreeSurface(m_pOSDATexture[index]);
#endif
    m_pOSDATexture[index] = NULL;
    CLog::Log(LOGDEBUG, "Deleted OSD textures (%i)", index);
  }
  m_iOSDTextureHeight[index] = 0;
}

void CXBoxRenderer::Setup_Y8A8Render()
{
#ifndef _LINUX
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAKILL, D3DTALPHAKILL_ENABLE );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_POINT /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_POINT /*g_stSettings.m_minFilter*/ );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_Y8A8VERTEX );
#endif
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
void CXBoxRenderer::CalculateFrameAspectRatio(int desired_width, int desired_height)
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
void CXBoxRenderer::CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride)
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

void CXBoxRenderer::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
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

  RECT rc = { 0, 0, w, h };

  int iOSDBuffer = (m_iOSDRenderBuffer + 1) % m_NumOSDBuffers;

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
#ifndef _LINUX
    if (
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_L8, 0, &m_pOSDYTexture[iOSDBuffer]) ||
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_A8, 0, &m_pOSDATexture[iOSDBuffer])
    )
#else
    if ( 1 )
#warning need to create textures
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
#ifndef _LINUX
  m_pOSDYTexture[iOSDBuffer]->Lock = 0;
  m_pOSDATexture[iOSDBuffer]->Lock = 0;

  // draw textures
  D3DLOCKED_RECT lr, lra;
  if (
    (D3D_OK == m_pOSDYTexture[iOSDBuffer]->LockRect(0, &lr, &rc, 0)) &&
    (D3D_OK == m_pOSDATexture[iOSDBuffer]->LockRect(0, &lra, &rc, 0))
  )
  {
    //clear the textures
    memset(lr.pBits, 0, lr.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    memset(lra.pBits, 0, lra.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    //draw the osd/subs
    CopyAlpha(w, h, src, srca, stride, (BYTE*)lr.pBits, (BYTE*)lra.pBits, lr.Pitch);
  }
  m_pOSDYTexture[iOSDBuffer]->UnlockRect(0);
  m_pOSDATexture[iOSDBuffer]->UnlockRect(0);
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
void CXBoxRenderer::RenderOSD()
{
  int iRenderBuffer = m_iOSDRenderBuffer;

  if (!m_pOSDYTexture[iRenderBuffer] || !m_pOSDATexture[iRenderBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;

  ResetEvent(m_eventOSDDone[iRenderBuffer]);

  CSingleLock lock(g_graphicsContext);

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;
  DRAWRECT osdRect = m_OSDRect;
  //  if (!viewportRect.bottom && !viewportRect.right)
  //    return;

  // Set state to render the image
#ifndef _LINUX
  m_pD3DDevice->SetTexture(0, m_pOSDYTexture[iRenderBuffer]);
  m_pD3DDevice->SetTexture(1, m_pOSDATexture[iRenderBuffer]);
#endif

  Setup_Y8A8Render();

  /* In mplayer's alpha planes, 0 is transparent, then 1 is nearly
  opaque upto 255 which is transparent */
  // means do alphakill + inverse alphablend
  /*
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  if (m_SubsOnOSD)
  {
  // subs use mplayer style alpha
  //m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_INVSRCALPHA );
  //m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );

   // Note the mplayer code actually does this
  //m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
  //m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
  }
  else
  {
  // OSD looks better with src+(1-a)*dst
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  }
  */

  // Set texture filters
  //m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  //m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  //m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  //m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // Render the image
#ifndef _LINUX
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, 0 );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, 0 );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, osdRect.left, osdRect.top, 0, 1.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, osdWidth, 0 );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, osdWidth, 0 );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, osdRect.right, osdRect.top, 0, 1.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, osdWidth, osdHeight );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, osdWidth, osdHeight );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, osdRect.right, osdRect.bottom, 0, 1.0f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0, osdHeight );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0, osdHeight );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, osdRect.left, osdRect.bottom, 0, 1.0f );
  m_pD3DDevice->End();

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE );

  m_pD3DDevice->SetScissors(0, FALSE, NULL);

  //Okey, when the gpu is done with the textures here, they are free to be modified again
  //this is very weird.. D3DCALLBACK_READ is not enough.. if that is used, we start flushing
  //the texutures to early.. I have no idea why really
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventOSDDone[iRenderBuffer]);
#endif

}

//********************************************************************************************************
//Get resolution based on current mode.
RESOLUTION CXBoxRenderer::GetResolution()
{
  if (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
  {
    return m_iResolution;
  }
  return g_graphicsContext.GetVideoResolution();
}

float CXBoxRenderer::GetAspectRatio()
{
  float fWidth = (float)m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropLeft - g_stSettings.m_currentVideoSettings.m_CropRight;
  float fHeight = (float)m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropTop - g_stSettings.m_currentVideoSettings.m_CropBottom;
  return m_fSourceFrameRatio * fWidth / fHeight * m_iSourceHeight / m_iSourceWidth;
}

void CXBoxRenderer::GetVideoRect(RECT &rectSrc, RECT &rectDest)
{
  rectSrc = rs;
  rectDest = rd;
}

void CXBoxRenderer::CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fInputFrameRatio, float fZoomAmount)
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


void CXBoxRenderer::ManageTextures()
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

  if( m_NumYV12Buffers < neededbuffers )
  {
    for(int i = m_NumYV12Buffers; i<neededbuffers;i++)
      CreateYV12Texture(i);

    m_NumYV12Buffers = neededbuffers;
  }
  else if( m_NumYV12Buffers > neededbuffers )
  {
    // delete from the end
    int i = m_NumYV12Buffers-1;
    for(; i>=neededbuffers;i--)
    {
      // don't delete any frame that is in use
      if(m_image[i].flags & IMAGE_FLAG_DYNAMIC)
        break;
      DeleteYV12Texture(i);
    }
    if(m_iYV12RenderBuffer > i)
        m_iYV12RenderBuffer = i;
    m_NumYV12Buffers = i+1;
  }
}

void CXBoxRenderer::ManageDisplay()
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

void CXBoxRenderer::ChooseBestResolution(float fps)
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
#ifndef _LINUX
    float fFrameDifference60 = abs(120.0f / fps - floor(120.0f / fps + 0.5f));
    float fFrameDifference50 = abs(100.0f / fps - floor(100.0f / fps + 0.5f));
#else
    float fFrameDifference60 = fabs(120.0f / fps - floor(120.0f / fps + 0.5f));
    float fFrameDifference50 = fabs(100.0f / fps - floor(100.0f / fps + 0.5f));
#endif
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

bool CXBoxRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_fps = fps;
  m_iSourceWidth = width;
  m_iSourceHeight = height;
  m_iFlags = flags;
  
  // setup what colorspace we live in
  if(flags & CONF_FLAGS_YUV_FULLRANGE)
    m_yuvrange = yuv_range_full;
  else
    m_yuvrange = yuv_range_lim;

  switch(CONF_FLAGS_YUVCOEF_MASK(flags))
  {
    case CONF_FLAGS_YUVCOEF_240M:
      m_yuvcoef = yuv_coef_smtp240m; break;
    case CONF_FLAGS_YUVCOEF_BT709:
      m_yuvcoef = yuv_coef_bt709; break;
    case CONF_FLAGS_YUVCOEF_BT601:    
      m_yuvcoef = yuv_coef_bt601; break;
    case CONF_FLAGS_YUVCOEF_EBU:
      m_yuvcoef = yuv_coef_ebu; break;
    default:
      m_yuvcoef = yuv_coef_bt601; break;
  }

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(m_fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();

  return true;
}

int CXBoxRenderer::NextYV12Texture()
{
#ifdef MP_DIRECTRENDERING
  int source = m_iYV12RenderBuffer;
  do {
    source = (source + 1) % m_NumYV12Buffers;    
  } while( source != m_iYV12RenderBuffer
    && m_image[source].flags & IMAGE_FLAG_INUSE);

  if( m_image[source].flags & IMAGE_FLAG_INUSE)
    return -1;
  else
    return source;

#else

  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
#endif
}

int CXBoxRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

#ifdef MP_DIRECTRENDERING 
    if( source < 0 )
    { /* no free source existed, so create one */
      CSingleLock lock(g_graphicsContext);
      if( CreateYV12Texture(m_NumYV12Buffers) )
      {
        source = m_NumYV12Buffers;
        m_NumYV12Buffers++;
        m_image[source].flags |= IMAGE_FLAG_DYNAMIC;
      }
    }
#endif

  if( source >= 0 && m_image[source].plane[0] )
  {
    if( readonly )
      m_image[source].flags |= IMAGE_FLAG_READING;
    else
    {
      if( WaitForSingleObject(m_eventTexturesDone[source], 500) == WAIT_TIMEOUT )
        CLog::Log(LOGWARNING, CStdString(__FUNCTION__) + " - Timeout waiting for texture %d", source);

      m_image[source].flags |= IMAGE_FLAG_WRITING;
    }


    *image = m_image[source];
    return source;
  }
  return -1;
}

void CXBoxRenderer::ReleaseImage(int source, bool preserve)
{
  if( m_image[source].flags & IMAGE_FLAG_WRITING )
    SetEvent(m_eventTexturesDone[source]);
  
  m_image[source].flags &= ~IMAGE_FLAG_INUSE;

  /* if image should be preserved reserve it so it's not auto seleceted */
  if( preserve )
    m_image[source].flags |= IMAGE_FLAG_RESERVED;  
}

void CXBoxRenderer::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_image[i].flags = 0;
    /* reset texure locks, abit uggly, could result in tearing */
    SetEvent(m_eventTexturesDone[i]); 
  }
}

void CXBoxRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();
}

void CXBoxRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_YUVTexture[m_iYV12RenderBuffer][FIELD_FULL][0]) return ;

  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();

#ifndef _LINUX
  if (clear)
    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
  {
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_CONSTANTALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVCONSTANTALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_BLENDCOLOR, D3DCOLOR_ARGB(alpha, 0, 0, 0) );
  }
  else
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);

  //Kick commands out to the GPU, or we won't get the callback for textures being done
  m_pD3DDevice->KickPushBuffer();
#endif

}

void CXBoxRenderer::FlipPage(int source)
{  
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  /* we always decode into to the next buffer */
  ++m_iOSDRenderBuffer %= m_NumOSDBuffers;

  /* if osd wasn't rendered this time around, previuse should not be */
  /* displayed on next frame */
  if( !m_OSDRendered )
    m_OSDWidth = m_OSDHeight = 0;

  m_OSDRendered = false;

#ifdef MP_DIRECTRENDERING
  __asm wbinvd
#endif

  return;
}


unsigned int CXBoxRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;
  
  int index = NextYV12Texture();
  if( index < 0 )
    return -1;

  if( WaitForSingleObject(m_eventTexturesDone[index], 500) == WAIT_TIMEOUT )
    CLog::Log(LOGWARNING, CStdString(__FUNCTION__) + " - Timeout waiting for texture %d", index);

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

  SetEvent(m_eventTexturesDone[index]);
  return 0;
}

unsigned int CXBoxRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_iResolution = PAL_4x3;

  m_iOSDRenderBuffer = 0;
  m_iYV12RenderBuffer = 0;
  m_NumOSDBuffers = 0;
  m_NumYV12Buffers = 0;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;
  // low memory pixel shader
  if (!m_hLowMemShader)
  {
#ifndef _LINUX
    // lowmem shader (not as accurate, but no need for interleaving of YUV)
    const char *lowmem =
      "xps.1.1\n"
      "def c0, 0.0625,0.0625,0.0625,0\n"
      "def c1, 0.58219,0.58219,0.58219,0\n"
      "def c2, 0.80216,0.80216,0.80216,0\n"
      "def c3, 1,0,0,0\n"
      "def c4, 0,0,1,0\n"
      "def c5, 1,0,1,0\n"
      "def c6, 0,1,0,0\n"
      "def c7, 0.58219,0.19668,0.4082287,0\n"
      "tex t0\n"
      "tex t1\n"
      "tex t2\n"
      "sub r0, t0, c0\n"
      "xmma_x2 discard, discard, r1, r0, c1, t2_bias, c2\n"
      //   "xmma_x2 discard, discard, v0, r0, c1, t1_bias, c8\n" c8 should be 1.0119955
      "mad_x2 v0,r0,c1,t1_bias\n"
      "xmma discard, discard, r1, r1, c3, v0, c4\n"
      "xmma discard, discard, r0, r0, c3, -t2_bias, c4\n"
      "xmma discard, discard, r0, r0, c5, -t1_bias, c6\n"
      "dp3_x2 r0, r0, c7\n"
      "xmma discard, discard, r0, r1, c5, r0, c6\n";

    XGBuffer* pShader;
    XGAssembleShader("LowMemShader", lowmem, strlen(lowmem), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hLowMemShader);
    pShader->Release();
#endif
  }

  return 0;
}

void CXBoxRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  // YV12 textures, subtitle and osd stuff
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteYV12Texture(i);
    DeleteOSDTextures(i);
  }
  
  if (m_hLowMemShader)
  {
#ifndef _LINUX
    m_pD3DDevice->DeletePixelShader(m_hLowMemShader);
#endif
    m_hLowMemShader = 0;
  }
}

void CXBoxRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;

  /* general stuff */
  RenderOSD();

  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (g_application.NeedRenderFullScreen())
    { // render our subtitles and osd
      g_application.RenderFullScreen();
    }

    if (!g_application.IsPaused())
    {
      g_application.RenderMemoryStatus();
    }
  }
}

void CXBoxRenderer::SetViewMode(int iViewMode)
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

void CXBoxRenderer::AutoCrop(bool bCrop)
{
  if (!m_YUVTexture[0][FIELD_FULL][PLANE_Y]) return ;

  if (bCrop)
  {
    CSingleLock lock(g_graphicsContext);
    // apply auto-crop filter - only luminance needed, and we run vertically down 'n'
    // runs down the image.
    int min_detect = 8;                                // reasonable amount (what mplayer uses)
    int detect = (min_detect + 16)*m_iSourceWidth;     // luminance should have minimum 16
#ifndef _LINUX
    D3DLOCKED_RECT lr;
    m_YUVTexture[0][FIELD_FULL][PLANE_Y]->LockRect(0, &lr, NULL, 0);
    // Crop top
    BYTE *s = (BYTE *)lr.pBits;
#else
    SDL_LockSurface(m_YUVTexture[0][FIELD_FULL][PLANE_Y]);
    BYTE *s = (BYTE *)m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pixels;
#endif
    int total;
    g_stSettings.m_currentVideoSettings.m_CropTop = m_iSourceHeight/2;
    for (unsigned int y = 0; y < m_iSourceHeight/2; y++)
    {
      total = 0;
      for (unsigned int x = 0; x < m_iSourceWidth; x++)
        total += s[x];
#ifndef _LINUX
      s += lr.Pitch;
#else
      s += m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pitch;
#endif
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropTop = y;
        break;
      }
    }
    // Crop bottom
#ifndef _LINUX
    s = (BYTE *)lr.pBits + (m_iSourceHeight-1)*lr.Pitch;
#else
    s = (BYTE *)m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pixels + (m_iSourceHeight-1)*m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pitch;
#endif
    g_stSettings.m_currentVideoSettings.m_CropBottom = m_iSourceHeight/2;
    for (unsigned int y = (int)m_iSourceHeight; y > m_iSourceHeight/2; y--)
    {
      total = 0;
      for (unsigned int x = 0; x < m_iSourceWidth; x++)
        total += s[x];
#ifndef _LINUX
      s -= lr.Pitch;
#else
      s -= m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pitch;
#endif
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropBottom = m_iSourceHeight - y;
        break;
      }
    }
    // Crop left
#ifndef _LINUX
    s = (BYTE *)lr.pBits;
#else
    s = (BYTE *)m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pixels;
#endif
    g_stSettings.m_currentVideoSettings.m_CropLeft = m_iSourceWidth/2;
    for (unsigned int x = 0; x < m_iSourceWidth/2; x++)
    {
      total = 0;
      for (unsigned int y = 0; y < m_iSourceHeight; y++)
#ifndef _LINUX
        total += s[y * lr.Pitch];
#else
        total += s[y * m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pitch];
#endif
      s++;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropLeft = x;
        break;
      }
    }
    // Crop right
#ifndef _LINUX
    s = (BYTE *)lr.pBits + (m_iSourceWidth-1);
#else
    s = (BYTE *)m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pixels + (m_iSourceWidth-1);
#endif
    g_stSettings.m_currentVideoSettings.m_CropRight= m_iSourceWidth/2;
    for (unsigned int x = (int)m_iSourceWidth-1; x > m_iSourceWidth/2; x--)
    {
      total = 0;
      for (unsigned int y = 0; y < m_iSourceHeight; y++)
#ifndef _LINUX
        total += s[y * lr.Pitch];
#else
        total += s[y * m_YUVTexture[0][FIELD_FULL][PLANE_Y]->pitch];
#endif
      s--;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropRight = m_iSourceWidth - x;
        break;
      }
    }
#ifndef _LINUX
    m_YUVTexture[0][FIELD_FULL][PLANE_Y]->UnlockRect(0);
#else
    SDL_UnlockSurface(m_YUVTexture[0][FIELD_FULL][PLANE_Y]);
#endif
  }
  else
  { // reset to defaults
    g_stSettings.m_currentVideoSettings.m_CropLeft = 0;
    g_stSettings.m_currentVideoSettings.m_CropRight = 0;
    g_stSettings.m_currentVideoSettings.m_CropTop = 0;
    g_stSettings.m_currentVideoSettings.m_CropBottom = 0;
  }
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
}

void CXBoxRenderer::RenderLowMem(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);
  int index = m_iYV12RenderBuffer;
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  if( WaitForSingleObject(m_eventTexturesDone[index], 500) == WAIT_TIMEOUT )
    CLog::Log(LOGWARNING, CStdString(__FUNCTION__) + " - Timeout waiting for texture %d", index);

  for (int i = 0; i < 3; ++i)
  {
#ifndef _LINUX
    m_pD3DDevice->SetTexture(i, m_YUVTexture[index][FIELD_FULL][i]);
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
#endif
  }

#ifndef _LINUX
  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_YV12VERTEX );
  m_pD3DDevice->SetPixelShader( m_hLowMemShader );
#endif

  //See RGB renderer for comment on this
  #define CHROMAOFFSET_HORIZ 0.25f

#ifndef _LINUX
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
#endif

}

#ifndef _LINUX
void CXBoxRenderer::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
#else
void CXBoxRenderer::CreateThumbnail(SDL_Surface * surface, unsigned int width, unsigned int height)
#endif
{
  CSingleLock lock(g_graphicsContext);
#ifndef _LINUX
  LPDIRECT3DSURFACE8 oldRT;
  RECT saveSize = rd;
  rd.left = rd.top = 0;
  rd.right = width;
  rd.bottom = height;
  m_pD3DDevice->GetRenderTarget(&oldRT);
  m_pD3DDevice->SetRenderTarget(surface, NULL);
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  RenderLowMem(0);
  rd = saveSize;
  m_pD3DDevice->SetRenderTarget(oldRT, NULL);
  oldRT->Release();
#endif
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CXBoxRenderer::DeleteYV12Texture(int index)
{
  YV12Image &im = m_image[index];
  YUVFIELDS &fields = m_YUVTexture[index];

  if( fields[FIELD_FULL][0] == NULL ) return;

  /* finish up all textures, and delete them */
  for(int f = 0;f<MAX_FIELDS;f++) {
    for(int p = 0;p<MAX_PLANES;p++) {
      if( fields[f][p] )
      {
#ifndef _LINUX
        fields[f][p]->BlockUntilNotBusy();
        SAFE_DELETE(fields[f][p]);
#endif
      }
    }
  }

#ifndef _LINUX
  /* data is allocated in one go */
  if (im.plane[0])
    XPhysicalFree(im.plane[0]);

  for(int p = 0;p<MAX_PLANES;p++)
    im.plane[p] = NULL;
#endif

  CLog::Log(LOGDEBUG, "Deleted YV12 texture %i", index);
}

void CXBoxRenderer::ClearYV12Texture(int index)
{
  if( WaitForSingleObject(m_eventTexturesDone[index], 1000) == WAIT_TIMEOUT )
    CLog::Log(LOGWARNING, CStdString(__FUNCTION__) + " - Timeout waiting for texture %d", index);

  YV12Image &im = m_image[index];

  memset(im.plane[0], 0,   im.stride[0] * im.height);
  memset(im.plane[1], 128, im.stride[1] * im.height>>im.cshift_y );
  memset(im.plane[2], 128, im.stride[2] * im.height>>im.cshift_y );

  SetEvent(m_eventTexturesDone[index]);
}

bool CXBoxRenderer::CreateYV12Texture(int index)
{
  DeleteYV12Texture(index);

  /* since we also want the field textures, pitch must be texture aligned */
  DWORD dwTextureSize;
  unsigned stride, p;
#ifndef _LINUX
#ifdef MP_DIRECTRENDERING
  unsigned memflags = PAGE_READWRITE;
#else
  unsigned memflags = PAGE_READWRITE | PAGE_WRITECOMBINE;
#endif
#endif

  YV12Image &im = m_image[index];
  YUVFIELDS &fields = m_YUVTexture[index];

  im.height = m_iSourceHeight;
  im.width = m_iSourceWidth;

#ifndef _LINUX
  im.stride[0] = ALIGN(m_iSourceWidth,D3DTEXTURE_ALIGNMENT);
  im.stride[1] = ALIGN(m_iSourceWidth>>1,D3DTEXTURE_ALIGNMENT);
  im.stride[2] = ALIGN(m_iSourceWidth>>1,D3DTEXTURE_ALIGNMENT);
#endif

  im.cshift_x = 1;
  im.cshift_y = 1;

#ifndef _LINUX
  for(int f = 0;f<MAX_FIELDS;f++) {
    for(p = 0;p<MAX_PLANES;p++) {
      fields[f][p] = new D3DTexture();
      fields[f][p]->AddRef();
    }
  }

  dwTextureSize = 0;
  /* Y */
  p = 0;
  stride = im.stride[p];
  im.plane[p] = (BYTE*)dwTextureSize;
                   XGSetTextureHeader(im.width             , im.height>>1             , 1, 0, D3DFMT_LIN_L8, 0, fields[2][p], dwTextureSize + stride, stride<<1);
                   XGSetTextureHeader(im.width             , im.height>>1             , 1, 0, D3DFMT_LIN_L8, 0, fields[1][p], dwTextureSize,          stride<<1);
  dwTextureSize += XGSetTextureHeader(im.width             , im.height                , 1, 0, D3DFMT_LIN_L8, 0, fields[0][p], dwTextureSize,          stride);

  /* U */
  p = 1;
  stride = im.stride[p];
  im.plane[p] = (BYTE*)dwTextureSize;
                   XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y>>1, 1, 0, D3DFMT_LIN_L8, 0, fields[2][p], dwTextureSize + stride, stride<<1);
                   XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y>>1, 1, 0, D3DFMT_LIN_L8, 0, fields[1][p], dwTextureSize,          stride<<1);
  dwTextureSize += XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y,    1, 0, D3DFMT_LIN_L8, 0, fields[0][p], dwTextureSize,          stride);

  /* V */
  p = 2;
  stride = im.stride[p];
  im.plane[p] = (BYTE*)dwTextureSize;
                   XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y>>1, 1, 0, D3DFMT_LIN_L8, 0, fields[2][p], dwTextureSize + stride, stride<<1);
                   XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y>>1, 1, 0, D3DFMT_LIN_L8, 0, fields[1][p], dwTextureSize,          stride<<1);
  dwTextureSize += XGSetTextureHeader(im.width>>im.cshift_x, im.height>>im.cshift_y,    1, 0, D3DFMT_LIN_L8, 0, fields[0][p], dwTextureSize,          stride);

  BYTE* data = (BYTE*)XPhysicalAlloc(dwTextureSize, MAXULONG_PTR, D3DTEXTURE_ALIGNMENT, memflags);

  if(!data)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unable to allocate decode texture of %d bytes", dwTextureSize);

    for(p = 0;p<MAX_PLANES;p++) {
      im.plane[p] = NULL;
      for(int f = 0;f<MAX_FIELDS;f++) {
        fields[f][p] = new D3DTexture();
        fields[f][p]->AddRef();
      }
    }
    return false;
  }

  for(p = 0;p<MAX_PLANES;p++) {
    im.plane[p] += (uintptr_t)data;
    for(int f = 0;f<MAX_FIELDS;f++) {
      fields[f][p]->Register(data);
    }
  }
#endif

  SetEvent(m_eventTexturesDone[index]);

  CLog::Log(LOGDEBUG, "Created YV12 texture %i", index);

  ClearYV12Texture(index);
  return true;
}

void CXBoxRenderer::TextureCallback(DWORD dwContext)
{
  SetEvent((HANDLE)dwContext);
}

