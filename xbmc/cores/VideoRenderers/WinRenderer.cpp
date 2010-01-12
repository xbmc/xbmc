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
#include "Util.h"
#include "Settings.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "MathUtils.h"

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
  memset(m_YUVMemoryTexture, 0, sizeof(m_YUVMemoryTexture));

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;
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

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return -1;

  YUVMEMORYPLANES &planes = m_YUVMemoryTexture[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;

  D3DLOCKED_RECT rect;
  for(int i=0;i<3;i++)
  {
    rect.pBits = planes[i];
    if(i == 0)
      rect.Pitch = m_sourceWidth;
    else
      rect.Pitch = m_sourceWidth / 2;
    image->stride[i] = rect.Pitch;
    image->plane[i] = (BYTE*)rect.pBits;
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

  if (!m_YUVMemoryTexture[m_iYV12RenderBuffer][0])
    return ;

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

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = 0;

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

  return 0;
}


void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_YUV2RGBEffect.Release();
  m_bConfigured = false;
  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);
  m_NumYV12Buffers = 0;
}

bool CWinRenderer::LoadEffect()
{
  CStdString filename = "special://xbmc/system/shaders/yuv2rgb_d3d.fx";

  XFILE::CFileStream file;
  if(!file.Open(filename))
  {
    CLog::Log(LOGERROR, "CWinRenderer::LoadEffect - failed to open file %s", filename.c_str());
    return false;
  }

  CStdString pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!m_YUV2RGBEffect.Create(pStrEffect))
  {
    CLog::Log(LOGERROR, "D3DXCreateEffectFromFile %s failed", pStrEffect.c_str());
    return false;
  }

  return true;
}

void CWinRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;
}

void CWinRenderer::RenderLowMem(DWORD flags)
{
  if (!m_YUV2RGBEffect.Get())
    LoadEffect();

  CSingleLock lock(g_graphicsContext);

  int index = m_iYV12RenderBuffer;
  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  // copy memory textures to video textures
  D3DLOCKED_RECT rect;
  LPDIRECT3DSURFACE9 videoSurface;
  D3DSURFACE_DESC desc;
  for(unsigned int i = 0; i < 3; i++)
  {
    BYTE* src = (BYTE *)m_YUVMemoryTexture[index][i];
    m_YUVVideoTexture[index][i].GetSurfaceLevel(0, &videoSurface);
    videoSurface->GetDesc(&desc);
    if(videoSurface->LockRect(&rect, NULL, 0) == D3D_OK)
    {
      if (rect.Pitch == desc.Width)
      {
        memcpy((BYTE *)rect.pBits, src, desc.Height * desc.Width);
      }
      else for(unsigned int j = 0; j < desc.Height; j++)
      {
        memcpy((BYTE *)rect.pBits + (j * rect.Pitch), src + (j * desc.Width), rect.Pitch);
      }
      videoSurface->UnlockRect();
    }
    SAFE_RELEASE(videoSurface);
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

  m_YUV2RGBEffect.SetMatrix( "g_ColorMatrix", &mat);
  m_YUV2RGBEffect.SetTechnique( "YUV2RGB_T" );
  m_YUV2RGBEffect.SetTexture( "g_YTexture",  m_YUVVideoTexture[index][0] ) ;
  m_YUV2RGBEffect.SetTexture( "g_UTexture",  m_YUVVideoTexture[index][1] ) ;
  m_YUV2RGBEffect.SetTexture( "g_VTexture",  m_YUVVideoTexture[index][2] ) ;

  UINT cPasses, iPass;
  if (!m_YUV2RGBEffect.Begin( &cPasses, 0 ))
  {
    CLog::Log(LOGERROR, "CWinRenderer::RenderLowMem - failed to begin d3d effect");
    return;
  }

  for( iPass = 0; iPass < cPasses; iPass++ )
  {
    if (!m_YUV2RGBEffect.BeginPass( iPass ))
    {
      CLog::Log(LOGERROR, "CWinRenderer::RenderLowMem - failed to begin d3d effect pass");
      break;
    }

    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
    pD3DDevice->SetTexture(0, NULL);
    pD3DDevice->SetTexture(1, NULL);
    pD3DDevice->SetTexture(2, NULL);

    m_YUV2RGBEffect.EndPass() ;
  }

  m_YUV2RGBEffect.End() ;
  pD3DDevice->SetPixelShader( NULL );
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
    RenderLowMem(0);
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
  YUVVIDEOPLANES &videoPlanes = m_YUVVideoTexture[index];
  YUVMEMORYPLANES &memoryPlanes = m_YUVMemoryTexture[index];

  videoPlanes[0].Release();
  videoPlanes[1].Release();
  videoPlanes[2].Release();

  SAFE_DELETE_ARRAY(memoryPlanes[0]);
  SAFE_DELETE_ARRAY(memoryPlanes[1]);
  SAFE_DELETE_ARRAY(memoryPlanes[2]);

  m_NumYV12Buffers = 0;
}

void CWinRenderer::ClearYV12Texture(int index)
{
  YUVMEMORYPLANES &planes = m_YUVMemoryTexture[index];
  D3DLOCKED_RECT rect;

  rect.pBits = planes[0];
  rect.Pitch = m_sourceWidth;
  memset(rect.pBits, 0,   rect.Pitch * m_sourceHeight);

  rect.pBits = planes[1];
  rect.Pitch = m_sourceWidth / 2;
  memset(rect.pBits, 128, rect.Pitch * m_sourceHeight>>1);

  rect.pBits = planes[2];
  rect.Pitch = m_sourceWidth / 2;
  memset(rect.pBits, 128, rect.Pitch * m_sourceHeight>>1);
}




bool CWinRenderer::CreateYV12Texture(int index)
{

  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);
  if (
    !m_YUVVideoTexture[index][0].Create(m_sourceWidth, m_sourceHeight, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED) ||
    !m_YUVVideoTexture[index][1].Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED) ||
    !m_YUVVideoTexture[index][2].Create(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 video texture %i", index);
    return false;
  }

  if (
    NULL == (m_YUVMemoryTexture[index][0] = new BYTE[m_sourceWidth * m_sourceHeight]) ||
    NULL == (m_YUVMemoryTexture[index][1] = new BYTE[m_sourceWidth / 2 * m_sourceHeight / 2]) ||
    NULL == (m_YUVMemoryTexture[index][2] = new BYTE[m_sourceWidth / 2* m_sourceHeight / 2]))
  {
    CLog::Log(LOGERROR, "Unable to create YV12 memory texture %i", index);
    return false;
  }

  ClearYV12Texture(index);
  CLog::Log(LOGDEBUG, "created yv12 texture %i", index);
  return true;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
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
  // this is the low memory renderer
  CWinRenderer::RenderLowMem(flags);
  CWinRenderer::Render(flags);
}

#endif
