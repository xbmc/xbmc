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
#include "../../stdafx.h"
#include "XBoxRenderer.h"
#include "../../application.h"
#include "../../util.h"
#include "../../XBVideoConfig.h"

//VBlank information
HANDLE g_eventVBlank=NULL;
void VBlankCallback(D3DVBLANKDATA *pData)
{
  PulseEvent(g_eventVBlank);
}


CXBoxRenderer::CXBoxRenderer(LPDIRECT3DDEVICE8 pDevice)
{
  m_pD3DDevice = pDevice;
  m_fSourceFrameRatio = 1.0f;
  m_iResolution = PAL_4x3;
  m_bFlipped = false;
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    m_pOSDYTexture[i] = NULL;
    m_pOSDATexture[i] = NULL;
    m_YTexture[i] = NULL;
    m_UTexture[i] = NULL;
    m_VTexture[i] = NULL;
  }
  m_hLowMemShader = 0;
  m_bPrepared=false;
  m_iAsyncFlipTime = 0;
  m_iFieldSync = FS_NONE;
  m_eventTexturesDone = CreateEvent(NULL,TRUE,TRUE,NULL);
  m_eventOSDDone = CreateEvent(NULL,TRUE,TRUE,NULL);

  if(!g_eventVBlank)
  {
    //Only do this on first run
    g_eventVBlank = CreateEvent(NULL,FALSE,FALSE,NULL);
    m_pD3DDevice->SetVerticalBlankCallback((D3DVBLANKCALLBACK)VBlankCallback);    
  }
}

CXBoxRenderer::~CXBoxRenderer()
{
  UnInit();

  CloseHandle(m_eventTexturesDone);
  CloseHandle(m_eventOSDDone);
}

//********************************************************************************************************
void CXBoxRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_pOSDYTexture[index])
  {
    m_pOSDYTexture[index]->Release();
    m_pOSDYTexture[index] = NULL;
  }
  if (m_pOSDATexture[index])
  {
    m_pOSDATexture[index]->Release();
    m_pOSDATexture[index] = NULL;
    CLog::Log(LOGDEBUG, "Deleted OSD textures (%i)", index);
  }
  m_iOSDTextureHeight[index] = 0;
}

void CXBoxRenderer::Setup_Y8A8Render()
{
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
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_Y8A8VERTEX );
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
    fast_memcpy(dst, src, w);
    fast_memcpy(dsta, srca, w);
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
    CLog::Log(LOGWARNING, "Zero dimensions specified to DrawAlpha, skipping");
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
  float EnlargeFactor = g_guiSettings.GetInt("Subtitles.EnlargePercentage") / 100.0f;

  const RECT& rv = g_graphicsContext.GetViewWindow();
  float xscale = EnlargeFactor * (float)(rv.right - rv.left) / (float)((g_settings.m_ResInfo[res].Overscan.right - g_settings.m_ResInfo[res].Overscan.left)) * ((float)m_iNormalDestWidth / (float)m_iOSDTextureWidth);
  float yscale = xscale * g_settings.m_ResInfo[res].fPixelRatio;
  osdRect.left = (float)rv.left + (float)(rv.right - rv.left - (float)w * xscale) / 2.0f;
  osdRect.right = osdRect.left + (float)w * xscale;
  float relbottom = ((float)(g_settings.m_ResInfo[res].iSubtitles - g_settings.m_ResInfo[res].Overscan.top)) / (g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top);
  osdRect.bottom = (float)rv.top + (float)(rv.bottom - rv.top) * relbottom;
  osdRect.top = osdRect.bottom - (float)h * yscale;

  RECT rc = { 0, 0, w, h };

  // flip buffers and wait for gpu
  int iOSDBuffer = ((m_iOSDBuffer + 1) % m_NumOSDBuffers);

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
    if (
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_L8, 0, &m_pOSDYTexture[iOSDBuffer]) ||
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_A8, 0, &m_pOSDATexture[iOSDBuffer])
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

  //Don't do anything here that would require locking of grapichcontext
  //it shouldn't be needed, and locking here will slow down prepared rendering
  if( m_NumOSDBuffers == 1 )
  {
    //Only do this when we have 1 buffer (that is in fullscreen)
    //tearing doesn't matter so much in gui + we have two buffers then
    if( WaitForSingleObject(m_eventOSDDone, 500) == WAIT_TIMEOUT )
    {
      //This should only happen if flippage wasn't called
      SetEvent(m_eventOSDDone);
    }
  }

  //We know the resources have been used at this point (or they are the second buffer, wich means they aren't in use anyways)
  //reset these so the gpu doesn't try to block on these
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
    fast_memset(lr.pBits, 0, lr.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    fast_memset(lra.pBits, 0, lra.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    //draw the osd/subs
    CopyAlpha(w, h, src, srca, stride, (BYTE*)lr.pBits, (BYTE*)lra.pBits, lr.Pitch);
  }
  m_pOSDYTexture[iOSDBuffer]->UnlockRect(0);
  m_pOSDATexture[iOSDBuffer]->UnlockRect(0);

  //set module variables to calculated values
  m_iOSDBuffer = iOSDBuffer;
  m_OSDRect = osdRect;
  m_OSDWidth = (float)w;
  m_OSDHeight = (float)h;

  m_OSDRendered = true;
}

//********************************************************************************************************
void CXBoxRenderer::RenderOSD()
{
  if (!m_pOSDYTexture[m_iOSDBuffer] || !m_pOSDATexture[m_iOSDBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;

  ResetEvent(m_eventOSDDone);

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  int buffer = m_iOSDBuffer;
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;
  DRAWRECT osdRect = m_OSDRect;
  //  if (!viewportRect.bottom && !viewportRect.right)
  //    return;

  // Set state to render the image
  m_pD3DDevice->SetTexture(0, m_pOSDYTexture[buffer]);
  m_pD3DDevice->SetTexture(1, m_pOSDATexture[buffer]);
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
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // Render the image
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
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventOSDDone);
  
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

void CXBoxRenderer::WaitForFlip()
{
  int iTmp = 0;
  m_bFlipped = false;
  while (!m_bFlipped && iTmp < 3000)
  {
    Sleep(1);
    iTmp++;
  }
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

  if( m_iFieldSync != FS_NONE )
  {
    //Make sure top is located on an odd scanline and bottom on an even one
    rd.top = rd.top & ~1;
    rd.bottom = rd.bottom & ~1;
  }

}


void CXBoxRenderer::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 2 buffers in windowed mode
  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (m_NumOSDBuffers != 1)
    {
      m_iOSDBuffer = 0;
      //delete second osd textures
      DeleteOSDTextures(1);
      m_NumOSDBuffers = 1;
      m_OSDWidth = m_OSDHeight = 0;
    }
    // Check our YV12 buffers.  Only need one of them...
    if (m_NumYV12Buffers < 1)
    { // need at least 1
      CreateYV12Texture(0);
    }
    if (m_NumYV12Buffers > 1)
    { // don't need more than 1
      DeleteYV12Texture(1);
    }
    m_iYV12DecodeBuffer = 0;
    m_NumYV12Buffers = 1;
  }
  else
  {
    if (m_NumOSDBuffers != 2)
    {
      m_NumOSDBuffers = 2;
      m_iOSDBuffer = 0;
      m_OSDWidth = m_OSDHeight = 0;
      // buffers will be created on demand in DrawAlpha() 
    }
    // Check the YV12 buffers - need two now
    if (m_NumYV12Buffers < 2)
    {
      CreateYV12Texture(1);
      if (m_NumYV12Buffers < 1) CreateYV12Texture(0);
      if (g_application.m_pPlayer && g_application.m_pPlayer->IsPaused())
        CopyYV12Texture(1);
      m_NumYV12Buffers = 2;
    }
  }
}

void CXBoxRenderer::ManageDisplay()
{
  const RECT& rv = g_graphicsContext.GetViewWindow();
  float fScreenWidth = (float)rv.right - rv.left;
  float fScreenHeight = (float)rv.bottom - rv.top;
  float fOffsetX1 = (float)rv.left;
  float fOffsetY1 = (float)rv.top;

  ManageTextures();

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

  // Work out if the framerate suits PAL50 or PAL60
  bool bPal60 = false;
  if (bUsingPAL && g_guiSettings.GetBool("MyVideos.PAL60Switching") && g_videoConfig.HasPAL60())
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

  // Work out if framesize suits 4:3 or 16:9
  // Uses the frame aspect ratio of 8/(3*sqrt(3)) (=1.53960) which is the optimal point
  // where the percentage of black bars to screen area in 4:3 and 16:9 is equal
  bool bWidescreen = false;
  if (g_guiSettings.GetBool("MyVideos.WidescreenSwitching"))
  { // allowed to switch
    if (bCanDoWidescreen && m_fSourceFrameRatio > 8.0f / (3.0f*sqrt(3.0f)))
      bWidescreen = true;
    else
      bWidescreen = false;
  }
  else
  { // user doesn't want us to switch - use the GUI setting
    bWidescreen = (g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].dwFlags & D3DPRESENTFLAG_WIDESCREEN) != 0;
  }

  // if we always upsample video to the GUI resolution then use it (with pal 60 if needed)
  // if we're not in fullscreen mode then use current resolution
  // if we're calibrating the video  then use current resolution
  if ( g_guiSettings.GetBool("VideoPlayer.UseGUIResolution") /*||
         (! ( g_graphicsContext.IsFullScreenVideo()|| g_graphicsContext.IsCalibrating())  )*/
     )
  {
    m_iResolution = g_graphicsContext.GetVideoResolution();
    // Check to see if we are using a PAL screen capable of PAL60
    if (bUsingPAL)
    {
      if (bPal60)
      {
        if (bWidescreen)
          m_iResolution = PAL60_16x9;
        else
          m_iResolution = PAL60_4x3;
      }
      else
      {
        if (bWidescreen)
          m_iResolution = PAL_16x9;
        else
          m_iResolution = PAL_4x3;
      }
    }
    else if (m_iResolution == NTSC_4x3 || m_iResolution == NTSC_16x9)
    {
      if (bWidescreen)
        m_iResolution = NTSC_16x9;
      else
        m_iResolution = NTSC_4x3;
    }
    else if (m_iResolution == HDTV_480p_4x3 || m_iResolution == HDTV_480p_16x9)
    {
      if (bWidescreen)
        m_iResolution = HDTV_480p_16x9;
      else
        m_iResolution = HDTV_480p_4x3;
    }
    // Change our screen resolution
    //  Sleep(1000);
    //   g_graphicsContext.SetVideoResolution(m_iResolution);
    return ;
  }

  // We are allowed to switch video resolutions, so we must
  // now decide which is the best resolution for the video we have
  if (bUsingPAL)  // PAL resolutions
  {
    // Currently does not allow HDTV solutions, as it is my beleif
    // that the XBox hardware only allows HDTV resolutions for NTSC systems.
    // this may need revising as more knowledge is obtained.
    if (bPal60)
    {
      if (bWidescreen)
        m_iResolution = PAL60_16x9;
      else
        m_iResolution = PAL60_4x3;
    }
    else    // PAL50
    {
      if (bWidescreen)
        m_iResolution = PAL_16x9;
      else
        m_iResolution = PAL_4x3;
    }
  }
  else      // NTSC resolutions
  {
    // Check if the picture warrants HDTV mode
    // And if HDTV modes (1080i and 720p) are available
    if ((m_iSourceHeight > 720 || m_iSourceWidth > 1280) && g_videoConfig.Has1080i())
    { //image suits 1080i if it's available
      m_iResolution = HDTV_1080i;
    }
    else if ((m_iSourceHeight > 480 || m_iSourceWidth > 720) && g_videoConfig.Has720p())                  //1080i is available
    { // image suits 720p if it is available
      m_iResolution = HDTV_720p;
    }
    else if ((m_iSourceHeight > 480 || m_iSourceWidth > 720) && g_videoConfig.Has1080i())
    { // image suits 720p and obviously 720p is unavailable
      m_iResolution = HDTV_1080i;
    }
    else  // either picture does not warrant HDTV, or HDTV modes are unavailable
    {
      if (g_videoConfig.Has480p())
      {
        if (bWidescreen)
          m_iResolution = HDTV_480p_16x9;
        else
          m_iResolution = HDTV_480p_4x3;
      }
      else
      {
        if (bWidescreen)
          m_iResolution = NTSC_16x9;
        else
          m_iResolution = NTSC_4x3;
      }
    }
  }

  // Finished - update our video resolution
  // Sleep(1000);
  //  g_graphicsContext.SetVideoResolution(m_iResolution);
}

void CXBoxRenderer::CreateTextures()
{
}

void CXBoxRenderer::SetupSubtitles()
{
  //set zoomamount to 1 temporary to get the default width of the destination viewwindow
  float zoomAmount = g_stSettings.m_fZoomAmount;
  g_stSettings.m_fZoomAmount = 1.0;
  ManageDisplay();
  g_stSettings.m_fZoomAmount = zoomAmount;
  m_iNormalDestWidth = rd.right - rd.left;
  m_iOSDTextureWidth = m_iNormalDestWidth;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;
}

unsigned int CXBoxRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps)
{
  m_fps = fps;
  m_iSourceWidth = width;
  m_iSourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(m_fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();
  SetupSubtitles();

  SetEvent(m_eventTexturesDone);
  SetEvent(m_eventOSDDone);

  return 0;
}


unsigned int CXBoxRenderer::DrawFrame(unsigned char *src[])
{
  OutputDebugString("video_draw_frame?? (should not happen)\n");
  return 0;
}

unsigned int CXBoxRenderer::GetImage(YV12Image *image)
{
  if (!image) return 0;

  //Don't do anything here that would require locking of grapichcontext
  //it shouldn't be needed, and locking here will slow down prepared rendering
  //Probably shouldn't even call blockonfence as it can clash with other blocking calls

  if( WaitForSingleObject(m_eventTexturesDone, 500) == WAIT_TIMEOUT )
  {
    //This should only happen if flippage wasn't called
    SetEvent(m_eventTexturesDone);
  }

  //We know the resources have been used at this point
  //reset these so the gpu doesn't try to block on these
  m_YTexture[m_iYV12DecodeBuffer]->Lock = 0;
  m_UTexture[m_iYV12DecodeBuffer]->Lock = 0;
  m_VTexture[m_iYV12DecodeBuffer]->Lock = 0;

  D3DLOCKED_RECT lr;

  image->width = m_iSourceWidth;
  image->height = m_iSourceHeight;

  m_YTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  image->plane[0] = (BYTE*)lr.pBits;
  image->stride[0] = lr.Pitch;

  m_UTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  image->plane[1] = (BYTE*)lr.pBits;
  image->stride[1] = lr.Pitch;

  m_VTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  image->plane[2] = (BYTE*)lr.pBits;
  image->stride[2] = lr.Pitch;

  return 1;
}

void CXBoxRenderer::ReleaseImage()
{
  m_YTexture[m_iYV12DecodeBuffer]->UnlockRect(0);
  m_UTexture[m_iYV12DecodeBuffer]->UnlockRect(0);
  m_VTexture[m_iYV12DecodeBuffer]->UnlockRect(0);
}

unsigned int CXBoxRenderer::PutImage(YV12Image *image)
{
  return 0;
}

void CXBoxRenderer::Update(bool bPauseDrawing)
{
  if (m_bConfigured)
  {
    CSingleLock lock(g_graphicsContext);
    bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
    g_graphicsContext.SetVideoResolution(bFullScreen ? m_iResolution : g_guiSettings.m_LookAndFeelResolution, !bFullScreen);
    RenderUpdate(false);
  }
}

void CXBoxRenderer::RenderUpdate(bool clear)
{
  if (!m_YTexture[0]) return ;

  CSingleLock lock(g_graphicsContext);
  ManageDisplay();
  if (clear)
    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );
  Render();

  //Kick commands out to the GPU, or we won't get the callback for textures being done
  m_pD3DDevice->KickPushBuffer();
}

void CXBoxRenderer::SetFieldSync(EFIELDSYNC mSync)
{
    EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.GetInterlaceMethod();
    if( mInt == VS_INTERLACEMETHOD_SYNC_AUTO )
    {
        //mSync = mSync;
    }
    else if( mInt == VS_INTERLACEMETHOD_SYNC_EVEN )
      mSync = FS_EVEN;
    else if( mInt == VS_INTERLACEMETHOD_SYNC_ODD )
      mSync = FS_ODD;
    else
      mSync = FS_NONE;

    if( m_iFieldSync != mSync )
    {
      //Need to lock here as this is not allowed to change while rendering
      CSingleLock lock(g_graphicsContext);      
      m_iFieldSync = mSync;
    }
}

void CXBoxRenderer::PrepareDisplay()
{
#ifdef _DEBUG
  static DWORD dwTime=0;
  static int iCount=0;
  DWORD dwTimeStamp = GetTickCount();
#endif

  if (g_graphicsContext.IsFullScreenVideo() )
  {    
    CSingleLock lock(g_graphicsContext);

    ManageDisplay();

    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

    Render();
    if (g_application.NeedRenderFullScreen())
    { // render our subtitles and osd
      g_application.RenderFullScreen();
    }
    m_pD3DDevice->KickPushBuffer();    
  }

  m_bPrepared = true;

#ifdef _DEBUG
  dwTime += GetTickCount() - dwTimeStamp;
  iCount++;

  if(iCount == 60)
  {
    CLog::DebugLog("RenderTime: %f ms average over last 60 frames", (float)dwTime / iCount);
    dwTime = 0;
    iCount = 0;
  }
#endif

}

void CXBoxRenderer::FlipPage(bool bAsync)
{
  if( bAsync )
  {
    if( CThread::ThreadHandle() == NULL ) CThread::Create();
    m_eventFrame.Set();
    return;
  }

  CSingleLock lock(g_graphicsContext);
  if( !m_bPrepared )
  {
    //This will prepare for rendering, ie swapping buffers and in fullscreen even rendering
    //it can have been done way earlier
    PrepareDisplay();
  }
  m_bPrepared=false;

  if (g_graphicsContext.IsFullScreenVideo() )
  {    

    //Make sure the push buffer is done before waiting for vblank, otherwise we can get tearing
    while( m_pD3DDevice->IsBusy() ) Sleep(1);

    D3DRASTER_STATUS mRaster;
    D3DFIELD_STATUS mStatus;
    m_pD3DDevice->GetDisplayFieldStatus(&mStatus);
    m_pD3DDevice->GetRasterStatus(&mRaster);

    bool bSync = (m_iFieldSync != FS_NONE && mStatus.Field != D3DFIELD_PROGRESSIVE);


    if( mRaster.InVBlank == 0 )
    {
      if( WaitForSingleObject(g_eventVBlank, 500) == WAIT_TIMEOUT )
        CLog::Log(LOGERROR, "CXBoxRenderer::FlipPage() - Waiting for vertical-blank timed out");

      m_pD3DDevice->GetDisplayFieldStatus(&mStatus);
    }

    //If we have interlaced video, we have to sync to only render on even fields
    if( bSync )
    {

      //If this was not the correct field. we have to wait for the next one.. damn
      if( (mStatus.Field == D3DFIELD_EVEN && m_iFieldSync == FS_ODD) ||
          (mStatus.Field == D3DFIELD_ODD && m_iFieldSync == FS_EVEN) )
      {
        if( WaitForSingleObject(g_eventVBlank, 500) == WAIT_TIMEOUT )
          CLog::Log(LOGERROR, "CXBoxRenderer::FlipPage() - Waiting for vertical-blank timed out");
      }
    }


    m_pD3DDevice->Present( NULL, NULL, NULL, NULL );

    //If textures hasn't been released earlier, release them here
    SetEvent(m_eventTexturesDone);
    SetEvent(m_eventOSDDone);    

  }  

  //if no osd was rendered before this call to flip_page make sure it doesn't get
  //rendered the next call to render not initialized by mplayer (like previewwindow + calibrationwindow);
  if (!m_OSDRendered)
  {
    m_OSDWidth = m_OSDHeight = 0;
  }
  m_OSDRendered = false;
  m_bFlipped = true;
  /*
   // when movie is running,
   // check the FPS again after 50 frames
   // after 50 frames mplayer has determined the REAL fps instead of just the one mentioned in the header
   // if its different we might need pal60
   m_lFrameCounter++;
   if (m_lFrameCounter==50)
   {
    char strFourCC[12];
    char strVideoCodec[256];
    unsigned int iWidth,iHeight;
    long tooearly, toolate;
    float fps;
    mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fps, &iWidth,&iHeight, &tooearly, &toolate);
    if (fps != m_fps)
    {
     ChooseBestResolution(fps);
    }
   }
   */
}

unsigned int CXBoxRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i = 0;

  //Don't do anything here that would require locking of grapichcontext
  //it shouldn't be needed, and locking here will slow down prepared rendering
  if( WaitForSingleObject(m_eventTexturesDone, 500) == WAIT_TIMEOUT )
  {
    //This should only happen if flippage wasn't called
    SetEvent(m_eventTexturesDone);
  }

  //We know the resources have been used at this point
  //reset these so the gpu doesn't try to block on these
  m_YTexture[m_iYV12DecodeBuffer]->Lock = 0;
  m_UTexture[m_iYV12DecodeBuffer]->Lock = 0;
  m_VTexture[m_iYV12DecodeBuffer]->Lock = 0;

  if (!m_YTexture[m_iYV12DecodeBuffer])
  {
    ++m_iYV12DecodeBuffer %= m_NumYV12Buffers;
  }

  D3DLOCKED_RECT lr;

  // copy Y
  m_YTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d = (BYTE*)lr.pBits + lr.Pitch * y + x;
  s = src[0];
  for (i = 0;i < h;i++)
  {
    fast_memcpy(d, s, w);
    s += stride[0];
    d += lr.Pitch;
  }
  m_YTexture[m_iYV12DecodeBuffer]->UnlockRect(0);

  w /= 2;h /= 2;x /= 2;y /= 2;

  // copy U
  m_UTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d = (BYTE*)lr.pBits + lr.Pitch * y + x;
  s = src[1];
  for (i = 0;i < h;i++)
  {
    fast_memcpy(d, s, w);
    s += stride[1];
    d += lr.Pitch;
  }
  m_UTexture[m_iYV12DecodeBuffer]->UnlockRect(0);

  // copy V
  m_VTexture[m_iYV12DecodeBuffer]->LockRect(0, &lr, NULL, 0);
  d = (BYTE*)lr.pBits + lr.Pitch * y + x;
  s = src[2];
  for (i = 0;i < h;i++)
  {
    fast_memcpy(d, s, w);
    s += stride[2];
    d += lr.Pitch;
  }
  m_VTexture[m_iYV12DecodeBuffer]->UnlockRect(0);

  return 0;
}

unsigned int CXBoxRenderer::PreInit()
{
  m_bConfigured = false;
  UnInit();
  m_iResolution = PAL_4x3;
  m_iOSDBuffer = 0;
  m_NumOSDBuffers = 0;
  m_NumYV12Buffers = 0;
  m_iYV12DecodeBuffer = 0;
  // setup the background colour
  m_clearColour = (g_guiSettings.GetInt("Videos.BlackBarColour") & 0xff) * 0x010101;
  // low memory pixel shader
  if (!m_hLowMemShader)
  {
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
  }

  return 0;
}

void CXBoxRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_bStop = true;
  m_eventFrame.PulseEvent();

  StopThread();

  // YV12 textures, subtitle and osd stuff
  for (int i = 0; i < 2; ++i)
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

void CXBoxRenderer::RenderBlank()
{ // clear the screen
  CSingleLock lock(g_graphicsContext);
  m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L );
  m_pD3DDevice->Present( NULL, NULL, NULL, NULL );
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
    g_stSettings.m_fZoomAmount = 1.0;
    // now we need to set g_stSettings.m_fPixelRatio so that
    // fOutputFrameRatio = 14:9.
    g_stSettings.m_fPixelRatio = (14.0f / 9.0f) / fSourceFrameRatio;
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
  if (!m_YTexture[0]) return;

  if (bCrop)
  {
    CSingleLock lock(g_graphicsContext);
    // apply auto-crop filter - only luminance needed, and we run vertically down 'n'
    // runs down the image.
    int min_detect = 8;                                // reasonable amount (what mplayer uses)
    int detect = (min_detect + 16)*m_iSourceWidth;     // luminance should have minimum 16
    D3DLOCKED_RECT lr;
    m_YTexture[0]->LockRect(0, &lr, NULL, 0);
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
    m_YTexture[0]->UnlockRect(0);
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

void CXBoxRenderer::RenderLowMem()
{
  //always render the buffer that is not currently being decoded to.
  int iRenderBuffer = ((m_iYV12DecodeBuffer + 1) % m_NumYV12Buffers);

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }
  ResetEvent(m_eventTexturesDone);

  m_pD3DDevice->SetTexture( 0, m_YTexture[iRenderBuffer]);
  m_pD3DDevice->SetTexture( 1, m_UTexture[iRenderBuffer]);
  m_pD3DDevice->SetTexture( 2, m_VTexture[iRenderBuffer]);

  for (int i = 0; i < 3; ++i)
  {
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  }

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
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
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone);

}

void CXBoxRenderer::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);
  LPDIRECT3DSURFACE8 oldRT;
  RECT saveSize = rd;
  rd.left = rd.top = 0;
  rd.right = width;
  rd.bottom = height;
  m_pD3DDevice->GetRenderTarget(&oldRT);
  m_pD3DDevice->SetRenderTarget(surface, NULL);
  RenderLowMem();
  rd = saveSize;
  m_pD3DDevice->SetRenderTarget(oldRT, NULL);
  oldRT->Release();
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CXBoxRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_YTexture[index])
  {
    m_YTexture[index]->Release();
    m_YTexture[index] = NULL;
  }
  if (m_UTexture[index])
  {
    m_UTexture[index]->Release();
    m_UTexture[index] = NULL;
  }
  if (m_VTexture[index])
  {
    m_VTexture[index]->Release();
    m_VTexture[index] = NULL;
    CLog::Log(LOGDEBUG, "Deleted YV12 texture (%i)", index);
  }
}

void CXBoxRenderer::ClearYV12Texture(int index)
{
  D3DLOCKED_RECT lr;
  m_YTexture[index]->LockRect(0, &lr, NULL, 0);
  fast_memset(lr.pBits, 0, lr.Pitch*m_iSourceHeight);
  m_YTexture[index]->UnlockRect(0);

  m_UTexture[index]->LockRect(0, &lr, NULL, 0);
  fast_memset(lr.pBits, 128, lr.Pitch*(m_iSourceHeight / 2));
  m_UTexture[index]->UnlockRect(0);

  m_VTexture[index]->LockRect(0, &lr, NULL, 0);
  fast_memset(lr.pBits, 128, lr.Pitch*(m_iSourceHeight / 2));
  m_VTexture[index]->UnlockRect(0);
}

void CXBoxRenderer::CopyYV12Texture(int dest)
{
  CSingleLock lock(g_graphicsContext);
  int src = 1-dest;
  D3DLOCKED_RECT lr_src, lr_dest;
  m_YTexture[src]->LockRect(0, &lr_src, NULL, 0);
  m_YTexture[dest]->LockRect(0, &lr_dest, NULL, 0);
  fast_memcpy(lr_dest.pBits, lr_src.pBits, lr_dest.Pitch*m_iSourceHeight);
  m_YTexture[dest]->UnlockRect(0);
  m_YTexture[src]->UnlockRect(0);

  m_UTexture[src]->LockRect(0, &lr_src, NULL, 0);
  m_UTexture[dest]->LockRect(0, &lr_dest, NULL, 0);
  fast_memcpy(lr_dest.pBits, lr_src.pBits, lr_dest.Pitch*(m_iSourceHeight / 2));
  m_UTexture[dest]->UnlockRect(0);
  m_UTexture[src]->UnlockRect(0);

  m_VTexture[src]->LockRect(0, &lr_src, NULL, 0);
  m_VTexture[dest]->LockRect(0, &lr_dest, NULL, 0);
  fast_memcpy(lr_dest.pBits, lr_src.pBits, lr_dest.Pitch*(m_iSourceHeight / 2));
  m_VTexture[dest]->UnlockRect(0);
  m_VTexture[src]->UnlockRect(0);
}

bool CXBoxRenderer::CreateYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);
  if (
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_LIN_L8, 0, &m_YTexture[index]) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth / 2, m_iSourceHeight / 2, 1, 0, D3DFMT_LIN_L8, 0, &m_UTexture[index]) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth / 2, m_iSourceHeight / 2, 1, 0, D3DFMT_LIN_L8, 0, &m_VTexture[index]))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 texture %i", index);
    return false;
  }
  ClearYV12Texture(index);

  // create the field-based textures for interlacing as well
  D3DLOCKED_RECT lr;
  // Y
  m_YTexture[index]->LockRect(0, &lr, NULL, 0);
  m_YFieldPitch = lr.Pitch;
  XGSetTextureHeader(m_iSourceWidth + m_YFieldPitch, m_iSourceHeight / 2, 1, 0, D3DFMT_LIN_L8, 0, &m_YFieldTexture[index], 0, lr.Pitch * 2);
  m_YFieldTexture[index].Register(lr.pBits);
  m_YTexture[index]->UnlockRect(0);
  // U
  m_UTexture[index]->LockRect(0, &lr, NULL, 0);
  m_UVFieldPitch = lr.Pitch;
  XGSetTextureHeader(m_iSourceWidth / 2 + m_UVFieldPitch, m_iSourceHeight / 4, 1, 0, D3DFMT_LIN_L8, 0, &m_UFieldTexture[index], 0, lr.Pitch * 2);
  m_UFieldTexture[index].Register(lr.pBits);
  m_UTexture[index]->UnlockRect(0);
  // V
  m_VTexture[index]->LockRect(0, &lr, NULL, 0);
  XGSetTextureHeader(m_iSourceWidth / 2 + m_UVFieldPitch, m_iSourceHeight / 4, 1, 0, D3DFMT_LIN_L8, 0, &m_VFieldTexture[index], 0, lr.Pitch * 2);
  m_VFieldTexture[index].Register(lr.pBits);
  m_VTexture[index]->UnlockRect(0);

  CLog::Log(LOGDEBUG, "created yv12 texture %i", index);
  return true;
}

void CXBoxRenderer::TextureCallback(DWORD dwContext)
{
  SetEvent((HANDLE)dwContext);
}

void CXBoxRenderer::Process()
{
  DWORD dwTimeStamp = 0;

  DWORD dwFlipTime = 300;
  DWORD dwFlipCount = 30;  

  m_iAsyncFlipTime = 10; //Just a guess to what delay we have

  SetPriority(THREAD_PRIORITY_TIME_CRITICAL);
  SetName("AsyncRenderer");
  while( !m_bStop )
  {
    //Wait for new frame or an stop event
    m_eventFrame.Wait();
    if( m_bStop )
    {
      //Stop was signaled, exit thread
      return;
    }

    DWORD dwTimeStamp = GetTickCount();

    try
    {

      CSingleLock lock(g_graphicsContext);
      CXBoxRenderer::FlipPage(false);
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "CXBoxRenderer::Process() - Exception thrown in flippage");
    }    

    dwFlipTime += GetTickCount() - dwTimeStamp;
    dwFlipCount++;

    //Keep flipcount around 120-240 to let later frames have more impact.
    //this is needed should user change framesync during playback
    if( dwFlipCount >= 240 )
    {
      dwFlipTime /= 2;
      dwFlipCount /= 2;
    }

    m_iAsyncFlipTime = (int)(dwFlipTime / dwFlipCount);

  }
}

