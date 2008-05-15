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
#include "stdafx.h"
#include "ScreenSaverFactory.h"
#include "Util.h"


CScreenSaverFactory::CScreenSaverFactory()
{}
CScreenSaverFactory::~CScreenSaverFactory()
{}

extern "C" void __declspec(dllexport) get_module(struct ScreenSaver* pScr);

CScreenSaver* CScreenSaverFactory::LoadScreenSaver(const CStdString& strScr) const
{
  // strip of the path & extension to get the name of the visualisation
  CStdString strName = CUtil::GetFileName(strScr);
  strName = strName.Left(strName.size() - 4);

  // load visualisation
  DllScreensaver* pDll = new DllScreensaver;
  pDll->SetFile(strScr);
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct ScreenSaver* pScr = (struct ScreenSaver*)malloc(sizeof(struct ScreenSaver));
  ZeroMemory(pScr, sizeof(struct ScreenSaver));
  pDll->GetModule(pScr);

  // and pass it to a new instance of CScreenSaver() which will handle the screensaver
  return new CScreenSaver(pScr, pDll, strName);
}
