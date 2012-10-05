/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// FileFactory1.h: interface for the CFileFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
#define AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "utils/StdString.h"

namespace XFILE
{
class CFileFactory
{
public:
  CFileFactory();
  virtual ~CFileFactory();
  static IFile* CreateLoader(const CStdString& strFileName);
  static IFile* CreateLoader(const CURL& url);
};
}
#endif // !defined(AFX_FILEFACTORY1_H__068E3138_B7CB_4BEE_B5CE_8AA8CADAB233__INCLUDED_)
