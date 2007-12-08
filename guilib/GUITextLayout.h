#pragma once

#include <vector>

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
  typedef std::vector<DWORD>::iterator iString;

  CGUIString(iString start, iString end, bool carriageReturn);

  CStdString GetAsString() const;

  std::vector<DWORD> m_text;
  bool m_carriageReturn; // true if we have a carriage return here
};

class CGUITextLayout
{
public:
  CGUITextLayout(CGUIFont *font, bool wrap, float fHeight=0.0);  // this may need changing - we may just use this class to replace CLabelInfo completely

  // main function to render strings
  void Render(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, bool solid = false);
  void RenderScrolling(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, CScrollInfo &scrollInfo);
  void RenderOutline(float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, DWORD alignment, float maxWidth);
  void GetTextExtent(float &width, float &height);
  bool Update(const CStdString &text, float maxWidth = 0);

  unsigned int GetTextLength() const;
  void GetFirstText(vector<DWORD> &text) const;
  void Reset();

  void SetWrap(bool bWrap=true);
  void SetMaxHeight(float fHeight);

  static void DrawText(CGUIFont *font, float x, float y, DWORD color, DWORD shadowColor, const CStdString &text, DWORD align);
  static void DrawOutlineText(CGUIFont *font, float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, const CStdString &text);
protected:
  void ParseText(const CStdString &text, vector<DWORD> &parsedText);
  void WrapText(vector<DWORD> &text, float maxWidth);
  void LineBreakText(vector<DWORD> &text);

  // our text to render
  vector<DWORD> m_colors;
  vector<CGUIString> m_lines;
  typedef vector<CGUIString>::iterator iLine;

  // the layout and font details
  CGUIFont *m_font;        // has style, colour info
  bool  m_wrap;            // wrapping (true if justify is enabled!)
  float m_maxHeight;
  // the default color (may differ from the font objects defaults)
  DWORD m_textColor;

  CStdString m_lastText;
private:
  static void AppendToUTF32(const CStdString &text, DWORD colStyle, vector<DWORD> &utf32);
  static void DrawOutlineText(CGUIFont *font, float x, float y, const vector<DWORD> &colors, DWORD outlineColor, DWORD outlineWidth, const vector<DWORD> &text, DWORD align, float maxWidth);
};

