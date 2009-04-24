#pragma once

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

class CGUIString
{
public:
  typedef std::vector<DWORD>::const_iterator iString;

  CGUIString(iString start, iString end, bool carriageReturn);

  CStdString GetAsString() const;

  std::vector<DWORD> m_text;
  bool m_carriageReturn; // true if we have a carriage return here
};

class CGUITextLayout
{
public:
  CGUITextLayout(CGUIFont *font, bool wrap, float fHeight=0.0f);  // this may need changing - we may just use this class to replace CLabelInfo completely

  // main function to render strings
  void Render(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, bool solid = false);
  void RenderScrolling(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, CScrollInfo &scrollInfo);
  void RenderOutline(float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, DWORD alignment, float maxWidth);
  void GetTextExtent(float &width, float &height);
  float GetTextWidth();
  float GetTextWidth(const CStdStringW &text) const;
  bool Update(const CStdString &text, float maxWidth = 0, bool forceLTRReadingOrder = false);
  void SetText(const CStdStringW &text, float maxWidth = 0, bool forceLTRReadingOrder = false);

  unsigned int GetTextLength() const;
  void GetFirstText(std::vector<DWORD> &text) const;
  void Reset();

  void SetWrap(bool bWrap=true);
  void SetMaxHeight(float fHeight);


  static void DrawText(CGUIFont *font, float x, float y, DWORD color, DWORD shadowColor, const CStdString &text, DWORD align);
  static void DrawOutlineText(CGUIFont *font, float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, const CStdString &text);
  static void Filter(CStdString &text);

protected:
  void ParseText(const CStdStringW &text, std::vector<DWORD> &parsedText);
  void LineBreakText(const std::vector<DWORD> &text, std::vector<CGUIString> &lines);
  void WrapText(const std::vector<DWORD> &text, float maxWidth);
  void BidiTransform(std::vector<CGUIString> &lines, bool forceLTRReadingOrder);
  CStdStringW BidiFlip(const CStdStringW &text, bool forceLTRReadingOrder);

  // our text to render
  std::vector<DWORD> m_colors;
  std::vector<CGUIString> m_lines;
  typedef std::vector<CGUIString>::iterator iLine;

  // the layout and font details
  CGUIFont *m_font;        // has style, colour info
  bool  m_wrap;            // wrapping (true if justify is enabled!)
  float m_maxHeight;
  // the default color (may differ from the font objects defaults)
  DWORD m_textColor;

  CStdString m_lastText;
private:
  inline bool IsSpace(DWORD letter) const XBMC_FORCE_INLINE
  {
    return (letter & 0xffff) == L' ';
  };
  inline bool CanWrapAtLetter(DWORD letter) const XBMC_FORCE_INLINE
  {
    DWORD ch = letter & 0xffff;
    return ch == L' ' || (ch >=0x4e00 && ch <= 0x9fff);
  };
  static void AppendToUTF32(const CStdString &utf8, DWORD colStyle, std::vector<DWORD> &utf32);
  static void AppendToUTF32(const CStdStringW &utf16, DWORD colStyle, std::vector<DWORD> &utf32);
  static void DrawOutlineText(CGUIFont *font, float x, float y, const std::vector<DWORD> &colors, DWORD outlineColor, DWORD outlineWidth, const std::vector<DWORD> &text, DWORD align, float maxWidth);
  static void ParseText(const CStdStringW &text, DWORD defaultStyle, std::vector<DWORD> &colors, std::vector<DWORD> &parsedText);

  static void utf8ToW(const CStdString &utf8, CStdStringW &utf16);
};

