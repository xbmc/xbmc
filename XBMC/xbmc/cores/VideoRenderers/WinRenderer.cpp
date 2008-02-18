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
#include "WinRenderer.h"
#include "Application.h"
#include "Util.h"
#include "XBVideoConfig.h"


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


CWinRenderer::CWinRenderer(LPDIRECT3DDEVICE8 pDevice)
{
  m_pD3DDevice = pDevice;
  m_fSourceFrameRatio = 1.0f;
  m_iResolution = PAL_4x3;
  memset(m_pOSDYTexture,0,sizeof(LPDIRECT3DTEXTURE8)*NUM_BUFFERS);
  memset(m_pOSDATexture,0,sizeof(LPDIRECT3DTEXTURE8)*NUM_BUFFERS);
  memset(m_YUVTexture, 0, sizeof(m_YUVTexture));

  m_hLowMemShader = 0;
  m_iYV12RenderBuffer = 0;

}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

//********************************************************************************************************
void CWinRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);

  if (m_pOSDYTexture[index])
    SAFE_RELEASE(m_pOSDYTexture[index]);

  if (m_pOSDATexture[index])
    SAFE_RELEASE(m_pOSDATexture[index]);
  
  m_iOSDTextureHeight[index] = 0;
  m_iOSDTextureWidth = 0;
}

void CWinRenderer::Setup_Y8A8Render()
{
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

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
  m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
  m_pD3DDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_TEX2 );
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
void CWinRenderer::CalculateFrameAspectRatio(int desired_width, int desired_height)
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
void CWinRenderer::CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride)
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

void CWinRenderer::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
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
    if (
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_L8, D3DPOOL_DEFAULT, &m_pOSDYTexture[iOSDBuffer]) ||
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_A8, D3DPOOL_DEFAULT, &m_pOSDATexture[iOSDBuffer])
    )
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

  //set module variables to calculated values
  m_OSDRect = osdRect;
  m_OSDWidth = (float)w;
  m_OSDHeight = (float)h;
  m_OSDRendered = true;
}

//********************************************************************************************************
void CWinRenderer::RenderOSD()
{
  int iRenderBuffer = m_iOSDRenderBuffer;

  if (!m_pOSDYTexture[iRenderBuffer] || !m_pOSDATexture[iRenderBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;

  CSingleLock lock(g_graphicsContext);

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;
  DRAWRECT osdRect = m_OSDRect;
  //  if (!viewportRect.bottom && !viewportRect.right)
  //    return;

  // Set state to render the image
  m_pD3DDevice->SetTexture(0, m_pOSDYTexture[iRenderBuffer]);
  m_pD3DDevice->SetTexture(1, m_pOSDATexture[iRenderBuffer]);
  Setup_Y8A8Render();

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4] =
  {
    { osdRect.left         , osdRect.top            , 0.0f,
      1.0f, 0,
      0.0f                 , 0.0f,
      0.0f                 , 0.0f,
    }, 
    { osdRect.right        , osdRect.top            , 0.0f,
      1.0f, 0,
      osdWidth / m_OSDWidth, 0.0f,
      osdWidth / m_OSDWidth, 0.0f,
    }, 
    { osdRect.right        , osdRect.bottom         , 0.0f,
      1.0f, 0,
      osdWidth / m_OSDWidth, osdHeight / m_OSDHeight,
      osdWidth / m_OSDWidth, osdHeight / m_OSDHeight,
    }, 
    { osdRect.left         , osdRect.bottom         , 0.0f,
      1.0f, 0,
      0.0f                 , osdHeight / m_OSDHeight,
      0.0f                 , osdHeight / m_OSDHeight,
    }
  };

  m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
}

//********************************************************************************************************
//Get resolution based on current mode.
RESOLUTION CWinRenderer::GetResolution()
{
  if (g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
  {
    return m_iResolution;
  }
  return g_graphicsContext.GetVideoResolution();
}

float CWinRenderer::GetAspectRatio()
{
  float fWidth = (float)m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropLeft - g_stSettings.m_currentVideoSettings.m_CropRight;
  float fHeight = (float)m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropTop - g_stSettings.m_currentVideoSettings.m_CropBottom;
  return m_fSourceFrameRatio * fWidth / fHeight * m_iSourceHeight / m_iSourceWidth;
}

void CWinRenderer::GetVideoRect(RECT &rectSrc, RECT &rectDest)
{
  rectSrc = rs;
  rectDest = rd;
}

void CWinRenderer::CalcNormalDisplayRect(float fOffsetX1, float fOffsetY1, float fScreenWidth, float fScreenHeight, float fInputFrameRatio, float fZoomAmount)
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


void CWinRenderer::ManageTextures()
{
  int neededbuffers = 0;
  if (m_NumOSDBuffers != 2)
  {
    m_NumOSDBuffers = 2;
    m_iOSDRenderBuffer = 0;
    m_OSDWidth = m_OSDHeight = 0;
    // buffers will be created on demand in DrawAlpha()
  }
  neededbuffers = 2;

  if( m_NumYV12Buffers < neededbuffers )
  {
    for(int i = m_NumYV12Buffers; i<neededbuffers;i++)
      CreateYV12Texture(i);

    m_NumYV12Buffers = neededbuffers;
  }
  else if( m_NumYV12Buffers > neededbuffers )
  {
    m_NumYV12Buffers = neededbuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;    

    for(int i = m_NumYV12Buffers-1; i>=neededbuffers;i--)
      DeleteYV12Texture(i);
  }
}

void CWinRenderer::ManageDisplay()
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

void CWinRenderer::ChooseBestResolution(float fps)
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
    float fFrameDifference60 = abs(120.0f / fps - floor(120.0f / fps + 0.5f));
    float fFrameDifference50 = abs(100.0f / fps - floor(100.0f / fps + 0.5f));
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

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_fps = fps;
  m_iSourceWidth = width;
  m_iSourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(m_fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();

  return true;
}

int CWinRenderer::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return -1;

  YUVPLANES &planes = m_YUVTexture[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_iSourceHeight;
  image->width = m_iSourceWidth;
  image->flags = 0;

  D3DLOCKED_RECT rect;
  for(int i=0;i<3;i++)
  {
    planes[i]->LockRect(0, &rect, NULL, 0);
    image->stride[i] = rect.Pitch;
    image->plane[i] = (BYTE*)rect.pBits;
  }

  return source;
}

void CWinRenderer::ReleaseImage(int source, bool preserve)
{
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return;

  YUVPLANES &planes = m_YUVTexture[source];
  for(int i=0;i<3;i++)
    planes[i]->UnlockRect(0);
}

void CWinRenderer::Reset()
{
}

void CWinRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  
  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  ManageTextures();
}

void CWinRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_YUVTexture[m_iYV12RenderBuffer][0]) return ;
  
  CSingleLock lock(g_graphicsContext);

  ManageDisplay();
  ManageTextures();
  if (clear)
    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);
}

void CWinRenderer::FlipPage(int source)
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


unsigned int CWinRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;
  
  int index = NextYV12Texture();
  if( index < 0 )
    return -1;
  
  D3DLOCKED_RECT rect;
  RECT target;

  target.left = x;
  target.right = x+w;
  target.top = y;
  target.bottom = y+h;

  YUVPLANES &planes = m_YUVTexture[index];

  // copy Y
  p = 0;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  w >>= 1; h >>= 1;
  x >>= 1; y >>= 1;
  target.top>>=1;
  target.bottom>>=1;
  target.left>>=1;
  target.right>>=1; 

  // copy U
  p = 1;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  // copy V
  p = 2;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  return 0;
}

unsigned int CWinRenderer::PreInit()
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
    // lowmem shader (not as accurate, but no need for interleaving of YUV)
    const char *lowmem =
      "ps.1.3\n"
      "def c0, 0      ,0.18664,0.96032,0\n"
      "def c1, 0.76120,0.38738,0      ,0\n"
      "def c2, 1,0,1,1\n"
      "def c3, 0.0625,0.0625,0.0625,0\n"
      "def c4, 0.58219,0.58219,0.58219,0.5\n"
      "def c5, 0.03639,0.03639,0.03639,0\n"
      "tex t0\n"
      "tex t1\n"
      "tex t2\n"
      //"xmma_x2 r0,r1,discard, t1_bias,c0, t2_bias,c1\n"
      "mul_x2 r0,t1_bias,c0\n"
      "mul_x2 r1,t2_bias,c1\n"
      //"xmma discard,discard,r0, r0,c2_bx2, r1,c2_bx2\n"
      "mul r0, r0,c2_bx2\n"
      "mad r0, r1, c2_bx2, r0\n"
      //"xmma_x2 discard,discard,r1, t0,c4, -c3,c4\n"
      "mul r1, t0,c4\n"
      "add_x2 r1, r1, -c5\n"
      "add_sat r0, r0,r1\n";

    LPD3DXBUFFER pShader, pError;
	  HRESULT hr;
	  hr = D3DXAssembleShader(lowmem, strlen(lowmem),  NULL, NULL, &pShader, &pError);
	  if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "CWinRenderer::PreInit: Call to D3DXAssembleShader failed!" );
      CLog::Log(LOGERROR,  (char*)pError->GetBufferPointer());
      return 1;
    }
    //m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hLowMemShader);
    pShader->Release();
  }

  return 0;
}

void CWinRenderer::UnInit()
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
    m_pD3DDevice->DeletePixelShader(m_hLowMemShader);
    m_hLowMemShader = 0;
  }
}

void CWinRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;

  /* general stuff */
  RenderOSD();
}

void CWinRenderer::SetViewMode(int iViewMode)
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

void CWinRenderer::AutoCrop(bool bCrop)
{
  if (!m_YUVTexture[0][PLANE_Y]) return ;

  if (bCrop)
  {
    CSingleLock lock(g_graphicsContext);

    // apply auto-crop filter - only luminance needed, and we run vertically down 'n'
    // runs down the image.
    int min_detect = 8;                                // reasonable amount (what mplayer uses)
    int detect = (min_detect + 16)*m_iSourceWidth;     // luminance should have minimum 16
    D3DLOCKED_RECT lr;
    m_YUVTexture[0][PLANE_Y]->LockRect(0, &lr, NULL, 0);
    int total;
    // Crop top
    BYTE *s = (BYTE *)lr.pBits;
    g_stSettings.m_currentVideoSettings.m_CropTop = m_iSourceHeight/2;
    for (unsigned int y = 0; y < m_iSourceHeight/2; y++)
    {
      total = 0;
      for (unsigned int x = 0; x < m_iSourceWidth; x++)
        total += s[x];
      s += lr.Pitch;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropTop = y;
        break;
      }
    }
    // Crop bottom
    s = (BYTE *)lr.pBits + (m_iSourceHeight-1)*lr.Pitch;
    g_stSettings.m_currentVideoSettings.m_CropBottom = m_iSourceHeight/2;
    for (unsigned int y = (int)m_iSourceHeight; y > m_iSourceHeight/2; y--)
    {
      total = 0;
      for (unsigned int x = 0; x < m_iSourceWidth; x++)
        total += s[x];
      s -= lr.Pitch;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropBottom = m_iSourceHeight - y;
        break;
      }
    }
    // Crop left
    s = (BYTE *)lr.pBits;
    g_stSettings.m_currentVideoSettings.m_CropLeft = m_iSourceWidth/2;
    for (unsigned int x = 0; x < m_iSourceWidth/2; x++)
    {
      total = 0;
      for (unsigned int y = 0; y < m_iSourceHeight; y++)
        total += s[y * lr.Pitch];
      s++;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropLeft = x;
        break;
      }
    }
    // Crop right
    s = (BYTE *)lr.pBits + (m_iSourceWidth-1);
    g_stSettings.m_currentVideoSettings.m_CropRight= m_iSourceWidth/2;
    for (unsigned int x = (int)m_iSourceWidth-1; x > m_iSourceWidth/2; x--)
    {
      total = 0;
      for (unsigned int y = 0; y < m_iSourceHeight; y++)
        total += s[y * lr.Pitch];
      s--;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropRight = m_iSourceWidth - x;
        break;
      }
    }
    m_YUVTexture[0][PLANE_Y]->UnlockRect(0);
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

void CWinRenderer::RenderLowMem(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);

  int index = m_iYV12RenderBuffer;
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  for (int i = 0; i < 3; ++i)
  {
    m_pD3DDevice->SetTexture(i, m_YUVTexture[index][i]);
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  }

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
 
  m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );  // was m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE ); ???
  m_pD3DDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_TEX3 );
  m_pD3DDevice->SetPixelShader( m_hLowMemShader );

  //See RGB renderer for comment on this
  #define CHROMAOFFSET_HORIZ 0.25f

  // Render the image
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
      FLOAT tu3, tv3;
  };

  CUSTOMVERTEX verts[4] = 
  {
    {
      (float)rd.left                                                     , (float)rd.top, 0.0f, 1.0f,
      ((float)rs.left) / m_iSourceWidth                                  , ((float)rs.top) / m_iSourceHeight,
      ((float)rs.left / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.top / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1),
      ((float)rs.left / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.top / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1)
    },
    {
      (float)rd.right                                                     , (float)rd.top, 0.0f, 1.0f,
      ((float)rs.right) / m_iSourceWidth                                  , ((float)rs.top) / m_iSourceHeight,
      ((float)rs.right / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.top / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1),
      ((float)rs.right / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.top / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1)
    },
    {
      (float)rd.right                                                     , (float)rd.bottom, 0.0f, 1.0f,
      ((float)rs.right) / m_iSourceWidth                                  , ((float)rs.bottom) / m_iSourceHeight,
      ((float)rs.right / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.bottom / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1),
      ((float)rs.right / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1) , ((float)rs.bottom / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1)
    },
    {
      (float)rd.left                                                      , (float)rd.bottom, 0.0f, 1.0f,
      ((float)rs.left) / m_iSourceWidth                                   , ((float)rs.bottom) / m_iSourceHeight,
      ((float)rs.left / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1)  , ((float)rs.bottom / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1),
      ((float)rs.left / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceWidth>>1)  , ((float)rs.bottom / 2.0f + CHROMAOFFSET_HORIZ) / (m_iSourceHeight>>1)
    }
  };

  m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTexture(2, NULL);

  m_pD3DDevice->SetPixelShader( NULL );

}

void CWinRenderer::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);

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
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  YUVPLANES &planes = m_YUVTexture[index];

  if (planes[0] || planes[1] || planes[2])
    CLog::Log(LOGDEBUG, "Deleted YV12 texture (%i)", index);

  if (planes[0])
    SAFE_RELEASE(planes[0]);
  if (planes[1])
    SAFE_RELEASE(planes[1]);
  if (planes[2])
    SAFE_RELEASE(planes[2]);  
}

void CWinRenderer::ClearYV12Texture(int index)
{  
  YUVPLANES &planes = m_YUVTexture[index];
  D3DLOCKED_RECT rect;
  
  planes[0]->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
  memset(rect.pBits, 0,   rect.Pitch * m_iSourceHeight);
  planes[0]->UnlockRect(0);

  planes[1]->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
  memset(rect.pBits, 128, rect.Pitch * m_iSourceHeight>>1);
  planes[1]->UnlockRect(0);

  planes[2]->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
  memset(rect.pBits, 128, rect.Pitch * m_iSourceHeight>>1);
  planes[2]->UnlockRect(0);
}




bool CWinRenderer::CreateYV12Texture(int index)
{

  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);
  if (
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVTexture[index][0]) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth / 2, m_iSourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVTexture[index][1]) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth / 2, m_iSourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVTexture[index][2]))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 texture %i", index);
    return false;
  }
  ClearYV12Texture(index);
  CLog::Log(LOGDEBUG, "created yv12 texture %i", index);
  return true;
}


CPixelShaderRenderer::CPixelShaderRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CWinRenderer(pDevice)
{
}

bool CPixelShaderRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CWinRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;

  m_bConfigured = true;
  return true;
}


void CPixelShaderRenderer::Render(DWORD flags)
{
  // this is the low memory renderer
  CWinRenderer::RenderLowMem(flags);
  CWinRenderer::Render(flags);
}
