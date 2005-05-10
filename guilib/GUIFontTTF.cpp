#include "stdafx.h"
#define XFONT_TRUETYPE
#include "GUIFontTTF.h"
#include "GraphicContext.h"

#define TEXTURE_WIDTH 512

CGUIFontTTF::CGUIFontTTF(const CStdString& strFontName) : CGUIFont(strFontName)
{
  m_pTrueTypeFont = NULL;
  m_texture = NULL;
  ZeroMemory(m_charTable, sizeof(m_charTable));
  m_char = NULL;
  m_maxChars = 0;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
}

void CGUIFontTTF::Clear()
{
  if (m_pTrueTypeFont)
    XFONT_Release(m_pTrueTypeFont);
  m_pTrueTypeFont = NULL;
  if (m_texture)
    m_texture->Release();
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;
}

// Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
bool CGUIFontTTF::Load(const CStdString& strFilename, int iHeight, int iStyle)
{
  m_iHeight = iHeight;
  m_iStyle = iStyle;

  if (m_pTrueTypeFont)
    XFONT_Release(m_pTrueTypeFont);

  if (m_texture)
    m_texture->Release();
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;

  for (int i = 0; i < 65536; i++)
    m_charTable[i] = (unsigned short)-1;
  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;
  // char gap has to be approximately height/3 in the case of italics
  m_charGap = (m_iStyle == XFONT_ITALICS || m_iStyle == XFONT_BOLDITALICS) ? m_iHeight/3 - 1 : 0;
  m_pD3DDevice = g_graphicsContext.Get3DDevice();

  // size of the font cache in bytes
  DWORD dwFontCacheSize = 64 * 1024;

  m_strFilename = strFilename;

  WCHAR wszFilename[256];
  swprintf(wszFilename, L"%S", strFilename.c_str());

  if ( FAILED( XFONT_OpenTrueTypeFont ( wszFilename, dwFontCacheSize, &m_pTrueTypeFont ) ) )
    return false;

  m_pTrueTypeFont->SetTextHeight( m_iHeight );
  m_pTrueTypeFont->SetTextStyle( m_iStyle );

  // Anti-Alias the font -- 0 for no anti-alias, 2 for some, 4 for MAX!
  m_pTrueTypeFont->SetTextAntialiasLevel( 2 );
  m_pTrueTypeFont->SetTextColor(0xffffffff);
  m_pTrueTypeFont->SetTextAlignment(XFONT_LEFT);

  float fTextHeight;
  GetTextExtent( L"W", &m_iMaxCharWidth, &fTextHeight);

  return true;
}

void CGUIFontTTF::DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                          const WCHAR* strText, DWORD cchText, DWORD dwFlags,
                          FLOAT fMaxPixelWidth)
{
  // Draw text as a single colour
  DrawTextInternal(fOriginX, fOriginY, &dwColor, NULL, strText, cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontTTF::DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                     const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                     FLOAT fMaxPixelWidth)
{
  // Draws text as multi-coloured polygons
  DrawTextInternal(fOriginX, fOriginY, pdw256ColorPalette, pbColours, strText, cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontTTF::DrawTextInternal( FLOAT sx, FLOAT sy, DWORD *pdw256ColorPalette, BYTE *pbColours, const WCHAR* strText, DWORD cchText, DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  // vertically centered
  if (dwFlags & XBFONT_CENTER_Y)
  {
    FLOAT w, h;
    GetTextExtent( strText, &w, &h );
    sy = floorf( sy - h / 2 );
  }

  // Check if we will really need to truncate the CStdString
  if ( dwFlags & XBFONT_TRUNCATED )
  {
    if ( fMaxPixelWidth <= 0.0f )
    {
      dwFlags &= (~XBFONT_TRUNCATED);
    }
    else
    {
      FLOAT w, h;
      GetTextExtent( strText, &w, &h, TRUE );

      // If not, then clear the flag
      if ( w <= fMaxPixelWidth )
        dwFlags &= (~XBFONT_TRUNCATED);
    }
  }

  WCHAR ellipse = L'.';
  unsigned int ellipsesWidth;
  m_pTrueTypeFont->GetTextExtent(&ellipse, 1, &ellipsesWidth);

  int posX = (int)sx;
  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;
  int alignedStartX;

  while ( cchText-- )
  {
    // If starting text on a new line, determine justification effects
    if ( bStartingNewLine )
    {
      if ( dwFlags & (XBFONT_RIGHT | XBFONT_CENTER_X) )
      {
        // Get the extent of this line
        FLOAT w, h;
        if ( dwFlags & XBFONT_TRUNCATED )
          w = fMaxPixelWidth;
        else
          GetTextExtent( strText, &w, &h, TRUE );

        // Offset this line's starting m_fCursorX value
        if ( dwFlags & XBFONT_RIGHT )
          posX = (int)floorf( sx - w );
        if ( dwFlags & XBFONT_CENTER_X )
          posX = (int)floorf( sx - w / 2 );
      }
      bStartingNewLine = FALSE;
      alignedStartX = posX;
    }

    // Get the current letter in the CStdString
    WCHAR letter = *strText++;

    DWORD dwColor;
    if (pbColours)  // multi colour version
      dwColor = pdw256ColorPalette[*pbColours++];
    else            // single colour version
      dwColor = *pdw256ColorPalette;

    // Handle the newline character
    if ( letter == L'\n' )
    {
      posX = (int)sx;
      sy += m_pTrueTypeFont->GetTextHeight();
      bStartingNewLine = TRUE;
      continue;
    }

    // grab the next character
    if (m_charTable[letter] == (unsigned short)-1)
      CacheCharacter(letter);

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( posX + m_char[m_charTable[letter]].right - m_char[m_charTable[letter]].left + 3 * ellipsesWidth > alignedStartX + fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        if (m_charTable[L'.'] == (unsigned short)-1)
          CacheCharacter(L'.');
        for (int i = 0; i < 3; i++)
        {
          RenderCharacter(posX, (int)sy, m_char[m_charTable[L'.']], dwColor);
          posX += ellipsesWidth;
        }
        return;
      }
    }
    RenderCharacter(posX, (int)sy, m_char[m_charTable[letter]], dwColor);
    posX += m_char[m_charTable[letter]].right - m_char[m_charTable[letter]].left;
  }
}

void CGUIFontTTF::GetTextExtent( const WCHAR* strText, FLOAT* pWidth,
                                 FLOAT* pHeight, BOOL bFirstLineOnly)
{
  unsigned width;
  // First let's calculate width
  WCHAR buf[1024];
  int len = wcslen(strText);
  int i = 0, j = 0;
  *pWidth = *pHeight = 0.0f;

  while (j < len)
  {
    for (j = i; j < len; j++)
    {
      if (strText[j] == L'\n')
      {
        break;
      }
    }

    wcsncpy(buf, strText + i, j - i);
    buf[j - i] = L'\0';
    m_pTrueTypeFont->GetTextExtent(buf, -1, &width);
    if (width > *pWidth)
      *pWidth = (float) width;
    *pHeight += m_iHeight;

    i = j + 1;

    if (bFirstLineOnly)
      return ;
  }

  return ;
}

void CGUIFontTTF::CacheCharacter(WCHAR ch)
{
  if (m_numChars >= m_maxChars)
  { // increment the size of our character buffer
    Character *newTable = new Character[m_maxChars + 256];
    if (m_char)
    {
      memcpy(newTable, m_char, m_maxChars*sizeof(Character));
      delete[] m_char;
    }
    m_char = newTable;
    m_maxChars += 256;
  }
  // ok, have enough room, let's create the new character
  // default to top right of texture so that the first character causes the
  // texture to be created
  short posX = TEXTURE_WIDTH;
  short posY = -(short)m_pTrueTypeFont->GetTextHeight();
  if (m_numChars > 0)
  {
    posX = m_char[m_numChars - 1].right + m_charGap + 1;    // extra pixel spacing for smaller fonts (due to kerning)
    posY = m_char[m_numChars - 1].top;
  }
  WCHAR text[2];
  text[0] = ch;
  text[1] = 0;
  float width, height;
  GetTextExtent(text, &width, &height);
  if (posX + width + m_charGap > TEXTURE_WIDTH)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    posX = 0;
    posY += m_pTrueTypeFont->GetTextHeight();
    // create the new larger texture
    LPDIRECT3DTEXTURE8 newTexture;
    if (D3D_OK != m_pD3DDevice->CreateTexture(TEXTURE_WIDTH, m_pTrueTypeFont->GetTextHeight() * (m_textureRows + 1), 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &newTexture))
    {
      CLog::Log(LOGERROR, "Unable to create texture for font");
      return;
    }
    D3DLOCKED_RECT lr;
    newTexture->LockRect(0, &lr, NULL, 0);
    if (m_texture)
    { // copy across from our current one, and clear the new row
      D3DLOCKED_RECT lr2;
      m_texture->LockRect(0, &lr2, NULL, 0);
      memcpy(lr.pBits, lr2.pBits, lr2.Pitch * m_textureRows * m_pTrueTypeFont->GetTextHeight());
      m_texture->UnlockRect(0);
      m_texture->Release();
    }
    memset((BYTE *)lr.pBits + lr.Pitch * m_textureRows * m_pTrueTypeFont->GetTextHeight(), 0, lr.Pitch * m_pTrueTypeFont->GetTextHeight());
    newTexture->UnlockRect(0);
    m_texture = newTexture;
    m_textureRows++;
  }
  // ok, now render it to our texture
  LPDIRECT3DSURFACE8 surface;
  m_texture->GetSurfaceLevel(0, &surface);
  m_pTrueTypeFont->TextOut(surface, text, 1, posX, posY);
  surface->Release();
  // and set it in our table
  m_charTable[ch] = m_numChars;
  m_char[m_numChars].left = posX;
  m_char[m_numChars].top = posY;
  m_char[m_numChars].right = posX + (unsigned short)width;
  m_char[m_numChars].bottom = posY + (unsigned short)height;
  m_numChars++;
}

void CGUIFontTTF::RenderCharacter(int posX, int posY, const Character &ch, D3DCOLOR dwColor)
{
  // just have to blit from our texture.
  m_pD3DDevice->SetTexture( 0, m_texture );

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_TEX1 );
  m_pD3DDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor);
  m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f ); // fix texel align

  // Render the image
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch.left, (float)ch.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX, (float)posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch.right + m_charGap, (float)ch.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX + m_charGap + ch.right - ch.left, (float)posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch.right + m_charGap, (float)ch.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX + m_charGap + ch.right - ch.left, (float)posY + ch.bottom - ch.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch.left, (float)ch.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX, (float)posY + ch.bottom - ch.top, 0, 1.0f );
  m_pD3DDevice->End();

  m_pD3DDevice->SetScreenSpaceOffset( 0, 0 );
  m_pD3DDevice->SetTexture(0, NULL);
}
