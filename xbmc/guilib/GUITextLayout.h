#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"

#include <vector>

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even without optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

class CGUIFont;
class CScrollInfo;

// Process will be:

// 1.  String is divided up into a "multiinfo" vector via the infomanager.
// 2.  The multiinfo vector is then parsed by the infomanager at rendertime and the resultant string is constructed.
// 3.  This is saved for comparison perhaps.  If the same, we are done.  If not, go to 4.
// 4.  The string is then parsed into a vector<CGUIString>.
// 5.  Each item in the vector is length-calculated, and then layout occurs governed by alignment and wrapping rules.
// 6.  A new vector<CGUIString> is constructed

typedef uint32_t character_t;
typedef uint32_t color_t;
typedef std::vector<character_t> vecText;
typedef std::vector<color_t> vecColors;

class CGUIString
{
public:
  typedef vecText::const_iterator iString;

  CGUIString(iString start, iString end, bool carriageReturn);

  CStdString GetAsString() const;

  vecText m_text;
  bool m_carriageReturn; // true if we have a carriage return here
};

class CGUITextLayout
{
public:
  CGUITextLayout(CGUIFont *font, bool wrap, float fHeight=0.0f, CGUIFont *borderFont = NULL);  // this may need changing - we may just use this class to replace CLabelInfo completely

  // main function to render strings
  void Render(float x, float y, float angle, color_t color, color_t shadowColor, uint32_t alignment, float maxWidth, bool solid = false);
  void RenderScrolling(float x, float y, float angle, color_t color, color_t shadowColor, uint32_t alignment, float maxWidth, CScrollInfo &scrollInfo);
  void RenderOutline(float x, float y, color_t color, color_t outlineColor, uint32_t alignment, float maxWidth);

  /*! \brief Returns the precalculated width and height of the text to be rendered (in constant time).
   \param width [out] width of text
   \param height [out] height of text
   \sa GetTextWidth, CalcTextExtent
   */
  void GetTextExtent(float &width, float &height) const;
  
  /*! \brief Returns the precalculated width of the text to be rendered (in constant time).
   \return width of text
   \sa GetTextExtent, CalcTextExtent
   */
  float GetTextWidth() const { return m_textWidth; };
  
  float GetTextWidth(const CStdStringW &text) const;
  bool Update(const CStdString &text, float maxWidth = 0, bool forceUpdate = false, bool forceLTRReadingOrder = false);
  bool UpdateW(const CStdStringW &text, float maxWidth = 0, bool forceUpdate = false, bool forceLTRReadingOrder = false);

  unsigned int GetTextLength() const;
  void GetFirstText(vecText &text) const;
  void Reset();

  void SetWrap(bool bWrap=true);
  void SetMaxHeight(float fHeight);


  static void DrawText(CGUIFont *font, float x, float y, color_t color, color_t shadowColor, const CStdString &text, uint32_t align);
  static void Filter(CStdString &text);

protected:
  void ParseText(const CStdStringW &text, vecText &parsedText);
  void LineBreakText(const vecText &text, std::vector<CGUIString> &lines);
  void WrapText(const vecText &text, float maxWidth);
  void BidiTransform(std::vector<CGUIString> &lines, bool forceLTRReadingOrder);
  CStdStringW BidiFlip(const CStdStringW &text, bool forceLTRReadingOrder);
  void CalcTextExtent();

  // our text to render
  vecColors m_colors;
  std::vector<CGUIString> m_lines;
  typedef std::vector<CGUIString>::iterator iLine;

  // the layout and font details
  CGUIFont *m_font;        // has style, colour info
  CGUIFont *m_borderFont;  // only used for outlined text

  bool  m_wrap;            // wrapping (true if justify is enabled!)
  float m_maxHeight;
  // the default color (may differ from the font objects defaults)
  color_t m_textColor;

  CStdStringW m_lastText;
  float m_textWidth;
  float m_textHeight;
private:
  inline bool IsSpace(character_t letter) const XBMC_FORCE_INLINE
  {
    return (letter & 0xffff) == L' ';
  };
  inline bool CanWrapAtLetter(character_t letter) const XBMC_FORCE_INLINE
  {
    character_t ch = letter & 0xffff;
    return ch == L' ' || (ch >=0x4e00 && ch <= 0x9fff);
  };
  static void AppendToUTF32(const CStdString &utf8, character_t colStyle, vecText &utf32);
  static void AppendToUTF32(const CStdStringW &utf16, character_t colStyle, vecText &utf32);
  static void ParseText(const CStdStringW &text, uint32_t defaultStyle, vecColors &colors, vecText &parsedText);

  static void utf8ToW(const CStdString &utf8, CStdStringW &utf16);
};

