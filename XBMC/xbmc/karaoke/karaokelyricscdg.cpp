//
// C++ Implementation: karaokelyricscdg
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "stdafx.h"
#include "Settings.h"
#include "Application.h"

#include "karaokelyricscdg.h"


#define TEX_COLOR DWORD  //Texture color format is A8R8G8B8
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_TEX1)


CKaraokeLyricsCDG::CKaraokeLyricsCDG( const CStdString& cdgFile )
  : CKaraokeLyrics()
{
  m_cdgFile = cdgFile;
  m_pCdg = 0;
  m_pReader = 0;
#if defined (HAS_SDL)
  m_pCdgTexture = 0;
#endif
  m_pLoader = 0;

  m_fgAlpha = 0xff000000;

#if !defined(HAS_SDL)
  m_pd3dDevice = 0;
#endif
}

CKaraokeLyricsCDG::~CKaraokeLyricsCDG()
{
  Shutdown();
}


static inline TEX_COLOR ConvertColor(CDG_COLOR CdgColor)
{
  TEX_COLOR red, green, blue, alpha;
  blue = (TEX_COLOR)((CdgColor & 0x000F) * 17);
  green = ((TEX_COLOR)(((CdgColor & 0x00F0) >> 4) * 17)) << 8;
  red = ((TEX_COLOR)(((CdgColor & 0x0F00) >> 8) * 17)) << 16;
  alpha = ((TEX_COLOR)(((CdgColor & 0xF000) >> 12) * 17)) << 24;

#if defined(HAS_SDL)
  // CGLTexture uses GL_BRGA format
  return alpha | blue | green | red;
#else
  return alpha | red | green | blue;
#endif
}


void CKaraokeLyricsCDG::RenderIntoBuffer( unsigned char *pixels, unsigned int width, unsigned int height, unsigned int pitch ) const
{
  for (UINT j = 0; j < height; j++ )
  {
    DWORD *texel = (DWORD *)( pixels + j * pitch );
    for (UINT i = 0; i < width; i++ )
    {
      BYTE ClutOffset = m_pCdg->GetClutOffset(j + m_pCdg->GetVOffset() , i + m_pCdg->GetHOffset());
      TEX_COLOR TexColor = ConvertColor(m_pCdg->GetColor(ClutOffset));

      if (TexColor >> 24) //Only override transp. for opaque alpha
      {
        TexColor &= 0x00FFFFFF;
        if (ClutOffset == m_pCdg->GetBackgroundColor())
          TexColor |= m_bgAlpha;
        else
          TexColor |= m_fgAlpha;
      }

      *texel++ = TexColor;
    }
  }
}

bool CKaraokeLyricsCDG::InitGraphics()
{
#ifndef HAS_SDL
  if (!m_pd3dDevice)
    m_pd3dDevice = g_graphicsContext.Get3DDevice();
  if (!m_pd3dDevice)
    return false;
#endif

  // set the background to be completely transparent if we use visualisations, or completely solid if not
  if ( g_advancedSettings.m_karaokeAlwaysEmptyOnCdgs )
    m_bgAlpha = 0xff000000;
  else
    m_bgAlpha = 0;

  if (!m_pCdgTexture)
  {
#if defined(HAS_SDL_OPENGL)
    m_pCdgTexture = new CGLTexture( SDL_CreateRGBSurface(SDL_SWSURFACE, WIDTH, HEIGHT, 32, RMASK, GMASK, BMASK, AMASK), false, true );
#elif defined(HAS_SDL_2D)
    m_pCdgTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, WIDTH, HEIGHT, 32, RMASK, GMASK, BMASK, AMASK);
#else // DirectX
    m_pd3dDevice->CreateTexture(WIDTH, HEIGHT, 0, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &m_pCdgTexture);
#endif
  }

  if ( !m_pCdgTexture )
  {
    CLog::Log(LOGERROR, "CDG renderer: failed to create texture" );
    return false;
  }

  return true;
}

void CKaraokeLyricsCDG::Shutdown()
{
  // Stop the reader
  if ( m_pReader )
  {
    m_pReader->DetachLoader();
    m_pReader->StopThread();
    delete m_pReader;
    m_pReader = 0;
  }

  // Stop the loader
  if ( m_pLoader )
  {
    m_pLoader->StopStream();
    delete m_pLoader;
    m_pLoader = 0;
  }

  m_pCdg = 0;

  if ( m_pCdgTexture )
  {
#if defined(HAS_SDL_OPENGL)
    delete m_pCdgTexture;
#elif defined(HAS_SDL_2D)
    SDL_FreeSurface(m_pCdgTexture);
#else
    SAFE_RELEASE(m_pCdgTexture);
#endif
    m_pCdgTexture = 0;
  }

  if ( m_pCdg )
  {
    delete m_pCdg;
    m_pCdg = 0;
  }
}


void CKaraokeLyricsCDG::Render()
{
  // Do not render if we have no texture
  if ( !m_pCdgTexture )
    return;

#if defined(HAS_SDL)
  // Calculate sizes
  int textureBytesSize = WIDTH * HEIGHT * 4;
  int texturePitch = WIDTH * 4;
  unsigned char *buf = new unsigned char[textureBytesSize];

  // Render the cdg text into memory buffer
  RenderIntoBuffer( buf, WIDTH, HEIGHT, texturePitch );

  // Update the texture
#ifdef HAS_SDL_OPENGL
  m_pCdgTexture->Update( WIDTH, HEIGHT, texturePitch, buf, false );
#else
  if (m_pCdgTexture != NULL && SDL_LockSurface(m_pCdgTexture) == 0)
    memcpy(m_pCdgTexture->pixels, buf, HEIGHT * texturePitch);

  SDL_UnlockSurface(m_pCdgTexture);
#endif

  delete [] buf;

  // Lock graphics context since we're touching textures array
  g_graphicsContext.BeginPaint();

  // Convert texture coordinates to (0..1)
  float u1 = BORDERWIDTH / WIDTH;
  float u2 = (float) (WIDTH - BORDERWIDTH) / WIDTH;
  float v1 = BORDERHEIGHT  / HEIGHT;
  float v2 = (float) (HEIGHT - BORDERHEIGHT) / HEIGHT;

    // Get screen coordinates
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  float cdg_left = (float)g_settings.m_ResInfo[res].Overscan.left;
  float cdg_right = (float)g_settings.m_ResInfo[res].Overscan.right;
  float cdg_top = (float)g_settings.m_ResInfo[res].Overscan.top;
  float cdg_bottom = (float)g_settings.m_ResInfo[res].Overscan.bottom;

#ifdef HAS_SDL_OPENGL
  // Set the active texture
  glActiveTextureARB(GL_TEXTURE0_ARB);

  // Load the texture into GPU
  m_pCdgTexture->LoadToGPU();

  // Reset colors
  glColor4f(1.0, 1.0, 1.0, 1.0);

  // Select the texture
  glBindTexture(GL_TEXTURE_2D, m_pCdgTexture->id);

  // Enable texture mapping
  glEnable(GL_TEXTURE_2D);

  // Turn blending on
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  // Fill both buffers
  glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

  // Set texture environment
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

  // Check the state
  VerifyGLState();

  // Begin drawing
  glBegin(GL_QUADS);

  // Draw left-top
  glTexCoord2f( u1, v1 );
  glVertex2f( cdg_left, cdg_top );

  // Draw right-top
  glTexCoord2f( u2, v1 );
  glVertex2f( cdg_right, cdg_top );

  // Draw right-bottom
  glTexCoord2f( u2, v2 );
  glVertex2f( cdg_right, cdg_bottom );

  // Draw left-bottom
  glTexCoord2f( u1, v2 );
  glVertex2f( cdg_left, cdg_bottom );

  // We're done
  glEnd();

  g_graphicsContext.EndPaint();
#else
  SDL_Rect dst;
  dst.x = cdg_left;
  dst.y = cdg_top;
  dst.w = cdg_right - cdg_left;
  dst.h = cdg_bottom - cdg_top;
  g_graphicsContext.BlitToScreen(m_pCdgTexture, NULL, &dst);
#endif

#else
    // Update DirectX structure
  D3DLOCKED_RECT LockedRect;
  m_pCdgTexture->LockRect(0, &LockedRect, NULL, 0L);
  m_Lyrics->RenderIntoBuffer( (unsigned char *)LockedRect.pBits, WIDTH, HEIGHT, LockedRect.Pitch );
  m_pCdgTexture->UnlockRect(0);

  m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );
  m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  m_pd3dDevice->SetTexture(0, m_pCdgTexture);

  m_pd3dDevice->Begin(D3DPT_QUADLIST);

  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)BORDERWIDTH, (float) BORDERHEIGHT);
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.left, (float) g_settings.m_ResInfo[res].Overscan.top, 0, 0 );

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)(WIDTH - BORDERWIDTH), (float) BORDERHEIGHT);
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.right, (float) g_settings.m_ResInfo[res].Overscan.top, 0, 0 );

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)(WIDTH - BORDERWIDTH), (float)(HEIGHT - BORDERHEIGHT));
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.right, (float) g_settings.m_ResInfo[res].Overscan.bottom, 0, 0);

  m_pd3dDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)BORDERWIDTH, (float)(HEIGHT - BORDERHEIGHT));
  m_pd3dDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)g_settings.m_ResInfo[res].Overscan.left, (float) g_settings.m_ResInfo[res].Overscan.bottom, 0, 0 );

  m_pd3dDevice->End();
#endif
}

bool CKaraokeLyricsCDG::Load()
{
  if ( !m_pLoader )
    m_pLoader = new CCdgLoader;

  if ( !m_pReader )
    m_pReader = new CCdgReader( this );

  if ( !m_pLoader || !m_pReader )
    return false;

  m_pLoader->StreamFile( m_cdgFile );
  m_pReader->Attach( m_pLoader );
  m_pReader->Start();
  m_pCdg = m_pReader->GetCdg();

  return true;
}


bool CKaraokeLyricsCDG::HasBackground()
{
  return true;
}
