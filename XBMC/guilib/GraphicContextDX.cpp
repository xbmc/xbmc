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

#include "include.h"
#include "GraphicContextDX.h"
#include "GUIFontManager.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "Settings.h"
#include "GUISettings.h"
#include "XBVideoConfig.h"
#include "TextureManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GUIAudioManager.h"

#ifdef HAS_DX

#define D3D_CLEAR_STENCIL 0x0l

#include "Surface.h"
#include "SkinInfo.h"

using namespace std;
using namespace Surface;

extern bool g_fullScreen;

CGraphicContextDX g_graphicsContext;

CGraphicContextDX::CGraphicContextDX(void)
{
  m_maxTextureSize = 2048;
}

CGraphicContextDX::~CGraphicContextDX(void)
{
  
}

void CGraphicContextDX::GetRenderVersion(int& maj, int& min)
{
  // not yet implemented
  maj = s_RenderMajVer = 0;
  min = s_RenderMinVer = 0;  
}

void CGraphicContextDX::SetD3DDevice(LPDIRECT3DDEVICE9 p3dDevice) 
{ 
  m_pd3dDevice = p3dDevice; 
}

void CGraphicContextDX::SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams)
{
  m_pd3dParams = p3dParams;
}

CRect CGraphicContextDX::GetRenderViewPort()
{
  D3DVIEWPORT9 viewport;
  Get3DDevice()->GetViewport(&viewport);

  CRect rect;
 
  rect.x1 = (float)viewport.X;
  rect.y2 = (float)viewport.Y;
  rect.y1 = (float)viewport.X + viewport.Width;
  rect.x2 = (float)viewport.Y + viewport.Height;

  return rect;
}

void CGraphicContextDX::SetRendrViewPort(CRect& viewPort)
{
  D3DVIEWPORT9 newviewport;

  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = (DWORD)viewPort.x1;
  newviewport.Y = (DWORD)viewPort.y1;
  newviewport.Width = (DWORD)(viewPort.x2 - viewPort.x1);
  newviewport.Height = (DWORD)(viewPort.y2 - viewPort.y1);
  m_pd3dDevice->SetViewport(&newviewport);
}


void CGraphicContextDX::Clear()
{
  if (!m_pd3dDevice) return;
  //Not trying to clear the zbuffer when there is none is 7 fps faster (pal resolution)
  if ((!m_pd3dParams) || (m_pd3dParams->EnableAutoDepthStencil == TRUE))
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
  else
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
}

void CGraphicContextDX::CaptureStateBlock()
{

}

void CGraphicContextDX::ApplyStateBlock()
{
  
}

void CGraphicContextDX::UpdateCameraPosition(const CPoint &camera)
{
   CPoint offset = camera - CPoint(m_iScreenWidth*0.5f, m_iScreenHeight*0.5f);

  // grab the viewport dimensions and location
  D3DVIEWPORT9 viewport;
  m_pd3dDevice->GetViewport(&viewport);
  float w = viewport.Width*0.5f;
  float h = viewport.Height*0.5f;

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity
  // here.
  D3DXMATRIX mtxWorld;
  D3DXMatrixIdentity(&mtxWorld);
  m_pd3dDevice->SetTransform(D3DTS_WORLD, &mtxWorld);

  // camera view.  Multiply the Y coord by -1 then translate so that everything is relative to the camera
  // position.
  D3DXMATRIX flipY, translate, mtxView;
  D3DXMatrixScaling(&flipY, 1.0f, -1.0f, 1.0f);
  D3DXMatrixTranslation(&translate, -(viewport.X + w + offset.x), -(viewport.Y + h + offset.y), 2*h);
  D3DXMatrixMultiply(&mtxView, &translate, &flipY);
  m_pd3dDevice->SetTransform(D3DTS_VIEW, &mtxView);

  // projection onto screen space
  D3DXMATRIX mtxProjection;
  D3DXMatrixPerspectiveOffCenterLH(&mtxProjection, (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mtxProjection);
}

bool CGraphicContextDX::ValidateSurface(CSurface* dest)
{
  return true;
}

CSurface* CGraphicContextDX::InitializeSurface()
{
  return NULL;
}

void CGraphicContextDX::ReleaseCurrentContext(Surface::CSurface* ctx)
{
 
}

void CGraphicContextDX::DeleteThreadContext() 
{
 
}

void CGraphicContextDX::AcquireCurrentContext(Surface::CSurface* ctx)
{
 
}

void CGraphicContextDX::BeginPaint(CSurface *dest, bool lock)
{
  
}

void CGraphicContextDX::EndPaint(CSurface *dest, bool lock)
{
 
}

void CGraphicContextDX::SetFullScreenRoot(bool fs)
{
  if (fs)
  {
    // Code from this point on should be platform dependent. The Win32 version could
    // probably use GetSystemMetrics/EnumDisplayDevices/GetDeviceCaps to query current
    // resolution on the requested display no. and set 'width' and 'height'

    m_iFullScreenWidth = m_iScreenWidth;
    m_iFullScreenHeight = m_iScreenHeight;

    DEVMODE settings;
    settings.dmSize = sizeof(settings);
    settings.dmDriverExtra = 0;
    settings.dmBitsPerPel = 32;
    settings.dmPelsWidth = m_iFullScreenWidth;
    settings.dmPelsHeight = m_iFullScreenHeight;
    settings.dmDisplayFrequency = (int)floorf(g_settings.m_ResInfo[m_Resolution].fRefreshRate);
    settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    if(settings.dmDisplayFrequency)
      settings.dmFields |= DM_DISPLAYFREQUENCY;
    if(ChangeDisplaySettings(&settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
      CLog::Log(LOGERROR, "CGraphicContext::SetFullScreenRoot - failed to change resolution");

    if (m_screenSurface)
    {
      m_screenSurface->RefreshCurrentContext();
      m_screenSurface->ResizeSurface(m_iFullScreenWidth, m_iFullScreenHeight);
    }

    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(m_iFullScreenWidth, m_iFullScreenHeight, 1, 1);
    g_renderManager.Recover();
  }
  else
  {
    ChangeDisplaySettings(NULL, 0);
    if (m_screenSurface)
    {
      m_screenSurface->RefreshCurrentContext();
      m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
    }
    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(g_settings.m_ResInfo[m_Resolution].iWidth, g_settings.m_ResInfo[m_Resolution].iHeight, 1, 1);
    g_renderManager.Recover();
  }

  m_bFullScreenRoot = fs;
  g_advancedSettings.m_fullScreen = fs;
  SetFullScreenViewWindow(m_Resolution);
}

void CGraphicContextDX::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
{
  if (res == AUTORES)
  {
    res = g_videoConfig.GetBestMode();
  }
  if (!IsValidResolution(res))
  { // Choose a failsafe resolution that we can actually display
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    res = g_videoConfig.GetSafeMode();
  }

  if (!m_pd3dParams)
  {
    m_Resolution = res;
    RESOLUTION_INFO resInfo;
    //g_videoConfig.GetResolutionInfo(m_Resolution, resInfo);
    m_iScreenWidth = 1280;
    m_iScreenHeight = 720;
    return ;
  }
  bool NeedReset = false;

  UINT interval = D3DPRESENT_INTERVAL_ONE;
  //if( m_bFullScreenVideo )
  //  interval = D3DPRESENT_INTERVAL_IMMEDIATE;

#ifdef PROFILE
  interval = D3DPRESENT_INTERVAL_IMMEDIATE;
#endif

  interval = 0;

  if (interval != m_pd3dParams->PresentationInterval)
  {
    m_pd3dParams->PresentationInterval = interval;
    NeedReset = true;
  }


  if (NeedZ != m_pd3dParams->EnableAutoDepthStencil)
  {
    m_pd3dParams->EnableAutoDepthStencil = NeedZ;
    NeedReset = true;
  }
  if (m_Resolution != res)
  {
    NeedReset = true;
    m_pd3dParams->BackBufferWidth = g_settings.m_ResInfo[res].iWidth;
    m_pd3dParams->BackBufferHeight = g_settings.m_ResInfo[res].iHeight;
    m_pd3dParams->Flags = g_settings.m_ResInfo[res].dwFlags;
    m_pd3dParams->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    if (res == HDTV_1080i || res == HDTV_720p || m_bFullScreenVideo)
      m_pd3dParams->BackBufferCount = 1;
    else
      m_pd3dParams->BackBufferCount = 2;

    if (res == PAL60_4x3 || res == PAL60_16x9)
    {
      if (m_pd3dParams->BackBufferWidth <= 720 && m_pd3dParams->BackBufferHeight <= 480)
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 60;
      }
      else
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 0;
      }
    }
    else
    {
      m_pd3dParams->FullScreen_RefreshRateInHz = 0;
    }
  }
  Lock();
  if (m_pd3dDevice)
  {
    if (NeedReset)
    {
      CLog::Log(LOGDEBUG, "Setting resolution %i", res);
      m_pd3dDevice->Reset(m_pd3dParams);
    }

    /* need to clear and preset, otherwise flicker filters won't take effect */
    if (NeedReset || forceClear)
    {
      m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
      m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    }

    m_iScreenWidth = m_pd3dParams->BackBufferWidth;
    m_iScreenHeight = m_pd3dParams->BackBufferHeight;
    m_bWidescreen = (m_pd3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN) != 0;
  }
  if (m_Resolution != INVALID && ((g_settings.m_ResInfo[m_Resolution].iWidth != g_settings.m_ResInfo[res].iWidth) ||
    (g_settings.m_ResInfo[m_Resolution].iHeight != g_settings.m_ResInfo[res].iHeight)))
  { // set the mouse resolution
    g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
    ResetOverscan(g_settings.m_ResInfo[res]);
  }

  SetFullScreenViewWindow(res);

  m_Resolution = res;
  if(NeedReset)
  {
    CLog::Log(LOGDEBUG, "We set resolution %i", m_Resolution);
    if (m_Resolution != INVALID)
      g_fontManager.ReloadTTFFonts();
  }

  Unlock();
}
void CGraphicContextDX::Flip()
{
  if (m_pd3dDevice) 
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

void CGraphicContextDX::ApplyHardwareTransform()
{
  
}

void CGraphicContextDX::RestoreHardwareTransform()
{
  
}

#endif
