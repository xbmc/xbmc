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

  class CAddOnControl_Spin
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void SetVisible(void *addonData, void* handle, bool visible);
    static void SetEnabled(void *addonData, void* handle, bool enabled);

    static void SetText(void *addonData, void* handle, const char *text);
    static void Reset(void *addonData, void* handle);
    static void SetType(void *addonData, void* handle, int type);

    static void AddStringLabel(void *addonData, void* handle, const char* label, const char* value);
    static void AddIntLabel(void *addonData, void* handle, const char* label, int value);

    static void SetStringValue(void *addonData, void* handle, const char* value);
    static void GetStringValue(void *addonData, void* handle, char &value, unsigned int &maxStringSize);

    static void SetIntRange(void *addonData, void* handle, int start, int end);
    static void SetIntValue(void *addonData, void* handle, int value);
    static int GetIntValue(void *addonData, void* handle);

    static void SetFloatRange(void *addonData, void* handle, float start, float end);
    static void SetFloatValue(void *addonData, void* handle, float value);
    static float GetFloatValue(void *addonData, void* handle);
    static void SetFloatInterval(void *addonData, void* handle, float interval);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
