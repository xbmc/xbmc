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

  struct CB_AddOnLib;

  struct CAddOnControl_SettingsSlider
  {
    static void Init(::V2::KodiAPI::CB_AddOnLib *callbacks);

    static void SetVisible(void *addonData, GUIHANDLE spinhandle, bool visible);
    static void SetEnabled(void *addonData, GUIHANDLE spinhandle, bool enabled);

    static void SetText(void *addonData, GUIHANDLE handle, const char *text);
    static void Reset(void *addonData, GUIHANDLE handle);

    static void SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
    static void SetIntValue(void *addonData, GUIHANDLE handle, int iValue);
    static int GetIntValue(void *addonData, GUIHANDLE handle);
    static void SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval);

    static void SetPercentage(void *addonData, GUIHANDLE handle, float fPercent);
    static float GetPercentage(void *addonData, GUIHANDLE handle);

    static void SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
    static void SetFloatValue(void *addonData, GUIHANDLE handle, float fValue);
    static float GetFloatValue(void *addonData, GUIHANDLE handle);
    static void SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval);
  };

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
