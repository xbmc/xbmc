/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <wxutil.h>
#include "TextFile.h"
#include "GFN.h"

typedef enum {TIME, FRAME} tmode; // the meaning of STSEntry::start/end

class STSStyle 
{
public:
  Com::SmartRect  marginRect; // measured from the sides
  int    scrAlignment; // 1 - 9: as on the numpad, 0: default
  int    borderStyle; // 0: outline, 1: opaque box
  double  outlineWidthX, outlineWidthY;
  double  shadowDepthX, shadowDepthY;
  COLORREF colors[4]; // usually: {primary, secondary, outline/background, shadow}
  BYTE  alpha[4];
  int    charSet;
  CStdStringW fontName;
  double  fontSize; // height
  double  fontScaleX, fontScaleY; // percent
  double  fontSpacing; // +/- pixels
  int    fontWeight;
  bool  fItalic;
  bool  fUnderline;
  bool  fStrikeOut;
  int    fBlur;
  double  fGaussianBlur;
  double  fontAngleZ, fontAngleX, fontAngleY;
  double  fontShiftX, fontShiftY;
  int    relativeTo; // 0: window, 1: video, 2: undefined (~window)

  STSStyle();

  void SetDefault();

  bool operator == (STSStyle& s);
  bool IsFontStyleEqual(STSStyle& s);

  void operator = (LOGFONT& lf);

  friend LOGFONTA& operator <<= (LOGFONTA& lfa, STSStyle& s);
  friend LOGFONTW& operator <<= (LOGFONTW& lfw, STSStyle& s);

  friend CStdString& operator <<= (CStdString& style, STSStyle& s);
  friend STSStyle& operator <<= (STSStyle& s, CStdString& style);
};

class CSTSStyleMap : public std::map<CStdString, STSStyle*>
{
public:
  CSTSStyleMap() {}
  virtual ~CSTSStyleMap() {Free();}
  void Free();
};

typedef struct 
{
  CStdStringW str;
  bool fUnicode;
  CStdString style, actor, effect;
  Com::SmartRect marginRect;
  int layer;
  int start, end;
  int readorder;
} STSEntry;

class STSSegment
{
public:
  int start, end;
  std::vector<int> subs;

  STSSegment() {};
  STSSegment(int s, int e) {start = s; end = e;}
  STSSegment(const STSSegment& stss) {*this = stss;}
  void operator = (const STSSegment& stss) {start = stss.start; end = stss.end; subs.assign(stss.subs.begin(), stss.subs.end());}
};

class CSimpleTextSubtitle : public std::vector<STSEntry>
{
  friend class CSubtitleEditorDlg;

protected:
  std::vector<STSSegment> m_segments;
  virtual void OnChanged() {}

public:
  CStdString m_name;
  LCID m_lcid;
  exttype m_exttype;
  tmode m_mode;
  CTextFile::enc m_encoding;
  CStdString m_path;

  Com::SmartSize m_dstScreenSize;
  int m_defaultWrapStyle;
  int m_collisions;
  bool m_fScaledBAS;

  bool m_fUsingAutoGeneratedDefaultStyle;

  CSTSStyleMap m_styles;

  enum EPARCompensationType
  {
    EPCTDisabled = 0,
    EPCTDownscale = 1,
    EPCTUpscale = 2,
    EPCTAccurateSize = 3
  };
  
  EPARCompensationType m_ePARCompensationType;
  double m_dPARCompensation;

public:
  CSimpleTextSubtitle();
  virtual ~CSimpleTextSubtitle();

  virtual void assign(CSimpleTextSubtitle& sts);
  virtual void Empty();

  void Sort(bool fRestoreReadorder = false);
  void CreateSegments();

  void Append(CSimpleTextSubtitle& sts, int timeoff = -1);

  bool Open(CStdString fn, int CharSet, CStdString name = _T(""));
  bool Open(CTextFile* f, int CharSet, CStdString name); 
  bool Open(BYTE* data, int len, int CharSet, CStdString name); 
  bool SaveAs(CStdString fn, exttype et, double fps = -1, CTextFile::enc = CTextFile::ASCII);

  void Add(CStdStringW str, bool fUnicode, int start, int end, CStdString style = _T("Default"), CStdString actor = _T(""), CStdString effect = _T(""), Com::SmartRect marginRect = Com::SmartRect(0,0,0,0), int layer = 0, int readorder = -1);

  STSStyle* CreateDefaultStyle(int CharSet);
  void ChangeUnknownStylesToDefault();
  void AddStyle(CStdString name, STSStyle* style); // style will be stored and freed in Empty() later
  bool CopyStyles(const CSTSStyleMap& styles, bool fAppend = false);

  bool SetDefaultStyle(STSStyle& s);
  bool GetDefaultStyle(STSStyle& s);

  void ConvertToTimeBased(double fps);
  void ConvertToFrameBased(double fps);

  int TranslateStart(int i, double fps); 
  int TranslateEnd(int i, double fps);
  int SearchSub(int t, double fps);

  int TranslateSegmentStart(int i, double fps); 
  int TranslateSegmentEnd(int i, double fps);
  const STSSegment* SearchSubs(int t, double fps, /*[out]*/ int* iSegment = NULL, int* nSegments = NULL);
  const STSSegment* GetSegment(int iSegment) {return iSegment >= 0 && iSegment < (int)m_segments.size() ? &m_segments[iSegment] : NULL;}

  STSStyle* GetStyle(int i);
  bool GetStyle(int i, STSStyle& stss);
  int GetCharSet(int i);
  bool IsEntryUnicode(int i);
  void ConvertUnicode(int i, bool fUnicode);

  CStdStringA GetStrA(int i, bool fSSA = false);
  CStdStringW GetStrW(int i, bool fSSA = false);
  CStdStringW GetStrWA(int i, bool fSSA = false);

#ifdef UNICODE
#define GetStr GetStrW
#else
#define GetStr GetStrA
#endif

  void SetStr(int i, CStdStringA str, bool fUnicode /* ignored */);
  void SetStr(int i, CStdStringW str, bool fUnicode);
};

extern BYTE CharSetList[];
extern TCHAR* CharSetNames[];
extern int CharSetLen;

class CHtmlColorMap : public std::map<CStdString, DWORD> {public: CHtmlColorMap();};
extern CHtmlColorMap g_colors;



