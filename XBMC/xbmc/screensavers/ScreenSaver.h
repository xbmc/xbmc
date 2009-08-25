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
// Screensaver.h: interface for the CScreensaver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "AddonDll.h"
#include "DllScreenSaver.h"

class CScreenSaver : public ADDON::CAddonDll<DllScreensaver, ScreenSaver, SCR_PROPS>
{
public:
  CScreenSaver(const ADDON::AddonProps &props) : CAddonDll(props) {};
  virtual ~CScreenSaver() {};

  // Things that MUST be supplied by the child classes
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO *info);
};

#endif // !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)

