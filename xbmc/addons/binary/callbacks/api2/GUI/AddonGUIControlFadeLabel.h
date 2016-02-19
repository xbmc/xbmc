#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

  struct CAddOnControl_FadeLabel
  {
    static void Init(::V2::KodiAPI::CB_AddOnLib *callbacks);

    static void SetVisible(void *addonData, GUIHANDLE handle, bool visible);
    static void SetEnabled(void *addonData, GUIHANDLE spinhandle, bool enabled);
    static void SetSelected(void *addonData, GUIHANDLE spinhandle, bool selected);

    static void AddLabel(void *addonData, GUIHANDLE handle, const char *label);
    static void GetLabel(void *addonData, GUIHANDLE handle, char &label, unsigned int &iMaxStringSize);
    static void SetScrolling(void *addonData, GUIHANDLE handle, bool scroll);
    static void Reset(void *addonData, GUIHANDLE handle);
  };

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
