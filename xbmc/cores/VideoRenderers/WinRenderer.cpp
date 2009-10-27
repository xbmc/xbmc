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

// http://www.martinreddy.net/gfx/faqs/colorconv.faq

YUVRANGE yuv_range_lim =  { 16, 235, 16, 240, 16, 240 };
YUVRANGE yuv_range_full = {  0, 255,  0, 255,  0, 255 };

YUVCOEF yuv_coef_bt601 = {
     0.0f,   1.403f,
  -0.344f,  -0.714f,
   1.773f,     0.0f,
};

YUVCOEF yuv_coef_bt709 = {
     0.0f,  1.5701f,
 -0.1870f, -0.4664f,
  1.8556f,     0.0f, /* page above have the 1.8556f as negative */
};

YUVCOEF yuv_coef_ebu = {
    0.0f,  1.140f,
 -0.396f, -0.581f,
  2.029f,    0.0f, 
};

YUVCOEF yuv_coef_smtp240m = {
     0.0f,  1.5756f,
 -0.2253f, -0.5000f, /* page above have the 0.5000f as positive */
  1.8270f,     0.0f,  
};


CWinRenderer::CWinRenderer(LPDIRECT3DDEVICE9 pDevice)
{
  m_pD3DDevice = pDevice;
  memset(m_pOSDYTexture,0,sizeof(LPDIRECT3DTEXTURE9)*NUM_BUFFERS);
  memset(m_pOSDATexture,0,sizeof(LPDIRECT3DTEXTURE9)*NUM_BUFFERS);
  memset(m_YUVMemoryTexture, 0, sizeof(m_YUVMemoryTexture));
  memset(m_YUVVideoTexture, 0, sizeof(m_YUVVideoTexture));

  m_iYV12RenderBuffer = 0;
  m_pYUV2RGBEffect = NULL;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

//********************************************************************************************************
void CWinRenderer::DeleteOSDTextures(int index)
{
  CSingleLock lock(g_graphicsContext);

  if (m_pOSDYTexture[index])
    SAFE_RELEASE(m_pOSDYTexture[index]);

  if (m_pOSDATexture[index])
    SAFE_RELEASE(m_pOSDATexture[index]);
  
  m_iOSDTextureHeight[index] = 0;
  m_iOSDTextureWidth = 0;
}

void CWinRenderer::Setup_Y8A8Render()
{
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  m_pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetSamplerState( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetSamplerState( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

  m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  m_pD3DDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_POINT /*g_stSettings.m_maxFilter*/ );
  m_pD3DDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_POINT /*g_stSettings.m_minFilter*/ );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
  m_pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX2 );
}

//***********************************************************************************************************
void CWinRenderer::CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride)
{
  for (int y = 0; y < h; ++y)
  {
    memcpy(dst, src, w);
    memcpy(dsta, srca, w);
    src += srcstride;
    srca += srcstride;
    dst += dststride;
    dsta += dststride;
  }
}

void CWinRenderer::DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
  // OSD is drawn after draw_slice / put_image
  // this means that the buffer has already been handed off to the RGB converter
  // solution: have separate OSD textures

  // if it's down the bottom, use sub alpha blending
  //  m_SubsOnOSD = (y0 > (int)(m_sourceRect.bottom - m_sourceRect.top) * 4 / 5);

  //Sometimes happens when switching between fullscreen and small window
  if( w == 0 || h == 0 )
  {
    CLog::Log(LOGINFO, "Zero dimensions specified to DrawAlpha, skipping");
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
  const CRect& rv = g_graphicsContext.GetViewWindow();

  // Vobsubs are defined to be 720 wide.
  // NOTE: This will not work nicely if we are allowing mplayer to render text based subs
  //       as it'll want to render within the pixel width it is outputting.

  float xscale;
  float yscale;

  if(true /*isvobsub*/) // xbox_video.cpp is fixed to 720x576 osd, so this should be fine
  { // vobsubs are given to us unscaled
    // scale them up to the full output, assuming vobsubs have same 
    // pixel aspect ratio as the movie, and are 720 pixels wide

    float pixelaspect = m_sourceFrameRatio * m_sourceHeight / m_sourceWidth;
    xscale = rv.Width() / 720.0f;
    yscale = xscale * g_settings.m_ResInfo[res].fPixelRatio / pixelaspect;
  }
  else
  { // text subs/osd assume square pixels, but will render to full size of view window
    // if mplayer could be fixed to use monitorpixelaspect when rendering it's osd
    // this would give perfect output, however monitorpixelaspect currently doesn't work
    // that way
    xscale = 1.0f;
    yscale = 1.0f;
  }
  
  // horizontal centering, and align to bottom of subtitles line
  osdRect.left = rv.x1 + (rv.Width() - (float)w * xscale) / 2.0f;
  osdRect.right = osdRect.left + (float)w * xscale;
  float relbottom = ((float)(g_settings.m_ResInfo[res].iSubtitles - g_settings.m_ResInfo[res].Overscan.top)) / (g_settings.m_ResInfo[res].Overscan.bottom - g_settings.m_ResInfo[res].Overscan.top);
  osdRect.bottom = rv.y1 + rv.Height() * relbottom;
  osdRect.top = osdRect.bottom - (float)h * yscale;

  RECT rc = { 0, 0, w, h };

  int iOSDBuffer = (m_iOSDRenderBuffer + 1) % m_NumOSDBuffers;

  //if new height is heigher than current osd-texture height, recreate the textures with new height.
  if (h > m_iOSDTextureHeight[iOSDBuffer])
  {
    CSingleLock lock(g_graphicsContext);

    DeleteOSDTextures(iOSDBuffer);
    m_iOSDTextureHeight[iOSDBuffer] = h;
    // Create osd textures for this buffer with new size
    if (
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_L8, D3DPOOL_DEFAULT, &m_pOSDYTexture[iOSDBuffer], NULL) ||
      D3D_OK != m_pD3DDevice->CreateTexture(m_iOSDTextureWidth, m_iOSDTextureHeight[iOSDBuffer], 1, 0, D3DFMT_LIN_A8, D3DPOOL_DEFAULT, &m_pOSDATexture[iOSDBuffer], NULL)
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

  // draw textures
  D3DLOCKED_RECT lr, lra;
  if (
    (D3D_OK == m_pOSDYTexture[iOSDBuffer]->LockRect(0, &lr, &rc, 0)) &&
    (D3D_OK == m_pOSDATexture[iOSDBuffer]->LockRect(0, &lra, &rc, 0))
  )
  {
    //clear the textures
    memset(lr.pBits, 0, lr.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    memset(lra.pBits, 0, lra.Pitch*m_iOSDTextureHeight[iOSDBuffer]);
    //draw the osd/subs
    CopyAlpha(w, h, src, srca, stride, (BYTE*)lr.pBits, (BYTE*)lra.pBits, lr.Pitch);
  }
  m_pOSDYTexture[iOSDBuffer]->UnlockRect(0);
  m_pOSDATexture[iOSDBuffer]->UnlockRect(0);

  //set module variables to calculated values
  m_OSDRect = osdRect;
  m_OSDWidth = (float)w;
  m_OSDHeight = (float)h;
  m_OSDRendered = true;
}

//********************************************************************************************************
void CWinRenderer::RenderOSD()
{
  int iRenderBuffer = m_iOSDRenderBuffer;

  if (!m_pOSDYTexture[iRenderBuffer] || !m_pOSDATexture[iRenderBuffer])
    return ;
  if (!m_OSDWidth || !m_OSDHeight)
    return ;

  CSingleLock lock(g_graphicsContext);

  //copy alle static vars to local vars because they might change during this function by mplayer callbacks
  float osdWidth = m_OSDWidth;
  float osdHeight = m_OSDHeight;
  DRAWRECT osdRect = m_OSDRect;
  //  if (!viewportRect.bottom && !viewportRect.right)
  //    return;

  // Set state to render the image
  m_pD3DDevice->SetTexture(0, m_pOSDYTexture[iRenderBuffer]);
  m_pD3DDevice->SetTexture(1, m_pOSDATexture[iRenderBuffer]);
  Setup_Y8A8Render();

  // clip the output if we are not in FSV so that zoomed subs don't go all over the GUI
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4] =
  {
    { osdRect.left         , osdRect.top            , 0.0f,
      1.0f, 0,
      0.0f                 , 0.0f,
      0.0f                 , 0.0f,
    }, 
    { osdRect.right        , osdRect.top            , 0.0f,
      1.0f, 0,
      osdWidth / m_OSDWidth, 0.0f,
      osdWidth / m_OSDWidth, 0.0f,
    }, 
    { osdRect.right        , osdRect.bottom         , 0.0f,
      1.0f, 0,
      osdWidth / m_OSDWidth, osdHeight / m_OSDHeight,
      osdWidth / m_OSDWidth, osdHeight / m_OSDHeight,
    }, 
    { osdRect.left         , osdRect.bottom         , 0.0f,
      1.0f, 0,
      0.0f                 , osdHeight / m_OSDHeight,
      0.0f                 , osdHeight / m_OSDHeight,
    }
  };

  m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
}

void CWinRenderer::ManageTextures()
{
  int neededbuffers = 0;
  if (m_NumOSDBuffers != 2)
  {
    m_NumOSDBuffers = 2;
    m_iOSDRenderBuffer = 0;
    m_OSDWidth = m_OSDHeight = 0;
    // buffers will be created on demand in DrawAlpha()
  }
  neededbuffers = 2;

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
  m_sourceWidth = width;
  m_sourceHeight = height;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

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
 
  if (clear)
    m_pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  Render(flags);
}

void CWinRenderer::FlipPage(int source)
{  
  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  /* we always decode into to the next buffer */
  ++m_iOSDRenderBuffer %= m_NumOSDBuffers;

  /* if osd wasn't rendered this time around, previuse should not be */
  /* displayed on next frame */
  if( !m_OSDRendered )
    m_OSDWidth = m_OSDHeight = 0;

  m_OSDRendered = false;

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

  m_iOSDRenderBuffer = 0;
  m_iYV12RenderBuffer = 0;
  m_NumOSDBuffers = 0;
  m_NumYV12Buffers = 0;
  m_OSDHeight = m_OSDWidth = 0;
  m_OSDRendered = false;

  m_iOSDTextureWidth = 0;
  m_iOSDTextureHeight[0] = 0;
  m_iOSDTextureHeight[1] = 0;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  return 0;
}


void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  // YV12 textures, subtitle and osd stuff
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteYV12Texture(i);
    DeleteOSDTextures(i);
  }

  g_Windowing.ReleaseEffect(m_pYUV2RGBEffect);
  m_pYUV2RGBEffect = NULL;
  m_bConfigured = false;
}

bool CWinRenderer::LoadEffect()
{
  CStdString pStrEffect = 
    "texture g_YTexture;"
    "texture g_UTexture;"
    "texture g_VTexture;"
    "sampler YSampler =" 
    "sampler_state"
    "{"
    "Texture = <g_YTexture>;"
    "MipFilter = LINEAR;"
    "MinFilter = LINEAR;"
    "MagFilter = LINEAR;"
    "};"

    "sampler USampler = "
    "sampler_state"
    "{"
    "Texture = <g_UTexture>;"
    "MipFilter = LINEAR;"
    "MinFilter = LINEAR;"
    "MagFilter = LINEAR;"
    "};"
    "sampler VSampler = "
    "sampler_state"
    "{"
    "Texture = <g_VTexture>;"
    "MipFilter = LINEAR;"
    "MinFilter = LINEAR;"
    "MagFilter = LINEAR;"
    "};"
    "struct VS_OUTPUT"
    "{"
    "float4 Position   : POSITION;"
    "float4 Diffuse    : COLOR0;"
    "float2 TextureUV  : TEXCOORD0;"
    "};"
    "struct PS_OUTPUT"
    "{"
    "float4 RGBColor : COLOR0;"   
    "};"
    "PS_OUTPUT YUV2RGB( VS_OUTPUT In)"
    "{" 
    "PS_OUTPUT OUT;"
    "OUT.RGBColor = In.Diffuse;"
    "float3 YUV = float3(tex2D (YSampler, In.TextureUV).x - (16.0 / 256.0) ,"
    "tex2D (USampler, In.TextureUV).x - (128.0 / 256.0), "
    "tex2D (VSampler, In.TextureUV).x - (128.0 / 256.0)); "
    "OUT.RGBColor.r = clamp((1.164 * YUV.x + 1.596 * YUV.z),0,255);"
    "OUT.RGBColor.g = clamp((1.164 * YUV.x - 0.813 * YUV.z - 0.391 * YUV.y), 0,255); "
    "OUT.RGBColor.b = clamp((1.164 * YUV.x + 2.018 * YUV.y),0,255);" 
    "OUT.RGBColor.a = 1.0;"
    "return OUT;"
    "}"
    "technique YUV2RGB_T"
    "{"
    "pass P0"
    "{ "         
    "PixelShader  = compile ps_2_0 YUV2RGB();"
    "}"
    "}";

  if(!g_Windowing.CreateEffect(pStrEffect, &m_pYUV2RGBEffect))
  {
    CLog::Log(LOGERROR, "D3DXCreateEffectFromFile %s failed", pStrEffect);
    return false;
  }

  return true;
}

void CWinRenderer::Render(DWORD flags)
{
  if( flags & RENDER_FLAG_NOOSD ) return;

  /* general stuff */
  RenderOSD();
}

void CWinRenderer::AutoCrop(bool bCrop)
{
  if (!m_YUVMemoryTexture[0][PLANE_Y]) return ;

  if (bCrop)
  {
    CSingleLock lock(g_graphicsContext);

    // apply auto-crop filter - only luminance needed, and we run vertically down 'n'
    // runs down the image.
    int min_detect = 8;                                // reasonable amount (what mplayer uses)
    int detect = (min_detect + 16)*m_sourceWidth;     // luminance should have minimum 16
    D3DLOCKED_RECT lr;
    lr.pBits = m_YUVMemoryTexture[0][PLANE_Y];
    lr.Pitch = m_sourceWidth;
   
    int total;
    // Crop top
    BYTE *s = (BYTE *)lr.pBits;
    g_stSettings.m_currentVideoSettings.m_CropTop = m_sourceHeight/2;
    for (unsigned int y = 0; y < m_sourceHeight/2; y++)
    {
      total = 0;
      for (unsigned int x = 0; x < m_sourceWidth; x++)
        total += s[x];
      s += lr.Pitch;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropTop = y;
        break;
      }
    }
    // Crop bottom
    s = (BYTE *)lr.pBits + (m_sourceHeight-1)*lr.Pitch;
    g_stSettings.m_currentVideoSettings.m_CropBottom = m_sourceHeight/2;
    for (unsigned int y = (int)m_sourceHeight; y > m_sourceHeight/2; y--)
    {
      total = 0;
      for (unsigned int x = 0; x < m_sourceWidth; x++)
        total += s[x];
      s -= lr.Pitch;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropBottom = m_sourceHeight - y;
        break;
      }
    }
    // Crop left
    s = (BYTE *)lr.pBits;
    g_stSettings.m_currentVideoSettings.m_CropLeft = m_sourceWidth/2;
    for (unsigned int x = 0; x < m_sourceWidth/2; x++)
    {
      total = 0;
      for (unsigned int y = 0; y < m_sourceHeight; y++)
        total += s[y * lr.Pitch];
      s++;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropLeft = x;
        break;
      }
    }
    // Crop right
    s = (BYTE *)lr.pBits + (m_sourceWidth-1);
    g_stSettings.m_currentVideoSettings.m_CropRight= m_sourceWidth/2;
    for (unsigned int x = (int)m_sourceWidth-1; x > m_sourceWidth/2; x--)
    {
      total = 0;
      for (unsigned int y = 0; y < m_sourceHeight; y++)
        total += s[y * lr.Pitch];
      s--;
      if (total > detect)
      {
        g_stSettings.m_currentVideoSettings.m_CropRight = m_sourceWidth - x;
        break;
      }
    }
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

void CWinRenderer::RenderLowMem(DWORD flags)
{
  if(m_pYUV2RGBEffect == NULL)
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
    m_YUVVideoTexture[index][i]->GetSurfaceLevel(0, &videoSurface);
    videoSurface->GetDesc(&desc);
    if(videoSurface->LockRect(&rect, NULL, 0) == D3D_OK)
    {
      for(unsigned int j = 0; j < desc.Height; j++)
      {
        memcpy((BYTE *)rect.pBits + (j * rect.Pitch), src + (j * desc.Width), rect.Pitch);
      }
      videoSurface->UnlockRect();
    }
    SAFE_RELEASE(videoSurface);
  }

  for (int i = 0; i < 3; ++i)
  {
    m_pD3DDevice->SetTexture(i, m_YUVVideoTexture[index][i]);
    m_pD3DDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
  }

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE  );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
 
  m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );  // was m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE ); ???
  m_pD3DDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX3 );

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

  m_pYUV2RGBEffect->SetTechnique( "YUV2RGB_T" );
  m_pYUV2RGBEffect->SetTexture( "g_YTexture",  m_YUVVideoTexture[index][0] ) ;
  m_pYUV2RGBEffect->SetTexture( "g_UTexture",  m_YUVVideoTexture[index][1] ) ;
  m_pYUV2RGBEffect->SetTexture( "g_VTexture",  m_YUVVideoTexture[index][2] ) ;

  UINT cPasses, iPass;
  if(FAILED (m_pYUV2RGBEffect->Begin( &cPasses, 0 )))
    return;

  for( iPass = 0; iPass < cPasses; iPass++ )
  {
    m_pYUV2RGBEffect->BeginPass( iPass );

    m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
    m_pD3DDevice->SetTexture(0, NULL);
    m_pD3DDevice->SetTexture(1, NULL);
    m_pD3DDevice->SetTexture(2, NULL);
  
    m_pYUV2RGBEffect->EndPass() ;
  }

  m_pYUV2RGBEffect->End() ;
  m_pD3DDevice->SetPixelShader( NULL );
}

void CWinRenderer::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);

  // create a new render surface to copy out of - note, this may be slow on some hardware
  // due to the TRUE parameter - you're supposed to use GetRenderTargetData.
  LPDIRECT3DSURFACE9 surface = NULL;
  if (D3D_OK == m_pD3DDevice->CreateRenderTarget(width, height, D3DFMT_LIN_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL))
  {
    LPDIRECT3DSURFACE9 oldRT;
    CRect saveSize = m_destRect;
    m_destRect.SetRect(0, 0, (float)width, (float)height);
    m_pD3DDevice->GetRenderTarget(0, &oldRT);
    m_pD3DDevice->SetRenderTarget(0, surface);
    m_pD3DDevice->BeginScene();
    RenderLowMem(0);
    m_pD3DDevice->EndScene();
    m_destRect = saveSize;
    m_pD3DDevice->SetRenderTarget(0, oldRT);
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

  if (videoPlanes[0] || videoPlanes[1] || videoPlanes[2])
    CLog::Log(LOGDEBUG, "Deleted YV12 texture (%i)", index);

  SAFE_RELEASE(videoPlanes[0]);
  SAFE_RELEASE(videoPlanes[1]);
  SAFE_RELEASE(videoPlanes[2]);

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
    D3D_OK != m_pD3DDevice->CreateTexture(m_sourceWidth, m_sourceHeight, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVVideoTexture[index][0], NULL) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVVideoTexture[index][1], NULL) ||
    D3D_OK != m_pD3DDevice->CreateTexture(m_sourceWidth / 2, m_sourceHeight / 2, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &m_YUVVideoTexture[index][2], NULL))
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


CPixelShaderRenderer::CPixelShaderRenderer(LPDIRECT3DDEVICE9 pDevice)
    : CWinRenderer(pDevice)
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
