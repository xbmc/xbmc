#include "include.h"
#include "GUIFontTTF.h"
#include "GUIFontManager.h"
#include "GraphicContext.h"

#include <math.h>

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
#elif !defined(__GNUC__)
  #pragma comment (lib,"../../guilib/freetype2/freetype221.lib")
#endif
#endif
#ifdef _LINUX
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define ROUND rintf
#define ROUND_TO_PIXEL rintf
#else
namespace MathUtils {
  inline int round_int (double x);
}

#define ROUND(x) (float)(MathUtils::round_int(x))

#ifdef HAS_XBOX_D3D
#define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x))
#else
#define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x)) - 0.5f
#endif

#endif // _LINUX

DWORD PadPow2(DWORD x);

#define CHARS_PER_TEXTURE_LINE 20 // number of characters to cache per texture line
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)

int CGUIFontTTF::justification_word_weight = 6;   // weight of word spacing over letter spacing when justifying.
                                                  // A larger number means more of the "dead space" is placed between
                                                  // words rather than between letters.


class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
  }

  FT_Face GetFont(const CStdString &filename, int size, float aspect)
  {
    // check if we have this font already
    for (unsigned int i = 0; i < m_fonts.size(); i++)
    {
      CFTFont &font = m_fonts[i];
      if (font.Matches(filename, size, aspect))
        return font.GetFace();
    }

    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::Log(LOGERROR, "Unable to initialize freetype library");
      return NULL;
    }

    FT_Face face;

    // ok, now load the font face
    if (FT_New_Face( m_library, filename.c_str(), 0, &face ))
      return NULL;

    unsigned int ydpi = GetDPI();
    unsigned int xdpi = (unsigned int)ROUND(ydpi * aspect);

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size( face, 0, size*64, xdpi, ydpi ))
    {
      FT_Done_Face(face);
      return NULL;
    }

    // push back the font
    CFTFont font(filename, size, aspect, face);
    m_fonts.push_back(font);

    return face;
  };

  void ReleaseFont(FT_Face face)
  {
    // find the font
    for (unsigned int i = 0; i < m_fonts.size(); i++)
    {
      CFTFont &font = m_fonts[i];
      if (font.Matches(face))
      {
        if (font.ReleaseFace())
          m_fonts.erase(m_fonts.begin() + i);
        break;
      }
    }
    // check if we can free the font library as well
    if (!m_fonts.size() && m_library)
    {
      FT_Done_FreeType(m_library);
      m_library = NULL;
    }
  }

  unsigned int GetDPI() const
  {
    return 72; // default dpi, matches what XPR fonts used to use for sizing
  };

private:
  class CFTFont
  {
  public:
    CFTFont(const CStdString &filename, int size, float aspect, FT_Face face)
    {
      m_filename = filename;
      m_size = size;
      m_aspect = aspect;
      m_face = face;
      m_references = 1;
    };

    bool Matches(const CStdString &filename, int size, float aspect) const
    {
      return (m_filename == filename && m_size == size && m_aspect == aspect);
    };
    
    bool Matches(FT_Face face) const
    {
      return m_face == face;
    };

    FT_Face GetFace()
    {
      m_references++;
      return m_face;
    };

    bool ReleaseFace()
    {
      assert(m_references);
      m_references--;
      if (!m_references)
      {
        FT_Done_Face(m_face);
        return true;
      }
      return false;
    };

  private:
    CStdString   m_filename;
    int          m_size;
    float        m_aspect;
    FT_Face      m_face;
    unsigned int m_references;
  };

  vector<CFTFont> m_fonts;
  FT_Library   m_library;
};

CFreeTypeLibrary g_freeTypeLibrary; // our freetype library

CGUIFontTTF::CGUIFontTTF(const CStdString& strFileName)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_dwNestedBeginCount = 0;
#ifdef HAS_SDL_OPENGL
  m_glTextureLoaded = false;
#endif
  m_face = NULL;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_strFileName = strFileName;
  m_referenceCount = 0;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTF::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}

void CGUIFontTTF::ClearCharacterCache()
{
  if (m_texture)
#ifndef HAS_SDL  
  {
    m_texture->Release();
  }
#elif defined(HAS_SDL_2D)
  {
    SDL_FreeSurface(m_texture);
  }
#elif defined(HAS_SDL_OPENGL)
  {
    SDL_FreeSurface(m_texture);
  }

  if (m_glTextureLoaded)
  {
    if (glIsTexture(m_glTexture))
      glDeleteTextures(1, &m_glTexture);
    m_glTextureLoaded = false;
  }
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
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_posX = 0;
  m_posY = 0;
  m_dwNestedBeginCount = 0;

  g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = NULL;
}

bool CGUIFontTTF::Load(const CStdString& strFilename, int iHeight, int iStyle, float aspect)
{
#ifndef HAS_SDL
  // create our character texture + font shader
  m_pD3DDevice = g_graphicsContext.Get3DDevice();
#endif

  m_face = g_freeTypeLibrary.GetFont(strFilename, iHeight, aspect);

  if (!m_face)
    return false;

  // grab the maximum cell height and width
  unsigned int m_cellWidth = m_face->bbox.xMax - m_face->bbox.xMin;
  m_cellHeight = m_face->bbox.yMax - m_face->bbox.yMin;
  m_cellBaseLine = m_face->bbox.yMax;
  m_lineHeight = m_face->height;

  unsigned int ydpi = g_freeTypeLibrary.GetDPI();
  unsigned int xdpi = (unsigned int)ROUND(ydpi * aspect);

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

  CLog::Log(LOGDEBUG, " Scaled size of font %s (%i): width = %i, height = %i",
    strFilename.c_str(), iHeight, m_cellWidth, m_cellHeight);
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
#ifdef HAS_SDL_OPENGL
  m_textureWidth = PadPow2(m_textureWidth);
#endif
  if (m_textureWidth > 4096) m_textureWidth = 4096;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ellipse->advance;

  return true;
}

void CGUIFontTTF::DrawTextInternal( FLOAT sx, FLOAT sy, DWORD *pdw256ColorPalette, BYTE *pbColours, const WCHAR* strText, DWORD cchText, DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  Begin();

  // save the origin, which is scaled separately
  m_originX = sx;
  m_originY = sy;

  // vertically centered
  if (dwFlags & XBFONT_CENTER_Y)
    sy = -0.5f*(m_cellHeight-2);
  else
    sy = 0;

  // Check if we will really need to truncate the CStdString
  if ( dwFlags & XBFONT_TRUNCATED )
  {
    if ( fMaxPixelWidth <= 0.0f)
      dwFlags &= ~XBFONT_TRUNCATED;
    else
    {
      float width;
      GetTextExtentInternal(strText, &width, NULL);
      if (width <= fMaxPixelWidth)
        dwFlags &= ~XBFONT_TRUNCATED;
    }
  }
  else if ( dwFlags & XBFONT_JUSTIFIED )
  {
    if ( fMaxPixelWidth <= 0.0f )
      dwFlags &= XBFONT_JUSTIFIED;
  }

  float lineX;
  float lineY;
  float cursorX;
  int numLines = 0;
  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;
  float spacePerLetter = 0; // for justification effects

  while ( cchText-- )
  {
    // If starting text on a new line, determine justification effects
    if ( bStartingNewLine )
    {
      lineX = 0;
      lineY = sy + (float)m_lineHeight * numLines;
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
        lineX -= w;
      }
      if ( dwFlags & XBFONT_JUSTIFIED )
      { // first compute the size of the text to render (single line) in both characters and pixels
        const WCHAR *spaceCalc = strText;
        unsigned int lineLength = 0;
        while (*spaceCalc && *spaceCalc != L'\n')
        {
          WCHAR letter = *spaceCalc++;
          if (letter == L'\r') continue;
          // spaces have multiple times the justification spacing of normal letters
          lineLength += (letter == L' ') ? justification_word_weight : 1;
        }
        float width;
        GetTextExtentInternal(strText, &width, NULL, TRUE);
        spacePerLetter = (fMaxPixelWidth - width) / (lineLength - 1);
      }
      bStartingNewLine = FALSE;
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
          RenderCharacter(lineX + cursorX, lineY, period, dwColor);
          cursorX += period->advance;
        }
        break;
      }
    }
    else if (fMaxPixelWidth > 0 && cursorX > fMaxPixelWidth)
      break;  // exceeded max allowed width - stop rendering

    RenderCharacter(lineX + cursorX, lineY, ch, dwColor);
    if ( dwFlags & XBFONT_JUSTIFIED )
    {
      if (letter == L' ')
        cursorX += ch->advance + spacePerLetter * justification_word_weight;
      else
        cursorX += ch->advance + spacePerLetter;
    }
    else
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
  *pWidth = 0.0f;
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

  if (pHeight)
    *pHeight = (float)(numLines - 1) * m_lineHeight + (m_cellHeight - 2); // -2 as we increment this for space in our texture
  return ;
}

CGUIFontTTF::Character* CGUIFontTTF::GetCharacter(WCHAR letter)
{
  // quick access to ascii chars
  if(letter < 255)
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
      if (dwNestedBeginCount) Begin();
      m_dwNestedBeginCount = dwNestedBeginCount;
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
  {
    CLog::Log(LOGDEBUG, "%s Failed to load glyph %x", __FUNCTION__, letter);
    return false;
  }
  // make bold if applicable
  if (m_iStyle == FONT_STYLE_BOLD || m_iStyle == FONT_STYLE_BOLD_ITALICS)
    FT_GlyphSlot_Embolden(m_face->glyph);
  // and italics if applicable
  if (m_iStyle == FONT_STYLE_ITALICS || m_iStyle == FONT_STYLE_BOLD_ITALICS)
    FT_GlyphSlot_Oblique(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::Log(LOGDEBUG, "%s Failed to get glyph %x", __FUNCTION__, letter);
    return false;
  }
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
  {
    CLog::Log(LOGDEBUG, "%s Failed to render glyph %x to a bitmap", __FUNCTION__, letter);
    return false;
  }
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  if (bitGlyph->left < 0)
    m_posX += -bitGlyph->left;

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
#ifdef HAS_XBOX
      LPDIRECT3DTEXTURE8 newTexture;
#endif
      // check for max height (can't be more than 4096 texels)
      if (newHeight > 4096)
      {
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: New cache texture is too large (%i > 4096 pixels long)", newHeight);
        FT_Done_Glyph(glyph);
        return false;
      }
            
#ifndef HAS_SDL      
      LPDIRECT3DTEXTURE8 newTexture;
      if (D3D_OK != D3DXCreateTexture(m_pD3DDevice, m_textureWidth, newHeight, 1, 0, D3DFMT_LIN_A8, D3DPOOL_MANAGED, &newTexture))
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
#ifdef HAS_SDL_OPENGL
      newHeight = PadPow2(newHeight);
      SDL_Surface* newTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, m_textureWidth, newHeight, 8,
          0, 0, 0, 0xff);
#else
		  SDL_Surface* newTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, m_textureWidth, newHeight, 32,
        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
#endif
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
  ch->offsetY = (short)max((short)m_cellBaseLine - bitGlyph->top, 0);
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
      bitmap.buffer, D3DFMT_LIN_A8, bitmap.pitch, NULL, &sourcerect, 
      D3DX_FILTER_NONE, 0x00000000);

    SAFE_RELEASE(target);
#else
    SDL_LockSurface(m_texture);

    unsigned char* source = (unsigned char*) bitmap.buffer;
#ifdef HAS_SDL_OPENGL
    unsigned char* target = (unsigned char*) m_texture->pixels + (m_posY + ch->offsetY) * m_texture->pitch + m_posX + bitGlyph->left;
    for (int y = 0; y < bitmap.rows; y++)
    {
      memcpy(target, source, bitmap.width);
      source += bitmap.width;
      target += m_texture->pitch;
    }
    // Since we have a new texture, we need to delete the old one
    // the Begin(); End(); stuff is handled by whoever called us
    if (m_glTextureLoaded)
    {
      if (glIsTexture(m_glTexture))
	glDeleteTextures(1, &m_glTexture);
      m_glTextureLoaded = false;
    }        
#else
    unsigned int *target = (unsigned int*) (m_texture->pixels) + 
        ((m_posY + ch->offsetY) * m_texture->pitch/4) + 
        (m_posX + bitGlyph->left);
    
    for (int y = 0; y < bitmap.rows; y++)
    {
      for (int x = 0; x < bitmap.width; x++)
      {
        target[x] = ((unsigned int) source[x] << 24) | 0x00ffffffL;
      }
     
      source += bitmap.width;
      target += (m_texture->pitch / 4);
    }
#endif
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
  if (m_dwNestedBeginCount == 0)
  {
#ifndef HAS_SDL
    // just have to blit from our texture.
    m_pD3DDevice->SetTexture( 0, m_texture );

    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 ); // only use diffuse
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    // no other texture stages needed
    m_pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

    m_pD3DDevice->SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

#ifdef HAS_XBOX_D3D
    // Render the image
    m_pD3DDevice->SetScreenSpaceOffset(-0.5f, -0.5f);
    m_pD3DDevice->Begin(D3DPT_QUADLIST);
#endif
#elif defined(HAS_SDL_OPENGL)
    if (!m_glTextureLoaded)
    {
      // Have OpenGL generate a texture object handle for us
      glGenTextures(1, &m_glTexture);
 
      // Bind the texture object
      glBindTexture(GL_TEXTURE_2D, m_glTexture);
      glEnable(GL_TEXTURE_2D);
 
      // Set the texture's stretching properties
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 
      // Set the texture image
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texture->w, m_texture->h, 0,
                   GL_ALPHA, GL_UNSIGNED_BYTE, m_texture->pixels); 
    
      VerifyGLState();
      m_glTextureLoaded = true;                
    }
  
    // Turn Blending On
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_glTexture);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    VerifyGLState();

    glBegin(GL_QUADS);
#endif
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

#ifndef HAS_SDL
#ifdef HAS_XBOX_D3D
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);
#endif
  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
#elif defined(HAS_SDL_OPENGL)
  glEnd();  
#endif
}

void CGUIFontTTF::RenderCharacter(float posX, float posY, const Character *ch, D3DCOLOR dwColor)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->right - ch->left;
  const float height = ch->bottom - ch->top;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->offsetX) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY) * g_graphicsContext.GetGUIScaleY(),
               (posX + ch->offsetX + width) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY + height) * g_graphicsContext.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
  CRect texture(ch->left, ch->top, ch->right, ch->bottom);
  g_graphicsContext.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x1 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1));
  float y1 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1));
  float z1 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1));

  float x2 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1));
  float y2 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1));
  float z2 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1));

  float x3 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2));
  float y3 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2));
  float z3 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2));

  float x4 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2));
  float y4 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2));
  float z4 = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2));

#ifdef HAS_XBOX_D3D
  m_pD3DDevice->SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor);

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y1);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x1, y1, z1, 1);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x2, y2, z2, 1);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y2);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x3, y3, z3, 1);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x4, y4, z4, 1);

#elif !defined(HAS_SDL)
struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
  };

  // tex coords converted to 0..1 range
  float tl = texture.x1 / m_textureWidth;
  float tr = texture.x2 / m_textureWidth;
  float tt = texture.y1 / m_textureHeight;
  float tb = texture.y2 / m_textureHeight;

  CUSTOMVERTEX verts[4] =  {
    { x1, y1, z1, dwColor, tl, tt},
    { x2, y2, z2, dwColor, tr, tt},
    { x3, y3, z3, dwColor, tr, tb},
    { x4, y4, z4, dwColor, tl, tb}
  };

  m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
#elif defined(HAS_SDL_2D)

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

  // Copy the surface to the screen (without angle). 
  SDL_Rect dstRect2 = { (Sint16) posX, (Sint16) posY, 0 , 0 };
  g_graphicsContext.BlitToScreen(tempSurface, NULL, &dstRect2);
  
  SDL_FreeSurface(tempSurface);  
#elif defined(HAS_SDL_OPENGL)
  // tex coords converted to 0..1 range
  float tl = texture.x1 / m_textureWidth;
  float tr = texture.x2 / m_textureWidth;
  float tt = texture.y1 / m_textureHeight;
  float tb = texture.y2 / m_textureHeight;
  
  GLubyte colors[4] = { (GLubyte)((dwColor >> 16) & 0xff), (GLubyte)((dwColor >> 8) & 0xff), (GLubyte)(dwColor & 0xff), (GLubyte)(dwColor >> 24) };
  
  // Top-left vertex (corner)
  glColor4ubv(colors); 
  glTexCoord2f(tl, tt);
  glVertex3f(x1, y1, z1);
   
  // Bottom-left vertex (corner)
  glColor4ubv(colors); 
  glTexCoord2f(tr, tt);
  glVertex3f(x2, y2, z2);
    
  // Bottom-right vertex (corner)
  glColor4ubv(colors); 
  glTexCoord2f(tr, tb);
  glVertex3f(x3, y3, z3);
    
  // Top-right vertex (corner)
  glColor4ubv(colors); 
  glTexCoord2f(tl, tb);
  glVertex3f(x4, y4, z4);
#endif
}

