//-----------------------------------------------------------------------------
// File: XBFont.cpp
//
// Desc: Texture-based font class. This class reads .abc font files that are
//       generated by the FontMaker tool. These .abc files are used to create
//       a texture with all the font's glyph, and also extract information on
//       the dimensions of each glyph.
//
//       Once created, this class is used to render text in a 3D scene with the
//       following function:
//          DrawText( fScreenY, fScreenSpaceY, dwTextColor, strText,
//                    dwJustificationFlags );
//
//       For performance, you can batch mulitple DrawText calls together
//       between Begin() and End() calls, as in the following example:
//          pFont->Begin();
//          pFont->DrawText( ... );
//          pFont->DrawText( ... );
//          pFont->DrawText( ... );
//          pFont->End();
//
//       The size (extent) of the text can be computed without rendering with
//       the following function:
//          GetTextExtent( strText, pfReturnedWidth, pfReturnedHeight,
//                         bComputeExtentUsingFirstLineOnly );
//
//       Finally, the font class can create a texture to hold rendered text,
//       which is useful for static text that must be rendered for many
//       frames, or can even be used within a 3D scene. (For instance, for a
//       player's name on a jersey.) Use the following function for this:
//          CreateTexture( strText, d3dTextureFormat );
//
//       See the XDK docs for more information.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       02.18.01 - Changes for March XDK release
//       04.15.01 - Using packed resources for May XDK
//       12.10.02 - Optimizing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifdef _XBOX

#include "../include.h"
#include <xgraphics.h>

#include "XBFont.h"
#include "../include.h"
#include "../GUIFont.h"

//-----------------------------------------------------------------------------
// Static objects
//-----------------------------------------------------------------------------
DWORD CXBFont::m_dwFontVertexShader = 0L;
DWORD CXBFont::m_dwFontPixelShader = 0L;




//-----------------------------------------------------------------------------
// Name: CXBFont()
// Desc: Font class constructor.
//-----------------------------------------------------------------------------
CXBFont::CXBFont()
{
  m_pFontTexture = NULL;

  m_fFontHeight = 36.0f;
  m_fFontTopPadding = 0.0f;
  m_fFontBottomPadding = 0.0f;
  m_fFontYAdvance = 36.0f;

  m_dwNumGlyphs = 0L;
  m_Glyphs = NULL;

  m_fCursorX = 0.0f;
  m_fCursorY = 0.0f;

  m_fXScaleFactor = 1.0f;
  m_fYScaleFactor = 1.0f;
  m_fSlantFactor = 0.0f;

  m_cMaxGlyph = 0;
  m_TranslatorTable = NULL;
  m_bTranslatorTableWasAllocated = FALSE;

  m_dwNestedBeginCount = 0L;

  m_dwCurrentColor = 0xffffffff; // white
}




//-----------------------------------------------------------------------------
// Name: ~CXBFont()
// Desc: Font class destructor.
//-----------------------------------------------------------------------------
CXBFont::~CXBFont()
{
  Destroy();
}




//-----------------------------------------------------------------------------
// Name: CreateShaders()
// Desc: Creates a vertex shader for rendering fonts, and a pixel shader for
//       font rendering to ensure NTSC safe colors
//-----------------------------------------------------------------------------
HRESULT CXBFont::CreateShaders()
{
  // Create the vertex shader
  if ( 0L == m_dwFontVertexShader )
  {
    // Specify the vertex declaration, used here to manually specify a constant.
    DWORD dwFontVertexDecl[] =
      {
        D3DVSD_STREAM(0),
        D3DVSD_REG( 0, D3DVSDT_FLOAT4 ),         // Vertex/tex coord combo
        D3DVSD_REG( 3, D3DVSDT_D3DCOLOR ),       // Color
        D3DVSD_END()
      };

    // Microcode for the vertex shader, where input is defined as:
    //    v0.xy = screen space position
    //    v0.zw = tex coords
    //    v3    = diffuse color
    static DWORD dwFontVertexShaderInstructions[] =
      {
        0x00032078,
        0x00000000, 0x00200015, 0x0836106c, 0x2070c800,  // mov oPos.xy, v0.xy
        0x00000000, 0x002000bf, 0x0836106c, 0x2070c848,  // mov oT0.xy, v0.zw
        0x00000000, 0x0020061b, 0x0836106c, 0x2070f819  // mov oD0, v3
      };

    // Create the vertex shader
    if ( FAILED( D3DDevice::CreateVertexShader( dwFontVertexDecl,
                 dwFontVertexShaderInstructions,
                 &m_dwFontVertexShader,
                 D3DUSAGE_PERSISTENTDIFFUSE ) ) )
      return E_FAIL;

  }

  // Create the pixel shader
  if ( 0L == m_dwFontPixelShader )
  {
    D3DPIXELSHADERDEF psd;
    ZeroMemory( &psd, sizeof(psd) );
    psd.PSCombinerCount = PS_COMBINERCOUNT( 1, 0 );
    psd.PSTextureModes = PS_TEXTUREMODES( PS_TEXTUREMODES_PROJECT2D, 0, 0, 0 );

    //------------- Stage 0 -------------
    psd.PSRGBInputs[0] = PS_COMBINERINPUTS( PS_REGISTER_T0 | PS_CHANNEL_RGB, PS_REGISTER_V0 | PS_CHANNEL_RGB, 0, 0 );
    psd.PSAlphaInputs[0] = PS_COMBINERINPUTS( PS_REGISTER_T0 | PS_CHANNEL_ALPHA, PS_REGISTER_V0 | PS_CHANNEL_ALPHA, 0, 0 );
    psd.PSRGBOutputs[0] = PS_COMBINEROUTPUTS( PS_REGISTER_R0, 0, 0, PS_COMBINEROUTPUT_AB_MULTIPLY );
    psd.PSAlphaOutputs[0] = PS_COMBINEROUTPUTS( PS_REGISTER_R0, 0, 0, PS_COMBINEROUTPUT_AB_MULTIPLY );

    //------------- Final combiner -------------
    psd.PSFinalCombinerInputsABCD = PS_COMBINERINPUTS( PS_REGISTER_C0, PS_REGISTER_R0, PS_REGISTER_ZERO, PS_REGISTER_C1 );
    psd.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS( 0, 0, PS_REGISTER_R0 | PS_CHANNEL_ALPHA, 0 );

    // Set constants to scale output to an NTSC-safe range
    // NTSC_MAX = 0.8549f => 0x00dadada
    // NTSC_MIN = 0.0625f => 0x00101010
    psd.PSFinalCombinerConstant0 = 0x00dadada;
    psd.PSFinalCombinerConstant1 = 0x00101010;
    psd.PSFinalCombinerConstants = PS_FINALCOMBINERCONSTANTS(0, 1, PS_GLOBALFLAGS_NO_TEXMODE_ADJUST);

    // Create the pixel shader, as defined above.
    if ( FAILED( D3DDevice::CreatePixelShader( &psd, &m_dwFontPixelShader ) ) )
      return E_FAIL;
  }

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Create the font's internal objects (texture and array of glyph info)
//       using the XPR packed resource file
//-----------------------------------------------------------------------------
HRESULT CXBFont::Create( const CHAR* strFontResourceFileName, DWORD dwResourceOffset )
{
  // Load the resource for the font
  if ( FAILED( m_xprResource.Create( strFontResourceFileName ) ) )
    return E_FAIL;

  return Create( m_xprResource.GetTexture( dwResourceOffset ),
                 m_xprResource.GetData( dwResourceOffset + sizeof(D3DTexture) ) );
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Create the font's internal objects (texture and array of glyph info)
//-----------------------------------------------------------------------------
HRESULT CXBFont::Create( D3DTexture* pFontTexture, VOID* pFontData )
{
  m_pFontTexture = pFontTexture;

  BYTE* pData = (BYTE*)pFontData;
  DWORD dwResourceType = ((DWORD*)pData)[0];
  DWORD dwResourceSize = ((DWORD*)pData)[1];
  pData += 2 * sizeof(DWORD);
  (VOID)dwResourceType; // not used
  (VOID)dwResourceSize; // not used

  DWORD dwFileVersion = *((DWORD*)pData); pData += sizeof(DWORD);

  // Check version of file (to make sure it matches up with the FontMaker tool)
  if ( dwFileVersion == 0x00000004 )
  {
    // Note: Version 0x00000004 is now considered to be the "old" way of
    // doing fonts. Support for this version of font files will be going
    // away, and a more recent version of FontMaker should now be used.
    m_fFontHeight = (FLOAT) * ((DWORD*)pData); pData += sizeof(DWORD);
    m_fFontYAdvance = m_fFontHeight;
    FLOAT fTexWidth = (FLOAT) * ((DWORD*)pData); pData += sizeof(DWORD);
    FLOAT fTexHeight = (FLOAT) * ((DWORD*)pData); pData += sizeof(DWORD);
    DWORD dwBPP = *((DWORD*)pData); pData += sizeof(DWORD);
    (VOID)dwBPP; // not used

    // Read the low and high char
    WCHAR cLowChar = *((WCHAR*)pData); pData += sizeof(WCHAR);
    WCHAR cHighChar = *((WCHAR*)pData); pData += sizeof(WCHAR);

    // Read the glyph attributes from the file
    m_Glyphs = (GLYPH_ATTR*)(pData + 4);

    m_cMaxGlyph = cHighChar;
    m_TranslatorTable = new WCHAR[m_cMaxGlyph + 1];
    m_bTranslatorTableWasAllocated = TRUE;

    for ( WORD i = 0; i <= m_cMaxGlyph; i++ )
    {
      if ( i < cLowChar )
        m_TranslatorTable[i] = 0;
      else
      {
        m_TranslatorTable[i] = i - cLowChar;

        GLYPH_ATTR* pGlyph = &m_Glyphs[m_TranslatorTable[i]];

        pGlyph->tu1 *= ( fTexWidth - 1) / fTexWidth;
        pGlyph->tv1 *= (fTexHeight - 1) / fTexHeight;
        pGlyph->tu2 *= ( fTexWidth - 1) / fTexWidth;
        pGlyph->tv2 *= (fTexHeight - 1) / fTexHeight;

        pGlyph->wWidth += 1;
      }
    }

    m_fFontHeight = m_fFontHeight + 1;
  }
  else if ( dwFileVersion == 0x00000005 )
  {
    // Note: Version 0x00000004 is considered to be the "new" way of
    // doing fonts, using the most recent version of the FontMaker tool.
    m_fFontHeight = *((FLOAT*)pData); pData += sizeof(FLOAT);
    m_fFontTopPadding = *((FLOAT*)pData); pData += sizeof(FLOAT);
    m_fFontBottomPadding = *((FLOAT*)pData); pData += sizeof(FLOAT);
    m_fFontYAdvance = *((FLOAT*)pData); pData += sizeof(FLOAT);

    // Point to the translator CStdString
    m_cMaxGlyph = ((WORD*)pData)[0];
    pData += sizeof(WORD);
    m_TranslatorTable = (WCHAR*)pData;
    pData += sizeof(WCHAR) * (m_cMaxGlyph + 1);
    m_bTranslatorTableWasAllocated = FALSE;

    // Read the glyph attributes from the file
    m_dwNumGlyphs = ((DWORD*)pData)[0];
    pData += sizeof(DWORD);
    m_Glyphs = (GLYPH_ATTR*)pData;
  }
  else
  {
    CLog::Log(LOGFATAL, "XBFont: Incorrect version number on font file!\n" );
    return E_FAIL;
  }

  // Creates the vertex shader for rendering fonts, and a pixel shader for
  // font rendering to ensure NTSC safe colors
  if ( FAILED( CreateShaders() ) )
    return E_FAIL;

  // Determine whether we should save/restore state
  D3DDEVICE_CREATION_PARAMETERS d3dcp;
  D3DDevice::GetCreationParameters( &d3dcp );
  m_bSaveState = (d3dcp.BehaviorFlags & D3DCREATE_PUREDEVICE) ? FALSE : TRUE;

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroys the font object
//-----------------------------------------------------------------------------
HRESULT CXBFont::Destroy()
{
  m_xprResource.Destroy();

  m_pFontTexture = NULL;

  m_fFontHeight = 0.0f;
  m_fFontTopPadding = 0.0f;
  m_fFontBottomPadding = 0.0f;
  m_fFontYAdvance = 0.0f;

  m_dwNumGlyphs = 0L;
  m_Glyphs = NULL;

  m_cMaxGlyph = 0;

  if ( m_bTranslatorTableWasAllocated )
    delete[] m_TranslatorTable;

  m_TranslatorTable = NULL;
  m_bTranslatorTableWasAllocated = FALSE;

  m_dwNestedBeginCount = 0L;

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SetCursorPosition()
// Desc: Sets the cursor position for drawing text
//-----------------------------------------------------------------------------
VOID CXBFont::SetCursorPosition( FLOAT fCursorX, FLOAT fCursorY )
{
  m_fCursorX = floorf( fCursorX + 0.5f );
  m_fCursorY = floorf( fCursorY + 0.5f );
}




//-----------------------------------------------------------------------------
// Name: SetScaleFactors()
// Desc: Sets X and Y scale factor to make rendered text bigger or smaller.
//       Note that since text is pre-anti-aliased and therefore point-filtered,
//       any scale factors besides 1.0f will degrade the quality.
//-----------------------------------------------------------------------------
VOID CXBFont::SetScaleFactors( FLOAT fXScaleFactor, FLOAT fYScaleFactor )
{
  m_fXScaleFactor = fXScaleFactor;
  m_fYScaleFactor = fYScaleFactor;
}




//-----------------------------------------------------------------------------
// Name: SetSlantFactor()
// Desc: Sets the slant factor for rendering slanted text.
//-----------------------------------------------------------------------------
VOID CXBFont::SetSlantFactor( FLOAT fSlantFactor )
{
  m_fSlantFactor = fSlantFactor;
}




//-----------------------------------------------------------------------------
// Name: GetTextExtent()
// Desc: Get the dimensions of a text CStdString
//-----------------------------------------------------------------------------
HRESULT CXBFont::GetTextExtent( const WCHAR* strText, FLOAT* pWidth,
                                FLOAT* pHeight, BOOL bFirstLineOnly ) const
{
  return GetTextExtent( strText, wcslen( strText), pWidth, pHeight, bFirstLineOnly );
}




//-----------------------------------------------------------------------------
// Name: GetTextExtent()
// Desc: Get the dimensions of a text CStdString
//-----------------------------------------------------------------------------
HRESULT CXBFont::GetTextExtent( const WCHAR* strText, DWORD cchText, FLOAT* pWidth,
                                FLOAT* pHeight, BOOL bFirstLineOnly ) const
{
  // Check parameters
  if ( NULL == strText || NULL == pWidth || NULL == pHeight )
    return E_INVALIDARG;

  // Set default text extent in output parameters
  (*pWidth) = 0.0f;
  (*pHeight) = 0.0f;

  // Initialize counters that keep track of text extent
  FLOAT sx = 0.0f;
  FLOAT sy = m_fFontHeight;

  // Loop through each character and update text extent
  while ( cchText-- )
  {
    WCHAR letter = *strText++;

    // Skip '\r'
    if ( letter == L'\r')
      continue;

    // Handle newline character
    if ( letter == L'\n' )
    {
      if ( bFirstLineOnly )
        return S_OK;
      sx = 0.0f;
      sy += m_fFontYAdvance;
    }

    // Translate unprintable characters
    GLYPH_ATTR* pGlyph = &m_Glyphs[GetGlyphToUse(letter)];

    // Get text extent for this character's glyph
    sx += pGlyph->wOffset;
    sx += pGlyph->wAdvance;

    // Store text extent of CStdString in output parameters
    if ( sx > (*pWidth) ) (*pWidth) = sx;
    if ( sy > (*pHeight) ) (*pHeight) = sy;
  }

  // Apply the scale factor to the result
  (*pWidth) *= m_fXScaleFactor;
  (*pHeight) *= m_fYScaleFactor;

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetTextWidth()
// Desc: Returns the width (in pixels) of a text CStdString
//-----------------------------------------------------------------------------
FLOAT CXBFont::GetTextWidth( const WCHAR* strText ) const
{
  FLOAT fTextWidth = 0.0f;
  FLOAT fTextHeight = 0.0f;

  GetTextExtent( strText, &fTextWidth, &fTextHeight );
  return fTextWidth;
}




//-----------------------------------------------------------------------------
// Name: Begin()
// Desc: Prepares the font vertex buffers for rendering.
//-----------------------------------------------------------------------------
HRESULT CXBFont::Begin()
{
  // Set state on the first call
  if ( 0 == m_dwNestedBeginCount )
  {
    // Save state
    if ( m_bSaveState )
    {
      // Note, we are not saving the texture, vertex, or pixel shader,
      //       since it's not worth the performance. We're more interested
      //       in saving state that would cause hard to find problems.
      D3DDevice::GetRenderState( D3DRS_ALPHABLENDENABLE, &m_dwSavedState[0] );
      D3DDevice::GetRenderState( D3DRS_SRCBLEND, &m_dwSavedState[1] );
      D3DDevice::GetRenderState( D3DRS_DESTBLEND, &m_dwSavedState[2] );
      D3DDevice::GetRenderState( D3DRS_ALPHATESTENABLE, &m_dwSavedState[3] );
      D3DDevice::GetRenderState( D3DRS_ALPHAREF, &m_dwSavedState[4] );
      D3DDevice::GetRenderState( D3DRS_ALPHAFUNC, &m_dwSavedState[5] );
      D3DDevice::GetRenderState( D3DRS_FILLMODE, &m_dwSavedState[6] );
      D3DDevice::GetRenderState( D3DRS_CULLMODE, &m_dwSavedState[7] );
      D3DDevice::GetRenderState( D3DRS_ZENABLE, &m_dwSavedState[8] );
      D3DDevice::GetRenderState( D3DRS_STENCILENABLE, &m_dwSavedState[9] );
      D3DDevice::GetRenderState( D3DRS_EDGEANTIALIAS, &m_dwSavedState[10] );
      D3DDevice::GetTextureStageState( 0, D3DTSS_MINFILTER, &m_dwSavedState[11] );
      D3DDevice::GetTextureStageState( 0, D3DTSS_MAGFILTER, &m_dwSavedState[12] );
    }

    // Set the necessary render states
    D3DDevice::SetTexture( 0, m_pFontTexture );
    D3DDevice::SetVertexShader( m_dwFontVertexShader );
    D3DDevice::SetPixelShader( m_dwFontPixelShader );
    D3DDevice::SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    D3DDevice::SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    D3DDevice::SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    D3DDevice::SetRenderState( D3DRS_YUVENABLE, FALSE);
    D3DDevice::SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
    D3DDevice::SetRenderState( D3DRS_ALPHAREF, 0x08 );
    D3DDevice::SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
    D3DDevice::SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    D3DDevice::SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    D3DDevice::SetRenderState( D3DRS_ZENABLE, FALSE );
    D3DDevice::SetRenderState( D3DRS_STENCILENABLE, FALSE );
    D3DDevice::SetRenderState( D3DRS_EDGEANTIALIAS, FALSE );
    D3DDevice::SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    D3DDevice::SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    D3DDevice::SetScreenSpaceOffset( -0.5f, -0.5f );

    // Begin the drawing of vertices
    D3DDevice::Begin( D3DPT_QUADLIST );
  }

  // Keep track of the nested begin/end calls.
  m_dwNestedBeginCount++;

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DrawText()
// Desc: Draws text as textured polygons
//-----------------------------------------------------------------------------
HRESULT CXBFont::DrawText( DWORD dwColor, const WCHAR* strText, DWORD dwFlags,
                           FLOAT fMaxPixelWidth )
{
  return CXBFont::DrawText( m_fCursorX, m_fCursorY, CAngle(0), dwColor, strText,
                            dwFlags, fMaxPixelWidth );
}




//-----------------------------------------------------------------------------
// Name: DrawText()
// Desc: Draws text as textured polygons
//-----------------------------------------------------------------------------
HRESULT CXBFont::DrawText( FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle,
                            DWORD dwColor, const WCHAR* strText,
                             DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  return CXBFont::DrawTextEx( fOriginX, fOriginY, angle, dwColor, strText, wcslen( strText ),
                              dwFlags, fMaxPixelWidth );
}




//-----------------------------------------------------------------------------
// Name: DrawTextEx()
// Desc: Draws text as textured polygons
//-----------------------------------------------------------------------------
HRESULT CXBFont::DrawTextEx( FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor,
                             const WCHAR* strText, DWORD cchText,
                             DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  // Set up stuff (i.e. lock the vertex buffer) to prepare for drawing text
  Begin();

  // Set the common color for all the vertices to follow
  D3DDevice::SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor );

  FLOAT fEllipsesPixelWidth = m_fXScaleFactor * 3.0f * (m_Glyphs[m_TranslatorTable[L'.']].wOffset + m_Glyphs[m_TranslatorTable[L'.']].wAdvance);

  if ( dwFlags & XBFONT_TRUNCATED )
  {
    // Check if we will really need to truncate the CStdString
    if ( fMaxPixelWidth <= 0.0f )
    {
      dwFlags &= (~XBFONT_TRUNCATED);
    }
    else
    {
      FLOAT w, h;
      GetTextExtent( strText, cchText, &w, &h, TRUE );

      // If not, then clear the flag
      if ( w <= fMaxPixelWidth )
        dwFlags &= (~XBFONT_TRUNCATED);
    }
  }

  // If vertically centered, offset the starting m_fCursorY value
  if ( dwFlags & XBFONT_CENTER_Y )
  {
    FLOAT w, h;
    GetTextExtent( strText, &w, &h );
    fOriginX += angle.sine * h * 0.5f;
    fOriginY -= angle.cosine * h * 0.5f;
  }

  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;
  FLOAT fAlignedOriginX;
  int numLines = 0;

  while ( cchText > 0 )
  {
    cchText--;
    // If starting text on a new line, determine justification effects
    if ( bStartingNewLine )
    {
      m_fCursorX = fOriginX - angle.sine * m_fFontYAdvance * numLines;
      m_fCursorY = fOriginY + angle.cosine * m_fFontYAdvance * numLines;
      if ( dwFlags & (XBFONT_RIGHT | XBFONT_CENTER_X) )
      {
        // Get the extent of this line
        FLOAT w, h;
        if ( dwFlags & XBFONT_TRUNCATED )
          w = fMaxPixelWidth;
        else
          GetTextExtent( strText, &w, &h, TRUE );

        if ( dwFlags & XBFONT_CENTER_X)
          w *= 0.5;

        // Offset this line's starting m_fCursorX value
        m_fCursorX -= angle.cosine * w;
        m_fCursorY -= angle.sine * w;
      }
      bStartingNewLine = FALSE;
      fAlignedOriginX = m_fCursorX;
      // align to an integer so that aliasing doesn't occur
      m_fCursorX = floorf(m_fCursorX + 0.5f);
      m_fCursorY = floorf(m_fCursorY + 0.5f);
    }

    // Get the current letter in the CStdString
    WCHAR letter = *strText++;

    // Skip '\r'
    if ( letter == L'\r')
      continue;

    // Handle the newline character
    if ( letter == L'\n' )
    {
      numLines++;
      bStartingNewLine = TRUE;
      continue;
    }

    // Translate unprintable characters
    GLYPH_ATTR* pGlyph = &m_Glyphs[ GetGlyphToUse(letter) ];

    FLOAT fOffset = m_fXScaleFactor * (FLOAT)pGlyph->wOffset;
    FLOAT fAdvance = m_fXScaleFactor * (FLOAT)pGlyph->wAdvance;
    FLOAT fWidth = m_fXScaleFactor * (FLOAT)pGlyph->wWidth;
    FLOAT fHeight = m_fYScaleFactor * m_fFontHeight;

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( m_fCursorX + fOffset + fWidth + fEllipsesPixelWidth + m_fSlantFactor > fAlignedOriginX + fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then go to the next line
        DrawText( m_fCursorX, m_fCursorY, angle, dwColor, L"..." );
        while (cchText > 0)
        {
          cchText--;
          WCHAR letter = *strText++;
          // Handle the newline character
          if ( letter == L'\n' )
          {
            numLines++;
            bStartingNewLine = TRUE;
            continue;
          }
        }
        if (bStartingNewLine)
          continue;
        End();
        return S_OK;
      }
    }

    // Setup the screen coordinates
    m_fCursorX += fOffset * angle.cosine;
    m_fCursorY += fOffset * angle.sine;

    FLOAT left1 = m_fCursorX + m_fSlantFactor;
    FLOAT left2 = m_fCursorX - angle.sine * fHeight;
    FLOAT right1 = left1 + angle.cosine * fWidth;
    FLOAT right2 = left2 + angle.cosine * fWidth;
    FLOAT top1 = m_fCursorY;
    FLOAT top2 = top1 + angle.sine * fWidth;
    FLOAT bottom1 = top1 + angle.cosine * fHeight;
    FLOAT bottom2 = top2 + angle.cosine * fHeight;

    m_fCursorX += fAdvance * angle.cosine;
    m_fCursorY += fAdvance * angle.sine;

    D3DDevice::SetVertexData4f( 0, left1, top1, pGlyph->tu1, pGlyph->tv1 );
    D3DDevice::SetVertexData4f( 0, right1, top2, pGlyph->tu2, pGlyph->tv1 );
    D3DDevice::SetVertexData4f( 0, right2, bottom2, pGlyph->tu2, pGlyph->tv2 );
    D3DDevice::SetVertexData4f( 0, left2, bottom1, pGlyph->tu1, pGlyph->tv2 );
  }

  // Call End() to complete the begin/end pair for drawing text
  End();

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: DrawColourText()
// Desc: Draws text as textured multi-colored polygons
//-----------------------------------------------------------------------------
HRESULT CXBFont::DrawColourText( FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette,
                                 const WCHAR* strText, BYTE* pbColours, DWORD cchText,
                                 DWORD dwFlags, FLOAT fMaxPixelWidth )
{
  // Set up stuff (i.e. lock the vertex buffer) to prepare for drawing text
  Begin();

  FLOAT fEllipsesPixelWidth = m_fXScaleFactor * 3.0f * (m_Glyphs[m_TranslatorTable[L'.']].wOffset + m_Glyphs[m_TranslatorTable[L'.']].wAdvance);

  if ( dwFlags & XBFONT_TRUNCATED )
  {
    // Check if we will really need to truncate the CStdString
    if ( fMaxPixelWidth <= 0.0f )
    {
      dwFlags &= (~XBFONT_TRUNCATED);
    }
    else
    {
      FLOAT w, h;
      GetTextExtent( strText, cchText, &w, &h, TRUE );

      // If not, then clear the flag
      if ( w <= fMaxPixelWidth )
        dwFlags &= (~XBFONT_TRUNCATED);
    }
  }

  // If vertically centered, offset the starting m_fCursorY value
  if ( dwFlags & XBFONT_CENTER_Y )
  {
    FLOAT w, h;
    GetTextExtent( strText, &w, &h );
    fOriginX += angle.sine * h * 0.5f;
    fOriginY -= angle.cosine * h * 0.5f;
  }

  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;
  int numLines = 0;

  while ( cchText-- )
  {
    // If starting text on a new line, determine justification effects
    if ( bStartingNewLine )
    {
      m_fCursorX = fOriginX - angle.sine * m_fFontYAdvance * numLines;
      m_fCursorY = fOriginY + angle.cosine * m_fFontYAdvance * numLines;
      if ( dwFlags & (XBFONT_RIGHT | XBFONT_CENTER_X) )
      {
        // Get the extent of this line
        FLOAT w, h;
        GetTextExtent( strText, &w, &h, TRUE );

        if (dwFlags & XBFONT_CENTER_X)
          w *= 0.5;

        m_fCursorX -= angle.cosine * w;
        m_fCursorY -= angle.sine * w;
      }
      bStartingNewLine = FALSE;
      // align to an integer so that aliasing doesn't occur
      m_fCursorX = floorf(m_fCursorX + 0.5f);
      m_fCursorY = floorf(m_fCursorY + 0.5f);
    }

    // Get the current letter in the CStdString
    WCHAR letter = *strText++;

    DWORD dwColor;
    if (pbColours)  // multi colour version
      dwColor = pdw256ColorPalette[*pbColours++];
    else            // single colour version
      dwColor = *pdw256ColorPalette;

    // Set the color for all the current letter vertices to follow
    D3DDevice::SetVertexDataColor( D3DVSDE_DIFFUSE, dwColor );

    // Handle the newline character
    if ( letter == L'\n' )
    {
      numLines++;
      bStartingNewLine = TRUE;
      continue;
    }

    // Translate unprintable characters
    GLYPH_ATTR* pGlyph = &m_Glyphs[ GetGlyphToUse(letter) ];

    FLOAT fOffset = m_fXScaleFactor * (FLOAT)pGlyph->wOffset;
    FLOAT fAdvance = m_fXScaleFactor * (FLOAT)pGlyph->wAdvance;
    FLOAT fWidth = m_fXScaleFactor * (FLOAT)pGlyph->wWidth;
    FLOAT fHeight = m_fYScaleFactor * m_fFontHeight;

    if ( dwFlags & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( m_fCursorX + fOffset + fWidth + fEllipsesPixelWidth + m_fSlantFactor > fOriginX + fMaxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        DrawText( m_fCursorX, m_fCursorY, angle, dwColor, L"..." );
        End();
        return S_OK;
      }
    }

    // Setup the screen coordinates
    m_fCursorX += fOffset * angle.cosine;
    m_fCursorY += fOffset * angle.sine;

    FLOAT left1 = m_fCursorX + m_fSlantFactor;
    FLOAT left2 = m_fCursorX - angle.sine * fHeight;
    FLOAT right1 = left1 + angle.cosine * fWidth;
    FLOAT right2 = left2 + angle.cosine * fWidth;
    FLOAT top1 = m_fCursorY;
    FLOAT top2 = top1 + angle.sine * fWidth;
    FLOAT bottom1 = top1 + angle.cosine * fHeight;
    FLOAT bottom2 = top2 + angle.cosine * fHeight;

    m_fCursorX += fAdvance * angle.cosine;
    m_fCursorY += fAdvance * angle.sine;

    D3DDevice::SetVertexData4f( 0, left1, top1, pGlyph->tu1, pGlyph->tv1 );
    D3DDevice::SetVertexData4f( 0, right1, top2, pGlyph->tu2, pGlyph->tv1 );
    D3DDevice::SetVertexData4f( 0, right2, bottom2, pGlyph->tu2, pGlyph->tv2 );
    D3DDevice::SetVertexData4f( 0, left2, bottom1, pGlyph->tu1, pGlyph->tv2 );
  }

  // Call End() to complete the begin/end pair for drawing text
  End();

  return S_OK;
}



//-----------------------------------------------------------------------------
// Name: End()
// Desc: Called after Begin(), this function triggers the rendering of the
//       vertex buffer contents filled during calls to DrawText().
//-----------------------------------------------------------------------------
HRESULT CXBFont::End()
{
  // Keep track of nested calls to begin/end.
  if ( 0L == m_dwNestedBeginCount )
    return E_FAIL;
  if ( --m_dwNestedBeginCount > 0 )
    return S_FALSE;

  // End the drawing of vertex data
  D3DDevice::End();
  D3DDevice::SetScreenSpaceOffset( 0, 0 );

  // Restore state
  if ( m_bSaveState )
  {
    D3DDevice::SetTexture( 0, NULL );
    D3DDevice::SetPixelShader( NULL );
    D3DDevice::SetRenderState( D3DRS_ALPHABLENDENABLE, m_dwSavedState[0] );
    D3DDevice::SetRenderState( D3DRS_SRCBLEND, m_dwSavedState[1] );
    D3DDevice::SetRenderState( D3DRS_DESTBLEND, m_dwSavedState[2] );
    D3DDevice::SetRenderState( D3DRS_YUVENABLE, FALSE);
    D3DDevice::SetRenderState( D3DRS_ALPHATESTENABLE, m_dwSavedState[3] );
    D3DDevice::SetRenderState( D3DRS_ALPHAREF, m_dwSavedState[4] );
    D3DDevice::SetRenderState( D3DRS_ALPHAFUNC, m_dwSavedState[5] );
    D3DDevice::SetRenderState( D3DRS_FILLMODE, m_dwSavedState[6] );
    D3DDevice::SetRenderState( D3DRS_CULLMODE, m_dwSavedState[7] );
    D3DDevice::SetRenderState( D3DRS_ZENABLE, m_dwSavedState[8] );
    D3DDevice::SetRenderState( D3DRS_STENCILENABLE, m_dwSavedState[9] );
    D3DDevice::SetRenderState( D3DRS_EDGEANTIALIAS, m_dwSavedState[10] );
    D3DDevice::SetTextureStageState( 0, D3DTSS_MINFILTER, m_dwSavedState[11] );
    D3DDevice::SetTextureStageState( 0, D3DTSS_MAGFILTER, m_dwSavedState[12] );
  }

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderToTexture()
// Desc: Creates a texture and renders a text CStdString into it. The texture
//       format defaults to a 32-bit linear texture
//-----------------------------------------------------------------------------
LPDIRECT3DTEXTURE8 CXBFont::CreateTexture( const WCHAR* strText,
    D3DCOLOR dwBackgroundColor,
    D3DCOLOR dwTextColor,
    D3DFORMAT d3dFormat )
{
  // Calculate texture dimensions
  FLOAT fTexWidth;
  FLOAT fTexHeight;
  GetTextExtent( strText, &fTexWidth, &fTexHeight );
  DWORD dwWidth = (DWORD)fTexWidth;
  DWORD dwHeight = (DWORD)fTexHeight;

  switch ( d3dFormat )
  {
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
  case D3DFMT_R5G6B5:
  case D3DFMT_X1R5G5B5:
    // For swizzled textures, make sure the dimensions are a power of two
    for ( DWORD wmask = 1; dwWidth&(dwWidth - 1); wmask = (wmask << 1) + 1 )
      dwWidth = ( dwWidth + wmask ) & ~wmask;
    for ( DWORD hmask = 1; dwHeight&(dwHeight - 1); hmask = (hmask << 1) + 1 )
      dwHeight = ( dwHeight + hmask ) & ~hmask;

    // Also, enforce a minimum pitch of 64 bytes
    dwWidth = max( 64 / XGBytesPerPixelFromFormat(d3dFormat), dwWidth );

    break;

  case D3DFMT_LIN_A8R8G8B8:
  case D3DFMT_LIN_X8R8G8B8:
  case D3DFMT_LIN_R5G6B5:
  case D3DFMT_LIN_X1R5G5B5:
    // For linear textures, make sure the stride is a multiple of 64 bytes
    dwWidth = ( dwWidth + 0x1f ) & ~0x1f;
    break;

  default:
    // All other formats are unsupported as render targets
    return NULL;
  }

  // Create the texture
  LPDIRECT3DTEXTURE8 pTexture;
  if ( FAILED( D3DDevice::CreateTexture( dwWidth, dwHeight, 1, 0L, d3dFormat,
                                         D3DPOOL_DEFAULT, &pTexture ) ) )
    return NULL;

  // Get the current backbuffer and zbuffer
  LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
  D3DDevice::GetRenderTarget( &pBackBuffer );
  D3DDevice::GetDepthStencilSurface( &pZBuffer );

  // Set the new texture as the render target
  LPDIRECT3DSURFACE8 pTextureSurface;
  pTexture->GetSurfaceLevel( 0, &pTextureSurface );
  D3DVIEWPORT8 vp = { 0, 0, dwWidth, dwHeight, 0.0f, 1.0f };
  D3DDevice::SetRenderTarget( pTextureSurface, NULL );
  D3DDevice::SetViewport( &vp );
  D3DDevice::Clear( 0L, NULL, D3DCLEAR_TARGET, dwBackgroundColor, 1.0f, 0L );

  // Render the text
  CXBFont::DrawText( 0, 0, CAngle(), dwTextColor, strText, 0L );

  // Restore the render target
  D3DVIEWPORT8 vpBackBuffer = { 0, 0, 640, 480, 0.0f, 1.0f };
  D3DDevice::SetRenderTarget( pBackBuffer, pZBuffer );
  D3DDevice::SetViewport( &vpBackBuffer );
  SAFE_RELEASE( pBackBuffer );
  SAFE_RELEASE( pZBuffer );
  SAFE_RELEASE( pTextureSurface );

  // Return the new texture
  return pTexture;
}


// ITitleFontRenderer support

//-----------------------------------------------------------------------------
// Name: ITitleFontRenderer::SetHeight
// Desc: Set the font height
//-----------------------------------------------------------------------------
STDMETHODIMP CXBFont::SetHeight( DWORD Height )
{
  // Since we are using a fixed font, this is ignored
  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ITitleFontRenderer::SetColor
// Desc: Set the current text color
//-----------------------------------------------------------------------------
STDMETHODIMP CXBFont::SetColor( D3DCOLOR Color )
{
  m_dwCurrentColor = (DWORD) Color;
  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ITitleFontRenderer::DrawText
// Desc: Render a chunk of text
//-----------------------------------------------------------------------------
STDMETHODIMP CXBFont::DrawText(
  IDirect3DSurface8* pSurface,
  LPCWSTR pText,
  DWORD CharCount,
  DWORD X,
  DWORD Y)
{
  if ( CharCount == 0 ) return S_OK;
  if ( CharCount == (DWORD) - 1) CharCount = wcslen( pText );
  // Save current render target and depth buffer
  LPDIRECT3DSURFACE8 pRenderTarget, pZBuffer;
  D3DDevice::GetRenderTarget( &pRenderTarget );
  D3DDevice::GetDepthStencilSurface( &pZBuffer );

  D3DDevice::SetRenderTarget( pSurface, NULL );
  HRESULT hr = DrawTextEx( (FLOAT) X, (FLOAT) Y, CAngle(), m_dwCurrentColor,
                           pText, CharCount, 0, 0.0 );
  // Restore render target and zbuffer
  D3DDevice::SetRenderTarget( pRenderTarget, pZBuffer );
  if ( pRenderTarget ) pRenderTarget->Release();
  if ( pZBuffer ) pZBuffer->Release();
  return hr;
}




//-----------------------------------------------------------------------------
// Name: ITitleFontRenderer::GetTextSize
// Desc: Calculate the size of text
//-----------------------------------------------------------------------------
STDMETHODIMP CXBFont::GetTextSize(
  LPCWSTR pText,
  DWORD CharCount,
  DWORD* pWidth,
  DWORD* pHeight)
{
  FLOAT fWidth;
  FLOAT fHeight;
  if ( CharCount == (DWORD) - 1) CharCount = wcslen( pText );
  HRESULT hr = GetTextExtent( pText, CharCount, &fWidth, &fHeight );
  *pWidth = (DWORD) fWidth;
  *pHeight = (DWORD) fHeight;
  return hr;
}

inline WCHAR CXBFont::GetGlyphToUse(WCHAR letter) const
{
  if (letter > m_cMaxGlyph || m_TranslatorTable[letter] == 0)
  { // remap our glyph
    letter = CGUIFont::RemapGlyph(letter);
    if (letter > m_cMaxGlyph)
      return 0;
  }
  return m_TranslatorTable[letter];
}



#endif
