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
#include "RGBRenderer.h"

#define SURFTOTEX(a) ((a)->Parent ? (a)->Parent : (D3DBaseTexture*)(a))


CRGBRenderer::CRGBRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
  memset(&m_444PTexture, 0, sizeof(m_444PTexture));

  m_hInterleavingShader = 0;
  m_hYUVtoRGBLookup = 0;
  m_UVLookup = NULL;
  m_UVErrorLookup = NULL;
  memset(&m_yuvcoef_last, 0, sizeof(YUVCOEF));
  memset(&m_yuvrange_last, 0, sizeof(YUVRANGE));
}

void CRGBRenderer::Delete444PTexture()
{
  CSingleLock lock(g_graphicsContext);
  if (m_444PTexture[FIELD_FULL])
  {
    D3DLOCKED_RECT lr;
    m_444PTexture[FIELD_FULL]->LockRect(0, &lr, NULL, 0);
    PVOID data = lr.pBits;
    m_444PTexture[FIELD_FULL]->UnlockRect(0);

    for(int f=0;f<MAX_FIELDS;f++)
    {
      m_444PTexture[f]->BlockUntilNotBusy();
      SAFE_DELETE(m_444PTexture[f]);
    }

    D3D_FreeContiguousMemory(data);
    CLog::Log(LOGDEBUG, "Deleted 444P video texture");
  }
}

void CRGBRenderer::Clear444PTexture()
{
  D3DLOCKED_RECT lr;
  m_444PTexture[FIELD_FULL]->LockRect(0, &lr, NULL, 0);
  memset(lr.pBits, 0x00000000, lr.Pitch*m_iSourceHeight);
  m_444PTexture[FIELD_FULL]->UnlockRect(0);
}

bool CRGBRenderer::Create444PTexture()
{
  CSingleLock lock(g_graphicsContext);
  if (!m_444PTexture[FIELD_FULL])
  {
    unsigned stride = ALIGN(m_iSourceWidth<<2,D3DTEXTURE_ALIGNMENT);
    void* data = D3D_AllocContiguousMemory( stride*m_iSourceHeight, D3DTEXTURE_ALIGNMENT );

    for(int f=0;f<MAX_FIELDS;f++)
    {
      m_444PTexture[f] = new D3DTexture();
      m_444PTexture[f]->AddRef();
    }

    XGSetTextureHeader(m_iSourceWidth, m_iSourceHeight,    1, 0, D3DFMT_LIN_A8R8G8B8, 0, m_444PTexture[0], 0,      stride);
    XGSetTextureHeader(m_iSourceWidth, m_iSourceHeight>>1, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, m_444PTexture[1], 0,      stride<<1);
    XGSetTextureHeader(m_iSourceWidth, m_iSourceHeight>>1, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, m_444PTexture[2], stride, stride<<1);

    for(int f=0;f<MAX_FIELDS;f++)
      m_444PTexture[f]->Register(data);

    CLog::Log(LOGINFO, "Created 444P texture");
  }
  Clear444PTexture();

  return true;
}

void CRGBRenderer::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 0 buffers in windowed mode
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    if (m_444PTexture[FIELD_FULL])
    { // don't need the YUV texture in the GUI
      Delete444PTexture();
    }
  }

  CXBoxRenderer::ManageTextures();
}

bool CRGBRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(!CXBoxRenderer::Configure(width, height, d_width, d_height, fps, flags))
    return false;
  
  // create our lookup textures for yv12->rgb translation,
  if(!CreateLookupTextures(m_yuvcoef, m_yuvrange) )
    return false;

  m_bConfigured = true;
  return true;
}

void CRGBRenderer::Render(DWORD flags)
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
    m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );    
    m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );    

    DWORD alphaenabled;
    m_pD3DDevice->GetRenderState( D3DRS_ALPHABLENDENABLE, &alphaenabled );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

    RECT rs_new = rd;
    D3DSurface* p444PSource[MAX_FIELDS];

    if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_BOTH || alphaenabled )
    {
      // scale it up to fill the entire texture
      rs_new.top = 0;
      rs_new.bottom = m_iSourceHeight;
      rs_new.left = 0;
      rs_new.right = m_iSourceWidth;

      if( flags & RENDER_FLAG_BOTH )
      { /* if we are rendering any type of field to buffer */
        /* output can't be target output */
        rs_new.top >>=1;
        rs_new.bottom >>=1;
      }
      
      /* we will need extra texture, make sure it exists */
      if (!m_444PTexture[FIELD_FULL])
        Create444PTexture();

      for(int i = 0;i<MAX_FIELDS;i++)
        m_444PTexture[i]->GetSurfaceLevel(0, &p444PSource[i]);
    }
    else
    {
      // we can render directly to backbuffer
      m_pD3DDevice->GetBackBuffer(0, 0, &p444PSource[FIELD_FULL]);

#if 0 
      // currently doesn't work properly, not sure why yet
      // no error is thrown, but rendering to these genrerated surfaces
      // doesn't result in any data on screen, not supported
      // further down currently either
      if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_BOTH )
      {
        rs_new.top >>=1;
        rs_new.bottom >>=1;

        
        const DWORD pitch = (((p444PSource[FIELD_FULL]->Size&D3DSIZE_PITCH_MASK)>>D3DSIZE_PITCH_SHIFT)+1)*64;
        const DWORD height = ((p444PSource[FIELD_FULL]->Size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1;
        const DWORD width = ((p444PSource[FIELD_FULL]->Size&D3DSIZE_WIDTH_MASK))+1;
        const D3DFORMAT format = (D3DFORMAT)((p444PSource[FIELD_FULL]->Format&D3DFORMAT_FORMAT_MASK)>>D3DFORMAT_FORMAT_SHIFT);

        p444PSource[FIELD_ODD] = new D3DSurface();
        p444PSource[FIELD_EVEN] = new D3DSurface();

        XGSetSurfaceHeader(width, height>>1, format, p444PSource[FIELD_ODD],  0,     pitch<<1);
        XGSetSurfaceHeader(width, height>>1, format, p444PSource[FIELD_EVEN], pitch, pitch<<1);

        p444PSource[FIELD_ODD]->Register((void*)(p444PSource[FIELD_FULL]->Data + D3DSURFACE_OWNSMEMORY));
        p444PSource[FIELD_EVEN]->Register((void*)(p444PSource[FIELD_FULL]->Data + D3DSURFACE_OWNSMEMORY));
      }
      else
#endif
      {
        p444PSource[FIELD_ODD] = p444PSource[FIELD_FULL];
        p444PSource[FIELD_ODD]->AddRef();
        p444PSource[FIELD_EVEN] = p444PSource[FIELD_FULL];
        p444PSource[FIELD_EVEN]->AddRef();
      }

    }

    if( (flags & RENDER_FLAG_FIELDMASK) == 0 )
    {      
      InterleaveYUVto444P(
          m_YUVTexture[index][FIELD_FULL],
          p444PSource[FIELD_FULL],
          rs, rs_new,
          1, 1,
          0.0f, 0.0f,
          CHROMAOFFSET_HORIZ, 0.0f);

    }
    else
    {
      RECT rs_half = rs;
      rs_half.top >>=1;
      rs_half.bottom >>=1;

      /* calculate any offset needed for any resize */
      float offsety;
      if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_BOTH )
        // keep field offsets, only compensate for scaling
        offsety = 0.25f * ((float(rs_half.bottom - rs_half.top) / float(rs_new.bottom - rs_new.top)) -  1.0f); 
      else
        // spatially align source, needs be done here if we are using same target surface here as in next step
        offsety = 0.25; 

      if( flags & RENDER_FLAG_ODD )
      {
        InterleaveYUVto444P(
            m_YUVTexture[index][FIELD_ODD],
            p444PSource[FIELD_ODD],
            rs_half, rs_new,
            1, 1,
            0.0f, +offsety,
            CHROMAOFFSET_HORIZ, +CHROMAOFFSET_VERT);
      }
      
      if( flags & RENDER_FLAG_EVEN )
      {
        InterleaveYUVto444P(
            m_YUVTexture[index][FIELD_EVEN],
            p444PSource[FIELD_EVEN],
            rs_half, rs_new,
            1, 1,
            0.0f, -offsety,
            CHROMAOFFSET_HORIZ, -CHROMAOFFSET_VERT);
      }
    }

    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, alphaenabled );

    //Okey, when the gpu is done with the textures here, they are free to be modified again
    if( !(flags & RENDER_FLAG_NOUNLOCK) )
      m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone[index]);

    // Now perform the YUV->RGB conversion in a single pass, and render directly to the screen
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

    m_pD3DDevice->SetVertexShader( FVF_YUVRGBVERTEX );
    m_pD3DDevice->SetPixelShader( m_hYUVtoRGBLookup );

    m_pD3DDevice->SetScreenSpaceOffset(-0.5f, -0.5f);

    // If we are interlacing, we split the render range up into separate quads - one
    // per line, and render from alternating fields into each quad.


    if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_BOTH )
    {
      /* top on odd scanline, bottom on even */
      rd.top = rd.top & ~1;
      rd.bottom = rd.bottom & ~1;

      unsigned height = (rd.bottom - rd.top) >> 1;
      float scale = (float)(rs_new.bottom - rs_new.top) / height;

      for( int field = 0; field < 2; field++ )
      {
        m_pD3DDevice->SetTexture( 0, SURFTOTEX(p444PSource[field+1]));

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

        /* correct the already existing spatial offset between fields for scaling */
        float offsety;
        if( field == FIELD_ODD )
          offsety = 0.25f*(scale - 1.0f);
        else
          offsety = -0.25f*(scale - 1.0f);

        for(unsigned int line = 0; line < height; line++)
        {
          float source_line = (float)rs_new.top + line * scale + offsety;
          float target_line = (float)rd.top + (line<<1) + field;

          m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.left,  source_line );
          m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.left,  target_line, 0, 1.0f );

          m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.right, source_line );
          m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.right, target_line, 0, 1.0f );

          m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.right, source_line+scale );
          m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.right, target_line+1.0f, 0, 1.0f );

          m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.left,  source_line+scale );
          m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.left,  target_line+1.0f, 0, 1.0f );
        }
        m_pD3DDevice->End();
      }
    }
    else
    {
      if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_ODD )
        m_pD3DDevice->SetTexture(0, SURFTOTEX(p444PSource[FIELD_ODD]));
      else if( (flags & RENDER_FLAG_FIELDMASK) == RENDER_FLAG_EVEN )
        m_pD3DDevice->SetTexture(0, SURFTOTEX(p444PSource[FIELD_EVEN]));
      else
        m_pD3DDevice->SetTexture(0, SURFTOTEX(p444PSource[FIELD_FULL]));

      /* hackish fix for odd rendering issue              */
      /* when backbuffer is used as render source         */
      /* if the top pixel lies outside the backbuffer     */
      /* (ie negative), we get weird lines in the result. */
      if( rs_new.top == rd.top && rs_new.bottom == rd.bottom )
        if(rd.top <0)
          rd.top = rs_new.top = 0;


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

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.left,  (float)rs_new.top);
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.left,  (float)rd.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.right, (float)rs_new.top);
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.right, (float)rd.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.right, (float)rs_new.bottom);
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.right, (float)rd.bottom, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs_new.left,  (float)rs_new.bottom);
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)rd.left,  (float)rd.bottom, 0, 1.0f );

      m_pD3DDevice->End();
    }

    m_pD3DDevice->SetScreenSpaceOffset(0.0f, 0.0f);

    m_pD3DDevice->SetTexture(0, NULL);
    m_pD3DDevice->SetTexture(1, NULL);
    m_pD3DDevice->SetTexture(2, NULL);
    m_pD3DDevice->SetTexture(3, NULL);

    for(int i=0;i<MAX_FIELDS;i++)
    {
      if( p444PSource[i] && p444PSource[i]->Common & D3DCOMMON_D3DCREATED )
      {
        SAFE_RELEASE(p444PSource[i]);
      }
      else
      {
        p444PSource[i]->BlockUntilNotBusy();
        SAFE_DELETE(p444PSource[i]);
      }
    }

    m_pD3DDevice->SetPixelShader( NULL );
  }

  CXBoxRenderer::Render(flags);
}

unsigned int CRGBRenderer::PreInit()
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

    XGAssembleShader("YUV2RGBShader", yuv2rgb, strlen(yuv2rgb), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hYUVtoRGBLookup);
    pShader->Release();

  }

  return 0;
}

void CRGBRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  Delete444PTexture();
  DeleteLookupTextures();

  if (m_hInterleavingShader)
  {
    m_pD3DDevice->DeletePixelShader(m_hInterleavingShader);
    m_hInterleavingShader = 0;
  }

  if (m_hYUVtoRGBLookup)
  {
    m_pD3DDevice->DeletePixelShader(m_hYUVtoRGBLookup);
    m_hYUVtoRGBLookup = 0;
  }
  CXBoxRenderer::UnInit();
}

/*

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

bool CRGBRenderer::CreateLookupTextures(const YUVCOEF &coef, const YUVRANGE &range)
{
  if(memcmp(&m_yuvcoef_last, &coef, sizeof(YUVCOEF)) == 0
  && memcmp(&m_yuvrange_last, &range, sizeof(YUVRANGE)) == 0)
    return true;

  DeleteLookupTextures();
  if (
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
  BYTE *pBuff = new BYTE[256 * 256 * 4];
  BYTE *pErrorBuff = new BYTE[256 * 256 * 4];
  if (pBuff && pErrorBuff)
  {
    // first column is our luminance data
    for (int y = 0; y < 256; y++)
    {
      float fY = (y - range.y_min) * 255.0f / (range.y_max - range.y_min);

      fY = CLAMP(fY, 0.0f, 255.0f);

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
    m_UVLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVLookup->UnlockRect(0);
    m_UVErrorLookup->LockRect(0, &lr, NULL, 0);
    XGSwizzleRect(pErrorBuff, 0, NULL, lr.pBits, 256, 256, NULL, 4);
    m_UVErrorLookup->UnlockRect(0);    

    m_yuvcoef_last = coef;
    m_yuvrange_last = range;
  }
  if(pBuff)
    delete[] pBuff;
  if(pErrorBuff)
    delete[] pErrorBuff;

  return true;
}

void CRGBRenderer::InterleaveYUVto444P(
      YUVPLANES          pSources,
      LPDIRECT3DSURFACE8 pTarget,
      RECT &source,    RECT &target,
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
      m_pD3DDevice->SetPixelShader( m_hInterleavingShader );

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
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)target.left, (float)target.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.top + offset_y);
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.right>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.right>>cshift_x) + coffset_x, (float)(source.top>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.top, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.right + offset_x, (float)source.bottom + offset_y);
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.right>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.right>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.right, (float)target.bottom, 0, 1.0f );

      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)source.left + offset_x, (float)source.bottom + offset_y);
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)(source.left>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)(source.left>>cshift_x) + coffset_x, (float)(source.bottom>>cshift_y) + coffset_y );
      m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX,    (float)target.left, (float)target.bottom, 0, 1.0f );

      m_pD3DDevice->End();

      m_pD3DDevice->SetScreenSpaceOffset(0.0f, 0.0f);

      m_pD3DDevice->SetTexture(0, NULL);
      m_pD3DDevice->SetTexture(1, NULL);
      m_pD3DDevice->SetTexture(2, NULL);

      if( pOldRT )
      {
        m_pD3DDevice->SetRenderTarget( pOldRT, NULL);
        pOldRT->Release();
      }
}

