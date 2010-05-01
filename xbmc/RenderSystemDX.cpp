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

#include "Settings.h"
#include "RenderSystemDX.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "GUIWindowManager.h"
#include "SingleLock.h"
#include "D3DResource.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "utils/SystemInfo.h"

using namespace std;

// Dynamic loading of Direct3DCreate9Ex to keep compatibility with 2000/XP.
typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)( UINT SDKVersion, IDirect3D9Ex **ppD3D);
static LPDIRECT3DCREATE9EX g_Direct3DCreate9Ex;
static HMODULE             g_D3D9ExHandle;

static bool LoadD3D9Ex()
{
  g_Direct3DCreate9Ex = (LPDIRECT3DCREATE9EX)GetProcAddress( GetModuleHandle("d3d9.dll"), "Direct3DCreate9Ex" );
  if(g_Direct3DCreate9Ex == NULL)
    return false;
  return true;
}

CRenderSystemDX::CRenderSystemDX() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_DIRECTX;

  m_pD3D        = NULL;
  m_pD3DDevice  = NULL;
  m_hFocusWnd   = NULL;
  m_hDeviceWnd  = NULL;
  m_nBackBufferWidth  = 0;
  m_nBackBufferHeight = 0;
  m_bFullScreenDevice = 0;
  m_bVSync          = true;
  m_nDeviceStatus   = S_OK;
  m_stateBlock      = NULL;
  m_inScene         = false;
  m_needNewDevice   = false;
  m_adapter         = D3DADAPTER_DEFAULT;
  m_screenHeight    = 0;
  m_systemFreq      = CurrentHostFrequency();
  m_useD3D9Ex       = false;
  m_defaultD3DUsage = 0;
  m_defaultD3DPool  = D3DPOOL_MANAGED;

  ZeroMemory(&m_D3DPP, sizeof(D3DPRESENT_PARAMETERS));
}

CRenderSystemDX::~CRenderSystemDX()
{
  DestroyRenderSystem();
}

bool CRenderSystemDX::InitRenderSystem()
{
  m_bVSync = true;
  m_renderCaps = 0;
  D3DADAPTER_IDENTIFIER9 AIdentifier;

  m_useD3D9Ex = (g_sysinfo.IsVistaOrHigher() && LoadD3D9Ex());
  m_pD3D = NULL;

  if (m_useD3D9Ex)
  {
    HRESULT hr;
    if (FAILED(hr=g_Direct3DCreate9Ex(D3D_SDK_VERSION, (IDirect3D9Ex**) &m_pD3D)))
      return false;
    CLog::Log(LOGDEBUG, "%s - using D3D9Ex", __FUNCTION__);
  }
  else
  {
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if(m_pD3D == NULL)
      return false;
  }
  
  UpdateMonitor();

  if(CreateDevice()==false)
    return false;

  if(m_pD3D->GetAdapterIdentifier(m_adapter, 0, &AIdentifier) == D3D_OK)
  {
    m_RenderRenderer = (const char*)AIdentifier.Description;
    m_RenderVendor   = (const char*)AIdentifier.Driver;
    m_RenderVersion.Format("%d.%d.%d.%04d", HIWORD(AIdentifier.DriverVersion.HighPart), LOWORD(AIdentifier.DriverVersion.HighPart),
                                            HIWORD(AIdentifier.DriverVersion.LowPart), LOWORD(AIdentifier.DriverVersion.LowPart));
  }

  // get our render capabilities
  D3DCAPS9 caps;
  m_pD3DDevice->GetDeviceCaps(&caps);

  if (SUCCEEDED(m_pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                           D3DDEVTYPE_HAL,
                                           D3DFMT_X8R8G8B8,
                                           0,
                                           D3DRTYPE_TEXTURE,
                                           D3DFMT_DXT5 )))
    m_renderCaps |= RENDER_CAPS_DXT;

  if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0)
  { // we're allowed NPOT textures
    m_renderCaps |= RENDER_CAPS_NPOT;
    m_renderCaps |= RENDER_CAPS_DXT_NPOT;
  }
  else if ((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
  { // we're allowed _some_ NPOT textures (namely non-DXT and only with D3DTADDRESS_CLAMP and no wrapping)
    m_renderCaps |= RENDER_CAPS_NPOT;
  }
  m_maxTextureSize = min(caps.MaxTextureWidth, caps.MaxTextureHeight);

  if (caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES)
  {
    m_defaultD3DUsage = D3DUSAGE_DYNAMIC;
    m_defaultD3DPool  = D3DPOOL_DEFAULT;
    CLog::Log(LOGDEBUG, "%s - using D3DCAPS2_DYNAMICTEXTURES", __FUNCTION__);
  }
  else
  {
    m_defaultD3DUsage = 0;
    m_defaultD3DPool  = D3DPOOL_MANAGED;
  }

  return true;
}

void CRenderSystemDX::SetRenderParams(unsigned int width, unsigned int height, bool fullScreen, float refreshRate)
{
  m_nBackBufferWidth  = width;
  m_nBackBufferHeight = height;
  m_bFullScreenDevice = fullScreen;
  m_refreshRate       = refreshRate;
}

void CRenderSystemDX::SetMonitor(HMONITOR monitor)
{
  if (!m_pD3D)
    return;

  // find the appropriate screen
  for (unsigned int adapter = 0; adapter < m_pD3D->GetAdapterCount(); adapter++)
  {
    HMONITOR hMonitor = m_pD3D->GetAdapterMonitor(adapter);
    if (hMonitor == monitor && adapter != m_adapter)
    {
      m_adapter       = adapter;
      m_needNewDevice = true;
      break;
    }
  }
}

bool CRenderSystemDX::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  SetRenderParams(width, height, fullScreen, refreshRate);

  CRect rc;
  rc.SetRect(0, 0, (float)width, (float)height);
  SetViewPort(rc);

  BuildPresentParameters();

  OnDeviceLost();
  OnDeviceReset();

  return true;
}

void CRenderSystemDX::BuildPresentParameters()
{
  ZeroMemory( &m_D3DPP, sizeof(D3DPRESENT_PARAMETERS) );
  bool useWindow = g_guiSettings.GetBool("videoscreen.fakefullscreen") || !m_bFullScreenDevice;
  m_D3DPP.Windowed           = useWindow;
  m_D3DPP.SwapEffect         = D3DSWAPEFFECT_DISCARD;
  m_D3DPP.BackBufferCount    = 1;
  m_D3DPP.EnableAutoDepthStencil = TRUE;
  m_D3DPP.hDeviceWindow      = m_hDeviceWnd;
  m_D3DPP.BackBufferWidth    = m_nBackBufferWidth;
  m_D3DPP.BackBufferHeight   = m_nBackBufferHeight;
  m_D3DPP.Flags              = D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  m_D3DPP.PresentationInterval = (m_bVSync) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
  m_D3DPP.FullScreen_RefreshRateInHz = (useWindow) ? 0 : (int)m_refreshRate;
  m_D3DPP.BackBufferFormat   = D3DFMT_X8R8G8B8;
  m_D3DPP.MultiSampleType    = D3DMULTISAMPLE_NONE;
  m_D3DPP.MultiSampleQuality = 0;


  // Try to create a 32-bit depth, 8-bit stencil
  if( FAILED( m_pD3D->CheckDeviceFormat( m_adapter,
    D3DDEVTYPE_HAL,  m_D3DPP.BackBufferFormat,  D3DUSAGE_DEPTHSTENCIL,
    D3DRTYPE_SURFACE, D3DFMT_D24S8 )))
  {
    // Bugger, no 8-bit hardware stencil, just try 32-bit zbuffer
    if( FAILED( m_pD3D->CheckDeviceFormat(m_adapter,
      D3DDEVTYPE_HAL,  m_D3DPP.BackBufferFormat,  D3DUSAGE_DEPTHSTENCIL,
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
    if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( m_adapter, D3DDEVTYPE_HAL,
      m_D3DPP.BackBufferFormat, m_D3DPP.BackBufferFormat, D3DFMT_D24S8 ) ) )
    {
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D24S8;
    }
    else
      m_D3DPP.AutoDepthStencilFormat = D3DFMT_D24X8;
  }

  if (m_useD3D9Ex)
  {
    ZeroMemory( &m_D3DDMEX, sizeof(D3DDISPLAYMODEEX) );
    m_D3DDMEX.Size             = sizeof(D3DDISPLAYMODEEX);
    m_D3DDMEX.Width            = m_D3DPP.BackBufferWidth;
    m_D3DDMEX.Height           = m_D3DPP.BackBufferHeight;
    m_D3DDMEX.RefreshRate      = m_D3DPP.FullScreen_RefreshRateInHz;
    m_D3DDMEX.Format           = m_D3DPP.BackBufferFormat;
    m_D3DDMEX.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
  }
}

bool CRenderSystemDX::DestroyRenderSystem()
{
  DeleteDevice();

  SAFE_RELEASE(m_stateBlock);
  SAFE_RELEASE(m_pD3D);
  SAFE_RELEASE(m_pD3DDevice);

  m_bRenderCreated = false;

  return true;
}

void CRenderSystemDX::DeleteDevice()
{
  CSingleLock lock(m_resourceSection);

  // tell any shared resources
  for (vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnDestroyDevice();

  SAFE_RELEASE(m_pD3DDevice);
  m_bRenderCreated = false;
}

void CRenderSystemDX::OnDeviceLost()
{
  CSingleLock lock(m_resourceSection);
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  SAFE_RELEASE(m_stateBlock);

  if (m_needNewDevice)
    DeleteDevice();
  else
  {
    // just resetting the device
    for (vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnLostDevice();
  }
}

void CRenderSystemDX::OnDeviceReset()
{
  CSingleLock lock(m_resourceSection);

  if (m_needNewDevice)
    CreateDevice();
  else
  {
    // just need a reset
    if (m_useD3D9Ex)
      m_nDeviceStatus = ((IDirect3DDevice9Ex*)m_pD3DDevice)->ResetEx(&m_D3DPP, m_D3DPP.Windowed ? NULL : &m_D3DDMEX);
    else
      m_nDeviceStatus = m_pD3DDevice->Reset(&m_D3DPP);
  }

  if (m_nDeviceStatus == S_OK)
  { // we're back
    for (vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnResetDevice();

    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  }
  else
  {
    for (vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
      (*i)->OnLostDevice();
  }
}

bool CRenderSystemDX::CreateDevice()
{
  // Code based on Ogre 3D engine
  CSingleLock lock(m_resourceSection);

  HRESULT hr;

  if(m_pD3D == NULL)
    return false;

  if(m_hDeviceWnd == NULL)
    return false;

  CLog::Log(LOGDEBUG, "%s on adapter %d", __FUNCTION__, m_adapter);

  D3DDEVTYPE devType = D3DDEVTYPE_HAL;

#if defined(DEBUG_PS) || defined (DEBUG_VS)
    devType = D3DDEVTYPE_REF
#endif

  BuildPresentParameters();

  if (m_useD3D9Ex)
  {
    hr = ((IDirect3D9Ex*)m_pD3D)->CreateDeviceEx(m_adapter, devType, m_hFocusWnd,
      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, m_D3DPP.Windowed ? NULL : &m_D3DDMEX, (IDirect3DDevice9Ex**)&m_pD3DDevice );

    if (FAILED(hr))
    {
      // Try a second time, may fail the first time due to back buffer count,
      // which will be corrected down to 1 by the runtime
      hr = ((IDirect3D9Ex*)m_pD3D)->CreateDeviceEx( m_adapter, devType, m_hFocusWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, m_D3DPP.Windowed ? NULL : &m_D3DDMEX, (IDirect3DDevice9Ex**)&m_pD3DDevice );
      if( FAILED( hr ) )
      {
        hr = ((IDirect3D9Ex*)m_pD3D)->CreateDeviceEx( m_adapter, devType, m_hFocusWnd,
          D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP,  m_D3DPP.Windowed ? NULL : &m_D3DDMEX, (IDirect3DDevice9Ex**)&m_pD3DDevice );
        if( FAILED( hr ) )
        {
          hr = ((IDirect3D9Ex*)m_pD3D)->CreateDeviceEx( m_adapter, devType, m_hFocusWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP,  m_D3DPP.Windowed ? NULL : &m_D3DDMEX, (IDirect3DDevice9Ex**)&m_pD3DDevice );
        }
        if(FAILED( hr ) )
          return false;
      }
    }
  }
  else
  {

    hr = m_pD3D->CreateDevice(m_adapter, devType, m_hFocusWnd,
      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, &m_pD3DDevice );

    if (FAILED(hr))
    {
      // Try a second time, may fail the first time due to back buffer count,
      // which will be corrected down to 1 by the runtime
      hr = m_pD3D->CreateDevice( m_adapter, devType, m_hFocusWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, &m_pD3DDevice );
      if( FAILED( hr ) )
      {
        hr = m_pD3D->CreateDevice( m_adapter, devType, m_hFocusWnd,
          D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, &m_pD3DDevice );
        if( FAILED( hr ) )
        {
          hr = m_pD3D->CreateDevice( m_adapter, devType, m_hFocusWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_D3DPP, &m_pD3DDevice );
        }
        if(FAILED( hr ) )
          return false;
      }
    }
  }

  D3DDISPLAYMODE mode;
  if (SUCCEEDED(m_pD3DDevice->GetDisplayMode(0, &mode)))
    m_screenHeight = mode.Height;
  else
    m_screenHeight = m_nBackBufferHeight;

  m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

  m_bRenderCreated = true;

  m_needNewDevice = false;

  // tell any shared objects about our resurrection
  for (vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnCreateDevice();

  return true;
}

bool CRenderSystemDX::PresentRenderImpl()
{
  HRESULT hr;

  if (!m_bRenderCreated)
    return false;

  if(m_nDeviceStatus != S_OK)
    return false;

  if (g_advancedSettings.m_sleepBeforeFlip > 0)
  {
    //save current thread priority and set thread priority to THREAD_PRIORITY_TIME_CRITICAL
    int priority = GetThreadPriority(GetCurrentThread());
    if (priority != THREAD_PRIORITY_ERROR_RETURN)
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    D3DRASTER_STATUS rasterStatus;
    int64_t          prev = CurrentHostCounter();

    while (SUCCEEDED(m_pD3DDevice->GetRasterStatus(0, &rasterStatus)))
    {
      //wait for the scanline to go over the given proportion of m_screenHeight mark
      if (!rasterStatus.InVBlank && rasterStatus.ScanLine >= g_advancedSettings.m_sleepBeforeFlip * m_screenHeight)
        break;

      //in theory it's possible this loop never exits, so don't let it run for longer than 100 ms
      int64_t now = CurrentHostCounter();
      if ((now - prev) * 10 > m_systemFreq)
        break;

      Sleep(1);
    }

    //restore thread priority
    if (priority != THREAD_PRIORITY_ERROR_RETURN)
      SetThreadPriority(GetCurrentThread(), priority);
  }
  hr = m_pD3DDevice->Present( NULL, NULL, 0, NULL );

  if( D3DERR_DEVICELOST == hr )
  {
    CLog::Log(LOGDEBUG, "%s - lost device", __FUNCTION__);
    return false;
  }

  if(FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s - Present failed with hr=%d", __FUNCTION__, hr);
    return false;
  }

  return true;
}

bool CRenderSystemDX::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  DWORD oldStatus = m_nDeviceStatus;
  if( FAILED( m_nDeviceStatus = m_pD3DDevice->TestCooperativeLevel() ) )
  {
    // The device has been lost but cannot be reset at this time.
    // Therefore, rendering is not possible and we'll have to return
    // and try again at a later time.
    if( m_nDeviceStatus == D3DERR_DEVICELOST )
    {
      if (m_nDeviceStatus != oldStatus)
        CLog::Log(LOGDEBUG, "D3DERR_DEVICELOST");
      OnDeviceLost();
      return false;
    }

    // The device has been lost but it can be reset at this time.
    if( m_nDeviceStatus == D3DERR_DEVICENOTRESET )
    {
      OnDeviceReset();
      if( FAILED(m_nDeviceStatus ) )
      {
        CLog::Log(LOGINFO, "m_pD3DDevice->Reset failed");
        return false;
      }
    }
  }

  if(FAILED (m_pD3DDevice->BeginScene()))
  {
    CLog::Log(LOGERROR, "m_pD3DDevice->BeginScene() failed");
    // When XBMC caught an exception after BeginScene(), EndScene() may never been called
    // and thus all following BeginScene() will fail too.
    if(FAILED (m_pD3DDevice->EndScene()))
      CLog::Log(LOGERROR, "m_pD3DDevice->EndScene() failed");
    return false;
  }
  m_inScene = true;
  return true;
}

bool CRenderSystemDX::EndRender()
{
  m_inScene = false;

  if (!m_bRenderCreated)
    return false;

  if(m_nDeviceStatus != S_OK)
    return false;

  if(FAILED (m_pD3DDevice->EndScene()))
  {
    CLog::Log(LOGERROR, "m_pD3DDevice->EndScene() failed");
    return false;
  }

  return true;
}

bool CRenderSystemDX::ClearBuffers(color_t color)
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

bool CRenderSystemDX::IsExtSupported(const char* extension)
{
  return false;
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
  if (m_bVSync != enable)
  {
    bool inScene(m_inScene);
    if (m_inScene)
      EndRender();
    m_bVSync = enable;
    ResetRenderSystem(m_nBackBufferWidth, m_nBackBufferHeight, m_bFullScreenDevice, m_refreshRate);
    if (inScene)
      BeginRender();
  }
}

void CRenderSystemDX::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;

  SAFE_RELEASE(m_stateBlock);
  m_pD3DDevice->CreateStateBlock(D3DSBT_ALL, &m_stateBlock);
}

void CRenderSystemDX::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  if (m_stateBlock)
    m_stateBlock->Apply();
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

  DWORD thisTime = CTimeUtils::GetTimeMS();

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

void CRenderSystemDX::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  D3DVIEWPORT9 d3dviewport;
  m_pD3DDevice->GetViewport(&d3dviewport);

  viewPort.x1 = (float)d3dviewport.X;
  viewPort.y1 = (float)d3dviewport.Y;
  viewPort.x2 = (float)d3dviewport.X + d3dviewport.Width;
  viewPort.y2 = (float)d3dviewport.Y + d3dviewport.Height;
}

void CRenderSystemDX::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  D3DVIEWPORT9 newviewport;

  newviewport.MinZ   = 0.0f;
  newviewport.MaxZ   = 1.0f;
  newviewport.X      = (DWORD)viewPort.x1;
  newviewport.Y      = (DWORD)viewPort.y1;
  newviewport.Width  = (DWORD)(viewPort.x2 - viewPort.x1);
  newviewport.Height = (DWORD)(viewPort.y2 - viewPort.y1);
  m_pD3DDevice->SetViewport(&newviewport);
}

void CRenderSystemDX::Register(ID3DResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CRenderSystemDX::Unregister(ID3DResource* resource)
{
  CSingleLock lock(m_resourceSection);
  vector<ID3DResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

#endif
