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
#include "VideoShaders/ConvolutionKernels.h"
#include "cores/dvdplayer/DVDCodecs/Video/DXVA.h"

// http://www.martinreddy.net/gfx/faqs/colorconv.faq

YUVRANGE yuv_range_lim =  { 16, 235, 16, 240, 16, 240 };
YUVRANGE yuv_range_full = {  0, 255,  0, 255,  0, 255 };

static float yuv_coef_bt601[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.344f,   1.773f,   0.0f },
    { 1.403f,   -0.714f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_bt709[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.1870f,  1.8556f,  0.0f },
    { 1.5701f,  -0.4664f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_ebu[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.3960f,  2.029f,   0.0f },
    { 1.140f,   -0.581f,   0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

static float yuv_coef_smtp240m[4][4] =
{
    { 1.0f,      1.0f,     1.0f,     0.0f },
    { 0.0f,     -0.2253f,  1.8270f,  0.0f },
    { 1.5756f,  -0.5000f,  0.0f,     0.0f },
    { 0.0f,      0.0f,     0.0f,     0.0f }
};

CWinRenderer::CWinRenderer()
{
  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

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

  m_YUV2RGBEffect.Release();
  m_YUV2RGBHQScalerEffect.Release();
  m_HQKernelTexture.Release();

  m_bConfigured = false;
  m_bFilterInitialized = false;

  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);

  m_NumYV12Buffers = 0;
}

bool CWinRenderer::LoadEffect(CD3DEffect &effect, CStdString filename)
{
  XFILE::CFileStream file;
  if(!file.Open(filename))
  {
    CLog::Log(LOGERROR, "CWinRenderer::LoadEffect - failed to open file %s", filename.c_str());
    return false;
  }

  CStdString pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!effect.Create(pStrEffect))
  {
    CLog::Log(LOGERROR, "D3DXCreateEffectFromFile %s failed", pStrEffect.c_str());
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

  if(m_YUV2RGBHQScalerEffect.Get())
    m_YUV2RGBHQScalerEffect.Release();

  if(m_HQKernelTexture.Get())
    m_HQKernelTexture.Release();

  CStdString effectString;

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  case VS_SCALINGMETHOD_LINEAR:
    m_bUseHQScaler = false;
    break;

  case VS_SCALINGMETHOD_CUBIC:
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
    effectString = "special://xbmc/system/shaders/yuv2rgb_4x4_d3d.fx";
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_LANCZOS3:
    effectString = "special://xbmc/system/shaders/yuv2rgb_6x6_d3d.fx";
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

  case VS_SCALINGMETHOD_AUTO:
    effectString = "special://xbmc/system/shaders/yuv2rgb_4x4_d3d.fx";
    m_bUseHQScaler = true;
    break;

  default:
    break;
  }

  if(m_bUseHQScaler)
  {

    if(m_scalingMethod == VS_SCALINGMETHOD_AUTO && m_sourceWidth >= 1280)
    {
      m_bUseHQScaler = false;
      return;
    }

    if(!LoadEffect(m_YUV2RGBHQScalerEffect, effectString))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to load shader %s.", effectString.c_str());
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "Video Renderering", "Failed to init video scaler, falling back to bilinear scaling.");
      m_bUseHQScaler = false;
      return;
    }

    if(!m_HQKernelTexture.Create(256, 1, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_MANAGED))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create kernel texture.");
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "Video Renderering", "Failed to init video scaler, falling back to bilinear scaling.");
      m_YUV2RGBHQScalerEffect.Release();
      m_bUseHQScaler = false;
      return;
    }

    CConvolutionKernel kern(m_scalingMethod == VS_SCALINGMETHOD_AUTO ? VS_SCALINGMETHOD_LANCZOS3_FAST : m_scalingMethod, 256);

    float *kernelVals = kern.GetFloatPixels();
    D3DXFLOAT16 float16Vals[256*4];

    for(int i = 0; i < 256*4; i++)
      float16Vals[i] = kernelVals[i];

    D3DLOCKED_RECT lr;
    m_HQKernelTexture.LockRect(0, &lr, NULL, D3DLOCK_DISCARD);
    memcpy(lr.pBits, float16Vals, sizeof(D3DXFLOAT16)*256*4);
    m_HQKernelTexture.UnlockRect(0);
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

  //If the GUI is active or we don't need scaling use the bilinear filter.
  if(!m_bUseHQScaler
    || !g_graphicsContext.IsFullScreenVideo()
    || g_graphicsContext.IsCalibrating()
    || (m_destRect.Width() == m_sourceWidth && m_destRect.Height() == m_sourceHeight))
  {
    RenderLowMem(m_YUV2RGBEffect, flags);
  }
  else
  {
    RenderLowMem(m_YUV2RGBHQScalerEffect, flags);
  }
}

void CWinRenderer::RenderLowMem(CD3DEffect &effect, DWORD flags)
{
  //If no effect is loaded, use the default.
  if (!effect.Get())
    LoadEffect(effect, "special://xbmc/system/shaders/yuv2rgb_d3d.fx");

  CSingleLock lock(g_graphicsContext);

  int index = m_iYV12RenderBuffer;
  SVideoBuffer& buf = m_VideoBuffers[index];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX3 );

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
      m_destRect.x1                                                      ,  m_destRect.y1, 0.0f, 1.0f,
      (m_sourceRect.x1) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y1, 0.0f, 1.0f,
      (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y1) / m_sourceHeight,
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x2                                                      ,  m_destRect.y2, 0.0f, 1.0f,
      (m_sourceRect.x2) / m_sourceWidth                                  , (m_sourceRect.y2) / m_sourceHeight,
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1) , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    },
    {
      m_destRect.x1                                                       ,  m_destRect.y2, 0.0f, 1.0f,
      (m_sourceRect.x1) / m_sourceWidth                                   , (m_sourceRect.y2) / m_sourceHeight,
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1)  , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1),
      (m_sourceRect.x1 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceWidth>>1)  , (m_sourceRect.y2 / 2.0f + CHROMAOFFSET_HORIZ) / (m_sourceHeight>>1)
    }
  };

  for(int i = 0; i < 4; i++)
  {
    verts[i].x -= 0.5;
    verts[i].y -= 0.5;
  }

  float contrast   = g_settings.m_currentVideoSettings.m_Contrast * 0.02f;
  float blacklevel = g_settings.m_currentVideoSettings.m_Brightness * 0.01f - 0.5f;

  D3DXMATRIX temp, mat;
  D3DXMatrixIdentity(&mat);

  if (!(m_flags & CONF_FLAGS_YUV_FULLRANGE))
  {
    D3DXMatrixTranslation(&temp, - 16.0f / 255
                               , - 16.0f / 255
                               , - 16.0f / 255);
    D3DXMatrixMultiply(&mat, &mat, &temp);

    D3DXMatrixScaling(&temp, 255.0f / (235 - 16)
                           , 255.0f / (240 - 16)
                           , 255.0f / (240 - 16));
    D3DXMatrixMultiply(&mat, &mat, &temp);
  }

  D3DXMatrixTranslation(&temp, 0.0f, - 0.5f, - 0.5f);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  switch(CONF_FLAGS_YUVCOEF_MASK(m_flags))
  {
   case CONF_FLAGS_YUVCOEF_240M:
     memcpy(temp.m, yuv_coef_smtp240m, 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_BT709:
     memcpy(temp.m, yuv_coef_bt709   , 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_BT601:
     memcpy(temp.m, yuv_coef_bt601   , 4*4*sizeof(float)); break;
   case CONF_FLAGS_YUVCOEF_EBU:
     memcpy(temp.m, yuv_coef_ebu     , 4*4*sizeof(float)); break;
   default:
     memcpy(temp.m, yuv_coef_bt601   , 4*4*sizeof(float)); break;
  }
  temp.m[3][3] = 1.0f;
  D3DXMatrixMultiply(&mat, &mat, &temp);

  D3DXMatrixTranslation(&temp, blacklevel, blacklevel, blacklevel);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  D3DXMatrixScaling(&temp, contrast, contrast, contrast);
  D3DXMatrixMultiply(&mat, &mat, &temp);

  float texSteps[] = {1.0f/(float)m_sourceWidth,        1.0f/(float)m_sourceHeight,
                      1.0f/(float)(m_sourceWidth >> 1), 1.0f/(float)(m_sourceHeight >> 1)};

  effect.SetMatrix( "g_ColorMatrix", &mat);
  effect.SetTechnique( "YUV2RGB_T" );
  effect.SetTexture( "g_YTexture",  buf.planes[0].texture ) ;
  effect.SetTexture( "g_UTexture",  buf.planes[1].texture ) ;
  effect.SetTexture( "g_VTexture",  buf.planes[2].texture ) ;
  effect.SetTexture( "g_KernelTexture", m_HQKernelTexture );
  effect.SetFloatArray("g_YStep", &texSteps[0], 2);
  effect.SetFloatArray("g_UVStep", &texSteps[2], 2);

  UINT cPasses, iPass;
  if (!effect.Begin( &cPasses, 0 ))
  {
    CLog::Log(LOGERROR, "CWinRenderer::RenderLowMem - failed to begin d3d effect");
    return;
  }

  for( iPass = 0; iPass < cPasses; iPass++ )
  {
    if (!effect.BeginPass( iPass ))
    {
      CLog::Log(LOGERROR, "CWinRenderer::RenderLowMem - failed to begin d3d effect pass");
      break;
    }

    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
    pD3DDevice->SetTexture(0, NULL);
    pD3DDevice->SetTexture(1, NULL);
    pD3DDevice->SetTexture(2, NULL);

    effect.EndPass() ;
  }

  effect.End() ;
  pD3DDevice->SetPixelShader( NULL );
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
    RenderLowMem(m_YUV2RGBEffect, 0);
    pD3DDevice->EndScene();
    m_destRect = saveSize;
    pD3DDevice->SetRenderTarget(0, oldRT);
    oldRT->Release();

    D3DLOCKED_RECT lockedRect;
    if (D3D_OK == surface->LockRect(&lockedRect, NULL, NULL))
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
  if (!buf.planes[0].texture.Create(m_sourceWidth    , m_sourceHeight    , 1, 0, D3DFMT_L8, D3DPOOL_MANAGED)
  ||  !buf.planes[1].texture.Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED)
  ||  !buf.planes[2].texture.Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED))
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
    || method == VS_SCALINGMETHOD_LANCZOS3
    || method == VS_SCALINGMETHOD_AUTO)
      return true;
  }
  else if(method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

void CWinRenderer::SVideoBuffer::Clear()
{
  SAFE_RELEASE(proc);
  id = 0;
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    planes[i].texture.Release();
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void CWinRenderer::SVideoBuffer::StartRender()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].rect.pBits)
      planes[i].texture.UnlockRect(0);
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void CWinRenderer::SVideoBuffer::StartDecode()
{
  SAFE_RELEASE(proc);
  id = 0;
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].texture.LockRect(0, &planes[i].rect, NULL, 0) == false)
    {
      memset(&planes[i].rect, 0, sizeof(planes[i].rect));
      CLog::Log(LOGERROR, "CWinRenderer::SVideoBuffer::StartDecode - failed to lock texture into memory");
    }
  }
}

CPixelShaderRenderer::CPixelShaderRenderer()
    : CWinRenderer()
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
  CWinRenderer::Render(flags);
}



#endif
