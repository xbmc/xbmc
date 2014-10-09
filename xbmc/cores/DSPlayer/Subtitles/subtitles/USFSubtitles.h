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

// metadata
typedef struct {CStdStringW name, email, url;} author_t;
typedef struct {CStdStringW code, text;} language_t;
typedef struct {CStdStringW title, date, comment; author_t author; language_t language, languageext;} metadata_t;
// style
typedef struct {CStdStringW alignment, relativeto, horizontal_margin, vertical_margin, rotate[3];} posattriblist_t;
typedef struct {CStdStringW face, size, color[4], weight, italic, underline, alpha, outline, shadow, wrap;} fontstyle_t;
typedef struct {CStdStringW name; fontstyle_t fontstyle; posattriblist_t pal;} style_t;
// effect
typedef struct {CStdStringW position; fontstyle_t fontstyle; posattriblist_t pal;} keyframe_t;
typedef struct {CStdStringW name; std::list<boost::shared_ptr<keyframe_t>> keyframes;} effect_t;
// subtitle/text
typedef struct {int start, stop; CStdStringW effect, style, str; posattriblist_t pal;} text_t;

#ifndef _INC_COMDEFSP
_COM_SMARTPTR_TYPEDEF(IXMLDOMNode, IID_IXMLDOMNode);
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument, IID_IXMLDOMDocument);
#endif
class CUSFSubtitles
{
  bool ParseUSFSubtitles(IXMLDOMNodePtr pNode);
   void ParseMetadata(IXMLDOMNodePtr pNode, metadata_t& m);
   void ParseStyle(IXMLDOMNodePtr pNode, style_t* s);
    void ParseFontstyle(IXMLDOMNodePtr pNode, fontstyle_t& fs);
    void ParsePal(IXMLDOMNodePtr pNode, posattriblist_t& pal);
   void ParseEffect(IXMLDOMNodePtr pNode, effect_t* e);
    void ParseKeyframe(IXMLDOMNodePtr pNode, keyframe_t* k);
   void ParseSubtitle(IXMLDOMNodePtr pNode, int start, int stop);
    void ParseText(IXMLDOMNodePtr pNode, CStdStringW& assstr);
    void ParseShape(IXMLDOMNodePtr pNode);

public:
  CUSFSubtitles();
  virtual ~CUSFSubtitles();

  bool Read(LPCTSTR fn);
//  bool Write(LPCTSTR fn); // TODO

  metadata_t metadata;
  std::list<boost::shared_ptr<style_t>> styles;
  std::list<boost::shared_ptr<effect_t>> effects;
  std::list<boost::shared_ptr<text_t>> texts;

  bool ConvertToSTS(CSimpleTextSubtitle& sts);
//  bool ConvertFromSTS(CSimpleTextSubtitle& sts); // TODO
};
