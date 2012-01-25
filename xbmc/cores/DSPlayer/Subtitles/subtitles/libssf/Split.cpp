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
#include "Split.h"
#include "Exception.h"

namespace ssf
{
  Split::Split(LPCWSTR sep, CStdStringW str, size_t limit, SplitType type)
  {
    DoSplit(sep, str, limit, type);
  }

  Split::Split(WCHAR sep, CStdStringW str, size_t limit, SplitType type)
  {
    DoSplit(CStdStringW(&sep), str, limit, type);
  }

  void Split::DoSplit(LPCWSTR sep, CStdStringW str, size_t limit, SplitType type)
  {
    clear();

    if(size_t seplen = wcslen(sep))
    {
      for(int i = 0, j = 0, len = str.GetLength(); 
        i <= len && (limit == 0 || size() < limit); 
        i = j + (int)seplen)
      {
        j = str.Find(sep, i);
        if(j < 0) j = len;

        CStdStringW s = i < j ? str.Mid(i, j - i) : L"";

        switch(type)
        {
        case Min: s.Trim(); // fall through
        case Def: if(s.empty()) break; // else fall through
        case Max: push_back(s); break;
        }
      }
    }
  }

  int Split::GetAtInt(size_t i)
  {
    if(i >= size()) throw Exception(_T("Index out of bounds"));
    return _wtoi(at(i));
  }

  float Split::GetAtFloat(size_t i)
  {
    if(i >= size()) throw Exception(_T("Index out of bounds"));
    return (float)_wtof(at(i));
  }
}