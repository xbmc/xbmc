#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace GUI
{
extern "C"
{

  struct CAddOnControl_SettingsSlider
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static void SetVisible(void *addonData, void* spinhandle, bool visible);
    static void SetEnabled(void *addonData, void* spinhandle, bool enabled);

    static void SetText(void *addonData, void* handle, const char *text);
    static void Reset(void *addonData, void* handle);

    static void SetIntRange(void *addonData, void* handle, int iStart, int iEnd);
    static void SetIntValue(void *addonData, void* handle, int iValue);
    static int GetIntValue(void *addonData, void* handle);
    static void SetIntInterval(void *addonData, void* handle, int iInterval);

    static void SetPercentage(void *addonData, void* handle, float fPercent);
    static float GetPercentage(void *addonData, void* handle);

    static void SetFloatRange(void *addonData, void* handle, float fStart, float fEnd);
    static void SetFloatValue(void *addonData, void* handle, float fValue);
    static float GetFloatValue(void *addonData, void* handle);
    static void SetFloatInterval(void *addonData, void* handle, float fInterval);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
