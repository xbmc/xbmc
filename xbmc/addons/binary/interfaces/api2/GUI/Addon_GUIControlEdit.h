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

  struct CAddOnControl_Edit
  {
    static void Init(struct CB_AddOnLib* interfaces);

    static void SetVisible(void* addonData, void* handle, bool visible);
    static void SetEnabled(void* addonData, void* spinhandle, bool enabled);

    static void SetInputType(void* addonData, void* handle, int type, const char* heading);

    static void SetLabel(void* addonData, void* handle, const char* label);
    static void GetLabel(void* addonData, void* handle, char& label, unsigned int& iMaxStringSize);

    static void SetText(void* addonData, void* handle, const char* text);
    static void GetText(void* addonData, void* handle, char& text, unsigned int& iMaxStringSize);

    static unsigned int GetCursorPosition(void* addonData, void* handle);
    static void SetCursorPosition(void* addonData, void* handle, unsigned int iPosition);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
