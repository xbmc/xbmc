#include "include.h"
#include "GUIFontTTF.h"
#include "GraphicContext.h"

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#ifdef HAS_SDL
#include <SDL/SDL_rotozoom.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SYNTHESIS_H

#define USE_RELEASE_LIBS

// our free type library (debug)
#ifdef _XBOX
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"guilib/freetype2/freetype221_D.lib")
#else
  #pragma comment (lib,"guilib/freetype2/freetype221.lib")
#endif
#else
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"../../guilib/freetype2/freetype221_D.lib")
#else
  #pragma comment (lib,"../../guilib/freetype2/freetype221.lib")
#endif
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

#ifdef HAS_XBOX_D3D
#define ROUND_TO_PIXEL(x) floorf(x + 0.5f)
#else
#define ROUND_TO_PIXEL(x) floorf(x + 0.5f) - 0.5f
#endif

#define CHARS_PER_TEXTURE_LINE 20 // number of characters to cache per texture line
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)

CGUIFontTTF::CGUIFontTTF(const CStdString& strFileName)
  : CGUIFontBase(strFileName)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_dwNestedBeginCount = 0;
#ifndef HAS_SDL  
  m_pixelShader = NULL;
  m_vertexShader = NULL;
#endif  
  m_face = NULL;
  m_library = NULL;
  memset(m_charquick, 0, sizeof(m_charquick));
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::ClearCharacterCache()
{
  if (m_texture)
#ifndef HAS_SDL  
    m_texture->Release();
#else
    SDL_FreeSurface(m_texture);
#endif
    
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = new Character[CHAR_CHUNK];
  memset(m_charquick, 0, sizeof(m_charquick));
  m_numChars = 0;
  m_maxChars = CHAR_CHUNK;
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;
}

void CGUIFontTTF::Clear()
{
  if (m_texture)
#ifndef HAS_SDL  
    m_texture->Release();
#else
    SDL_FreeSurface(m_texture);
#endif
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  memset(m_charquick, 0, sizeof(m_charquick));
#ifndef HAS_SDL  
  m_vertexShader = NULL;
  m_pixelShader = NULL;
#endif  
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
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

bool CGUIFontTTF::Load(const CStdString& strFilename, int iHeight, int iStyle, float aspect)
{
#ifndef HAS_SDL
  // create our character texture + font shader
  m_pD3DDevice = g_graphicsContext.Get3DDevice();
  CreateShader();
#endif

  m_library = g_freeTypeLibrary.Get();
  if (!m_library)
    return false;

  // ok, now load the font face
  if (FT_New_Face( m_library, strFilename.c_str(), 0, &m_face ))
    return false;

  unsigned int ydpi = 72;
  unsigned int xdpi = (unsigned int)ROUND(ydpi * aspect);

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
  m_lineHeight++;

  CLog::Log(LOGDEBUG, "%s Scaled size of font %s (%i): width = %i, height = %i",
    __FUNCTION__, strFilename.c_str(), iHeight, m_cellWidth, m_cellHeight);
  m_iHeight = iHeight;
  m_iStyle = iStyle;

  if (m_texture)
#ifndef HAS_SDL  
    m_texture->Release();
#else
    SDL_FreeSurface(m_texture);
#endif
  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;

  m_maxChars = 0;
  m_numChars = 0;

  m_strFilename = strFilename;

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;
  if (m_textureWidth > 4096) m_textureWidth = 4096;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;

  // load in the 'W' to get the "max" width of the font
  Character *w = GetCharacter(L'W');
  if (w)
    m_iMaxCharWidth = (unsigned int)(w->right - w->left);

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ellipse->advance;

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
    sy -= (m_cellHeight-2)*0.5f;

  // Check if we will really need to truncate the CStdString
  if ( dwFlags & XBFONT_TRUNCATED )
  {
    if ( fMaxPixelWidth <= 0.0f)
      dwFlags &= ~XBFONT_TRUNCATED;
    else
    {
      float width, height;
      GetTextExtentInternal(strText, &width, &height);
      if (width <= fMaxPixelWidth)
        dwFlags &= ~XBFONT_TRUNCATED;
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
        GetTextExtentInternal( strText, &w, &h, TRUE );

        if ( dwFlags & XBFONT_TRUNCATED && w > fMaxPixelWidth )
          w = fMaxPixelWidth;
          
        if ( dwFlags & XBFONT_CENTER_X)
          w *= 0.5f;
        // Offset this line's starting position
        lineX -= angle.cosine * w;
        lineY -= angle.sine * w;
      }
      bStartingNewLine = FALSE;
      // align to an integer so that aliasing doesn't occur
      lineX = ROUND_TO_PIXEL(lineX);
      lineY = ROUND_TO_PIXEL(lineY);
      cursorX = 0; // current position along the line
    }

    // Get the current letter in the CStdString
    WCHAR letter = *strText++;

    DWORD dwColor;
    if (pbColours)  // multi colour version
      dwColor = pdw256ColorPalette[*pbColours++];
    else            // single colour version
      dwColor = *pdw256ColorPalette;

    // Skip '\r'
    if ( letter == L'\r' )
      continue;
    // Handle the newline character
    if ( letter == L'\n' )
    {
      numLines++;
      bStartingNewLine = TRUE;
      continue;
    }

    // grab the next character
    Character *ch = GetCharacter(letter);
    if (!ch) continue;

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( cursorX + ch->advance + 3 * m_ellipsesWidth > fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        Character *period = GetCharacter(L'.');
        if (!period)
          break;

        for (int i = 0; i < 3; i++)
        {
          float posX = lineX + cursorX*angle.cosine;
          float posY = lineY + cursorX*angle.sine;
          RenderCharacter(posX, posY, angle, period, dwColor);
          cursorX += period->advance;
        }
        break;
      }
    }
    float posX = lineX + cursorX*angle.cosine;
    float posY = lineY + cursorX*angle.sine;
    RenderCharacter(posX, posY, angle, ch, dwColor);
    cursorX += ch->advance;
  }

  End();
}

void CGUIFontTTF::GetTextExtentInternal( const WCHAR* strText, FLOAT* pWidth,
                                 FLOAT* pHeight, BOOL bFirstLineOnly)
{
  // First let's calculate width
  int len = wcslen(strText);
  int i = 0, j = 0;
  *pWidth = *pHeight = 0.0f;
  int numLines = 0;

  while (j < len)
  {
    for (j = i; j < len; j++)
    {
      if (strText[j] == L'\n')
      {
        break;
      }
    }

    float width = 0;
    for (int k = i; k < j && k < len; k++)
    {
      Character *c = GetCharacter(strText[k]);
      if (c) width += c->advance;
    }
    if (width > *pWidth)
      *pWidth = width;
    numLines++;

    i = j + 1;

    if (bFirstLineOnly)
      break;
  }

  *pHeight = (float)(numLines - 1) * m_lineHeight + (m_cellHeight - 2); // -2 as we increment this for space in our texture
  return ;
}

CGUIFontTTF::Character* CGUIFontTTF::GetCharacter(WCHAR letter)
{
  // quick access to ascii chars
  if(letter < 255 && letter > 0)
    if(m_charquick[letter])
      return m_charquick[letter];

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
      return NULL;
    }
  }
  if (dwNestedBeginCount) Begin();
  m_dwNestedBeginCount = dwNestedBeginCount;

  // fixup quick access
  memset(m_charquick, 0, sizeof(m_charquick));
  for(int i=0;i<m_numChars;i++)
  {
    if(m_char[i].letter<255)
      m_charquick[m_char[i].letter] = m_char+i;
  }

  return m_char + low;
}

bool CGUIFontTTF::CacheCharacter(WCHAR letter, Character *ch)
{
  int glyph_index = FT_Get_Char_Index( m_face, letter );

  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph( m_face, glyph_index, FT_LOAD_TARGET_LIGHT ))
    return false;
  // make bold if applicable
  if (m_iStyle == FONT_STYLE_BOLD || m_iStyle == FONT_STYLE_BOLD_ITALICS)
    FT_GlyphSlot_Embolden(m_face->glyph);
  // and italics if applicable
  if (m_iStyle == FONT_STYLE_ITALICS || m_iStyle == FONT_STYLE_BOLD_ITALICS)
    FT_GlyphSlot_Oblique(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
    return false;
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
    return false;
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  if (bitGlyph->left < 0)
    m_posX += -bitGlyph->left;

#ifndef HAS_SDL
  D3DFORMAT format;
  if(m_pixelShader)
    format = D3DFMT_LIN_L8;
  else
    format = D3DFMT_LIN_A8;
#endif

  // check we have enough room for the character
  if (m_posX + bitGlyph->left + bitmap.width > (int)m_textureWidth)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    m_posX = 0;
    m_posY += m_cellHeight;
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    if(m_posY + m_cellHeight >= m_textureHeight)
    {
      // create the new larger texture
      unsigned newHeight = m_posY + m_cellHeight;
      // check for max height (can't be more than 4096 texels)
      if (newHeight > 4096)
      {
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: New cache texture is too large (%i > 4096 pixels long)", newHeight);
        FT_Done_Glyph(glyph);
        return false;
      }
            
#ifndef HAS_SDL      
      LPDIRECT3DTEXTURE8 newTexture;
      if (D3D_OK != D3DXCreateTexture(m_pD3DDevice, m_textureWidth, newHeight, 1, 0, format, D3DPOOL_MANAGED, &newTexture))
      {
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: Error creating new cache texture for size %i", m_iHeight);
        FT_Done_Glyph(glyph);
        return false;
      }
      // correct texture sizes
      D3DSURFACE_DESC desc;
      newTexture->GetLevelDesc(0, &desc);
      m_textureHeight = desc.Height;
      m_textureWidth = desc.Width;

      // clear texture, doesn't cost much
      D3DLOCKED_RECT rect;
      newTexture->LockRect(0, &rect, NULL, 0);
      memset(rect.pBits, 0, rect.Pitch * m_textureHeight);
      newTexture->UnlockRect(0);

      if (m_texture)
      { // copy across from our current one using gpu
        LPDIRECT3DSURFACE8 pTarget, pSource;
        newTexture->GetSurfaceLevel(0, &pTarget);
        m_texture->GetSurfaceLevel(0, &pSource);

        m_pD3DDevice->CopyRects(pSource, NULL, 0, pTarget, NULL);

        SAFE_RELEASE(pTarget);
        SAFE_RELEASE(pSource);
        SAFE_RELEASE(m_texture);
      }      
#else
		SDL_Surface* newTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, m_textureWidth, newHeight, 32,
        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
      if (!newTexture || newTexture->pixels == NULL)
      {
        CLog::Log(LOGERROR, "GUIFontTTF::CacheCharacter: Error creating new cache texture for size %i", m_iHeight);
        FT_Done_Glyph(glyph);
        return false;
      }
      m_textureHeight = newTexture->h;
      m_textureWidth = newTexture->w;
      
      if (m_texture)
      {
        unsigned char* src = (unsigned char*) m_texture->pixels;
        unsigned char* dst = (unsigned char*) newTexture->pixels;
        for (int y = 0; y < m_texture->h; y++)
        {
          memcpy(dst, src, m_texture->pitch);
          src += m_texture->pitch;
          dst += newTexture->pitch;
        }
        SDL_FreeSurface(m_texture);
      }
#endif

      m_texture = newTexture;
    }
  }

  // set the character in our table
  ch->letter = letter;
  ch->offsetX = (short)bitGlyph->left;
  ch->offsetY = (short)max(m_cellBaseLine - bitGlyph->top, (unsigned int) 0);
  ch->left = (float)m_posX + ch->offsetX;
  ch->top = (float)m_posY + ch->offsetY;
  ch->right = ch->left + bitmap.width;
  ch->bottom = ch->top + bitmap.rows;
  ch->advance = ROUND( (float)m_face->glyph->advance.x / 64 );

  // we need only render if we actually have some pixels
  if (bitmap.width * bitmap.rows)
  {
#ifndef HAS_SDL  
    // render this onto our normal texture using gpu
    LPDIRECT3DSURFACE8 target;
    m_texture->GetSurfaceLevel(0, &target);

    RECT sourcerect = { 0, 0, bitmap.width, bitmap.rows };
    RECT targetrect;
    targetrect.top = m_posY + ch->offsetY;
    targetrect.left = m_posX + bitGlyph->left;
    targetrect.bottom = targetrect.top + bitmap.rows;
    targetrect.right = targetrect.left + bitmap.width;

    D3DXLoadSurfaceFromMemory( target, NULL, &targetrect, 
      bitmap.buffer, format, bitmap.pitch, NULL, &sourcerect, 
      D3DX_FILTER_NONE, 0x00000000);

    SAFE_RELEASE(target);
#else
    SDL_LockSurface(m_texture);

    unsigned char* source = (unsigned char*) bitmap.buffer;
    unsigned int* target = (unsigned int*) (m_texture->pixels) + 
         ((m_posY + ch->offsetY) * m_texture->pitch / 4) + 
          (m_posX + bitGlyph->left);
          
    for (int y = 0; y < bitmap.rows; y++)
    {
    	for (int x = 0; x < bitmap.width; x++)
    	{
	     target[x] = (unsigned int) source[x] << 24 | 0x00ffffffL;
	   }
	   
	   source += bitmap.width;
	   target += (m_texture->pitch / 4);
	 }
	        
    SDL_UnlockSurface(m_texture);
#endif    
  }
  m_posX += (unsigned short)max(ch->right - ch->left + ch->offsetX, ch->advance + 1);
  m_numChars++;

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTF::Begin()
{
#ifndef HAS_SDL
  if (m_dwNestedBeginCount == 0)
  {
    // just have to blit from our texture.
    m_pD3DDevice->SetTexture( 0, m_texture );

    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 ); // only use diffuse

    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

    m_pD3DDevice->SetVertexShader(m_vertexShader);
    m_pD3DDevice->SetPixelShader(m_pixelShader);

#ifdef HAS_XBOX_D3D
    // Render the image
    m_pD3DDevice->SetScreenSpaceOffset(-0.5f, -0.5f);
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
#endif
  }
  // Keep track of the nested begin/end calls.
  m_dwNestedBeginCount++;
#endif
}

void CGUIFontTTF::End()
{
#ifndef HAS_SDL
  if (m_dwNestedBeginCount == 0)
    return;

  if (--m_dwNestedBeginCount > 0)
    return;

#ifdef HAS_XBOX_D3D
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);
#endif
  m_pD3DDevice->SetPixelShader(NULL);
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
#endif
}

void CGUIFontTTF::RenderCharacter(float posX, float posY, const CAngle &angle, const Character *ch, D3DCOLOR dwColor)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->right - ch->left;
  const float height = ch->bottom - ch->top;

  /* top left of our texture isn't the topleft of the textcell */
  /* celltop could be higher than m_iHeight over baseline */
  posX += ch->offsetX * angle.cosine - ch->offsetY * angle.sine;
  posY += ch->offsetX * angle.sine + ch->offsetY * angle.cosine;

#ifdef HAS_XBOX_D3D
  m_pD3DDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor);

  m_pD3DDevice->SetVertexData4f( 0, posX, posY, ch->left, ch->top );
  m_pD3DDevice->SetVertexData4f( 0, posX + angle.cosine * width, posY + angle.sine * width, ch->right, ch->top );
  m_pD3DDevice->SetVertexData4f( 0, posX + angle.cosine * width - angle.sine * height, posY + angle.sine * width + angle.cosine * height, ch->right, ch->bottom );
  m_pD3DDevice->SetVertexData4f( 0, posX - angle.sine * height, posY + angle.cosine * height, ch->left, ch->bottom );
#elif !defined(HAS_SDL)
struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
  };

  // tex coords converted to 0..1 range
  float tl = ch->left / m_textureWidth;
  float tr = ch->right / m_textureWidth;
  float tt = ch->top / m_textureHeight;
  float tb = ch->bottom / m_textureHeight;

  CUSTOMVERTEX verts[4] =  {
    { posX                                         , posY                                         , 0.0f, 1.0f, dwColor, tl, tt},
    { posX + angle.cosine*width                    , posY + angle.sine*width                      , 0.0f, 1.0f, dwColor, tr, tt},
    { posX + angle.cosine*width - angle.sine*height, posY + angle.sine*width + angle.cosine*height, 0.0f, 1.0f, dwColor, tr, tb},
    { posX - angle.sine*height                     , posY + angle.cosine*height                   , 0.0f, 1.0f, dwColor, tl, tb} 
  };

  m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
#else

  // Copy the character to a temporary surface so we can adjust its colors 
  SDL_Surface* tempSurface = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA, (int) width, (int) height, 32,
        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

  SDL_LockSurface(tempSurface);
  SDL_LockSurface(m_texture);
  
  unsigned int* src = (unsigned int*) m_texture->pixels + ((int) ch->top * m_texture->w) + ((int) ch->left);
  unsigned int* dst = (unsigned int*) tempSurface->pixels;
  
  // Calculate the alpha per pixel based on the alpha channel of the pixel and the alpha channel of
  // the requested color 
  int alpha;
  float alphaFactor = (float) ((dwColor & 0xff000000) >> 24) / 255;
  for (int y = 0; y < tempSurface->h; y++)
  {
  	 for (int x = 0; x < tempSurface->w; x++)
  	 {
      alpha = (int) (alphaFactor * (((unsigned int) src[x] & 0xff000000) >> 24));
      dst[x] = (alpha << 24) | (dwColor & 0x00ffffff);
	 }
	   
	src += (m_texture->pitch / 4);
	dst += tempSurface->w;
  }
  
  SDL_UnlockSurface(tempSurface);
  SDL_UnlockSurface(m_texture);

  if (angle.theta != 0)
  {
    // This angular stuff somewhat works but it is not really aligned. This will have to do for now 
    SDL_Surface* angledSurface = rotozoomSurface(tempSurface, 360 - angle.theta, 1.0, 0);
    SDL_Rect dstRect2 = { (Sint16) posX, (Sint16) posY, 0 , 0 };
    SDL_BlitSurface(angledSurface, NULL, g_graphicsContext.getScreenSurface(), &dstRect2);
  }
  else
  {
    // Copy the surface to the screen (without angle). 
    SDL_Rect dstRect2 = { (Sint16) posX, (Sint16) posY, 0 , 0 };
    g_graphicsContext.BlitToScreen(tempSurface, NULL, &dstRect2);
  }
  
  SDL_FreeSurface(tempSurface);
#endif
}

#ifndef HAS_SDL
void CGUIFontTTF::CreateShader()
{
  if (!m_pixelShader)
  {
    // shader from the alpha texture to the full 32bit font.  Basically, anything with
    // alpha > 0 is filled in fully in the colour channels

    const char *fonts =
      "ps.1.1\n"
      "tex t0\n"
      "mov r0.rgb, v0\n"
      "+ mul r0.a, v0.a, t0.b\n";

    LPD3DXBUFFER pShader = NULL;
    if( D3D_OK != D3DXAssembleShader(fonts, strlen(fonts), NULL, NULL, &pShader, NULL) )
      CLog::Log(LOGINFO, "%s - Failed to assemble pixel shader", __FUNCTION__);
    else
    {

      if (D3D_OK != m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_pixelShader))
      {
        CLog::Log(LOGINFO, "%s - Failed to create pixel shader",  __FUNCTION__);
        m_pixelShader = 0;
      }
      pShader->Release();
    }
  }

#ifdef HAS_XBOX_D3D
  // since this vertex declaration is different from
  // the standard, it can't be used when drawprimitive
  // is to be used.

  // Create the vertex shader
  if (m_pixelShader && !m_vertexShader)
  {
    // our vertex declaration
    DWORD vertexDecl[] =
      {
        D3DVSD_STREAM(0),
        D3DVSD_REG( 0, D3DVSDT_FLOAT4 ),         // xy vertex, zw texture coord
        D3DVSD_REG( 3, D3DVSDT_D3DCOLOR ),       // diffuse color
        D3DVSD_END()
      };

        // shader for the vertex format we use
    static DWORD vertexShader[] =
      {
        0x00032078,
        0x00000000, 0x00200015, 0x0836106c, 0x2070c800,  // mov oPos.xy, v0.xy
        0x00000000, 0x002000bf, 0x0836106c, 0x2070c848,  // mov oT0.xy, v0.zw
        0x00000000, 0x0020061b, 0x0836106c, 0x2070f819  // mov oD0, v3
      };

    if (D3D_OK != m_pD3DDevice->CreateVertexShader(vertexDecl,vertexShader, &m_vertexShader, D3DUSAGE_PERSISTENTDIFFUSE))
      CLog::Log(LOGERROR, __FUNCTION__" - Failed to create vertex shader");
  }
#endif

  if(!m_vertexShader)
    m_vertexShader = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
}
#endif
