/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "stdafx.h"
#include "ComboRenderer.h"
#include "Application.h"
#include "Settings.h"

CComboRenderer::CComboRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
  m_bHasDimView = false;
  m_RGBSurface[0] = NULL;
  m_RGBSurface[1] = NULL;
  m_YUY2Texture[0] = NULL;
  m_YUY2Texture[1] = NULL;
  m_hPixelShader = 0;
  m_iYUY2RenderBuffer = 0;
  m_iYUY2Buffers = 2;
}

void CComboRenderer::DeleteYUY2Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_RGBSurface[index])
    SAFE_RELEASE(m_RGBSurface[index]);

  if (m_YUY2Texture[index])
  {
    SAFE_RELEASE(m_YUY2Texture[index]);
    CLog::Log(LOGDEBUG, "Deleted yuy2 textures (%d)", index);
  }
}

void CComboRenderer::ClearYUY2Texture(int index)
{
  D3DLOCKED_RECT lr;
  // Clear our RGB/YUY2 texture
  m_RGBSurface[index]->LockRect(&lr, NULL, 0);
  memset(lr.pBits, 0x00800080, lr.Pitch*m_iSourceHeight);
  m_RGBSurface[index]->UnlockRect();
}

bool CComboRenderer::CreateYUY2Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  DeleteYUY2Texture(index);
  // Create our textures...

  if (D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_YUY2, 0, &m_YUY2Texture[index]))
    return false;

  m_YUY2Texture[index]->GetSurfaceLevel(0, &m_RGBSurface[index]);
  m_RGBSurface[index]->Format &= ~D3DFORMAT_FORMAT_MASK;
  m_RGBSurface[index]->Format |= D3DFMT_LIN_A8R8G8B8 << D3DFORMAT_FORMAT_SHIFT;

  m_RGBSurface[index]->Size &= ~D3DSIZE_WIDTH_MASK;
  m_RGBSurface[index]->Size |= ((m_iSourceWidth>>1) - 1) & D3DSIZE_WIDTH_MASK;

  ClearYUY2Texture(index);
  CLog::Log(LOGDEBUG, "Created yuy2 textures (%d)", index);
  return true;
}

void CComboRenderer::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 0 buffers in windowed mode
  if (g_graphicsContext.IsFullScreenVideo())
  {
    m_iYUY2Buffers = 2;

    if (!m_RGBSurface[0] && m_iYUY2Buffers > 0)
      CreateYUY2Texture(0);
    if (!m_RGBSurface[1] && m_iYUY2Buffers > 1)
      CreateYUY2Texture(1);

    if(m_iYV12RenderBuffer >= m_iYUY2Buffers)
      m_iYV12RenderBuffer = 0;
  }
  else
  {
    m_iYUY2Buffers = 0;

    if (m_RGBSurface[0])
      DeleteYUY2Texture(0);

    if (m_RGBSurface[1])
      DeleteYUY2Texture(1);
  }

  CXBoxRenderer::ManageTextures();
}

void CComboRenderer::ManageDisplay()
{
  const RECT& rv = g_graphicsContext.GetViewWindow();
  float fScreenWidth = (float)rv.right - rv.left;
  float fScreenHeight = (float)rv.bottom - rv.top;
  float fOffsetX1 = (float)rv.left;
  float fOffsetY1 = (float)rv.top;
  float fPixelRatio = g_settings.m_fPixelRatio;
  float fMaxScreenWidth = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iWidth;
  float fMaxScreenHeight = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iHeight;
  if (fOffsetX1 < 0) fOffsetX1 = 0;
  if (fOffsetY1 < 0) fOffsetY1 = 0;
  if (fScreenWidth + fOffsetX1 > fMaxScreenWidth) fScreenWidth = fMaxScreenWidth - fOffsetX1;
  if (fScreenHeight + fOffsetY1 > fMaxScreenHeight) fScreenHeight = fMaxScreenHeight - fOffsetY1;

  // Correct for HDTV_1080i -> 540p
  if (GetResolution() == HDTV_1080i)
  {
    fOffsetY1 /= 2;
    fScreenHeight /= 2;
    fPixelRatio *= 2;
  }

  // source rect
  rs.left = g_settings.m_currentVideoSettings.m_CropLeft;
  rs.top = g_settings.m_currentVideoSettings.m_CropTop;
  rs.right = m_iSourceWidth - g_settings.m_currentVideoSettings.m_CropRight;
  rs.bottom = m_iSourceHeight - g_settings.m_currentVideoSettings.m_CropBottom;

  CalcNormalDisplayRect(fOffsetX1, fOffsetY1, fScreenWidth, fScreenHeight, GetAspectRatio() * fPixelRatio, g_settings.m_fZoomAmount);

  // check whether we need to alter our source rect
  if (rd.left < fOffsetX1 || rd.right > fOffsetX1 + fScreenWidth)
  {
    // wants to be wider than we allow, so fix
    float fRequiredWidth = (float)rd.right - rd.left;
    if (rs.right <= rs.left) rs.right = rs.left+1;
    float fHorizScale = fRequiredWidth / (float)(rs.right - rs.left);
    float fNewWidth = fScreenWidth / fHorizScale;
    rs.left = (rs.right - rs.left - (int)fNewWidth) / 2;
    rs.right = rs.left + (int)fNewWidth;
    rd.left = (int)fOffsetX1;
    rd.right = (int)(fOffsetX1 + fScreenWidth);
  }
  if (rd.top < fOffsetY1 || rd.bottom > fOffsetY1 + fScreenHeight)
  {
    // wants to be wider than we allow, so fix
    float fRequiredHeight = (float)rd.bottom - rd.top;
    if (rs.bottom <= rs.top) rs.bottom = rs.top+1;
    float fVertScale = fRequiredHeight / (float)(rs.bottom - rs.top);
    float fNewHeight = fScreenHeight / fVertScale;
    rs.top = (rs.bottom - rs.top - (int)fNewHeight) / 2;
    rs.bottom = rs.top + (int)fNewHeight;
    rd.top = (int)fOffsetY1;
    rd.bottom = (int)(fOffsetY1 + fScreenHeight);
  }
}

bool CComboRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CXBoxRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;

  m_bConfigured = true;
  return true;
}

void CComboRenderer::Update(bool bPauseDrawing)
{
  if(!m_bConfigured) return;
  CSingleLock lock(g_graphicsContext);

  if(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating())
    m_pD3DDevice->EnableOverlay(!bPauseDrawing);
  else
    m_pD3DDevice->EnableOverlay(FALSE);

  CXBoxRenderer::Update(bPauseDrawing);
}

void CComboRenderer::FlipPage(int source)
{
  if(m_iYUY2Buffers)
    m_iYUY2RenderBuffer = ++m_iYUY2RenderBuffer % m_iYUY2Buffers;

  CXBoxRenderer::FlipPage(source);
}

void CComboRenderer::YV12toYUY2()
{
  int index = m_iYV12RenderBuffer;
  if (!m_RGBSurface[m_iYUY2RenderBuffer]) return;

  /* if we have dimmed our texture, don't overwrite it */
  if( g_application.IsInScreenSaver() && m_bHasDimView ) return;

  if( WaitForSingleObject(m_eventTexturesDone[index], 500) == WAIT_TIMEOUT )
    CLog::Log(LOGWARNING, __FUNCTION__" - Timeout waiting for texture %d", index);

  // Do the YV12 -> YUY2 conversion.
  // ALWAYS use buffer 0 in this case (saves 12 bits/pixel)
  m_pD3DDevice->SetTexture( 0, m_YUVTexture[index][FIELD_FULL][PLANE_Y] );
  m_pD3DDevice->SetTexture( 1, m_YUVTexture[index][FIELD_FULL][PLANE_U] );
  m_pD3DDevice->SetTexture( 2, m_YUVTexture[index][FIELD_FULL][PLANE_Y] );
  m_pD3DDevice->SetTexture( 3, m_YUVTexture[index][FIELD_FULL][PLANE_V] );

  for (int i = 0; i < 4; ++i)
  {
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_POINT );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_POINT );
  }
  // U and V need to use linear filtering, as they're being doubled vertically
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_YUYVVERTEX );
  m_pD3DDevice->SetPixelShader( m_hPixelShader );
  // Render the image
  LPDIRECT3DSURFACE8 pOldRT;
  m_pD3DDevice->GetRenderTarget(&pOldRT);
  m_pD3DDevice->SetRenderTarget(m_RGBSurface[m_iYUY2RenderBuffer], NULL);

  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)1.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)0.5f, (float)0.5f);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)0.0f, (float)0.0f, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 1.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth / 2.0f + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)m_iSourceWidth / 2.0f + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth / 2.0f, (float)0.0f, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 1.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth / 2.0f + 0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth + 0.5f, (float)m_iSourceHeight + 0.5f);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)m_iSourceWidth / 2.0f + 0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth / 2.0f, (float)m_iSourceHeight, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)1.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)0.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)0.0f, (float)m_iSourceHeight, 0, 1.0f );
  m_pD3DDevice->End();

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTexture(2, NULL);
  m_pD3DDevice->SetTexture(3, NULL);

  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetPixelShader( NULL );
  m_pD3DDevice->SetRenderTarget(pOldRT, NULL);

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

  pOldRT->Release();

  //Okey, when the gpu is done with the textures here, they are free to be modified again
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone[index]);

  m_pD3DDevice->KickPushBuffer();

  m_bHasDimView = false;
}

void CComboRenderer::Render(DWORD flags)
{
  if ( m_RGBSurface[m_iYUY2RenderBuffer] == NULL )
  {
    RenderLowMem(flags);
  }
  else
  {
    YV12toYUY2();
    CheckScreenSaver();

    /* clear target area, otherwise we won't get any picture */
    D3DRECT target;
    target.x1 = rd.left;
    target.x2 = rd.right;
    target.y1 = rd.top;
    target.y2 = rd.bottom;
    m_pD3DDevice->Clear( 1L, &target, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

    // Don't render if we are waiting an overlay event
    while (!m_pD3DDevice->GetOverlayUpdateStatus()) Sleep(1);

    LPDIRECT3DSURFACE8 pSurface;
    m_YUY2Texture[m_iYUY2RenderBuffer]->GetSurfaceLevel(0, &pSurface);
    m_pD3DDevice->UpdateOverlay( pSurface, &rs, &rd, TRUE, m_clearColour );
    pSurface->Release();
  }

  CXBoxRenderer::Render(flags | RENDER_FLAG_NOOSDALPHA);
}

unsigned int CComboRenderer::PreInit()
{
  CXBoxRenderer::PreInit();
  // May have to set clearColour non-zero in future for HW overlays method?
//  if (!m_clearColour)
//    m_clearColour = 0x010001;

  m_bHasDimView = false;
  // Create the pixel shader
  if (!m_hPixelShader)
  {
    // shader to interleave separate Y U and V planes into a single YUY2 output
    const char* shader =
      "xps.1.1\n"
      "def c0,1,0,0,0\n"
      "def c1,0,1,0,0\n"
      "def c2,0,0,1,0\n"
      "def c3,0,0,0,1\n"
      "tex t0\n" // Y1 plane (Y plane)
      "tex t1\n" // U plane
      "tex t2\n" // Y2 plane (Y plane shifted 1 pixel to the left)
      "tex t3\n" // V plane
      "xmma discard,discard,r0, t0, c0, t1, c1\n"
      "xmma discard,discard,r1, t2, c2, t3.b, c3\n"
      "add r0, r0, r1\n";
    XGBuffer* pShader;
    XGAssembleShader("XBMCShader", shader, strlen(shader), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hPixelShader);
    pShader->Release();
  }

  m_pD3DDevice->EnableOverlay(TRUE);
  return 0;
}

void CComboRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_pD3DDevice->EnableOverlay(FALSE);
  DeleteYUY2Texture(0);
  DeleteYUY2Texture(1);

  if (m_hPixelShader)
  {
    m_pD3DDevice->DeletePixelShader(m_hPixelShader);
    m_hPixelShader = 0;
  }

  CXBoxRenderer::UnInit();
}

void CComboRenderer::CheckScreenSaver()
{
  if (g_application.IsInScreenSaver() && !m_bHasDimView)
  {
    D3DLOCKED_RECT lr;
    float fAmount = (float)g_guiSettings.GetInt("screensaver.dimlevel") / 100.0f;
    if ( D3D_OK == m_YUY2Texture[m_iYUY2RenderBuffer]->LockRect(0, &lr, NULL, 0 ))
    {
      // Drop brightness of current surface to 20%
      DWORD strideScreen = lr.Pitch;
      for (DWORD y = 0; y < UINT (rs.top + rs.bottom); y++)
      {
        BYTE *pDest = (BYTE*)lr.pBits + strideScreen * y;
        for (DWORD x = 0; x < UINT ((rs.left + rs.right) >> 1); x++)
        {
          pDest[0] = BYTE (pDest[0] * fAmount); // Y1
          pDest[1] = BYTE ((pDest[1] - 128) * fAmount + 128); // U (with 128 shift!)
          pDest[2] = BYTE (pDest[2] * fAmount); // Y2
          pDest[3] = BYTE ((pDest[3] - 128) * fAmount + 128); // V (with 128 shift!)
          pDest += 4;
        }
      }
      m_YUY2Texture[m_iYUY2RenderBuffer]->UnlockRect(0);

    }
    m_bHasDimView = true;
  }
}

void CComboRenderer::SetupScreenshot()
{
  if (!g_graphicsContext.IsFullScreenVideo())
    return;
  CSingleLock lock(g_graphicsContext);
  // first, grab the current overlay texture and convert it to RGB
  LPDIRECT3DTEXTURE8 pRGB = NULL;
  if (D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &pRGB))
  {
    return ;
  }
  D3DLOCKED_RECT lr, lr2;
  m_YUY2Texture[m_iYUY2RenderBuffer]->LockRect(0, &lr, NULL, 0);
  pRGB->LockRect(0, &lr2, NULL, 0);
  // convert to RGB via software converter
  BYTE *s = (BYTE *)lr.pBits;
  LONG *d = (LONG *)lr2.pBits;
  LONG dpitch = lr2.Pitch / 4;
  for (unsigned int y = 0; y < m_iSourceHeight; y++)
  {
    for (unsigned int x = 0; x < m_iSourceWidth; x += 2)
    {
      d[x] = YUV2RGB(s[2 * x], s[2 * x + 1], s[2 * x + 3]);
      d[x + 1] = YUV2RGB(s[2 * x + 2], s[2 * x + 1], s[2 * x + 3]);
    }
    s += lr.Pitch;
    d += dpitch;
  }
  m_YUY2Texture[m_iYUY2RenderBuffer]->UnlockRect(0);
  pRGB->UnlockRect(0);
  // ok - now lets dump the RGB texture to a file to test
  // ok, now this needs to be rendered to the screen
  m_pD3DDevice->SetTexture( 0, pRGB);

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE /*TRUE*/ );
  m_pD3DDevice->SetVertexShader( FVF_RGBVERTEX );
  // Render the image
  m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f); // fix texel align
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.bottom, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.bottom, 0, 1.0f );
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetScissors(0, FALSE, NULL );

  RenderOSD();

  if (g_application.NeedRenderFullScreen())
  { // render our subtitles and osd
    g_application.RenderFullScreen();
  }

  m_pD3DDevice->Present( NULL, NULL, NULL, NULL );

  while (pRGB->IsBusy()) Sleep(1);
  pRGB->Release();

  return ;
}

LONG CComboRenderer::YUV2RGB(BYTE y, BYTE u, BYTE v)
{
  YUVCOEF &coef = yuv_coef_bt601;
  YUVRANGE &range = yuv_range_lim;

  // normalize
  float Yp = (y - range.y_min) * 255.0f / (range.y_max - range.y_min);
  float Up = (u - range.u_min) * 255.0f / (range.u_max - range.u_min) - 127.5f;
  float Vp = (v - range.v_min) * 255.0f / (range.v_max - range.v_min) - 127.5f;

  // recalculate
  float R = Yp + coef.r_up * Up + coef.r_vp * Vp;
  float G = Yp + coef.g_up * Up + coef.g_vp * Vp;
  float B = Yp + coef.b_up * Up + coef.b_vp * Vp;

  // clamp
  R = CLAMP(R, 0.0f, 255.0f);
  G = CLAMP(G, 0.0f, 255.0f);
  B = CLAMP(B, 0.0f, 255.0f);

  return ((int)R << 16) + ((int)G << 8) + (int)B;
}
