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

  struct CAddOnControl_FadeLabel
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static void SetVisible(void *addonData, void* handle, bool visible);
    static void SetEnabled(void *addonData, void* spinhandle, bool enabled);
    static void SetSelected(void *addonData, void* spinhandle, bool selected);

    static void AddLabel(void *addonData, void* handle, const char *label);
    static void GetLabel(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize);
    static void SetScrolling(void *addonData, void* handle, bool scroll);
    static void Reset(void *addonData, void* handle);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
