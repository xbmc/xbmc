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

class CCDecoder
{
  CSimpleTextSubtitle m_sts;
  CStdString m_fn, m_rawfn;
  __int64 m_time;
  bool m_fEndOfCaption;
  WCHAR m_buff[16][33], m_disp[16][33];
  Com::SmartPoint m_cursor;

  void SaveDisp(__int64 time);
  void MoveCursor(int x, int y);
  void OffsetCursor(int x, int y);
  void PutChar(WCHAR c);

public:
  CCDecoder(CStdString fn = _T(""), CStdString rawfn = _T(""));
  virtual ~CCDecoder();
  void DecodeCC(BYTE* buff, int len, __int64 time);
  void ExtractCC(BYTE* buff, int len, __int64 time);
  CSimpleTextSubtitle& GetSTS() {return m_sts;}
};


