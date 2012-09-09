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
#pragma once

#include "AddonDll.h"
#include "include/xbmc_scr_types.h"

typedef DllAddon<ScreenSaver, SCR_PROPS> DllScreenSaver;

namespace ADDON
{

class CScreenSaver : public ADDON::CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>
{
public:
  CScreenSaver(const AddonProps &props) : ADDON::CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>(props) {};
  CScreenSaver(const cp_extension_t *ext) : ADDON::CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>(ext) {};
  CScreenSaver(const char *addonID);
  virtual ~CScreenSaver() {}

  // Things that MUST be supplied by the child classes
  bool CreateScreenSaver();
  void Start();
  void Render();
  void GetInfo(SCR_INFO *info);
  void Destroy();
};

} /*namespace ADDON*/
