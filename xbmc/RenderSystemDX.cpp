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


#include "stdafx.h"

#ifdef HAS_DX

#include "Settings.h"
#include "RenderSystemDX.h"

CRenderSystemDX::CRenderSystemDX() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_DIRECTX;

  m_pD3D = NULL;
  m_pD3DDevice = NULL;
  m_hFocusWnd = NULL;
  m_hDeviceWnd = NULL;
  m_nBackBufferWidth = 0;
  m_nBackBufferHeight = 0;
  m_bFullScreenDevice = 0;
  m_bVSync = true;
  m_nDeviceStatus = S_OK;

  ZeroMemory(&m_D3DPP, sizeof(D3DPRESENT_PARAMETERS));
}

CRenderSystemDX::~CRenderSystemDX()
{
  DestroyRenderSystem();
}

bool CRenderSystemDX::InitRenderSystem()
{
  m_bVSync = true;
  m_iVSyncMode = 0;
  m_maxTextureSize = 8192;

  m_pD3D = NULL;

  m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
  if(m_pD3D == NULL)
    return false;

  CreateResources();
  
  return true;
}

bool CRenderSystemDX::ResetRenderSystem(int width, int height)
{
  m_nBackBufferWidth = width;
  m_nBackBufferHeight = height;

  CRect rc;
  rc.SetRect(0, 0, width, height);

  SetViewPort(rc);

  m_D3DPP.BackBufferWidth = m_nBackBufferWidth;
  m_D3DPP.BackBufferHeight = m_nBackBufferHeight;

  OnDeviceLost();
  OnDeviceReset();
  
  return true;
}

bool CRenderSystemDX::DestroyRenderSystem()
{
  SAFE_RELEASE(m_pD3D);
  SAFE_RELEASE(m_pD3DDevice);

  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemDX::CreateResources()
{
  if(!CreateDevice())
    return false;

  return true;
}

void CRenderSystemDX::DeleteResources()
{
  
}

void CRenderSystemDX::OnDeviceLost()
{
  // notify all objects
  for(unsigned int i = 0; i < m_vecEffects.size(); i++)
  {
    m_vecEffects[i]->OnLostDevice();
  }
}

void CRenderSystemDX::OnDeviceReset()
{
  // reset all required resources
  m_nDeviceStatus = m_pD3DDevice->Reset(&m_D3DPP);

  for(unsigned int i = 0; i < m_vecEffects.size(); i++)
  {
    m_vecEffects[i]->OnResetDevice();
  }
}

bool CRenderSystemDX::CreateDevice()
{
  // Code based on Ogre 3D engine

  HRESULT hr;

  if(m_pD3D == NULL)
    return false;

  if(m_hDeviceWnd == NULL)
    return false;

  D3DDEVTYPE devType = D3DDEVTYPE_HAL;

#if defined(DEBUG_PS) || defined (DEBUG_VS)
    devType = D3DDEVTYPE_REF
#endif

  ZeroMemory( &m_D3DPP, sizeof(D3DPRESENT_PARAMETERS) );
  m_D3DPP.Windowed					= true;
  m_D3DPP.SwapEffect				= D3DSWAPEFFECT_DISCARD;
  m_D3DPP.BackBufferCount			= 1;
  m_D3DPP.EnableAutoDepthStencil	= true;
  m_D3DPP.hDeviceWindow			= m_hDeviceWnd;
  m_D3DPP.BackBufferWidth			= m_nBackBufferWidth;
  m_D3DPP.BackBufferHeight			= m_nBackBufferHeight;

  if (m_bVSync)
  {
    m_D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  }
  else
  {
    m_D3DPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  }

  m_D3DPP.BackBufferFormat = D3DFMT_X8R8G8B8;

  // Try to create a 32-bit depth, 8-bit stencil
  if( FAILED( m_pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT,
    devType,  m_D3DPP.BackBufferFormat,  D3DUSAGE_DEPTHSTENCIL, 
    D3DRTYPE_SURFACE, D3DFMT_D24S8 )))
  {
    // Bugger, no 8-bit hardware stencil, just try 32-bit zbuffer 
    if( FAILED( m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT,
      devType,  m_D3DPP.BackBufferFormat,  D3DUSAGE_DEPTHSTENCIL, 
      D3DRTYPE_SURFACE, D3DFMT_D32 )))
    {
      // Jeez, what a naff card. Fall back on 16-bit depth buffering
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D16;
    }
    else
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D32;
  }
  else
  {
    if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( D3DADAPTER_DEFAULT, devType,
      m_D3DPP.BackBufferFormat, m_D3DPP.BackBufferFormat, D3DFMT_D24S8 ) ) )
    {
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D24S8; 
    } 
    else 
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D24X8; 
  }


  m_D3DPP.MultiSampleType = D3DMULTISAMPLE_NONE;
  m_D3DPP.MultiSampleQuality = 0;

  hr = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, devType, m_hFocusWnd,
    D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3DPP, &m_pD3DDevice );
  if (FAILED(hr))
  {
    // Try a second time, may fail the first time due to back buffer count,
    // which will be corrected down to 1 by the runtime
    hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, devType, m_hFocusWnd,
      D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_D3DPP, &m_pD3DDevice );
    if( FAILED( hr ) )
    {
      hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, devType, m_hFocusWnd,
        D3DCREATE_MIXED_VERTEXPROCESSING, &m_D3DPP, &m_pD3DDevice );
      if( FAILED( hr ) )
      {
        hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, devType, m_hFocusWnd,
          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_D3DPP, &m_pD3DDevice );
      }
      if(FAILED( hr ) )
        return false;
    }
  }

  m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

  m_bRenderCreated = true;

  return true;
}

bool CRenderSystemDX::PresentRenderImpl()
{
  HRESULT hr;

  if (!m_bRenderCreated)
    return false;

  if(m_nDeviceStatus != S_OK)
    return false;

  hr = m_pD3DDevice->Present( NULL, NULL, 0, NULL );

  if( D3DERR_DEVICELOST == hr )
    return false;

  if(FAILED(hr))
    return false;

  return true;

  return true;
}

bool CRenderSystemDX::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  if( FAILED( m_nDeviceStatus = m_pD3DDevice->TestCooperativeLevel() ) )
  {
    // The device has been lost but cannot be reset at this time. 
    // Therefore, rendering is not possible and we'll have to return 
    // and try again at a later time.
    if( m_nDeviceStatus == D3DERR_DEVICELOST )
    {
      CLog::Log(LOGINFO, "D3DERR_DEVICELOST");
      OnDeviceLost();
      return false;
    }

    // The device has been lost but it can be reset at this time. 
    if( m_nDeviceStatus == D3DERR_DEVICENOTRESET )
    {
      OnDeviceReset();
      if( FAILED(m_nDeviceStatus ) )
      {
        CLog::Log(LOGINFO, "m_pD3DDevice->Reset falied");
        return false;
      }
    }
  }

  if(FAILED (m_pD3DDevice->BeginScene()))
  {
    CLog::Log(LOGINFO, "m_pD3DDevice->EndScene() failed");
    return false;
  }

  return true;
}

bool CRenderSystemDX::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  if(m_nDeviceStatus != S_OK)
    return false;

  if(FAILED (m_pD3DDevice->EndScene()))
  {
    CLog::Log(LOGINFO, "m_pD3DDevice->EndScene() falied");
    return false;
  }

  return true;
}

bool CRenderSystemDX::ClearBuffers(DWORD color)
{
   HRESULT hr;

  if (!m_bRenderCreated)
    return false;

  if( FAILED( hr = m_pD3DDevice->Clear( 
    0, 
    NULL, 
    D3DCLEAR_TARGET,
    color, 
    1.0, 
    0 ) ) )
    return false;

  return true;
}

bool CRenderSystemDX::ClearBuffers(float r, float g, float b, float a)
{
  if (!m_bRenderCreated)
    return false;

  D3DXCOLOR color(r, g, b, a);

  return ClearBuffers((DWORD)color); 
}

bool CRenderSystemDX::IsExtSupported(CStdString strExt)
{
  return true;
}

bool CRenderSystemDX::PresentRender()
{
  if (!m_bRenderCreated)
    return false;
  
  bool result = PresentRenderImpl();
 
  return result;
}

void CRenderSystemDX::SetVSync(bool enable)
{
  SetVSyncImpl(enable);
}

void CRenderSystemDX::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{ 
  if (!m_bRenderCreated)
    return;

  // grab the viewport dimensions and location
  D3DVIEWPORT9 viewport;
  m_pD3DDevice->GetViewport(&viewport);
  float w = viewport.Width*0.5f;
  float h = viewport.Height*0.5f;

  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity
  // here.
  D3DXMATRIX mtxWorld;
  D3DXMatrixIdentity(&mtxWorld);
  m_pD3DDevice->SetTransform(D3DTS_WORLD, &mtxWorld);

  // camera view.  Multiply the Y coord by -1 then translate so that everything is relative to the camera
  // position.
  D3DXMATRIX flipY, translate, mtxView;
  D3DXMatrixScaling(&flipY, 1.0f, -1.0f, 1.0f);
  D3DXMatrixTranslation(&translate, -(viewport.X + w + offset.x), -(viewport.Y + h + offset.y), 2*h);
  D3DXMatrixMultiply(&mtxView, &translate, &flipY);
  m_pD3DDevice->SetTransform(D3DTS_VIEW, &mtxView);

  // projection onto screen space
  D3DXMATRIX mtxProjection;
  D3DXMatrixPerspectiveOffCenterLH(&mtxProjection, (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &mtxProjection);
}

bool CRenderSystemDX::TestRender()
{
  static DWORD lastTime = 0;
  static float delta = 0;

  DWORD thisTime = timeGetTime();

  if(thisTime - lastTime > 10)
  {
    lastTime = thisTime;
    delta++;
  }

  CLog::Log(LOGINFO, "Delta =  %d", delta);

  if(delta > m_nBackBufferWidth)
    delta = 0;

  LPDIRECT3DVERTEXBUFFER9 pVB = NULL;

  // A structure for our custom vertex type
  struct CUSTOMVERTEX
  {
    FLOAT x, y, z, rhw; // The transformed position for the vertex
    DWORD color;        // The vertex color
  };

  // Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

  // Initialize three vertices for rendering a triangle
  CUSTOMVERTEX vertices[] =
  {
    { delta + 100.0f,  50.0f, 0.5f, 1.0f, 0xffff0000, }, // x, y, z, rhw, color
    { delta+200.0f, 250.0f, 0.5f, 1.0f, 0xff00ff00, },
    {  delta, 250.0f, 0.5f, 1.0f, 0xff00ffff, },
  };

  // Create the vertex buffer. Here we are allocating enough memory
  // (from the default pool) to hold all our 3 custom vertices. We also
  // specify the FVF, so the vertex buffer knows what data it contains.
  if( FAILED( m_pD3DDevice->CreateVertexBuffer( 3 * sizeof( CUSTOMVERTEX ),
    0, D3DFVF_CUSTOMVERTEX,
    D3DPOOL_DEFAULT, &pVB, NULL ) ) )
  {
    return false;;
  }

  // Now we fill the vertex buffer. To do this, we need to Lock() the VB to
  // gain access to the vertices. This mechanism is required becuase vertex
  // buffers may be in device memory.
  VOID* pVertices;
  if( FAILED( pVB->Lock( 0, sizeof( vertices ), ( void** )&pVertices, 0 ) ) )
    return false;
  memcpy( pVertices, vertices, sizeof( vertices ) );
  pVB->Unlock();

  m_pD3DDevice->SetStreamSource( 0, pVB, 0, sizeof( CUSTOMVERTEX ) );
  m_pD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
  m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 1 );

  pVB->Release();

  return true;
}

void CRenderSystemDX::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::CalculateMaxTexturesize()
{
 
}

void CRenderSystemDX::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  D3DVIEWPORT9 d3dviewport;
  m_pD3DDevice->GetViewport(&d3dviewport);

  viewPort.x1 = (float)d3dviewport.X;
  viewPort.y2 = (float)d3dviewport.Y;
  viewPort.y1 = (float)d3dviewport.X + d3dviewport.Width;
  viewPort.x2 = (float)d3dviewport.Y + d3dviewport.Height;
}

void CRenderSystemDX::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  D3DVIEWPORT9 newviewport;

  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = (DWORD)viewPort.x1;
  newviewport.Y = (DWORD)viewPort.y1;
  newviewport.Width = (DWORD)(viewPort.x2 - viewPort.x1);
  newviewport.Height = (DWORD)(viewPort.y2 - viewPort.y1);
  m_pD3DDevice->SetViewport(&newviewport);
}


// The rendering system need to knows about effects created since they require
// reseting when the device is lost
bool CRenderSystemDX::CreateEffect(CStdString& name, ID3DXEffect** pEffect)
{
  HRESULT hr;

  hr = D3DXCreateEffect(m_pD3DDevice, name, name.length(), NULL, NULL, 0, NULL, pEffect, NULL );

  if(hr == S_OK)
  {
    m_vecEffects.push_back(*pEffect);
    return true;
  }

  return false;
}

void CRenderSystemDX::ReleaseEffect(ID3DXEffect* pEffect)
{
  for (vector<ID3DXEffect *>::iterator iter = m_vecEffects.begin(); 
    iter != m_vecEffects.end();
    ++iter)
  {
    if(*iter == pEffect)
    {
      m_vecEffects.erase(iter);
      return;
    }
  }
}

#endif
