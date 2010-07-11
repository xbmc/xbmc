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

#include "WinRenderer.h"
#include "Application.h"
#include "Util.h"
#include "Settings.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "MathUtils.h"
#include "cores/dvdplayer/DVDCodecs/Video/DXVA.h"
#include "VideoShaders/WinVideoFilter.h"

CWinRenderer::CWinRenderer()
{
  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

  m_colorShader = NULL;
  m_scalerShader = NULL;

  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;

  m_bUseHQScaler = false;
  m_bFilterInitialized = false;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

void CWinRenderer::ManageTextures()
{
  int neededbuffers = 2;

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

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(m_sourceWidth  != width
  || m_sourceHeight != height)
  {
    m_sourceWidth = width;
    m_sourceHeight = height;
    // need to recreate textures
    m_NumYV12Buffers = 0;
    m_iYV12RenderBuffer = 0;
  }

  m_flags = flags;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);

  ManageDisplay();

  m_bConfigured = true;

  return true;
}

int CWinRenderer::NextYV12Texture()
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  else
    return -1;
}

void CWinRenderer::AddProcessor(DXVA::CProcessor* processor, int64_t id)

{
  int source = NextYV12Texture();
  if(source < 0)
    return;
  SVideoBuffer& buf = m_VideoBuffers[source];
  SAFE_RELEASE(buf.proc);
  buf.proc = processor->Acquire();
  buf.id   = id;
}

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return -1;

  SVideoBuffer &buf = m_VideoBuffers[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;

  for(int i=0;i<3;i++)
  {
    image->stride[i] = buf.planes[i].rect.Pitch;
    image->plane[i]  = (BYTE*)buf.planes[i].rect.pBits;
  }

  return source;
}

void CWinRenderer::ReleaseImage(int source, bool preserve)
{
  // no need to release anything here since we're using system memory
}

void CWinRenderer::Reset()
{
}

void CWinRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CWinRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) return;
  ManageTextures();

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

void CWinRenderer::FlipPage(int source)
{
  if(source == AUTOSOURCE)
    source = NextYV12Texture();

  m_VideoBuffers[m_iYV12RenderBuffer].StartDecode();

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = 0;

  m_VideoBuffers[m_iYV12RenderBuffer].StartRender();

#ifdef MP_DIRECTRENDERING
  __asm wbinvd
#endif

  return;
}


unsigned int CWinRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  /*
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
  */

  return 0;
}

unsigned int CWinRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_resolution = RES_PAL_4x3;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  g_Windowing.Get3DDevice()->GetDeviceCaps(&m_deviceCaps);

  return 0;
}

void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
  if (m_IntermediateStencilSurface.Get())
    m_IntermediateStencilSurface.Release();

  SAFE_RELEASE(m_colorShader)
  SAFE_RELEASE(m_scalerShader)
  
  m_bConfigured = false;
  m_bFilterInitialized = false;

  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);

  m_NumYV12Buffers = 0;
}

bool CWinRenderer::SetupIntermediateRenderTarget()
{
  // Initialize a render target for intermediate rendering - same size as the video source
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if(!m_IntermediateTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGNOTICE, __FUNCTION__": Failed to create 10 bit render target.  Trying 8 bit...");
    if(!m_IntermediateTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create render target texture. Going back to bilinear scaling.");
      return false;
    }
  }

  //Pixel shaders need a matching depth-stencil surface.
  LPDIRECT3DSURFACE9 tmpSurface;
  D3DSURFACE_DESC tmpDesc;
  //Use the same depth stencil format as the backbuffer.
  pD3DDevice->GetDepthStencilSurface(&tmpSurface);
  tmpSurface->GetDesc(&tmpDesc);
  tmpSurface->Release();
  if (!m_IntermediateStencilSurface.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_DEPTHSTENCIL, tmpDesc.Format, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create depth stencil. Going back to bilinear scaling.");
    m_IntermediateTarget.Release();
    return false;
  }
  return true;
}

void CWinRenderer::UpdateVideoFilter()
{
  if (m_scalingMethodGui == g_settings.m_currentVideoSettings.m_ScalingMethod && m_bFilterInitialized)
    return;

  m_bFilterInitialized = true;

  m_scalingMethodGui = g_settings.m_currentVideoSettings.m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  m_singleStage = false;
  m_bUseHQScaler = false;

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  case VS_SCALINGMETHOD_LINEAR:
    break;

  case VS_SCALINGMETHOD_CUBIC:
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_LANCZOS3:
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_SINC8:
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "D3D: TODO: This scaler has not yet been implemented");
    break;

  case VS_SCALINGMETHOD_BICUBIC_SOFTWARE:
  case VS_SCALINGMETHOD_LANCZOS_SOFTWARE:
  case VS_SCALINGMETHOD_SINC_SOFTWARE:
    CLog::Log(LOGERROR, "D3D: TODO: Software scaling has not yet been implemented");
    break;

  default:
    break;
  }

  // Scaler auto + SD -> Lanczos3 optim. Otherwise bilinear.
  if(m_scalingMethod == VS_SCALINGMETHOD_AUTO && m_sourceWidth < 1280)
  {
    m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
    m_bUseHQScaler = true;
    m_singleStage = false;
  }

  SAFE_RELEASE(m_scalerShader)

  if (m_bUseHQScaler)
  {
    m_singleStage = false;

    m_scalerShader = new CConvolutionShader();
    if (!m_scalerShader->Create(m_scalingMethod))
    {
      SAFE_RELEASE(m_scalerShader);
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "Video Renderering", "Failed to init video scaler, falling back to bilinear scaling.");
      m_bUseHQScaler = false;
    }
  }

  if (!m_bUseHQScaler)
    m_singleStage = true;

  // Scaler is figured out. Now the colour conversion part.

  if(m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
  if (m_IntermediateStencilSurface.Get())
    m_IntermediateStencilSurface.Release();

  if (!m_singleStage && !SetupIntermediateRenderTarget())
  {
    SAFE_RELEASE(m_scalerShader)
    m_singleStage = true;
  }

  SAFE_RELEASE(m_colorShader)

  if (!m_singleStage)
  {
    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(false))
    {
      m_IntermediateTarget.Release();
      m_IntermediateStencilSurface.Release();
      SAFE_RELEASE(m_scalerShader)
      SAFE_RELEASE(m_colorShader);
      m_singleStage = true;
      m_bUseHQScaler = false;
    }
  }

  if (m_singleStage) //fallback from HQ scalers and multipass creation above
  {
    SAFE_RELEASE(m_colorShader)

    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(true))
      SAFE_RELEASE(m_colorShader);
    // should fallback on D3D accelerated or sw method
  }
}

void CWinRenderer::Render(DWORD flags)
{
  if(CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    CWinRenderer::RenderProcessor(flags);
    return;
  }

  UpdateVideoFilter();

  // Optimize later? we could get by with bilinear under some circumstances
  /*if(!m_bUseHQScaler
    || !g_graphicsContext.IsFullScreenVideo()
    || g_graphicsContext.IsCalibrating()
    || (m_destRect.Width() == m_sourceWidth && m_destRect.Height() == m_sourceHeight))
    */
  CSingleLock lock(g_graphicsContext);

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  if (m_singleStage)
  {
    Stage1(flags);
  }
  else
  {
    Stage1(flags);
    Stage2(flags);
  }
}

void CWinRenderer::Stage1(DWORD flags)
{
  if (m_singleStage)
  {
      m_colorShader->Render(m_sourceWidth, m_sourceHeight, m_sourceRect, m_destRect,
                              g_settings.m_currentVideoSettings.m_Contrast,
                              g_settings.m_currentVideoSettings.m_Brightness,
                              m_flags,
                              &m_VideoBuffers[m_iYV12RenderBuffer]);
  }
  else
  {
    // Switch the render target to the temporary destination
    LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
    LPDIRECT3DSURFACE9 newRT, oldRT, oldDS, newDS;
    m_IntermediateTarget.GetSurfaceLevel(0, &newRT);
    m_IntermediateStencilSurface.GetSurfaceLevel(0, &newDS);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, newRT);
    pD3DDevice->GetDepthStencilSurface(&oldDS);
    pD3DDevice->SetDepthStencilSurface(newDS);

    CRect rtRect(0.0f, 0.0f, m_sourceWidth, m_sourceHeight);

    m_colorShader->Render(m_sourceWidth, m_sourceHeight, m_sourceRect, rtRect,
                                g_settings.m_currentVideoSettings.m_Contrast,
                                g_settings.m_currentVideoSettings.m_Brightness,
                                m_flags,
                                &m_VideoBuffers[m_iYV12RenderBuffer]);

    // Restore the render target
    pD3DDevice->SetRenderTarget(0, oldRT);
    pD3DDevice->SetDepthStencilSurface(oldDS);

    oldDS->Release();
    oldRT->Release();
    newDS->Release();
    newRT->Release();
  }
}

void CWinRenderer::Stage2(DWORD flags)
{
  m_scalerShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_sourceRect, m_destRect);
}

void CWinRenderer::RenderProcessor(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);
  RECT rect;
  rect.top    = m_destRect.y1;
  rect.bottom = m_destRect.y2;
  rect.left   = m_destRect.x1;
  rect.right  = m_destRect.x2;

  SVideoBuffer& image = m_VideoBuffers[m_iYV12RenderBuffer];
  if(image.proc == NULL)
    return;

  IDirect3DSurface9* target;
  if(FAILED(g_Windowing.Get3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &target)))
  {
    CLog::Log(LOGERROR, "CWinRenderer::RenderSurface - failed to get back buffer");
    return;
  }

  image.proc->Render(rect, target, image.id);

  target->Release();
}

void CWinRenderer::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);

  // create a new render surface to copy out of - note, this may be slow on some hardware
  // due to the TRUE parameter - you're supposed to use GetRenderTargetData.
  LPDIRECT3DSURFACE9 surface = NULL;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (D3D_OK == pD3DDevice->CreateRenderTarget(width, height, D3DFMT_LIN_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL))
  {
    LPDIRECT3DSURFACE9 oldRT;
    CRect saveSize = m_destRect;
    m_destRect.SetRect(0, 0, (float)width, (float)height);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, surface);
    pD3DDevice->BeginScene();
    Render(0);
    pD3DDevice->EndScene();
    m_destRect = saveSize;
    pD3DDevice->SetRenderTarget(0, oldRT);
    oldRT->Release();

    D3DLOCKED_RECT lockedRect;
    if (D3D_OK == surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY))
    {
      texture->LoadFromMemory(width, height, lockedRect.Pitch, XB_FMT_A8R8G8B8, (unsigned char *)lockedRect.pBits);
      surface->UnlockRect();
    }
    surface->Release();
  }
}


//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  SVideoBuffer &buf = m_VideoBuffers[index];
  buf.Clear();
  m_NumYV12Buffers = 0;
}

void CWinRenderer::ClearYV12Texture(int index)
{
  SVideoBuffer &buf = m_VideoBuffers[index];
  memset(buf.planes[0].rect.pBits, 0,   buf.planes[0].rect.Pitch * m_sourceHeight);
  memset(buf.planes[1].rect.pBits, 128, buf.planes[1].rect.Pitch * m_sourceHeight>>1);
  memset(buf.planes[2].rect.pBits, 128, buf.planes[2].rect.Pitch * m_sourceHeight>>1);
}

bool CWinRenderer::CreateYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);

  SVideoBuffer &buf = m_VideoBuffers[index];

  if ( !buf.planes[0].texture.Create(m_sourceWidth    , m_sourceHeight    , 1, g_Windowing.DefaultD3DUsage(), D3DFMT_L8, g_Windowing.DefaultD3DPool())
    || !buf.planes[1].texture.Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, g_Windowing.DefaultD3DUsage(), D3DFMT_L8, g_Windowing.DefaultD3DPool())
    || !buf.planes[2].texture.Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, g_Windowing.DefaultD3DUsage(), D3DFMT_L8, g_Windowing.DefaultD3DPool()))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 video texture %i", index);
    return false;
  }

  buf.StartDecode();

  ClearYV12Texture(index);

  if(index == m_iYV12RenderBuffer)
    buf.StartRender();

  CLog::Log(LOGDEBUG, "created yv12 texture %i", index);
  return true;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    if(method == VS_INTERLACEMETHOD_NONE)
      return true;
    return false;
  }

  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinRenderer::Supports(ERENDERFEATURE feature)
{
  if(feature == RENDERFEATURE_BRIGHTNESS)
    return true;
  
  if(feature == RENDERFEATURE_CONTRAST)
    return true;

  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if(CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    if(method == VS_SCALINGMETHOD_LINEAR)
      return true;
    return false;
  }

  if(D3DSHADER_VERSION_MAJOR(m_deviceCaps.PixelShaderVersion) >= 3)
  {
    if(method == VS_SCALINGMETHOD_LINEAR
    || method == VS_SCALINGMETHOD_CUBIC
    || method == VS_SCALINGMETHOD_LANCZOS2
    || method == VS_SCALINGMETHOD_LANCZOS3_FAST
    || method == VS_SCALINGMETHOD_AUTO)
      return true;

    //lanczos3 is only allowed through advancedsettings.xml because it's very slow
    if (g_advancedSettings.m_videoAllowLanczos3 && method == VS_SCALINGMETHOD_LANCZOS3)
      return true;
  }
  else if(method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

void SVideoBuffer::Clear()
{
  SAFE_RELEASE(proc);
  id = 0;
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    planes[i].texture.Release();
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void SVideoBuffer::StartRender()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].rect.pBits)
      planes[i].texture.UnlockRect(0);
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void SVideoBuffer::StartDecode()
{
  SAFE_RELEASE(proc);
  id = 0;
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].texture.LockRect(0, &planes[i].rect, NULL, D3DLOCK_DISCARD) == false)
    {
      memset(&planes[i].rect, 0, sizeof(planes[i].rect));
      CLog::Log(LOGERROR, "CWinRenderer::SVideoBuffer::StartDecode - failed to lock texture into memory");
    }
  }
}

#endif
