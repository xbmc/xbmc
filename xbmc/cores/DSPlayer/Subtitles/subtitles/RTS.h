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

#include "STS.h"
#include "Rasterizer.h"
#include "..\SubPic\ISubPic.h"

class CMyFont
{
public:
  int m_ascent, m_descent;
  HFONT hFont;

  CMyFont(STSStyle& style);
  ~CMyFont();
};

class CPolygon;

class CWord : public Rasterizer
{
  bool m_fDrawn;
  Com::SmartPoint m_p;

  void Transform(Com::SmartPoint org);

  void Transform_C( Com::SmartPoint &org );
  void Transform_SSE2( Com::SmartPoint &org );

  bool CreateOpaqueBox();

protected:
  CStdStringW m_str;

  virtual bool CreatePath() = 0;

public:
  bool m_fWhiteSpaceChar, m_fLineBreak;

  STSStyle m_style;

  CPolygon* m_pOpaqueBox;

  int m_ktype, m_kstart, m_kend;

  int m_width, m_ascent, m_descent;

  CWord(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend); // str[0] = 0 -> m_fLineBreak = true (in this case we only need and use the height of m_font from the whole class)
  virtual ~CWord();

  virtual CWord* Copy() = 0;
  virtual bool Append(CWord* w);

  void Paint(Com::SmartPoint p, Com::SmartPoint org);
};

class CText : public CWord
{
protected:
  virtual bool CreatePath();

public:
  CText(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend);

  virtual CWord* Copy();
  virtual bool Append(CWord* w);
};

class CPolygon : public CWord
{
  bool GetLONG(CStdStringW& str, LONG& ret);
  bool GetPOINT(CStdStringW& str, POINT& ret);
  bool ParseStr();

protected:
  double m_scalex, m_scaley;
  int m_baseline;

  std::vector<BYTE> m_pathTypesOrg;
  std::vector<Com::SmartPoint> m_pathPointsOrg;

  virtual bool CreatePath();

public:
  CPolygon(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend, double scalex, double scaley, int baseline);
  virtual ~CPolygon();

  virtual CWord* Copy();
  virtual bool Append(CWord* w);
};

class CClipper : public CPolygon
{
private:
  CWord* Copy();
  virtual bool Append(CWord* w);

public:
  CClipper(CStdStringW str, Com::SmartSize size, double scalex, double scaley, bool inverse);
  virtual ~CClipper();

  Com::SmartSize m_size;
  bool m_inverse;
  BYTE* m_pAlphaMask;
};

class CLine : public std::list<CWord*>
{
public:
  int m_width, m_ascent, m_descent, m_borderX, m_borderY;

  virtual ~CLine();

  void Compact();

#ifdef _VSMOD // patch m006. moveable vector clip
  Com::SmartRect PaintShadow(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt);
  Com::SmartRect PaintOutline(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt);
  Com::SmartRect PaintBody(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt);
#else
  Com::SmartRect PaintShadow(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha);
  Com::SmartRect PaintOutline(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha);
  Com::SmartRect PaintBody(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha);
#endif
};

enum eftype
{
  EF_MOVE = 0,  // {\move(x1=param[0], y1=param[1], x2=param[2], y2=param[3], t1=t[0], t2=t[1])} or {\pos(x=param[0], y=param[1])}
  EF_ORG,      // {\org(x=param[0], y=param[1])}
  EF_FADE,    // {\fade(a1=param[0], a2=param[1], a3=param[2], t1=t[0], t2=t[1], t3=t[2], t4=t[3])} or {\fad(t1=t[1], t2=t[2])
  EF_BANNER,    // Banner;delay=param[0][;lefttoright=param[1];fadeawaywidth=param[2]]
  EF_SCROLL,    // Scroll up/down=param[3];top=param[0];bottom=param[1];delay=param[2][;fadeawayheight=param[4]]
#ifdef _VSMOD // patch m006. moveable vector clip
  EF_VECTCLP
#endif
};

#ifdef _VSMOD // patch m006. moveable vector clip
  #define EF_NUMBEROFEFFECTS 6
#else
  #define EF_NUMBEROFEFFECTS 5
#endif

class Effect
{
public:
  enum eftype type;
  int param[9];
  int t[4];
};

class CSubtitle : public std::list<CLine*>
{
  int GetFullWidth();
  int GetFullLineWidth(std::list<CWord *>::iterator it);
  int GetWrapWidth(std::list<CWord *>::iterator it, int maxwidth);
  CLine* GetNextLine(std::list<CWord *>::iterator& it, int maxwidth);

public:
  int m_scrAlignment;
  int m_wrapStyle;
  bool m_fAnimated;
  int m_relativeTo;

  Effect* m_effects[EF_NUMBEROFEFFECTS];

  std::list<CWord*> m_words;

  CClipper* m_pClipper;

  Com::SmartRect m_rect, m_clip;
  int m_topborder, m_bottomborder;
  bool m_clipInverse;

  double m_scalex, m_scaley;

public:
  CSubtitle();
  virtual ~CSubtitle();
  virtual void Empty();

  void CreateClippers(Com::SmartSize size);

  void MakeLines(Com::SmartSize size, Com::SmartRect marginRect);
};

class CScreenLayoutAllocator
{
  typedef struct
  {
    Com::SmartRect r;
    int segment, entry, layer;
  } SubRect;

  std::list<SubRect> m_subrects;

public:
  virtual void Empty();

  void AdvanceToSegment(int segment, const std::vector<int>& sa);
  Com::SmartRect AllocRect(CSubtitle* s, int segment, int entry, int layer, int collisions);
};

class __declspec(uuid("537DCACA-2812-4a4f-B2C6-1A34C17ADEB0"))
CRenderedTextSubtitle : public CSimpleTextSubtitle, public ISubPicProviderImpl, public ISubStream
{
  std::map<int, CSubtitle*> m_subtitleCache;

  CScreenLayoutAllocator m_sla;

  Com::SmartSize m_size;
  Com::SmartRect m_vidrect;

  // temp variables, used when parsing the script
  int m_time, m_delay;
  int m_animStart, m_animEnd;
  double m_animAccel;
  int m_ktype, m_kstart, m_kend;
  int m_nPolygon;
  int m_polygonBaselineOffset;
  STSStyle *m_pStyleOverride; // the app can decide to use this style instead of a built-in one
  bool m_doOverrideStyle;

  void ParseEffect(CSubtitle* sub, CStdString str);
  void ParseString(CSubtitle* sub, CStdStringW str, STSStyle& style);
  void ParsePolygon(CSubtitle* sub, CStdStringW str, STSStyle& style);
  bool ParseSSATag(CSubtitle* sub, CStdStringW str, STSStyle& style, STSStyle& org, bool fAnimate = false);
  bool ParseHtmlTag(CSubtitle* sub, CStdStringW str, STSStyle& style, STSStyle& org);

  double CalcAnimation(double dst, double src, bool fAnimate);

  CSubtitle* GetSubtitle(int entry);

protected:
  virtual void OnChanged();

public:
  CRenderedTextSubtitle(CCritSec* pLock, STSStyle *styleOverride = NULL, bool doOverride = false);
  virtual ~CRenderedTextSubtitle();

  virtual void assign(CSimpleTextSubtitle& sts);
  virtual void Empty();
  // call to signal this RTS to ignore any of the styles and apply the given override style
  void SetOverride(bool doOverride = true, STSStyle *styleOverride = NULL) { m_doOverrideStyle = doOverride; if(styleOverride != NULL) m_pStyleOverride = styleOverride; }

public:
  bool Init(Com::SmartSize size, Com::SmartRect vidrect); // will call Deinit()
  void Deinit();

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // ISubPicProvider
  STDMETHODIMP_(int) GetStartPosition(REFERENCE_TIME rt, double fps);
  STDMETHODIMP_(int) GetNext(int pos);
  STDMETHODIMP_(REFERENCE_TIME) GetStart(int pos, double fps);
  STDMETHODIMP_(REFERENCE_TIME) GetStop(int pos, double fps);
  STDMETHODIMP_(bool) IsAnimated(int pos);
  STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

  // IPersist
  STDMETHODIMP GetClassID(CLSID* pClassID);

  // ISubStream
  STDMETHODIMP_(int) GetStreamCount();
  STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
  STDMETHODIMP_(int) GetStream();
  STDMETHODIMP SetStream(int iStream);
  STDMETHODIMP Reload();
};
