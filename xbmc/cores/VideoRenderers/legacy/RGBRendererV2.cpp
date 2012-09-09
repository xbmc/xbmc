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
#include "RGBRendererV2.h"

#define SURFTOTEX(a) ((a)->Parent ? (a)->Parent : (D3DBaseTexture*)(a))

//#define DBGBOB

CRGBRendererV2::CRGBRendererV2(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
  m_444PTextureFull = NULL;
  m_444PTextureField = NULL;

  m_hInterleavingShader = 0;
  m_hInterleavingShaderAlpha = 0;
  m_hYUVtoRGBLookup = 0;
  m_UVLookup = NULL;
  m_UVErrorLookup = NULL;
  m_motionpass = 5;
  memset(&m_yuvcoef_last, 0, sizeof(YUVCOEF));
  memset(&m_yuvrange_last, 0, sizeof(YUVRANGE));
}

void CRGBRendererV2::FlipPage(int source)
{
  m_444GeneratedFull = false;

  CXBoxRenderer::FlipPage(source);
}

void CRGBRendererV2::Delete444PTexture()
{
  CSingleLock lock(g_graphicsContext);
  SAFE_RELEASE(m_444PTextureFull);
  SAFE_RELEASE(m_444PTextureField);
  CLog::Log(LOGDEBUG, "Deleted 444P video textures");
}

void CRGBRendererV2::Clear444PTexture(bool full, bool field)
{
  CSingleLock lock(g_graphicsContext);
  if(m_444PTextureFull && full)
  {
    D3DLOCKED_RECT lr;
    m_444PTextureFull->LockRect(0, &lr, NULL, 0);
    memset(lr.pBits, 0x00, lr.Pitch*m_iSourceHeight);
    m_444PTextureFull->UnlockRect(0);
  }

  if(m_444PTextureField && field)
  {
    D3DLOCKED_RECT lr;
    m_444PTextureField->LockRect(0, &lr, NULL, 0);
#ifdef DBGBOB
    memset(lr.pBits, 0xFF, lr.Pitch*m_iSourceHeight>>1);
#else
    memset(lr.pBits, 0x00, lr.Pitch*m_iSourceHeight>>1);
#endif
    m_444PTextureField->UnlockRect(0);
  }
  m_444GeneratedFull = false;
}

bool CRGBRendererV2::Create444PTexture(bool full, bool field)
{
  CSingleLock lock(g_graphicsContext);
  if (!m_444PTextureFull && full)
  {
    if(D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_444PTextureFull))
      return false;

    CLog::Log(LOGINFO, "Created 444P full texture");
  }

  if (!m_444PTextureField && field)
  {
    if(D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight>>1, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_444PTextureField))
      return false;
    CLog::Log(LOGINFO, "Created 444P field texture");
  }
  return true;
}

void CRGBRendererV2::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 0 buffers in windowed mode
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    if (m_444PTextureFull || m_444PTextureField)
      Delete444PTexture();
  }

  CXBoxRenderer::ManageTextures();

  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (!m_444PTextureFull)
    {
      Create444PTexture(true, false);
      Clear444PTexture(true, false);
    }
  }
}

bool CRGBRendererV2::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CXBoxRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;

  // create our lookup textures for yv12->rgb translation,
  if(!CreateLookupTextures(m_yuvcoef, m_yuvrange) )
    return false;

  m_bConfigured = true;
  return true;
}

void CRGBRendererV2::Render(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);
  if ( !g_graphicsContext.IsFullScreenVideo() )
  {
    RenderLowMem(flags);
  }
  else
  {
    int index = m_iYV12RenderBuffer;

    if( !(flags & RENDER_FLAG_NOLOCK) )
      if( WaitForSingleObject(m_eventTexturesDone[index], 500) == WAIT_TIMEOUT )
        CLog::Log(LOGWARNING, __FUNCTION__" - Timeout waiting for texture %d", index);

    D3DSurface* p444PSourceFull = NULL;
    D3DSurface* p444PSourceField = NULL;

    if( flags & (RENDER_FLAG_TOP|RENDER_FLAG_BOT) )
    {
      if(!m_444PTextureField)
      {
        Create444PTexture(false, true);
        Clear444PTexture(false, true);
      }
      if(!m_444PTextureField)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Couldn't create field texture");
        return;
      }

      m_444PTextureField->GetSurfaceLevel(0, &p444PSourceField);
    }

    if(!m_444PTextureFull)
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Couldn't create full texture");
      return;
    }

    m_444PTextureFull->GetSurfaceLevel(0, &p444PSourceFull);

    //UV in interlaced video is seen as being closer to first line in first field and closer to second line in second field
    //we shift it with an offset of 1/4th pixel (1/8 in UV planes)
    //This need only be done when field scaling
    #define CHROMAOFFSET_VERT 0.125f

    //Each chroma sample is not said to be between the first and second sample as in the vertical case
    //first Y(1) <=> UV(1), Y(2) <=> ( UV(1)+UV(2) ) / 2, Y(3) <=> UV(2)
    //we wish to offset this by 1/2 pxiel to le left, which in the half rez of UV planes means 1/4th
    #define CHROMAOFFSET_HORIZ 0.25f


    //Example of how YUV has it's Luma and Chroma data stored
    //for progressive video
    //L L L L L L L L L L
    //C   C   C   C   C
    //L L L L L L L L L L

    //Example of how YUV has Chroma subsampled in interlaced displays
    //FIELD 1               FIELD 2
    //L L L L L L L L L L
    //C   C   C   C   C
    //                      L L L L L L L L L L
    //
    //L L L L L L L L L L
    //                      C   C   C   C   C
    //                      L L L L L L L L L L
    //
    //L L L L L L L L L L
    //C   C   C   C   C
    //                      L L L L L L L L L L
    //
    //.........................................
    //.........................................

    m_pD3DDevice->SetRenderState( D3DRS_SWATHWIDTH, 15 );
    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );

    DWORD alphaenabled;
    m_pD3DDevice->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphaenabled );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );

    RECT rsf = { rs.left, rs.top>>1, rs.right, rs.bottom>>1 };

    if( !m_444GeneratedFull )
    {
      m_444GeneratedFull = true;
      InterleaveYUVto444P(
          m_YUVTexture[index][FIELD_FULL],
          NULL,          // use motion from last frame as motion value
          p444PSourceFull,
          rs, rs, rs,
          1, 1,
          0.0f, 0.0f,
          CHROMAOFFSET_HORIZ, 0.0f);
    }
#ifdef DBGBOB
    m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
#endif

    if( flags & RENDER_FLAG_TOP )
    {
      InterleaveYUVto444P(
          m_YUVTexture[index][FIELD_TOP],
          m_444PTextureFull, // use a downscaled motion value from the full frame,
          p444PSourceField,
          rsf, rs, rsf,
          1, 1,
          0.0f, 0.0f,
          CHROMAOFFSET_HORIZ, +CHROMAOFFSET_VERT);
    }
    else if( flags & RENDER_FLAG_BOT )
    {
      InterleaveYUVto444P(
          m_YUVTexture[index][FIELD_BOT],
          m_444PTextureFull, // use a downscaled motion value from the full frame,
          p444PSourceField,
          rsf, rs, rsf,
          1, 1,
          0.0f, 0.0f,
          CHROMAOFFSET_HORIZ, -CHROMAOFFSET_VERT);
    }

#ifdef DBGBOB
    m_pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
#endif

    //Okey, when the gpu is done with the textures here, they are free to be modified again
    if( !(flags & RENDER_FLAG_NOUNLOCK) )
      m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone[index]);

    // Now perform the YUV->RGB conversion in a single pass, and render directly to the screen
    m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f );

    if(true)
    {
      // NOTICE, field motion can have been replaced by downscaled frame motion
      // this method uses the difference between fields to estimate motion
      // it work sorta, but it can't for example handle horizontal
      // hairlines wich only exist in one field, they will flicker
      // as they get considered motion


      // render the full frame
      m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      RenderYUVtoRGB(m_444PTextureFull, rs, rd, 0.0f, 0.0f);

      // render the field texture ontop
      if(m_444PTextureField && p444PSourceField)
      {
        m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        m_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
        m_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, m_motionpass);

        if(flags & RENDER_FLAG_TOP)
          RenderYUVtoRGB(m_444PTextureField, rsf, rd, 0.0f, 0.25);
        else
          RenderYUVtoRGB(m_444PTextureField, rsf, rd, 0.0f, -0.25);
      }
    }
    else
    {
      // this method will use the difference between this and previous
      // frame as an estimate for motion. this will currently fail
      // on the first field that has motion. as then only that line
      // has motion. if the alpha channel where first downscaled
      // to the field texture lineary we should get away from that

      // render the field texture first
      if(m_444PTextureField && p444PSourceField)
      {
        m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        if(flags & RENDER_FLAG_TOP)
          RenderYUVtoRGB(m_444PTextureField, rsf, rd, 0.0f, 0.25);
        else
          RenderYUVtoRGB(m_444PTextureField, rsf, rd, 0.0f, -0.25);
      }

      // fill in any place we have no motion with full texture
      m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
      m_pD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
      m_pD3DDevice->SetRenderState(D3DRS_ALPHAREF, m_motionpass);
      RenderYUVtoRGB(m_444PTextureFull, rs, rd, 0.0f, 0.0f);
    }

    m_pD3DDevice->SetScreenSpaceOffset(0.0f, 0.0f);
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, alphaenabled );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );

    m_pD3DDevice->SetTexture(0, NULL);
    m_pD3DDevice->SetTexture(1, NULL);
    m_pD3DDevice->SetTexture(2, NULL);
    m_pD3DDevice->SetTexture(3, NULL);

    m_pD3DDevice->SetPixelShader( NULL );

    SAFE_RELEASE(p444PSourceFull);
    SAFE_RELEASE(p444PSourceField);
  }

  CXBoxRenderer::Render(flags);
}

unsigned int CRGBRendererV2::PreInit()
{
  CXBoxRenderer::PreInit();
  // Create the pixel shader
  if (!m_hInterleavingShader)
  {
    CSingleLock lock(g_graphicsContext);
    // shader to interleave separate Y U and V planes into a single YUV output
    const char* interleave =
      "xps.1.1\n"
      "def c0,1,0,0,0\n"
      "def c1,0,1,0,0\n"
      "def c2,0,0,1,0\n"
      "tex t0\n"
      "tex t1\n"
      "tex t2\n"
      "tex t3\n"

      // interleave our data
      "xmma discard,discard,r0, t0,c0, t1,c1\n"
      "mad r0, t2,c2,r0\n"

      "sub_x4 r1, r0,t3\n"      // calculate the differens in this pixel values
      "dp3    r1.rgba, r1,r1\n" // take the absolute of the "yuv" difference vector
//      "add_d2 r1.a, r1, t3\n"   // average with previouse value to avoid minor changes
      "mov    r0.a, r1";

    const char* interleavealpha =
      "xps.1.1\n"
      "def c0,1,0,0,0\n"
      "def c1,0,1,0,0\n"
      "def c2,0,0,1,0\n"
      "tex t0\n"
      "tex t1\n"
      "tex t2\n"
      "tex t3\n"

      // interleave our data
      "xmma discard,discard,r0, t0,c0, t1,c1\n"
      "mad r0, t2,c2,r0\n"

      // use alpha from t3
      "mov r0.a, t3";

    // shader for 14bit accurate YUV to RGB (single pass :)
    const char* yuv2rgb =
      "xps.1.1\n"
      "def c0,0.0117647,0.0117647,0.0117647,0\n"
      "tex t0\n"
      "texreg2ar t1, t0\n"
      "texreg2gb t2, t0\n"
      "texreg2gb t3, t0\n"
      "add r0, t1, t2_bx2\n"
      "add r1, t1.a, t3\n"
      "mad r0, r1, c0, r0\n"
      "mov r0.a, t0";

    XGBuffer* pShader;
    XGAssembleShader("InterleaveShader", interleave, strlen(interleave), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hInterleavingShader);
    pShader->Release();

    XGAssembleShader("InterleaveShaderAlpha", interleavealpha, strlen(interleavealpha), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hInterleavingShaderAlpha);
    pShader->Release();

    XGAssembleShader("YUV2RGBShader", yuv2rgb, strlen(yuv2rgb), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hYUVtoRGBLookup);
    pShader->Release();

  }

  return 0;
}

void CRGBRendererV2::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  Delete444PTexture();
  DeleteLookupTextures();

  if (m_hInterleavingShader)
  {
    m_pD3DDevice->DeletePixelShader(m_hInterleavingShader);
    m_hInterleavingShader = 0;
  }

  if (m_hInterleavingShaderAlpha)
  {
    m_pD3DDevice->DeletePixelShader(m_hInterleavingShaderAlpha);
    m_hInterleavingShaderAlpha = 0;
  }

  if (m_hYUVtoRGBLookup)
  {
    m_pD3DDevice->DeletePixelShader(m_hYUVtoRGBLookup);
    m_hYUVtoRGBLookup = 0;
  }
  CXBoxRenderer::UnInit();
}

void CRGBRendererV2::DeleteLookupTextures()
{
  if (m_UVLookup)
  {
    m_UVLookup->Release();
    m_UVLookup = NULL;
  }
  if (m_UVErrorLookup)
  {
    m_UVErrorLookup->Release();
    m_UVErrorLookup = NULL;
  }
}

bool CRGBRendererV2::CreateLookupTextures(const YUVCOEF &coef, const YUVRANGE &range)
{
  if(memcmp(&m_yuvcoef_last, &coef, sizeof(YUVCOEF)) == 0
  && memcmp(&m_yuvrange_last, &range, sizeof(YUVRANGE)) == 0)
    return true;

  DeleteLookupTextures();
  if (
    D3D_OK != m_pD3DDevice->CreateTexture(1  , 256, 1, 0, D3DFMT_A8L8    , 0, &m_YLookup) ||
    D3D_OK != m_pD3DDevice->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, 0, &m_UVLookup) ||
    D3D_OK != m_pD3DDevice->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, 0, &m_UVErrorLookup)
  )
  {
    DeleteLookupTextures();
    CLog::Log(LOGERROR, "Could not create RGB lookup textures");
    return false;
  }

  // fill in the lookup texture
  // create a temporary buffer as we need to swizzle the result
  D3DLOCKED_RECT lr;
  BYTE *pBuffY =     new BYTE[1 * 256 * 2];
  BYTE *pBuff =      new BYTE[256 * 256 * 4];
  BYTE *pErrorBuff = new BYTE[256 * 256 * 4];

  if(pBuffY)
  {
    // first column is our luminance data
    for (int y = 0; y < 256; y++)
    {
      float fY = (y - range.y_min) * 255.0f / (range.y_max - range.y_min);

      fY = CLAMP(fY, 0.0f, 255.0f);

      float fWhole = floor(fY);
      float fFrac = floor((fY - fWhole) * 85.0f + 0.5f);   // 0 .. 1.0

      pBuffY[2*y] = (BYTE)fWhole;
      pBuffY[2*y+1] = (BYTE)fFrac;
    }
  }

  if (pBuff && pErrorBuff)
  {
    for (int u = 1; u < 256; u++)
    {
      for (int v = 0; v < 256; v++)
      {
        // convert to -0.5 .. 0.5 ( -127.5 .. 127.5 )
        float fV = (v - range.v_min) * 255.f / (range.v_max - range.v_min) - 127.5f;
        float fU = (u - range.u_min) * 255.f / (range.u_max - range.u_min) - 127.5f;

        fU = CLAMP(fU, -127.5f, 127.5f);
        fV = CLAMP(fV, -127.5f, 127.5f);

        // have U and V, calculate R, G and B contributions (lie between 0 and 255)
        // -1 is mapped to 0, 1 is mapped to 255
        float r = coef.r_up * fU + coef.r_vp * fV;
        float g = coef.g_up * fU + coef.g_vp * fV;
        float b = coef.b_up * fU + coef.b_vp * fV;

        float r_rnd = floor(r * 0.5f - 0.5f) * 2 + 1;
        float g_rnd = floor(g * 0.5f - 0.5f) * 2 + 1;
        float b_rnd = floor(b * 0.5f - 0.5f) * 2 + 1;

        float ps_r = (r_rnd - 1) * 0.5f + 128.0f;
        float ps_g = (g_rnd - 1) * 0.5f + 128.0f;
        float ps_b = (b_rnd - 1) * 0.5f + 128.0f;

        ps_r = CLAMP(ps_r, 0.0f, 255.0f);
        ps_g = CLAMP(ps_g, 0.0f, 255.0f);
        ps_b = CLAMP(ps_b, 0.0f, 255.0f);

        float r_frac = floor((r - r_rnd) * 85.0f + 0.5f);
        float b_frac = floor((b - b_rnd) * 85.0f + 0.5f);
        float g_frac = floor((g - g_rnd) * 85.0f + 0.5f);

        pBuff[4*u + 1024*v + 0] = (BYTE)ps_b;
        pBuff[4*u + 1024*v + 1] = (BYTE)ps_g;
        pBuff[4*u + 1024*v + 2] = (BYTE)ps_r;
        pBuff[4*u + 1024*v + 3] = 0;
        pErrorBuff[4*u + 1024*v + 0] = (BYTE)b_frac;
        pErrorBuff[4*u + 1024*v + 1] = (BYTE)g_frac;
        pErrorBuff[4*u + 1024*v + 2] = (BYTE)r_frac;
        pErrorBuff[4*u + 1024*v + 3] = 0;
      }
    }
    m_YLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pBuffY, 0, NULL, lr.pBits, 1, 256, NULL, 2);
    m_YLookup->UnlockRect(0);
    m_UVLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVLookup->UnlockRect(0);
    m_UVErrorLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pErrorBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVErrorLookup->UnlockRect(0);

    m_yuvcoef_last = coef;
    m_yuvrange_last = range;
  }
  delete[] pBuff;
  delete[] pErrorBuff;
  delete[] pBuffY;

  return true;
}
void CRGBRendererV2::InterleaveYUVto444P(
      YUVPLANES          pSources,
      LPDIRECT3DTEXTURE8 pAlpha,
      LPDIRECT3DSURFACE8 pTarget,
      RECT &source, RECT &sourcealpha, RECT &target,
      unsigned cshift_x,  unsigned cshift_y,
      float    offset_x,  float    offset_y,
      float    coffset_x, float    coffset_y)
{
  coffset_x += offset_x / (1<<cshift_x);
  coffset_y += offset_y / (1<<cshift_y);

  for (int i = 0; i < 3; ++i)
  {
    m_pD3DDevice->SetTexture( i, pSources[i]);
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  }

  m_pD3DDevice->SetVertexShader( FVF_YV12VERTEX );

  if(pAlpha)
  {
    m_pD3DDevice->SetTexture(3, pAlpha);
    m_pD3DDevice->SetPixelShader( m_hInterleavingShaderAlpha );
  }
  else
  {
    m_pD3DDevice->SetTexture(3, SURFTOTEX(pTarget));
    m_pD3DDevice->SetPixelShader( m_hInterleavingShader );
  }

  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );


  LPDIRECT3DSURFACE8 pOldRT = NULL;
  if( pTarget )
  {
    m_pD3DDevice->GetRenderTarget(&pOldRT);
    m_pD3DDevice->SetRenderTarget(pTarget, NULL);
  }

  m_pD3DDevice->SetScreenSpaceOffset(-0.5f, -0.5f);
  m_pD3DDevice->Begin(D3DPT_QUADLIST);

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.left + offset_x, (float)source.top + offset_y);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.left>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.left>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)sourcealpha.left, (float)sourcealpha.top);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.left, (float)target.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.top + offset_y);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.right>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.right>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)sourcealpha.right, (float)sourcealpha.top);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.bottom + offset_y);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.right>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.right>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)sourcealpha.right, (float)sourcealpha.bottom);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.bottom, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.left + offset_x, (float)source.bottom + offset_y);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.left>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.left>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)sourcealpha.left, (float)sourcealpha.bottom);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.left, (float)target.bottom, 0, 1.0f );

  m_pD3DDevice->End();

  m_pD3DDevice->SetScreenSpaceOffset(0.0f, 0.0f);

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTexture(2, NULL);
  m_pD3DDevice->SetTexture(3, NULL);

  if( pOldRT )
  {
    m_pD3DDevice->SetRenderTarget( pOldRT, NULL);
    pOldRT->Release();
  }
}

void CRGBRendererV2::RenderYUVtoRGB(
      D3DBaseTexture* pSource,
      RECT &source, RECT &target,
      float offset_x, float offset_y)
{
    m_pD3DDevice->SetTexture( 0, pSource);
    m_pD3DDevice->SetTexture( 1, m_YLookup);
    m_pD3DDevice->SetTexture( 2, m_UVLookup);
    m_pD3DDevice->SetTexture( 3, m_UVErrorLookup);

    for (int i = 0; i < 4; ++i)
    {
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    }

    m_pD3DDevice->SetVertexShader( FVF_YUVRGBVERTEX );
    m_pD3DDevice->SetPixelShader( m_hYUVtoRGBLookup );

    // render the full frame
    m_pD3DDevice->Begin(D3DPT_QUADLIST);

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 0.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 0.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 0.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 0.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 0.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 0.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 1.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 1.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 1.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 1.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 1.0f );
    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 1.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.left + offset_x,  (float)source.top + offset_y);
    m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.left,  (float)target.top, 0, 1.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.top + offset_y);
    m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.top, 0, 1.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.bottom + offset_y);
    m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.bottom, 0, 1.0f );

    m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.left + offset_x,  (float)source.bottom + offset_y);
    m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.left,  (float)target.bottom, 0, 1.0f );

    m_pD3DDevice->End();

}
