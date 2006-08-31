#include "include.h"
#include "GUIFontTTF.h"
#include "GraphicContext.h"
#include <xgraphics.h>

// stuff for freetype
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SYNTHESIS_H

#define USE_RELEASE_LIBS

// our free type library (debug)
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"guilib/freetype2/freetype221_D.lib")
#else
  #pragma comment (lib,"guilib/freetype2/freetype221.lib")
#endif

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
    m_references = 0;
  }

  FT_Library Get()
  {
    if (!m_library)
    {
      FT_Init_FreeType(&m_library);
    }
    if (m_library)
      m_references++;
    return m_library;
  };
  void Release()
  {
    if (m_references)
      m_references--;
    if (!m_references && m_library)
    {
      FT_Done_FreeType(m_library);
      m_library = NULL;
    }
  };

private:
  FT_Library   m_library;
  unsigned int m_references;
};

CFreeTypeLibrary g_freeTypeLibrary; // our freetype library

#define ROUND(x) floorf(x + 0.5f)

#define TEXTURE_WIDTH 512
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)

CGUIFontTTF::CGUIFontTTF(const CStdString& strFileName)
  : CGUIFontBase(strFileName)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_dwNestedBeginCount = 0;
  m_fontShader = NULL;

  m_face = NULL;
  m_library = NULL;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::ClearCharacterCache()
{
  if (m_texture)
    m_texture->Release();
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = new Character[CHAR_CHUNK];
  m_numChars = 0;
  m_maxChars = CHAR_CHUNK;
  m_textureRows = 0;
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = TEXTURE_WIDTH;
  m_posY = -(int)m_cellHeight;
}

void CGUIFontTTF::Clear()
{
  if (m_texture)
    m_texture->Release();
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_fontShader = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;
  m_posX = 0;
  m_posY = 0;
  m_dwNestedBeginCount = 0;

  FT_Done_Face( m_face );
  m_face = NULL;

  if (m_library)
  {
    g_freeTypeLibrary.Release();
    m_library = NULL;
  }
}

// Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
bool CGUIFontTTF::Load(const CStdString& strFilename, int iHeight, int iStyle)
{
  m_library = g_freeTypeLibrary.Get();
  if (!m_library)
    return false;

  // ok, now load the font face
  if (FT_New_Face( m_library, strFilename.c_str(), 0, &m_face ))
    return false;

  unsigned int xdpi = 72;
  unsigned int ydpi = 72;

  // we set our screen res currently to 96dpi in both directions (windows default)
  // we cache our characters (for rendering speed) so it's probably
  // not a good idea to allow free scaling of fonts - rather, just
  // scaling to pixel ratio on screen perhaps?
  if (FT_Set_Char_Size( m_face, 0, iHeight*64, xdpi, ydpi ))
    return false;

  // grab the maximum cell height and width
  unsigned int m_cellWidth = m_face->bbox.xMax - m_face->bbox.xMin;
  m_cellHeight = m_face->bbox.yMax - m_face->bbox.yMin;
  m_cellBaseLine = m_face->bbox.yMax;
  m_lineHeight = m_face->height;

  m_cellWidth *= iHeight * xdpi;
  m_cellWidth /= (72 * m_face->units_per_EM);

  m_cellHeight *= iHeight * ydpi;
  m_cellHeight /= (72 * m_face->units_per_EM);

  m_cellBaseLine *= iHeight * ydpi;
  m_cellBaseLine /= (72 * m_face->units_per_EM);

  m_lineHeight *= iHeight * ydpi;
  m_lineHeight /= (72 * m_face->units_per_EM);

  // increment by 1 for good measure to give space in our texture
  m_cellWidth++;
  m_cellHeight+=2;
  m_cellBaseLine++;

  CLog::Log(LOGDEBUG, __FUNCTION__" Scaled size of font %s (%i): width = %i, height = %i", 
    strFilename.c_str(), iHeight, m_cellWidth, m_cellHeight);
  m_iHeight = iHeight;
  m_iStyle = iStyle;

  if (m_texture)
    m_texture->Release();
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;

  m_maxChars = 0;
  m_numChars = 0;
  m_textureRows = 0;

  m_pD3DDevice = g_graphicsContext.Get3DDevice();

  m_strFilename = strFilename;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = TEXTURE_WIDTH;
  m_posY = -(int)m_cellHeight;

  // load in the 'W' to get the "max" width of the font
  m_cellAscent = 0;
  Character *w = GetCharacter(L'W');
  if (w)
  {
    m_iMaxCharWidth = w->right - w->left;
    m_cellAscent = w->bottom - w->top;
  }

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ROUND(ellipse->advance);

  // create our character texture + font shader
  CreateShader();
  return true;
}

void CGUIFontTTF::DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor,
                          const WCHAR* strText, DWORD cchText, DWORD dwFlags,
                          FLOAT fMaxPixelWidth)
{
  // Draw text as a single colour
  DrawTextInternal(fOriginX, fOriginY, angle, &dwColor, NULL, strText, cchText>2048?2048:cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontTTF::DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette,
                                     const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                     FLOAT fMaxPixelWidth)
{
  // Draws text as multi-coloured polygons
    DrawTextInternal(fOriginX, fOriginY, angle, pdw256ColorPalette, pbColours, strText, cchText>2048?2048:cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontTTF::DrawTextInternal( FLOAT sx, FLOAT sy, const CAngle &angle, DWORD *pdw256ColorPalette, BYTE *pbColours, const WCHAR* strText, DWORD cchText, DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  Begin();

  // vertically centered
  if (dwFlags & XBFONT_CENTER_Y)
  {
    sy -= m_lineHeight * 0.5f;
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
      GetTextExtentInternal( strText, &w, &h, TRUE );

      // If not, then clear the flag
      if ( w <= fMaxPixelWidth )
        dwFlags &= (~XBFONT_TRUNCATED);
    }
  }

  float lineX;
  float lineY;
  float cursorX;
  int numLines = 0;
  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;

  while ( cchText-- )
  {
    // If starting text on a new line, determine justification effects
    if ( bStartingNewLine )
    {
      lineX = sx - angle.sine * m_lineHeight * numLines;
      lineY = sy + angle.cosine * m_lineHeight * numLines;
      if ( dwFlags & (XBFONT_RIGHT | XBFONT_CENTER_X) )
      {
        // Get the extent of this line
        FLOAT w, h;
        if ( dwFlags & XBFONT_TRUNCATED )
          w = fMaxPixelWidth;
        else
          GetTextExtentInternal( strText, &w, &h, TRUE );
        if ( dwFlags & XBFONT_CENTER_X)
          w *= 0.5f;
        // Offset this line's starting position
        lineX -= angle.cosine * w;
        lineY -= angle.sine * w;
      }
      bStartingNewLine = FALSE;
      // align to an integer so that aliasing doesn't occur
      lineX = floorf(lineX + 0.5f);
      lineY = floorf(lineY + 0.5f);
      cursorX = 0; // current position along the line
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
      numLines++;
      bStartingNewLine = TRUE;
      continue;
    }

    // grab the next character
    Character *ch = GetCharacter(letter);

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( cursorX + ch->advance + 3 * m_ellipsesWidth > fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        for (int i = 0; i < 3; i++)
        {
          float posX = lineX + cursorX*angle.cosine;
          float posY = lineY + cursorX*angle.sine;
          RenderCharacter(posX, posY, angle, GetCharacter(L'.'), dwColor);
          cursorX += m_ellipsesWidth;
        }
        End();
        return;
      }
    }
    float posX = lineX + cursorX*angle.cosine;
    float posY = lineY + cursorX*angle.sine;
    RenderCharacter(posX, posY, angle, ch, dwColor);
    cursorX += ROUND(ch->advance);
  }
  End();
}

void CGUIFontTTF::GetTextExtentInternal( const WCHAR* strText, FLOAT* pWidth,
                                 FLOAT* pHeight, BOOL bFirstLineOnly)
{
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
    float width = 0;
    float height = 0;
    WCHAR *ch = buf;
    while (*ch)
    {
      Character *c = GetCharacter(*ch++);
      if (c) width += ROUND(c->advance);
    }
    if (width > *pWidth)
      *pWidth = width;
    *pHeight += m_lineHeight;

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
  if (dwNestedBeginCount) End();
  if (!CacheCharacter(letter, m_char + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::Log(LOGDEBUG, "GUIFontTTF::GetCharacter: Unable to cache character.  Clearing character cache of %i characters", m_numChars);
    ClearCharacterCache();
    low = 0;
    if (!CacheCharacter(letter, m_char + low))
    {
      CLog::Log(LOGERROR, "GUIFontTTF::GetCharacter: Unable to cache character (out of memory?)");
    }
  }
  if (dwNestedBeginCount) Begin();
  m_dwNestedBeginCount = dwNestedBeginCount;
  return m_char + low;
}

bool CGUIFontTTF::CacheCharacter(WCHAR letter, Character *ch)
{
  int glyph_index = FT_Get_Char_Index( m_face, letter );

  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph( m_face, glyph_index, FT_LOAD_TARGET_LIGHT ))
    return false;
  // make bold if applicable
  if (m_iStyle == XFONT_BOLD || m_iStyle == XFONT_BOLDITALICS)
    FT_GlyphSlot_Embolden(m_face->glyph);
  // and italics if applicable
  if (m_iStyle == XFONT_ITALICS || m_iStyle == XFONT_BOLDITALICS)
    FT_GlyphSlot_Oblique(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
    return false;
  // and get it's bounding box
  FT_BBox bbox;
  FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_PIXELS, &bbox );
  // at this point we have the information regarding the character that we need
  unsigned int width = bbox.xMax - bbox.xMin;
  // check we have enough room for the character
  if (m_posX + width > TEXTURE_WIDTH)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    int newHeight = m_cellHeight * (m_textureRows + 1);
    m_posX = 0;
    m_posY += m_cellHeight;
    // create the new larger texture
    LPDIRECT3DTEXTURE8 newTexture;
    // check for max height (can't be more than 4096 texels)
    if (newHeight > 4096)
    {
      CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: New cache texture is too large (%i > 4096 pixels long)", newHeight);
      return false;
    }
    if (D3D_OK != m_pD3DDevice->CreateTexture(TEXTURE_WIDTH, newHeight, 1, 0, D3DFMT_LIN_L8, 0, &newTexture))
    {
      CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: Error creating new cache texture for size %i", m_iHeight);
      return false;
    }
    D3DLOCKED_RECT lr;
    newTexture->LockRect(0, &lr, NULL, 0);
    if (m_texture)
    { // copy across from our current one, and clear the new row
      D3DLOCKED_RECT lr2;
      m_texture->LockRect(0, &lr2, NULL, 0);
      memcpy(lr.pBits, lr2.pBits, lr2.Pitch * m_textureRows * m_cellHeight);
      m_texture->UnlockRect(0);
      m_texture->Release();
    }
    memset((BYTE *)lr.pBits + lr.Pitch * m_textureRows * m_cellHeight, 0, lr.Pitch * m_cellHeight);
    newTexture->UnlockRect(0);
    m_texture = newTexture;
    m_textureRows++;
  }
  // ok, render the glyph
  // TODO: Ideally this function would automatically render into our texture
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
    return false;
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  // rendered, now copy into our texture
  D3DLOCKED_RECT rect;
  m_texture->LockRect(0, &rect, NULL, 0);
  if (bitGlyph->left < 0)
    m_posX += -bitGlyph->left;
  //CLog::Log(LOGDEBUG, __FUNCTION__" tex location %i,%i, y offset %i, x offset %i", m_posX, m_posY, bitGlyph->top, bitGlyph->left);
  for (int y = 0; y < bitmap.rows; y++)
  {
    BYTE *dest = (BYTE *)rect.pBits + (m_posY + y + m_cellBaseLine - bitGlyph->top) * rect.Pitch + m_posX + bitGlyph->left;
    BYTE *source = (BYTE *)bitmap.buffer + y * bitmap.pitch;
    for (int x = 0; x < bitmap.width; x++)
    {
      *dest++ = *source++;
    }
  }
  m_texture->UnlockRect(0);

  // and set it in our table
  ch->letter = letter;
  ch->originX = m_posX;
  ch->originY = m_posY;
  ch->left = bitGlyph->left;
  ch->top = m_cellBaseLine - bitGlyph->top;
  ch->right = ch->left + bitmap.width;
  ch->bottom = ch->top + bitmap.rows;
  ch->advance = (float)m_face->glyph->advance.x / 64;
  m_posX += (unsigned short)max(ch->right, ch->advance + 1);
  m_numChars++;

  // free the glyph
  FT_Done_Glyph(glyph);
  return true;
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

void CGUIFontTTF::RenderCharacter(float posX, float posY, const CAngle &angle, const Character *ch, D3DCOLOR dwColor)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = (float)(ch->right - ch->left);
  const float height = (float)(ch->bottom - ch->top);

  /* top left of our texture isn't the topleft of the textcell */
  /* celltop could be higher than m_iHeight over baseline */
  posX += ch->left * angle.cosine - ch->top * angle.sine;
  posY += ch->left * angle.sine + ch->top * angle.cosine;

  m_pD3DDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor);

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->originX + ch->left, (float)ch->originY + ch->top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posX, posY, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->originX + ch->right, (float)ch->originY + ch->top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posX + angle.cosine * width, posY + angle.sine * width, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->originX + ch->right, (float)ch->originY + ch->bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posX + angle.cosine * width - angle.sine * height, posY + angle.sine * width + angle.cosine * height, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)ch->originX + ch->left, (float)ch->originY + ch->bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, posX - angle.sine * height, posY + angle.cosine * height, 0, 1.0f );
}

void CGUIFontTTF::CreateShader()
{
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

    XGBuffer* pShader;
    XGAssembleShader("FontsShader", fonts, strlen(fonts), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_fontShader);
    pShader->Release();
  }
}
