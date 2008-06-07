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
// KaiVector.h: interface for the CKaiVector class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CKaiVector
{
public:
  CKaiVector();
  virtual ~CKaiVector();

  void Load(const CStdString& strPath);
  void Save(const CStdString& strPath);

  static CKaiVector* From(CStdString& strUrl);

  void AddTitle(DWORD aTitleId, CStdString& aVector);
  bool GetTitle(DWORD aTitleId, CStdString& aVector);
  bool ContainsTitle(DWORD aTitleId);
  bool IsEmpty();

protected:

  typedef std::map<DWORD, CStdString> TITLEVECTORMAP;
  TITLEVECTORMAP m_mapTitles;
  CRITICAL_SECTION m_critical;
  bool m_bDirty;
};

#endif // !defined(AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
