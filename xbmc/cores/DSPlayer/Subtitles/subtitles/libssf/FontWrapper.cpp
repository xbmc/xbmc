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
#include "FontWrapper.h"

namespace ssf
{
  FontWrapper::FontWrapper(HDC hDC, HFONT hFont, const CStdStringW& key)
    : m_hFont(hFont)
    , m_key(key)
  {
    HFONT hFontOld = SelectFont(hDC, hFont);

    GetTextMetrics(hDC, &m_tm);

    if(DWORD nNumPairs = GetKerningPairs(hDC, 0, NULL))
    {
      KERNINGPAIR* kp = DNew KERNINGPAIR[nNumPairs];
      GetKerningPairs(hDC, nNumPairs, kp);
      for(DWORD i = 0; i < nNumPairs; i++) 
        m_kerning[(kp[i].wFirst<<16)|kp[i].wSecond] = kp[i].iKernAmount;
      delete [] kp;
    }

    SelectFont(hDC, hFontOld);
  }

  FontWrapper::~FontWrapper()
  {
    DeleteFont(m_hFont);
  }

  int FontWrapper::GetKernAmount(WCHAR c1, WCHAR c2)
  {
    int size = 0;
    std::map<DWORD, int>::iterator it = m_kerning.find((c1<<16)|c2);
    return it != m_kerning.end() ? it->second : 0;
  }
}