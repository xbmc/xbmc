/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#ifdef HAS_DX

#include "WinDsRenderer.h"
#include "Util.h"
#include "Settings.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "MathUtils.h"
#include "DShowUtil/DShowUtil.h"
#include "Subtitles/DsSubtitleManager.h"
CWinDsRenderer::CWinDsRenderer()
{
}

CWinDsRenderer::~CWinDsRenderer()
{
  UnInit();
}

bool CWinDsRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_flags = flags;

  // need to recreate textures
  

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();
  return true;
}

void CWinDsRenderer::Reset()
{
}

void CWinDsRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CWinDsRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) return;
  
  CSingleLock lock(g_graphicsContext);

  ManageDisplay();
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (clear)
    pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);
}

unsigned int CWinDsRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;

  UnInit();
  m_resolution = RES_PAL_4x3;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;
  m_D3DVideoTexture = new CD3DTexture();
  return 0;
}


void CWinDsRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);
  //Need to fix this
  //m_D3DVideoTexture->Release();
  m_D3DMemorySurface.Release();
  m_bConfigured = false;
}

void CWinDsRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;
}

void CWinDsRenderer::AutoCrop(bool bCrop)
{
  //was that working for dshow

}

void CWinDsRenderer::PaintVideoTexture(CD3DTexture* videoTexture,IDirect3DSurface9* videoSurface)
{
  
  if (videoTexture)
    m_D3DVideoTexture = videoTexture;
  if (videoSurface)
    m_D3DMemorySurface = videoSurface;
}

void CWinDsRenderer::RenderDshowBuffer(DWORD flags)
{
  LPDIRECT3DDEVICE9 m_pD3DDevice = g_Windowing.Get3DDevice();
  m_pD3DDevice->SetPixelShader( NULL );
  CSingleLock lock(g_graphicsContext);
  
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  HRESULT hr;
  D3DSURFACE_DESC desc;
  if(!m_D3DVideoTexture || FAILED(m_D3DVideoTexture->GetLevelDesc(0, &desc)))
    return;

  float w = (float)desc.Width;
  float h = (float)desc.Height;
  struct CUSTOMVERTEX {
      float x, y, z;
	  float rhw; 
	  float tu, tv;
  };
  CUSTOMVERTEX verts[4] =
  {
    {//m_destRect    m_sourceRect
      m_destRect.x1      , m_destRect.y1 ,
      0.0f   , 1.0f, 
      0      , 0
    },
    {
      m_destRect.x2      , m_destRect.y1 ,
      0.0f   , 1.0f,
      1      , 0
    },
    {
      m_destRect.x2      , m_destRect.y2 ,
      0.0f   , 1.0f, 
      1      , 1
    },
    {
      m_destRect.x1      , m_destRect.y2 ,
      0.0f   , 1.0f,
      0      , 1
    },
  };
  for(int i = 0; i < countof(verts); i++)
  {
    verts[i].x -= 0.5;
    verts[i].y -= 0.5;
  }
  //D3DFVF_TEX1
  hr = m_pD3DDevice->SetTexture(0, m_D3DVideoTexture->Get());
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  hr = m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  
  hr = m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  hr = m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
  hr = m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
  hr = m_pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  hr = m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  hr = m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
  hr = m_pD3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); 
  hr = m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED); 
  hr = m_pD3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
  hr = m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(verts[0]));
  m_pD3DDevice->SetTexture(0, NULL);

  //Render Subtitles
  if (g_dllMpcSubs.Enabled())
    g_dllMpcSubs.Render(0,0,(int)m_destRect.Width(),(int)m_destRect.Height());
  if (FAILED(hr))
    CLog::Log(LOGDEBUG,"RenderDshowBuffer TextureCopy CWinDsRenderer::RenderDshowBuffer"); 
}

bool CWinDsRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinDsRenderer::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

CDsPixelShaderRenderer::CDsPixelShaderRenderer()
    : CWinDsRenderer()
{
}

bool CDsPixelShaderRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CWinDsRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;
  m_bConfigured = true;
  return true;
}


void CDsPixelShaderRenderer::Render(DWORD flags)
{
	CWinDsRenderer::Render(flags);
  CWinDsRenderer::RenderDshowBuffer(flags);
}

#endif
