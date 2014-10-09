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

#include "stdafx.h"
#include "File.h"

#ifndef iswcsym
#define iswcsym(c) (iswalnum(c) || (c) == '_')
#endif

namespace ssf
{
  File::File()
  {
  }

  File::~File()
  {
  }

  void File::Parse(InputStream& s, LPCWSTR predef)
  {
    Reference* pRef = CreateRootRef();

    SetPredefined(true);

    try {ParseDefs(WCharInputStream(predef), pRef);}
    catch(Exception& e) {ASSERT(0);}

    SetPredefined(false);

    ParseDefs(s, pRef);

    Commit();

    if(s.PeekChar() != Stream::EOS)
    {
      
    }
  }

  void File::ParseDefs(InputStream& s, Reference* pParentRef)
  {
    while(s.SkipWhiteSpace(L";") != '}' && s.PeekChar() != Stream::EOS)
    {
      NodePriority priority = PNormal;
      std::list<CStdStringW> types;
      CStdStringW name;

      int c = s.SkipWhiteSpace();

      if(c == '*') {s.GetChar(); priority = PLow;}
      else if(c == '!') {s.GetChar(); priority = PHigh;}

      ParseTypes(s, types);

      if(s.SkipWhiteSpace() == '#')
      {
        s.GetChar();
        ParseName(s, name);
      }

      if(types.empty())
      {
        if(name.empty()) s.ThrowError(_T("syntax error"));
        types.push_back(L"?");
      }

      Reference* pRef = pParentRef;

      while(types.size() > 1)
      {
        pRef = CreateRef(CreateDef(pRef, types.front()));
        types.pop_front();
      }

      Definition* pDef = NULL;

      if(!types.empty())
      {
        pDef = CreateDef(pRef, types.front(), name, priority);
        types.pop_front();
      }

      c = s.SkipWhiteSpace(L":=");

      if(c == '"' || c == '\'') ParseQuotedString(s, pDef);
      else if(iswdigit(c) || c == '+' || c == '-') ParseNumber(s, pDef);
      else if(pDef->IsType(L"@")) ParseBlock(s, pDef);
      else ParseRefs(s, pDef);
    }

    s.GetChar();
  }

  void File::ParseTypes(InputStream& s, std::list<CStdStringW>& types)
  {
    types.clear();

    CStdStringW str;

    for(int c = s.SkipWhiteSpace(); iswcsym(c) || c == '.' || c == '@'; c = s.PeekChar())
    {
      c = s.GetChar();

      if(c == '.') 
      {
        if(str.empty()) s.ThrowError(_T("'type' cannot be an empty string"));
        if(!iswcsym(s.PeekChar())) s.ThrowError(_T("unexpected dot after type '%s'"), CStdString(str));

        types.push_back(str);
        str.Empty();
      }
      else
      {
        if(str.empty() && iswdigit(c)) s.ThrowError(_T("'type' cannot start with a number"));
        if((!str.empty() || !types.empty()) && c == '@') s.ThrowError(_T("unexpected @ in 'type'"));

        str += (WCHAR)c;
      }
    }

    if(!str.empty())
    {
      types.push_back(str);
    }
  }

  void File::ParseName(InputStream& s, CStdStringW& name)
  {
    name.Empty();

    for(int c = s.SkipWhiteSpace(); iswcsym(c); c = s.PeekChar())
    {
      if(name.empty() && iswdigit(c)) s.ThrowError(_T("'name' cannot start with a number"));
      name += (WCHAR)s.GetChar();
    }
  }

  void File::ParseQuotedString(InputStream& s, Definition* pDef)
  {
    CStdStringW v;

    int quote = s.SkipWhiteSpace();
    if(quote != '"' && quote != '\'') s.ThrowError(_T("expected qouted string"));
    s.GetChar();

    for(int c = s.PeekChar(); c != Stream::EOS; c = s.PeekChar())
    {
      c = s.GetChar();
      if(c == quote) {pDef->SetAsValue(Definition::string, v); return;}
      if(c == '\n') s.ThrowError(_T("qouted string terminated unexpectedly by a new-line character"));
      if(c == '\\') c = s.GetChar();
      if(c == Stream::EOS) s.ThrowError(_T("qouted string terminated unexpectedly by EOS"));
      v += (WCHAR)c;
    }

    s.ThrowError(_T("unterminated quoted string"));
  }

  void File::ParseNumber(InputStream& s, Definition* pDef)
  {
    CStdStringW v, u;

    for(int c = s.SkipWhiteSpace(); iswxdigit(c) || wcschr(L"+-.x:", c); c = s.PeekChar())
    {
      if((c == '+' || c == '-') && !v.empty()
      || (c == '.' && (v.empty() || v.Find('.') >= 0 || v.Find('x') >= 0))
      || (c == 'x' && v != L"0")
      || (c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F') && v.Find(L"0x") != 0
      || (c == ':' && v.empty()))
      {
        s.ThrowError(_T("unexpected character '%c' in number"), (TCHAR)c);
      }

      v += (WCHAR)s.GetChar();
    }

    if(v.empty()) s.ThrowError(_T("invalid number"));

    for(int c = s.SkipWhiteSpace(); iswcsym(c); c = s.PeekChar())
    {
      u += (WCHAR)s.GetChar();
    }

    pDef->SetAsNumber(v, u);
  }

  void File::ParseBlock(InputStream& s, Definition* pDef)
  {
    CStdStringW v;

    int c = s.SkipWhiteSpace(L":=");
    if(c != '{') s.ThrowError(_T("expected '{'"));
    s.GetChar();

    int depth = 0;

    for(int c = s.PeekChar(); c != Stream::EOS; c = s.PeekChar())
    {
      c = s.GetChar();
      if(c == '}' && depth == 0) {pDef->SetAsValue(Definition::block, v); return;}
      if(c == '\\') {v += (WCHAR)c; c = s.GetChar();}
      else if(c == '{') depth++;
      else if(c == '}') depth--;
      if(c == Stream::EOS) s.ThrowError(_T("block terminated unexpectedly by EOS"));
      v += (WCHAR)c;
    }

    s.ThrowError(_T("unterminated block"));
  }

  void File::ParseRefs(InputStream& s, Definition* pParentDef, LPCWSTR term)
  {
    int c = s.SkipWhiteSpace();

    do
    {
      if(pParentDef->IsValue()) s.ThrowError(_T("cannot mix references with other values"));

      if(c == '{')
      {
        s.GetChar();
        ParseDefs(s, CreateRef(pParentDef));
      }
      else if(iswcsym(c))
      {
        CStdStringW str;
        ParseName(s, str);

        // TODO: allow spec references: parent.<type>, self.<type>, child.<type>

        Definition* pDef = GetDefByName(str);
        if(!pDef) s.ThrowError(_T("cannot find definition of '%s'"), CStdString(str));

        if(!pParentDef->IsVisible(pDef)) s.ThrowError(_T("cannot access '%s' from here"), CStdString(str));

        pParentDef->push_back(pDef);
      }
      else if(!wcschr(term, c) && c != Stream::EOS)
      {
        s.ThrowError(_T("unexpected character '%c'"), (TCHAR)c);
      }

      c = s.SkipWhiteSpace();
    }
    while(!wcschr(term, c) && c != Stream::EOS);
  }
}
