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
#include "StreamsManager.h"
#include "IPaintCallback.h"

CWinDsRenderer::CWinDsRenderer():
  m_bConfigured(false),
  m_D3DVideoTexture(NULL),
  m_D3DMemorySurface(NULL),
  m_paintCallback(NULL)
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

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);

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
  return 0;
}


void CWinDsRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);
  m_D3DMemorySurface = NULL;
  m_D3DVideoTexture = NULL;

  m_bConfigured = false;
}

void CWinDsRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) 
    return;
}

void CWinDsRenderer::AutoCrop(bool bCrop)
{
}

void CWinDsRenderer::RegisterDsCallback(IPaintCallback *callback)
{
  m_paintCallback = callback;
}
void CWinDsRenderer::UnRegisterDsCallback()
{
  m_paintCallback = NULL;
}
void CWinDsRenderer::RenderDShowBuffer( DWORD flags )
{
  CSingleLock lock(g_graphicsContext);
  if (m_paintCallback)
  {
    m_paintCallback->OnPaint(m_destRect);
  }
  //RenderVideoTexture();
  RenderSubtitleTexture();
}

void CWinDsRenderer::RenderSubtitleTexture()
{
  if (CStreamsManager::getSingleton()->SubtitleManager)
  {

    LPDIRECT3DDEVICE9 m_pD3DDevice = g_Windowing.Get3DDevice();
    if (! m_pD3DDevice)
      return;

    Com::SmartPtr<IDirect3DTexture9> pTexture;
    Com::SmartRect pSrc, pDst, pSize(m_destRect.x1, m_destRect.y1, m_destRect.x2, m_destRect.y2);
    //Com::SmartRect pSrc, pDst, pSize;
    //GetWindowRect(g_Windowing.GetHwnd(), &pSize);
    if (SUCCEEDED(CStreamsManager::getSingleton()->SubtitleManager->GetTexture(pTexture, pSrc, pDst, pSize)))
    {
      do
      {
        CSingleLock lock(g_graphicsContext);

        // HACK
        //pDst.top -= m_destRect.y1;
        //pDst.bottom -= m_destRect.y1;

        D3DSURFACE_DESC d3dsd;
        ZeroMemory(&d3dsd, sizeof(d3dsd));
        if(FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/)
	        break;

        float w = (float)d3dsd.Width;
        float h = (float)d3dsd.Height;

        struct
        {
	        float x, y, z, rhw;
	        float tu, tv;
        }
        pVertices[] =
        {
	        {(float)pDst.left, (float)pDst.top, 0.5f, 2.0f, (float)pSrc.left / w, (float)pSrc.top / h},
	        {(float)pDst.right, (float)pDst.top, 0.5f, 2.0f, (float)pSrc.right / w, (float)pSrc.top / h},
	        {(float)pDst.left, (float)pDst.bottom, 0.5f, 2.0f, (float)pSrc.left / w, (float)pSrc.bottom / h},
	        {(float)pDst.right, (float)pDst.bottom, 0.5f, 2.0f, (float)pSrc.right / w, (float)pSrc.bottom / h},
        };

        HRESULT hr = S_OK;

        hr = m_pD3DDevice->SetTexture(0, pTexture);

        DWORD abe, sb, db;
        hr = m_pD3DDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe);
        hr = m_pD3DDevice->GetRenderState(D3DRS_SRCBLEND, &sb);
        hr = m_pD3DDevice->GetRenderState(D3DRS_DESTBLEND, &db);

        hr = m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        hr = m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        hr = m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        hr = m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        hr = m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
        hr = m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

        hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        hr = m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        hr = m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        hr = m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        hr = m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

        hr = m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        hr = m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        hr = m_pD3DDevice->SetPixelShader(NULL);

        hr = m_pD3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        hr = m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

        hr = m_pD3DDevice->SetTexture(0, NULL);

        hr = m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
        hr = m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, sb);
        hr = m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, db);

      } while(0);
    }
  }
}

void CWinDsRenderer::RenderVideoTexture()
{
  CSingleLock lock(g_graphicsContext);

  HRESULT hr;
  D3DSURFACE_DESC desc;

  LPDIRECT3DDEVICE9 m_pD3DDevice = g_Windowing.Get3DDevice();
  if (! m_pD3DDevice)
    return;

  if (!m_D3DVideoTexture || FAILED(m_D3DVideoTexture->GetLevelDesc(0, &desc)))
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
    {
      m_destRect.x1, m_destRect.y1, 0.0f, 1.0f, 0, 0
    },
    {
      m_destRect.x2, m_destRect.y1, 0.0f, 1.0f, 1, 0
    },
    {
      m_destRect.x2 ,m_destRect.y2, 0.0f, 1.0f, 1, 1
    },
    {
      m_destRect.x1 ,m_destRect.y2, 0.0f, 1.0f, 0, 1
    },
  };

  for(int i = 0; i < countof(verts); i++)
  {
    verts[i].x -= 0.5;
    verts[i].y -= 0.5;
  }

  hr = m_pD3DDevice->SetTexture(0, m_D3DVideoTexture);

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
  m_pD3DDevice->SetPixelShader( NULL );
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

bool CWinDsRenderer::Supports( ERENDERFEATURE method )
{
  //TODO: Implement that
  return false;
}

CDsPixelShaderRenderer::CDsPixelShaderRenderer(bool isevr)
    : CWinDsRenderer()
{
  m_bIsEvr = isevr;
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
  CWinDsRenderer::RenderDShowBuffer(flags);
}

#endif
