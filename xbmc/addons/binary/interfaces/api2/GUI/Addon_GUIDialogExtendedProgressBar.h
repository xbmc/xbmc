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

  struct CAddOnDialog_ExtendedProgress
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static void* New(void *addonData, const char *title);
    static void Delete(void *addonData, void* handle);
    static void Title(void *addonData, void* handle, char &title, unsigned int &iMaxStringSize);
    static void SetTitle(void *addonData, void* handle, const char *title);
    static void Text(void *addonData, void* handle, char &text, unsigned int &iMaxStringSize);
    static void SetText(void *addonData, void* handle, const char *text);
    static bool IsFinished(void *addonData, void* handle);
    static void MarkFinished(void *addonData, void* handle);
    static float Percentage(void *addonData, void* handle);
    static void SetPercentage(void *addonData, void* handle, float fPercentage);
    static void SetProgress(void *addonData, void* handle, int currentItem, int itemCount);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
