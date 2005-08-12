/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "../../stdafx.h"
#include "RGBRenderer.h"
#include "GUIFontManager.h"
#include "../../util.h"


// coefficients used for YUV->RGB coefficient
#define Y_SCALE 1.164383561643835616438356164383f
#define UV_SCALE 1.1434977578475336322869955156951f
#define R_Vp 1.403f
#define G_Up -0.344f
#define G_Vp -0.714f
#define B_Up 1.770f

// formula is:

// y' = (y-16)*y_scale;
// u' = (u-128)*uv_scale;
// v' = (v-128)*uv_scale;

// r = y          + 1.403v
// g = y - 0.344u - 0.714v
// b = y + 1.770u



CRGBRenderer::CRGBRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
  m_YUVTexture = NULL;
  m_hInterleavingShader = 0;
  m_hYUVtoRGBLookup = 0;
  m_UVLookup = NULL;
  m_UVErrorLookup = NULL;
}

void CRGBRenderer::DeleteYUVTexture()
{
  g_graphicsContext.Lock();
  if (m_YUVTexture)
  {
    m_YUVTexture->Release();
    m_YUVTexture = NULL;
    CLog::Log(LOGDEBUG, "Deleted YUV video texture");
  }
  g_graphicsContext.Unlock();
}

void CRGBRenderer::ClearYUVTexture()
{
  D3DLOCKED_RECT lr;
  m_YUVTexture->LockRect(0, &lr, NULL, 0);
  fast_memset(lr.pBits, 0x00800080, lr.Pitch*m_iSourceHeight);
  m_YUVTexture->UnlockRect(0);
}

bool CRGBRenderer::CreateYUVTexture()
{
  g_graphicsContext.Lock();
  if (!m_YUVTexture)
  {
    if (D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_YUVTexture))
    {
      CLog::Log(LOGERROR, "Could not create YUV interleave texture");
      g_graphicsContext.Unlock();
      return false;
    }
    CLog::Log(LOGINFO, "Created YUV texture");
  }
  ClearYUVTexture();

  // Create the interlacing texture as well
  D3DLOCKED_RECT lr;
  m_YUVTexture->LockRect(0, &lr, NULL, 0);
  m_YUVFieldPitch = lr.Pitch / 4;
  XGSetTextureHeader(m_iSourceWidth + m_YUVFieldPitch, m_iSourceHeight / 2, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_YUVFieldTexture, 0, lr.Pitch * 2);
  m_YUVFieldTexture.Register(lr.pBits);
  m_YUVTexture->UnlockRect(0);

  g_graphicsContext.Unlock();
  return true;
}

void CRGBRenderer::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 2 buffers in windowed mode
  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (m_NumYV12Buffers != 1)
    { // we need to create the YUV texture
      CreateYUVTexture();
    }
  }
  else
  {
    if (m_NumYV12Buffers < 2)
    { // don't need the YUV texture in the GUI
      DeleteYUVTexture();
    }
  }
  CXBoxRenderer::ManageTextures();
}

unsigned int CRGBRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps)
{
  CXBoxRenderer::Configure(width, height, d_width, d_height, fps);
  CreateLookupTextures();
  m_bConfigured = true;
  return 0;
}

void CRGBRenderer::PrepareDisplay()
{
  ++m_iYV12DecodeBuffer %= m_NumYV12Buffers;
  CXBoxRenderer::PrepareDisplay();
}

void CRGBRenderer::Render()
{
  if ( !g_graphicsContext.IsFullScreenVideo() )
  {
    RenderLowMem();    
  }
  else
  {
    //always render the buffer that is not currently being decoded to.
    int iRenderBuffer = ((m_iYV12DecodeBuffer + 1) % m_NumYV12Buffers);
    if (!m_YUVTexture) return ;

    ResetEvent(m_eventTexturesDone);

    // First do the interleaving YV12->YUV, with chroma upsampling
    // if we are field syncing interlaced material, the chroma upsampling must be done per-field
    // rather than per-frame
    if (  m_iFieldSync != FS_NONE  )
    {
      m_pD3DDevice->SetTexture( 0, &m_YFieldTexture[iRenderBuffer]);
      m_pD3DDevice->SetTexture( 1, &m_UFieldTexture[iRenderBuffer]);
      m_pD3DDevice->SetTexture( 2, &m_VFieldTexture[iRenderBuffer]);
    }
    else
    {
      m_pD3DDevice->SetTexture( 0, m_YTexture[iRenderBuffer]);
      m_pD3DDevice->SetTexture( 1, m_UTexture[iRenderBuffer]);
      m_pD3DDevice->SetTexture( 2, m_VTexture[iRenderBuffer]);
    }

    for (int i = 0; i < 3; ++i)
    {
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    }

    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
    m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
    m_pD3DDevice->SetVertexShader( FVF_YV12VERTEX );
    m_pD3DDevice->SetPixelShader( m_hInterleavingShader );

    LPDIRECT3DSURFACE8 pYUVSurface, pOldRT;
    m_pD3DDevice->GetRenderTarget(&pOldRT);

    // we render into our striped surface
    if ( m_iFieldSync != FS_NONE )
      m_YUVFieldTexture.GetSurfaceLevel(0, &pYUVSurface);
    else
      m_YUVTexture->GetSurfaceLevel(0, &pYUVSurface);
    m_pD3DDevice->SetRenderTarget(pYUVSurface, NULL);

    // Render the image
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
    if( m_iFieldSync != FS_NONE )
    {
      //UV in interlaced video is seen as being closer to first line in first field and closer to second line in second field
      //we shift it with and offset of 1/4th pixel as counted in U height. meaning 1/8 in UV
      #define CHROMAOFFSET 0.125f


      // first feild is the left side of our textures
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.5f, 0.5f + CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.5f, 0.5f + CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, 0.0f, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth*0.5f + 0.5f, 0.5f + CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth*0.5f + 0.5f, 0.5f + CHROMAOFFSET);
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f + CHROMAOFFSET  );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f + CHROMAOFFSET  );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth, (float)m_iSourceHeight*0.5f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.5f, (float)m_iSourceHeight*0.25f + 0.5f + CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.5f, (float)m_iSourceHeight*0.25f + 0.5f + CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, 0.0f, (float)m_iSourceHeight*0.5f, 0, 1.0f );
      
      // second field is the right side of our textures
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_YFieldPitch + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_UVFieldPitch + 0.5f, 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_UVFieldPitch + 0.5f, 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_YUVFieldPitch, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_YFieldPitch + m_iSourceWidth + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_UVFieldPitch + m_iSourceWidth*0.5f + 0.5f, 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_UVFieldPitch + m_iSourceWidth*0.5f + 0.5f, 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_YUVFieldPitch + m_iSourceWidth, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_YFieldPitch + m_iSourceWidth + 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_UVFieldPitch + m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_UVFieldPitch + m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_YUVFieldPitch + m_iSourceWidth, (float)m_iSourceHeight*0.5f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_YFieldPitch + 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_UVFieldPitch + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_UVFieldPitch + 0.5f, (float)m_iSourceHeight*0.25f + 0.5f - CHROMAOFFSET );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_YUVFieldPitch, (float)m_iSourceHeight*0.5f, 0, 1.0f );
    }
    else
    {
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, 0.0f, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth*0.5f + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth*0.5f + 0.5f, 0.5f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth, 0.0f, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 0.5f, (float)m_iSourceHeight + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth*0.5f + 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth, (float)m_iSourceHeight, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, 0.5f, (float)m_iSourceHeight + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.5f, (float)m_iSourceHeight*0.5f + 0.5f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, 0.0f, (float)m_iSourceHeight, 0, 1.0f );
    }
    m_pD3DDevice->End();

    m_pD3DDevice->SetTexture(0, NULL);
    m_pD3DDevice->SetTexture(1, NULL);
    m_pD3DDevice->SetTexture(2, NULL);

    m_pD3DDevice->SetRenderTarget( pOldRT, NULL);
    pOldRT->Release();
    pYUVSurface->Release();


    //Okey, when the gpu is done with the textures here, they are free to be modified again
    m_pD3DDevice->InsertCallback(D3DCALLBACK_READ,&TextureCallback, (DWORD)m_eventTexturesDone);

    // Now perform the YUV->RGB conversion in a single pass, and render directly to the screen
    if ( m_iFieldSync != FS_NONE )
      m_pD3DDevice->SetTexture( 0, &m_YUVFieldTexture);
    else
      m_pD3DDevice->SetTexture( 0, m_YUVTexture);
    m_pD3DDevice->SetTexture( 1, m_UVLookup);
    m_pD3DDevice->SetTexture( 2, m_UVLookup);
    m_pD3DDevice->SetTexture( 3, m_UVErrorLookup);

    for (int i = 0; i < 4; ++i)
    {
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
      m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    }

    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
    m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
    m_pD3DDevice->SetVertexShader( FVF_YUVRGBVERTEX );
    m_pD3DDevice->SetPixelShader( m_hYUVtoRGBLookup );

    // If we are interlacing, we split the render range up into separate quads - one
    // per line, and render from alternating fields into each quad.
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
    if( m_iFieldSync != FS_NONE  )
    {
      float sourceScale = (float)(rs.bottom - rs.top) / (rd.bottom - rd.top);
      int middleSource = (rd.bottom - rd.top) >> 1;
      for (int i = 0; i < middleSource; i++)
      {
        float sTop = sourceScale * i + rs.top * 0.5f + 0.5f;
        // Render the first field
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + 0.5f, sTop );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)i*2 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + 0.5f, sTop );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)i*2 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + 0.5f, sTop + sourceScale );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)i*2 + 1 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + 0.5f, sTop + sourceScale );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)i*2 + 1 + rd.top, 0, 1.0f );

        // Render the second feild from the right hand side of the texture
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + m_YUVFieldPitch + 0.5f, sTop );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 0.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)i*2 + 1 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + m_YUVFieldPitch + 0.5f, sTop );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 0.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)i*2 + 1 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + m_YUVFieldPitch + 0.5f, sTop + sourceScale );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 1.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)i*2 + 2 + rd.top, 0, 1.0f );

        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + m_YUVFieldPitch + 0.5f, sTop + sourceScale );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 1.0f );
        m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)i*2 + 2 + rd.top, 0, 1.0f );
      }
    }
    else
    {
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + 0.5f, (float)rs.top + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 0.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 0.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 0.0f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + 0.5f, (float)rs.top + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 0.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 0.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 0.0f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right + 0.5f, (float)rs.bottom + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 1.0f, 1.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 1.0f, 1.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 1.0f, 1.0f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.bottom, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left + 0.5f, (float)rs.bottom + 0.5f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, 0.0f, 1.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, 0.0f, 1.0f );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, 0.0f, 1.0f );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.bottom, 0, 1.0f );
    }
    m_pD3DDevice->End();

    m_pD3DDevice->SetTexture(0, NULL);
    m_pD3DDevice->SetTexture(1, NULL);
    m_pD3DDevice->SetTexture(2, NULL);
    m_pD3DDevice->SetTexture(3, NULL);

    m_pD3DDevice->SetPixelShader( NULL );
  }
  RenderOSD();


#ifndef _DEBUG
  if (g_stSettings.m_bShowFreeMem)
#endif
    if (g_graphicsContext.IsFullScreenVideo())
    {
      // in debug mode, show freememory
      CStdStringW wszText;
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      wszText.Format(L"FreeMem %d/%d", stat.dwAvailPhys, stat.dwTotalPhys);

      CGUIFont* pFont = g_fontManager.GetFont("font13");
      if (pFont)
      {
        pFont->DrawText( 60, 40, 0xffffffff, wszText);
      }
    }
}

unsigned int CRGBRenderer::PreInit()
{
  CXBoxRenderer::PreInit();
  // Create the pixel shader
  if (!m_hInterleavingShader)
  {
    // shader to interleave separate Y U and V planes into a single YUV output
    const char* interleave =
      "xps.1.1\n"
      "def c0,1,0,0,0\n"
      "def c1,0,1,0,0\n"
      "def c2,0,0,1,0\n"
      "tex t0\n"
      "tex t1\n"
      "tex t2\n"
      "xmma discard,discard,r0, t0,c0, t1,c1\n"
      "mad r0, t2,c2,r0\n";

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
      "mad r0, r1, c0, r0\n";

    XGBuffer* pShader;
    XGAssembleShader("InterleaveShader", interleave, strlen(interleave), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hInterleavingShader);
    pShader->Release();

    XGBuffer* pShader2;
    XGAssembleShader("YUV2RGBShader", yuv2rgb, strlen(yuv2rgb), 0, NULL, &pShader2, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader2->GetBufferPointer(), &m_hYUVtoRGBLookup);
    pShader2->Release();
  }
  return 0;
}

void CRGBRenderer::UnInit()
{
  CXBoxRenderer::UnInit();
  DeleteYUVTexture();
  DeleteLookupTextures();
}

/*
// Actual YUV -> RGB routine using 32 bit floats
LONG CRGBRenderer::YUV2RGB(BYTE y, BYTE u, BYTE v, float &R, float &G, float &B)
{
 // normalize
 float Yp = (y-16)*Y_SCALE;
 float Up = (u-128)*UV_SCALE;
 float Vp = (v-128)*UV_SCALE;
 
 // clamp to valid ranges
 if (Yp > 255) Yp=255;
 if (Yp < 0) Yp=0;
 if (Up > 127) Up=127;
 if (Up < -128) Up=-128;
 if (Vp > 127) Vp=127;
 if (Vp < -128) Vp=-128;
 
 R = Yp + R_Vp * Vp;
 G = Yp + G_Up * Up + G_Vp * Vp;
 B = Yp + B_Up * Up;
 
 // clamp
 if (R < 0) R = 0;
 if (R > 255) R = 255;
 if (G < 0) G = 0;
 if (G > 255) G = 255;
 if (B < 0) B = 0;
 if (B > 255) B = 255;
 
 int r = (int)floor(R+0.5f);
 int g = (int)floor(G+0.5f);
 int b = (int)floor(B+0.5f);
 
 return (r << 16) + (g << 8) + b;
}*/

/*
// Simulation routines - what the pixel shader really does
float CRGBRenderer::nine_bits(float in)
{
 float out = floor(in + 0.5f);
 if (out > 255.0f) out = 255.0f;
 if (out < -255.0f) out = -255.0f;
 return out;
}
 
LONG CRGBRenderer::yuv2rgb_ps2(BYTE Y, BYTE U, BYTE V)
{
 float y = (float)(Y-16)*Y_SCALE;
 float u = (float)(U-128)*UV_SCALE;
 float v = (float)(V-128)*UV_SCALE;
 
 if (y > 255) y=255;
 if (y < 0) y=0;
 if (u > 127) u=127;
 if (u < -128) u=-128;
 if (v > 127) v=127;
 if (v < -128) v=-128;
 
 float y_ps = floor(y);
 float y_ps_err = floor(85.0f * (y - y_ps) + 0.5f);
 
 float uv[3], uv_ps[3], uv_ps_err[3], rgb[3], rgb_err[3];
 uv[0] = R_Vp * v;
 uv[1] = G_Up * u + G_Vp * v;
 uv[2] = B_Up * u;
 
 for (int i=0; i<3; i++)
 {
  uv_ps[i] = floor(uv[i]*0.5f-0.5f)*2+1;
  uv_ps_err[i] = floor(85.0f * (uv[i]-uv_ps[i]) + 0.5f);
 
  rgb[i] = y_ps + uv_ps[i];
  rgb_err[i] = y_ps_err + uv_ps_err[i];
 
  // clamp
  rgb[i] = nine_bits(rgb[i]);
  rgb_err[i] = nine_bits(rgb_err[i]);
 
  rgb[i] += floor(rgb_err[i]/85.0f + 0.5f);
  if (rgb[i] < 0) rgb[i] = 0;
  if (rgb[i] > 255) rgb[i] = 255;
 }
 
 int r = (int)rgb[0];
 int g = (int)rgb[1];
 int b = (int)rgb[2];
 
 return (r << 16) + (g << 8) + b;
}*/

void CRGBRenderer::DeleteLookupTextures()
{
  g_graphicsContext.Lock();
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
  g_graphicsContext.Unlock();
}

bool CRGBRenderer::CreateLookupTextures()
{
  if (
    D3D_OK != m_pD3DDevice->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, 0, &m_UVLookup) ||
    D3D_OK != m_pD3DDevice->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, 0, &m_UVErrorLookup)
  )
  {
    CLog::Log(LOGERROR, "Could not create RGB lookup textures");
    return false;
  }

  // fill in the lookup texture
  // create a temporary buffer as we need to swizzle the result
  D3DLOCKED_RECT lr;
  BYTE *pBuff = new BYTE[256 * 256 * 4];
  BYTE *pErrorBuff = new BYTE[256 * 256 * 4];
  if (pBuff && pErrorBuff)
  {
    // first column is our luminance data
    for (int y = 0; y < 256; y++)
    {
      float fY = (float)(y - 16) * Y_SCALE;
      if (fY < 0.0f)
        fY = 0.0f;
      if (fY > 255.0f)
        fY = 255.0f;
      float fWhole = floor(fY);
      float fFrac = floor((fY - fWhole) * 85.0f + 0.5f);   // 0 .. 1.0
      for (int i = 0; i < 3; i++)
      {
        pBuff[1024*y + i] = (BYTE)fWhole;
        pErrorBuff[1024*y + i] = 0;
      }
      pBuff[1024*y + 3] = (BYTE)fFrac; // alpha
      pErrorBuff[1024*y + 3] = 0;
    }
    for (int u = 1; u < 256; u++)
    {
      for (int v = 0; v < 256; v++)
      {
        // convert to -0.5 .. 0.5
        float fV = (float)(v - 128) * UV_SCALE;
        float fU = (float)(u - 128) * UV_SCALE;

        if (fU > 127) fU = 127;
        if (fU < -128) fU = -128;
        if (fV > 127) fV = 127;
        if (fV < -128) fV = -128;

        // have U and V, calculate R, G and B contributions (lie between 0 and 255)
        // -1 is mapped to 0, 1 is mapped to 255
        float r = R_Vp * fV;
        float g = G_Up * fU + G_Vp * fV;
        float b = B_Up * fU;

        float r_rnd = floor(r * 0.5f - 0.5f) * 2 + 1;
        float g_rnd = floor(g * 0.5f - 0.5f) * 2 + 1;
        float b_rnd = floor(b * 0.5f - 0.5f) * 2 + 1;

        float ps_r = (r_rnd - 1) * 0.5f + 128.0f;
        float ps_g = (g_rnd - 1) * 0.5f + 128.0f;
        float ps_b = (b_rnd - 1) * 0.5f + 128.0f;

        if (ps_r > 255.0f)
          ps_r = 255.0f;
        if (ps_r < 0.0f)
          ps_r = 0.0f;
        if (ps_g > 255.0f)
          ps_g = 255.0f;
        if (ps_g < 0.0f)
          ps_g = 0.0f;
        if (ps_b > 255.0f)
          ps_b = 255.0f;
        if (ps_b < 0.0f)
          ps_b = 0.0f;

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
    m_UVLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVLookup->UnlockRect(0);
    delete[] pBuff;
    m_UVErrorLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pErrorBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVErrorLookup->UnlockRect(0);
    delete[] pErrorBuff;
  }
  return true;
}
