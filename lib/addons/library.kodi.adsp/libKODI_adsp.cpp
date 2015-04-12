/*
 *      Copyright (C) 2013-2014 Team KODI
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "../../../addons/library.kodi.adsp/libKODI_adsp.h"
#include "addons/AddonCallbacks.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

extern "C"
{

DLLEXPORT void* ADSP_register_me(void *hdl)
{
  CB_ADSPLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libKODI_adsp-ERROR: ADSPLib_register_me is called with NULL handle !!!\n");
  else
  {
    cb = ((AddonCB*)hdl)->ADSPLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libKODI_adsp-ERROR: ADSPLib_register_me can't get callback table from KODI !!!\n");
  }
  return cb;
}

DLLEXPORT void ADSP_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->ADSPLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_ADSPLib*)cb);
}

DLLEXPORT void ADSP_add_menu_hook(void *hdl, void* cb, AE_DSP_MENUHOOK *hook)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->AddMenuHook(((AddonCB*)hdl)->addonData, hook);
}

DLLEXPORT void ADSP_remove_menu_hook(void *hdl, void* cb, AE_DSP_MENUHOOK *hook)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->RemoveMenuHook(((AddonCB*)hdl)->addonData, hook);
}

DLLEXPORT void ADSP_register_mode(void *hdl, void* cb, AE_DSP_MODES::AE_DSP_MODE *mode)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->RegisterMode(((AddonCB*)hdl)->addonData, mode);
}

DLLEXPORT void ADSP_unregister_mode(void *hdl, void* cb, AE_DSP_MODES::AE_DSP_MODE *mode)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->UnregisterMode(((AddonCB*)hdl)->addonData, mode);
}

};
