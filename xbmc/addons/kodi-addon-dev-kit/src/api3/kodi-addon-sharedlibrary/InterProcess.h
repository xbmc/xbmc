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
#include "kodi/api3/definitions.hpp"

#include KITINCLUDE(ADDON_API_LEVEL, AddonLib.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, .internal/AddonLib_internal.hpp)

#include <stdlib.h>
#include <string>

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

extern "C"
{

  using namespace API_NAMESPACE_NAME::KodiAPI;

  namespace ADDON
  {
    typedef struct AddonCB
    {
      const char* libBasePath;  ///< Never, never change this!!!
      void*       addonData;
    } AddonCB;
  }

  class CKODIAddon_InterProcess
  {
  public:
    CKODIAddon_InterProcess();
    virtual ~CKODIAddon_InterProcess();

    int InitLibAddon(void* hdl);
    int Finalize();
    void Log(const addon_log loglevel, const char* string);

    ADDON::AddonCB* m_Handle;
    CB_AddOnLib*    m_Callbacks;

  protected:
    _register_level*  KODI_register;
    _unregister_me*   KODI_unregister;

  private:
    void* m_libKODI_addon;
    struct cb_array
    {
      const char* libPath;
    };
  };

  extern CKODIAddon_InterProcess g_interProcess;

} /* extern "C" */
