/*
 *      Copyright (C) 2005-2010 Team XBMC
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

// C++ Implementation: karaokelyricscdg

#include "system.h"
#include "AdvancedSettings.h"
#include "GraphicContext.h"
#include "Settings.h"
#include "Application.h"
#include "GUITexture.h"
#include "Texture.h"
#include "karaokelyricscdg.h"
#include "utils/log.h"

#define TEX_COLOR DWORD  //Texture color format is A8R8G8B8
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_TEX1)


CKaraokeLyricsCDG::CKaraokeLyricsCDG( const CStdString& cdgFile )
  : CKaraokeLyrics()
{
  m_cdgFile = cdgFile;
  m_pCdg = 0;
  m_pReader = 0;
  m_pCdgTexture = 0;
  m_pLoader = 0;

  m_fgAlpha = 0xff000000;
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

#if defined(HAS_GL) || defined(HAS_GLES)
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
  // set the background to be completely transparent if we use visualisations, or completely solid if not
  if ( g_advancedSettings.m_karaokeAlwaysEmptyOnCdgs )
    m_bgAlpha = 0xff000000;
  else
    m_bgAlpha = 0;

  if (!m_pCdgTexture)
  {
    m_pCdgTexture = new CTexture(WIDTH, HEIGHT, XB_FMT_A8R8G8B8);
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
    delete m_pCdgTexture;
    m_pCdgTexture = NULL;
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

  // Render the cdg text into memory buffer and update our texture
  unsigned char *buf = new unsigned char[WIDTH * HEIGHT * 4];
  if (buf)
  {
    RenderIntoBuffer( buf, WIDTH, HEIGHT, WIDTH * 4 );
    m_pCdgTexture->Update( WIDTH, HEIGHT, WIDTH * 4, XB_FMT_A8R8G8B8, buf, false );
  }
  delete [] buf;

  // Convert texture coordinates to (0..1)
  CRect texCoords((float)BORDERWIDTH / WIDTH, (float)BORDERHEIGHT  / HEIGHT,
                  (float)(WIDTH - BORDERWIDTH) / WIDTH, (float)(HEIGHT - BORDERHEIGHT) / HEIGHT);

  // Get screen coordinates
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  CRect vertCoords((float)g_settings.m_ResInfo[res].Overscan.left,
                   (float)g_settings.m_ResInfo[res].Overscan.top,
                   (float)g_settings.m_ResInfo[res].Overscan.right,
                   (float)g_settings.m_ResInfo[res].Overscan.bottom);

  CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_pCdgTexture, &texCoords);
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


bool CKaraokeLyricsCDG::HasVideo()
{
  return false;
}

void CKaraokeLyricsCDG::GetVideoParameters(CStdString & path, __int64 & offset)
{
}
