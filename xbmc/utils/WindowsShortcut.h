// WindowsShortcut.h: interface for the CWindowsShortcut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_)
#define AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

class CWindowsShortcut
{
public:
  CWindowsShortcut();
  virtual ~CWindowsShortcut();
  static bool IsShortcut(const string& strFileName);
  static bool GetShortcut(const string& strFileName, string& strFileOrDir);
};

#endif // !defined(AFX_WINDOWSSHORTCUT_H__A905CF83_3C3D_44FF_B3EF_778D70676D2C__INCLUDED_)
