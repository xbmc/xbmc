#include "include.h"
#define XFONT_TRUETYPE
#include "GUIFontTTF.h"
#include "GraphicContext.h"
#include <xgraphics.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TEXTURE_WIDTH 512
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)
#define TTF_FONT_CACHE_SIZE 32*1024

CGUIFontTTF::CGUIFontTTF(const CStdString& strFontName) : CGUIFont(strFontName)
{
  m_pTrueTypeFont = NULL;
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_dwNestedBeginCount = 0;
  m_charTexture = NULL;
  m_fontShader = NULL;
  m_copyShader = NULL;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
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
  if (m_charTexture)
    m_charTexture->Release();
  m_charTexture = NULL;
  m_fontShader = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;
  m_posX = 0;
  m_posY = 0;
  m_dwNestedBeginCount = 0;
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

  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;
  // char gap has to be approximately height/3 in the case of italics
  m_charGap = (m_iStyle == XFONT_ITALICS || m_iStyle == XFONT_BOLDITALICS) ? m_iHeight/3 - 1 : 0;
  m_pD3DDevice = g_graphicsContext.Get3DDevice();

  // size of the font cache in bytes
  DWORD dwFontCacheSize = TTF_FONT_CACHE_SIZE;

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

  unsigned int width;
  m_pTrueTypeFont->GetTextExtent( L"W", 1, &width);
  m_iMaxCharWidth = (float)width;
  // cache the ellipses width
  m_pTrueTypeFont->GetTextExtent( L".", 1, &m_ellipsesWidth);

  unsigned int cellheight;
  m_pTrueTypeFont->GetFontMetrics(&cellheight, &m_descent);

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = TEXTURE_WIDTH;
  m_posY = -(m_iHeight + (int)m_descent);

  // create our character texture + font shader
  CreateShaderAndTexture();
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
  Begin();
  // vertically centered
  if (dwFlags & XBFONT_CENTER_Y)
  {
    sy = floorf( sy - (m_iHeight - m_descent + 1) / 2 );
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
    Character *ch = GetCharacter(letter);

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( posX + ch->width + 3 * m_ellipsesWidth > alignedStartX + fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        for (int i = 0; i < 3; i++)
        {
          RenderCharacter(posX, (int)sy, GetCharacter(L'.'), dwColor);
          posX += m_ellipsesWidth;
        }
        End();
        return;
      }
    }
    RenderCharacter(posX, (int)sy, ch, dwColor);
    posX += ch->width;
  }
  End();
}

void CGUIFontTTF::GetTextExtent( const WCHAR* strText, FLOAT* pWidth,
                                 FLOAT* pHeight, BOOL bFirstLineOnly)
{
  unsigned width;
  // First let's calculate width
  int len = wcslen(strText);
  WCHAR* buf = new WCHAR[len+1];
  buf[0] = L'\0';
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
    {
      delete[] buf;
      return ;
    }
  }

  delete[] buf;
  return ;
}

CGUIFontTTF::Character* CGUIFontTTF::GetCharacter(WCHAR letter)
{
  int low = 0;
  int high = m_numChars - 1;
  int mid;
  while (low <= high)
  {
    mid = (low + high) >> 1;
    if (letter > m_char[mid].letter)
      low = mid + 1;
    else if (letter < m_char[mid].letter)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  // increase the size of the buffer if we need it
  if (m_numChars >= m_maxChars)
  { // need to increase the size of the buffer
    Character *newTable = new Character[m_maxChars + CHAR_CHUNK];
    if (m_char)
    {
      memcpy(newTable, m_char, low * sizeof(Character));
      memcpy(newTable + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
      delete[] m_char;
    }
    m_char = newTable;
    m_maxChars += CHAR_CHUNK;

  }
  else
  { // just move the data along as necessary
    memmove(m_char + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
  }
  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  DWORD dwNestedBeginCount = m_dwNestedBeginCount;
  m_dwNestedBeginCount = 1;
  End();
  CacheCharacter(letter, m_char + low);
  Begin();
  m_dwNestedBeginCount = dwNestedBeginCount;
  return m_char + low;
}

void CGUIFontTTF::CacheCharacter(WCHAR letter, Character *ch)
{
  WCHAR text[2];
  text[0] = letter;
  text[1] = 0;
  unsigned int width;
  m_pTrueTypeFont->GetTextExtent(&letter, 1, &width);
  if (m_posX + width + m_charGap > TEXTURE_WIDTH)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    m_posX = 0;
    m_posY += m_iHeight + m_descent;
    // create the new larger texture
    LPDIRECT3DTEXTURE8 newTexture;
    if (D3D_OK != m_pD3DDevice->CreateTexture(TEXTURE_WIDTH, (m_iHeight + m_descent) * (m_textureRows + 1), 1, 0, D3DFMT_LIN_L8, 0, &newTexture))
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
      memcpy(lr.pBits, lr2.pBits, lr2.Pitch * m_textureRows * (m_iHeight + m_descent));
      m_texture->UnlockRect(0);
      m_texture->Release();
    }
    memset((BYTE *)lr.pBits + lr.Pitch * m_textureRows * (m_iHeight + m_descent), 0, lr.Pitch * (m_iHeight + m_descent));
    newTexture->UnlockRect(0);
    m_texture = newTexture;
    m_textureRows++;
  }
  // ok, now render it to our temp texture
  LPDIRECT3DSURFACE8 surface;
  // clear surface
  m_charTexture->GetSurfaceLevel(0, &surface);
  D3DLOCKED_RECT lr;
  surface->LockRect(&lr, NULL, 0);
  memset(lr.pBits, 0, lr.Pitch * (m_iHeight + m_descent));
  surface->UnlockRect();
  m_pTrueTypeFont->TextOut(surface, text, 1, 0, 0);
  surface->Release();

  // copy to our font texture
  CopyTexture(width);
  // and set it in our table
  ch->letter = letter;
  ch->left = m_posX;
  ch->top = m_posY;
  ch->right = m_posX + (unsigned short)width;
  ch->bottom = m_posY + (unsigned short)m_iHeight;
  ch->width = (unsigned short)(width + m_charGap);
  ch->height = (unsigned short)m_iHeight;
  m_posX += ch->width + 1;  // 1 pixel extra to control kerning.
  m_numChars++;
}

void CGUIFontTTF::Begin()
{
  if (m_dwNestedBeginCount == 0)
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
    m_pD3DDevice->SetPixelShader(m_fontShader);
    m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f ); // fix texel align

    // Render the image
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
  }
  // Keep track of the nested begin/end calls.
  m_dwNestedBeginCount++;
}

void CGUIFontTTF::End()
{
  if (m_dwNestedBeginCount == 0)
    return;

  if (--m_dwNestedBeginCount > 0)
    return;

  m_pD3DDevice->End();

  m_pD3DDevice->SetScreenSpaceOffset( 0, 0 );
  m_pD3DDevice->SetPixelShader(NULL);
  m_pD3DDevice->SetTexture(0, NULL);
}

void CGUIFontTTF::RenderCharacter(int posX, int posY, const Character *ch, D3DCOLOR dwColor)
{
  m_pD3DDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor);

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->left, (float)ch->top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX, (float)posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->right + m_charGap, (float)ch->top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX + ch->width, (float)posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->right + m_charGap, (float)ch->bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX + ch->width, (float)posY + ch->height, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->left, (float)ch->bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)posX, (float)posY + ch->height, 0, 1.0f );
}

void CGUIFontTTF::CreateShaderAndTexture()
{
  if (m_charTexture)
    m_charTexture->Release();
  m_charTexture = NULL;

  m_pD3DDevice->CreateTexture(m_iHeight * 2, (m_iHeight + m_descent), 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_charTexture);
  D3DLOCKED_RECT lr;
  m_charTexture->LockRect(0, &lr, NULL, 0);
  memset(lr.pBits, 0, lr.Pitch * (m_iHeight + m_descent));
  m_charTexture->UnlockRect(0);

  if (!m_fontShader)
  {
    // shader from the alpha texture to the full 32bit font.  Basically, anything with
    // alpha > 0 is filled in fully in the colour channels
    const char *fonts =
      "xps.1.1\n"
      "def c0,0,0,0,0\n"
      "def c1,1,1,1,0\n"
      "def c2,0.0039,0.0039,0.0039,0.0039\n"
      "def c3,0,0,0,1\n"
      "tex t0\n"
      "mul r1, t0.b, v0.a\n"     // modulate the 2 alpha values into r1
      "sub r0, r1, c2_bias\n"    // compare alpha value in r1 to minimum alpha
      "cnd r0, r0.a, v0, c0\n"   // and if greater, copy the colour to r0, else set r0 to transparent.
      "xmma discard, discard, r0, r0, c1, r1, c3\n"; // add colour value in r0 to alpha value in r1

    // shader to copy from the alpha channel of a 32 bit texture to a LIN_L8 texture.
    // copies the alpha channel into the blue channel, which is outputted.
    const char *copy =
      "xps.1.1\n"
      "tex t0\n"
      "mov r0, t0.a\n";

    XGBuffer* pShader;
    XGAssembleShader("FontsShader", fonts, strlen(fonts), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_fontShader);
    pShader->Release();

    XGBuffer* pShader2;
    XGAssembleShader("CopyShader", copy, strlen(copy), 0, NULL, &pShader2, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader2->GetBufferPointer(), &m_copyShader);
    pShader2->Release();
  }
}

void CGUIFontTTF::CopyTexture(int width)
{
  LPDIRECT3DSURFACE8 newRT, oldRT;
  m_texture->GetSurfaceLevel(0, &newRT);
  m_pD3DDevice->GetRenderTarget(&oldRT);
  m_pD3DDevice->SetRenderTarget(newRT, NULL);
  m_pD3DDevice->SetTexture(0, m_charTexture);

  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetPixelShader(m_copyShader);

  m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f ); // fix texel align
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)0, (float)0 );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_posX, (float)m_posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)width, (float)0 );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_posX + width, (float)m_posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)width, (float)m_iHeight );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_posX + width, (float)m_posY + m_iHeight, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)0, (float)m_iHeight );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_posX, (float)m_posY + m_iHeight, 0, 1.0f );
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset( 0, 0 );

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetPixelShader(NULL);
  m_pD3DDevice->SetRenderTarget(oldRT, NULL);
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  oldRT->Release();
  newRT->Release();
}
