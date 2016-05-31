#pragma once
/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../version.h"
#include "kodi/api2/definitions.hpp"

#include KITINCLUDE(ADDON_API_LEVEL, AddonLib.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, internal/AddonLib_internal.hpp)

#include <stdlib.h>
#include <string>

extern "C"
{

  using namespace API_NAMESPACE_NAME::KodiAPI;

  namespace ADDON
  {
    typedef struct AddonCB
    {
      const char* libBasePath;  ///< Never, never change this!!!
      void*       addonData;
      void*       interface;
    } AddonCB;
  }

} /* extern "C" */

class CKODIAddonInterface
{
public:
  static int InitLibAddon(void* hdl);
  static int Finalize();

  static ADDON::AddonCB* m_Handle;
  static CB_AddOnLib* m_interface;
};
