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

#include "StdAfx.h"
#include "usfsubtitles.h"
#include <msxml.h>
#include "..\DSUtil\smartptr.h"

#define DeclareNameAndValue(pNode, name, val) \
    _bstr_t foo; \
    pNode->get_nodeName((BSTR *)&foo); \
    CStdString name(foo); ::SysFreeString(foo); name.ToLower(); \
    _variant_t val; \
    pNode->get_nodeValue(&val); \

#define BeginEnumAttribs(pNode, pChild, name, value) \
  {Com::SmartPtr<IXMLDOMNamedNodeMap> pAttribs; \
  if(SUCCEEDED(pNode->get_attributes(&pAttribs)) && pAttribs != NULL) \
    { \
    IXMLDOMNodePtr pChild; \
        for(pAttribs->nextNode(&pChild); pChild; pChild = NULL, pAttribs->nextNode(&pChild)) \
        { \

#define EndEnumAttribs }}}

#define BeginEnumChildren(pNode, pChild) \
  {IXMLDOMNodePtr pChild, pNext; \
  for(pNode->get_firstChild(&pChild); pChild; pNext = NULL, pChild->get_nextSibling(&pNext), pChild = pNext) \
    { \

#define EndEnumChildren }}

static CStdStringW GetText(IXMLDOMNodePtr pNode)
{
  BSTR bstr;
  pNode->get_text(&bstr);

  return(CStdStringW(bstr));
}

static CStdStringW GetXML(IXMLDOMNodePtr pNode)
{
  BSTR bstr;
  pNode->get_xml(&bstr);
  CStdStringW str(bstr);
  str.Remove('\r');
  str.Replace('\n', ' ');
  for(int i = 0; (i = str.Find(L" ", i)) >= 0; )
  {
		for(++i; i < str.GetLength() && (str[i] == ' ');)
      str.Delete(i);
  }
  return(str);
}

static CStdStringW GetAttrib(CStdStringW attrib, IXMLDOMNodePtr pNode)
{
  CStdStringW ret;

  BeginEnumAttribs(pNode, pChild, name, val)
  {
    DeclareNameAndValue(pChild, name, val);

    if(CStdStringW(name) == attrib && val.vt == VT_BSTR) // TODO: prepare for other types
    {
      ret = val.bstrVal;
      break;
    }
  }
  EndEnumAttribs

  return(ret);
}

static int TimeToInt(CStdStringW str)
{
  std::list<CStdStringW> sl;
  size_t i = 0;
  std::vector<CStdStringW> tokens; str.Tokenize(L":.,", tokens);
  if (!tokens.empty())
  {
    for(CStdStringW token = tokens[i]; i < tokens.size(); token = tokens[++i])
      sl.push_front(token);
  }

  if(sl.size() > 4)
    return(-1);

  int time = 0;
  
  int mul[4] = {1,1000,60*1000,60*60*1000};
  std::list<CStdStringW>::iterator it = sl.begin();
  for(; it != sl.end(); ++it)
  {
    const WCHAR* s = *it;
    WCHAR* tmp = NULL;
    int t = wcstol(s, &tmp, 10);
    if(s >= tmp) return(-1);
    time += t * mul[i];
  }

  return(time);
}

static DWORD StringToDWORD(CStdStringW str)
{
  if(str.IsEmpty()) return(0);
  if(str[0] == '#') return((DWORD)wcstol(str, NULL, 16));
  else return((DWORD)wcstol(str, NULL, 10));  
}

static DWORD ColorToDWORD(CStdStringW str)
{
  if(str.IsEmpty()) return(0);

  DWORD ret = 0;

  if(str[0] == '#')
  {
    ret = (DWORD)wcstol(str.TrimLeft('#'), NULL, 16);
  }
  else
  {
    std::map<CStdString, DWORD>::const_iterator it = g_colors.find(CStdString(str));
    if (it == g_colors.end())
      ret = 0;
    else
      ret = it->second;
  }

  ret = ((ret&0xff)<<16)|(ret&0xff00ff00)|((ret>>16)&0xff);

  return(ret);
}

static int TranslateAlignment(CStdStringW alignment)
{
  return
    !alignment.CompareNoCase(L"BottomLeft") ? 1 :
    !alignment.CompareNoCase(L"BottomCenter") ? 2 :
    !alignment.CompareNoCase(L"BottomRight") ? 3 :
    !alignment.CompareNoCase(L"MiddleLeft") ? 4 :
    !alignment.CompareNoCase(L"MiddleCenter") ? 5 :
    !alignment.CompareNoCase(L"MiddleRight") ? 6 :
    !alignment.CompareNoCase(L"TopLeft") ? 7 :
    !alignment.CompareNoCase(L"TopCenter") ? 8 :
    !alignment.CompareNoCase(L"TopRight") ? 9 :
    2;
}

static int TranslateMargin(CStdStringW margin, int wndsize)
{
  int ret = 0;

  if(!margin.IsEmpty())
  {
    ret = wcstol(margin, NULL, 10);
    if(margin.Find('%') >= 0) ret = wndsize * ret / 100;
  }

  return(ret);
}

////////////////

CUSFSubtitles::CUSFSubtitles()
{
}

CUSFSubtitles::~CUSFSubtitles()
{
}

bool CUSFSubtitles::Read(LPCTSTR fn)
{
  VARIANT_BOOL vb;
  IXMLDOMDocumentPtr pDoc;
  if(FAILED(pDoc.CreateInstance(CLSID_DOMDocument))
  || FAILED(pDoc->put_async(VARIANT_FALSE))
  || FAILED(pDoc->load(_variant_t(fn), &vb)) || vb != VARIANT_TRUE)
    return(false);

  styles.clear();
  effects.clear();
  texts.clear();

  IXMLDOMNodePtr ppNode;
  pDoc.QueryInterface(__uuidof(IXMLDOMNode), ppNode.GetInterfacePtr() );
  if(!ParseUSFSubtitles(ppNode))
    return(false);

  std::list<boost::shared_ptr<style_t>>::iterator pos = styles.begin();
  while(pos != styles.end())
  {
    style_t* def = pos->get(); pos++;

    if(def->name.CompareNoCase(L"Default"))
      continue;

    std::list<boost::shared_ptr<style_t>>::iterator pos2 = styles.begin();
    while(pos2 != styles.end())
    {
      style_t* s = pos2->get(); pos2++;

      if(!s->name.CompareNoCase(L"Default"))
        continue;

      if(s->fontstyle.face.IsEmpty()) s->fontstyle.face = def->fontstyle.face;
      if(s->fontstyle.size.IsEmpty()) s->fontstyle.size = def->fontstyle.size;
      if(s->fontstyle.color[0].IsEmpty()) s->fontstyle.color[0] = def->fontstyle.color[0];
      if(s->fontstyle.color[1].IsEmpty()) s->fontstyle.color[1] = def->fontstyle.color[1];
      if(s->fontstyle.color[2].IsEmpty()) s->fontstyle.color[2] = def->fontstyle.color[2];
      if(s->fontstyle.color[3].IsEmpty()) s->fontstyle.color[3] = def->fontstyle.color[3];
      if(s->fontstyle.italic.IsEmpty()) s->fontstyle.italic = def->fontstyle.italic;
      if(s->fontstyle.weight.IsEmpty()) s->fontstyle.weight = def->fontstyle.weight;
      if(s->fontstyle.underline.IsEmpty()) s->fontstyle.underline = def->fontstyle.underline;
      if(s->fontstyle.alpha.IsEmpty()) s->fontstyle.alpha = def->fontstyle.alpha;
      if(s->fontstyle.outline.IsEmpty()) s->fontstyle.outline = def->fontstyle.outline;
      if(s->fontstyle.shadow.IsEmpty()) s->fontstyle.shadow = def->fontstyle.shadow;
      if(s->fontstyle.wrap.IsEmpty()) s->fontstyle.wrap = def->fontstyle.wrap;
      if(s->pal.alignment.IsEmpty()) s->pal.alignment = def->pal.alignment;
      if(s->pal.relativeto.IsEmpty()) s->pal.relativeto = def->pal.relativeto;
      if(s->pal.horizontal_margin.IsEmpty()) s->pal.horizontal_margin = def->pal.horizontal_margin;
      if(s->pal.vertical_margin.IsEmpty()) s->pal.vertical_margin = def->pal.vertical_margin;
      if(s->pal.rotate[0].IsEmpty()) s->pal.rotate[0] = def->pal.rotate[0];
      if(s->pal.rotate[1].IsEmpty()) s->pal.rotate[1] = def->pal.rotate[1];
      if(s->pal.rotate[2].IsEmpty()) s->pal.rotate[2] = def->pal.rotate[2];
    }

    break;
  }

  std::list<boost::shared_ptr<text_t>>::iterator pos3 = texts.begin();
  while(pos3 != texts.end())
  {
    text_t* t = pos3->get(); pos3++;
    if(t->style.IsEmpty()) t->style = L"Default";
  }

  return(true);
}

bool CUSFSubtitles::ConvertToSTS(CSimpleTextSubtitle& sts)
{
  sts.m_name = metadata.language.text;
  sts.m_encoding = CTextFile::UTF8;
  sts.m_dstScreenSize = Com::SmartSize(640, 480);
  sts.m_fScaledBAS = true;
  sts.m_defaultWrapStyle = 1;

  // TODO: map metadata.language.code to charset num (windows doesn't have such a function...)
  int charSet = DEFAULT_CHARSET; 

  std::list<boost::shared_ptr<style_t>>::iterator pos = styles.begin();
  while(pos != styles.end())
  {
    style_t* s = pos->get(); pos++;

    if(!s->name.CompareNoCase(L"Default") && !s->fontstyle.wrap.IsEmpty())
    {
      sts.m_defaultWrapStyle = 
        !s->fontstyle.wrap.CompareNoCase(L"no") ? 2 :
        !s->fontstyle.wrap.CompareNoCase(L"auto") ? 1 :
        1;
    }

    STSStyle* stss = DNew STSStyle;
    if(!stss) continue;

    if(!s->pal.horizontal_margin.IsEmpty())
      stss->marginRect.left = stss->marginRect.right = TranslateMargin(s->pal.horizontal_margin, sts.m_dstScreenSize.cx);
    if(!s->pal.vertical_margin.IsEmpty())
      stss->marginRect.top = stss->marginRect.bottom = TranslateMargin(s->pal.vertical_margin, sts.m_dstScreenSize.cy);

    stss->scrAlignment = TranslateAlignment(s->pal.alignment);

    if(!s->pal.relativeto.IsEmpty()) stss->relativeTo = 
      !s->pal.relativeto.CompareNoCase(L"window") ? 0 :
      !s->pal.relativeto.CompareNoCase(L"video") ? 1 :
      0;

    stss->borderStyle = 0;
    if(!s->fontstyle.outline.IsEmpty()) stss->outlineWidthX = stss->outlineWidthY = wcstol(s->fontstyle.outline, NULL, 10);
    if(!s->fontstyle.shadow.IsEmpty()) stss->shadowDepthX = stss->shadowDepthY = wcstol(s->fontstyle.shadow, NULL, 10);

    for(int i = 0; i < 4; i++)
    {
      DWORD color = ColorToDWORD(s->fontstyle.color[i]);
      int alpha = (BYTE)wcstol(s->fontstyle.alpha, NULL, 10);

      stss->colors[i] = color & 0xffffff;
      stss->alpha[i] = (BYTE)(color>>24);

      stss->alpha[i] = stss->alpha[i] + (255 - stss->alpha[i]) * min(max(alpha, 0), 100) / 100;
    }

    if(!s->fontstyle.face.IsEmpty()) stss->fontName = s->fontstyle.face;
    if(!s->fontstyle.size.IsEmpty()) stss->fontSize = wcstol(s->fontstyle.size, NULL, 10);
    if(!s->fontstyle.weight.IsEmpty()) stss->fontWeight = 
      !s->fontstyle.weight.CompareNoCase(L"normal") ? FW_NORMAL :
      !s->fontstyle.weight.CompareNoCase(L"bold") ? FW_BOLD :
      !s->fontstyle.weight.CompareNoCase(L"lighter") ? FW_LIGHT :
      !s->fontstyle.weight.CompareNoCase(L"bolder") ? FW_SEMIBOLD :
      wcstol(s->fontstyle.weight, NULL, 10);
    if(stss->fontWeight == 0) stss->fontWeight = FW_NORMAL;
    if(!s->fontstyle.italic.IsEmpty()) stss->fItalic = s->fontstyle.italic.CompareNoCase(L"yes") == 0;
    if(!s->fontstyle.underline.IsEmpty()) stss->fUnderline = s->fontstyle.underline.CompareNoCase(L"yes") == 0;
    if(!s->pal.rotate[0].IsEmpty()) stss->fontAngleZ = wcstol(s->pal.rotate[0], NULL, 10);
    if(!s->pal.rotate[1].IsEmpty()) stss->fontAngleX = wcstol(s->pal.rotate[1], NULL, 10);
    if(!s->pal.rotate[2].IsEmpty()) stss->fontAngleY = wcstol(s->pal.rotate[2], NULL, 10);

    stss->charSet = charSet;

    sts.AddStyle(WToT(s->name), stss);
  }

  std::list<boost::shared_ptr<text_t>>::iterator pos2 = texts.begin();
  while(pos2 != texts.end())
  {
    text_t* t = pos2->get(); pos2++;

    if(!t->pal.alignment.IsEmpty())
    {
      CStdStringW s;
      s.Format(L"{\\an%d}", TranslateAlignment(t->pal.alignment));
      t->str = s + t->str;
    }

    Com::SmartRect marginRect;
    marginRect.SetRectEmpty();

    if(!t->pal.horizontal_margin.IsEmpty())
      marginRect.left = marginRect.right = TranslateMargin(t->pal.horizontal_margin, sts.m_dstScreenSize.cx);
    if(!t->pal.vertical_margin.IsEmpty())
      marginRect.top = marginRect.bottom = TranslateMargin(t->pal.vertical_margin, sts.m_dstScreenSize.cy);

    WCHAR rtags[3][8] = {L"{\\rz%d}", L"{\\rx%d}", L"{\\ry%d}"};
    for(int i = 0; i < 3; i++)
    {
      if(int angle = wcstol(t->pal.rotate[i], NULL, 10))
      {
        CStdStringW str;
        str.Format(rtags[i], angle);
        t->str = str + t->str;
      }
    }

    if(t->style.CompareNoCase(L"Default") != 0)
    {
      std::list<boost::shared_ptr<style_t>>::iterator pos = styles.begin();
      while(pos != styles.end())
      {
        style_t* s = pos->get(); pos++;
        if(s->name == t->style && !s->fontstyle.wrap.IsEmpty())
        {
          int WrapStyle = 
            !s->fontstyle.wrap.CompareNoCase(L"no") ? 2 :
            !s->fontstyle.wrap.CompareNoCase(L"auto") ? 1 :
            1;

          if(WrapStyle != sts.m_defaultWrapStyle)
          {
            CStdStringW str;
                        str.Format(L"{\\q%d}", WrapStyle);
            t->str = str + t->str;
          }

          break;
        }
      }
    }

    // TODO: apply effects as {\t(..)} after usf's standard clearly defines them

    sts.Add(t->str, true, t->start, t->stop, WToT(t->style), _T(""), _T(""), marginRect);
  }

  return(true);
}

bool CUSFSubtitles::ParseUSFSubtitles(IXMLDOMNodePtr pNode)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"usfsubtitles")
  {
    // metadata

    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"metadata")
      {
        ParseMetadata(pChild, metadata);
      }
    }
    EndEnumChildren

    // styles

    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"styles")
      {
        BeginEnumChildren(pChild, pGrandChild) // :)
        {
          DeclareNameAndValue(pGrandChild, name, val);

          if(name == L"style")
          {
            boost::shared_ptr<style_t> s(DNew style_t);
            if(s.get())
            {
              ParseStyle(pGrandChild, s.get());
              styles.push_back(s);
            }
          }
        }
        EndEnumChildren
      }
    }
    EndEnumChildren

    // effects

    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"effects")
      {
        BeginEnumChildren(pChild, pGrandChild) // :)
        {
          DeclareNameAndValue(pGrandChild, name, val);

          if(name == L"effect")
          {
            boost::shared_ptr<effect_t> e(DNew effect_t);
            if(e.get())
            {
              ParseEffect(pGrandChild, e.get());
              effects.push_back(e);
            }
          }
        }
        EndEnumChildren
      }
    }
    EndEnumChildren

    // subtitles

    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"subtitles")
      {
        BeginEnumChildren(pChild, pGrandChild) // :)
        {
          DeclareNameAndValue(pGrandChild, name, val);

          if(name == L"subtitle")
          {
            CStdStringW sstart = GetAttrib(L"start", pGrandChild);
            CStdStringW sstop = GetAttrib(L"stop", pGrandChild);
            CStdStringW sduration = GetAttrib(L"duration", pGrandChild);
            if(sstart.IsEmpty() || (sstop.IsEmpty() && sduration.IsEmpty()))
              continue;

            int start = TimeToInt(sstart);
            int stop = !sstop.IsEmpty() ? TimeToInt(sstop) : (start + TimeToInt(sduration));

            ParseSubtitle(pGrandChild, start, stop);
          }
        }
        EndEnumChildren
      }
    }
    EndEnumChildren

    return(true);
  }

  BeginEnumChildren(pNode, pChild)
  {
        if(ParseUSFSubtitles(pChild))
    {
      return(true);
    }
  }
  EndEnumChildren

  return(false);
}

void CUSFSubtitles::ParseMetadata(IXMLDOMNodePtr pNode, metadata_t& m)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"title")
  {
    m.title = GetText(pNode);
  }
  else if(name == L"date")
  {
    m.date = GetText(pNode);
  }
  else if(name == L"comment")
  {
    m.comment = GetText(pNode);
  }
  else if(name == L"author")
  {
    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"name")
        m.author.name = GetText(pChild);
      else if(name == L"email")
        m.author.email = GetText(pChild);
      else if(name == L"url")
        m.author.url = GetText(pChild);
    }
    EndEnumChildren

    return;
  }
  else if(name == L"language")
  {
    m.language.text = GetText(pNode);
    m.language.code = GetAttrib(L"code", pNode);
  }
  else if(name == L"languageext")
  {
    m.languageext.text = GetText(pNode);
    m.languageext.code = GetAttrib(L"code", pNode);
  }

  BeginEnumChildren(pNode, pChild)
  {
    ParseMetadata(pChild, metadata);
  }
  EndEnumChildren
}

void CUSFSubtitles::ParseStyle(IXMLDOMNodePtr pNode, style_t* s)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"style")
  {
    s->name = GetAttrib(L"name", pNode);
  }
  else if(name == L"fontstyle")
  {
    ParseFontstyle(pNode, s->fontstyle);
    return;
  }
  else if(name == L"position")
  {
    ParsePal(pNode, s->pal);
    return;
  }

  BeginEnumChildren(pNode, pChild)
  {
    ParseStyle(pChild, s);
  }
  EndEnumChildren
}

void CUSFSubtitles::ParseFontstyle(IXMLDOMNodePtr pNode, fontstyle_t& fs)
{
  fs.face = GetAttrib(L"face", pNode);
  fs.size = GetAttrib(L"size", pNode);
  fs.color[0] = GetAttrib(L"color", pNode);
  fs.color[1] = GetAttrib(L"back-color", pNode);
  fs.color[2] = GetAttrib(L"outline-color", pNode);
  fs.color[3] = GetAttrib(L"shadow-color", pNode);
  fs.italic = GetAttrib(L"italic", pNode);
  fs.weight = GetAttrib(L"weight", pNode);
  fs.underline = GetAttrib(L"underline", pNode);
  fs.alpha = GetAttrib(L"alpha", pNode);
  fs.outline = GetAttrib(L"outline-level", pNode);
  fs.shadow = GetAttrib(L"shadow-level", pNode);
  fs.wrap = GetAttrib(L"wrap", pNode);
}

void CUSFSubtitles::ParsePal(IXMLDOMNodePtr pNode, posattriblist_t& pal)
{
  pal.alignment = GetAttrib(L"alignment", pNode);
  pal.relativeto = GetAttrib(L"relative-to", pNode);
  pal.horizontal_margin = GetAttrib(L"horizontal-margin", pNode);
  pal.vertical_margin = GetAttrib(L"vertical-margin", pNode);
  pal.rotate[0] = GetAttrib(L"rotate-z", pNode);
  pal.rotate[1] = GetAttrib(L"rotate-x", pNode);
  pal.rotate[2] = GetAttrib(L"rotate-y", pNode);
}

void CUSFSubtitles::ParseEffect(IXMLDOMNodePtr pNode, effect_t* e)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"effect")
  {
    e->name = GetAttrib(L"name", pNode);
  }
  else if(name == L"keyframes")
  {
    BeginEnumChildren(pNode, pChild)
    {
      DeclareNameAndValue(pChild, name, val);

      if(name == L"keyframe")
      {
        boost::shared_ptr<keyframe_t> k(DNew keyframe_t);
        if(k.get())
        {
          ParseKeyframe(pChild, k.get());
          e->keyframes.push_back(k);
        }
      }
    }
    EndEnumChildren

    return;
  }

  BeginEnumChildren(pNode, pChild)
  {
    ParseEffect(pChild, e);
  }
  EndEnumChildren
}

void CUSFSubtitles::ParseKeyframe(IXMLDOMNodePtr pNode, keyframe_t* k)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"keyframe")
  {
    k->position = GetAttrib(L"position", pNode);
  }
  else if(name == L"fontstyle")
  {
    ParseFontstyle(pNode, k->fontstyle);
    return;
  }
  else if(name == L"position")
  {
    ParsePal(pNode, k->pal);
    return;
  }
}

void CUSFSubtitles::ParseSubtitle(IXMLDOMNodePtr pNode, int start, int stop)
{
  DeclareNameAndValue(pNode, name, val);

  if(name == L"text" || name == L"karaoke")
  {
    boost::shared_ptr<text_t> t(DNew text_t);
    if(t.get())
    {
      t->start = start;
      t->stop = stop;
      t->style = GetAttrib(L"style", pNode);
      t->effect = GetAttrib(L"effect", pNode);
      ParsePal(pNode, t->pal);
      ParseText(pNode, t->str);
      texts.push_back(t);
    }

    return;
  }
//  else if

  BeginEnumChildren(pNode, pChild)
  {
    ParseSubtitle(pChild, start, stop);
  }
  EndEnumChildren
}

void CUSFSubtitles::ParseText(IXMLDOMNodePtr pNode, CStdStringW& str)
{
  DeclareNameAndValue(pNode, name, val);

  CStdStringW prefix, postfix;

  if(name == L"b")
  {
    prefix = L"{\\b1}";
    postfix = L"{\\b}";
  }
  else if(name == L"i")
  {
    prefix = L"{\\i1}";
    postfix = L"{\\i}";
  }
  else if(name == L"u")
  {
    prefix = L"{\\u1}";
    postfix = L"{\\u}";
  }
  else if(name == L"font")
  {
    fontstyle_t fs;
    ParseFontstyle(pNode, fs);

    if(!fs.face.IsEmpty()) {prefix += L"{\\fn" + fs.face + L"}"; postfix += L"{\\fn}";}
    if(!fs.size.IsEmpty()) {prefix += L"{\\fs" + fs.size + L"}"; postfix += L"{\\fs}";}
    if(!fs.outline.IsEmpty()) {prefix += L"{\\bord" + fs.outline + L"}"; postfix += L"{\\bord}";}
    if(!fs.shadow.IsEmpty()) {prefix += L"{\\shad" + fs.shadow + L"}"; postfix += L"{\\shad}";}

    for(int i = 0; i < 4; i++)
    {
      if(!fs.color[i].IsEmpty())
      {
        CStdStringW s;
        s.Format(L"{\\%dc&H%06x&}", i+1, ColorToDWORD(fs.color[i]));
        prefix += s;
        s.Format(L"{\\%dc}", i+1);
        postfix += s;
      }
    }
  }
  else if(name == L"k")
  {
    int t = wcstol(GetAttrib(L"t", pNode), NULL, 10);
    CStdStringW s;
    s.Format(L"{\\kf%d}", t / 10);
    str += s;
    return;
  }
  else if(name == L"br")
  {
    str += L"\\N";
    return;
  }
  else if(name == L"#text")
  {
    str += GetXML(pNode);
    return;
  }

  BeginEnumChildren(pNode, pChild)
  {
    CStdStringW s;
    ParseText(pChild, s);
    str += s;
  }
  EndEnumChildren

  str = prefix + str + postfix;
}

void CUSFSubtitles::ParseShape(IXMLDOMNodePtr pNode)
{
  // no specs on this yet
}
