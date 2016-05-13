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

  struct CAddOnDialog_Progress
  {
    static void Init(struct CB_AddOnLib *interfaces);

    static void* New(void *addonData);
    static void Delete(void *addonData, void* handle);
    static void Open(void *addonData, void* handle);
    static void SetHeading(void *addonData, void* handle, const char *heading);
    static void SetLine(void *addonData, void* handle, unsigned int iLine, const char *line);
    static void SetCanCancel(void *addonData, void* handle, bool bCanCancel);
    static bool IsCanceled(void *addonData, void* handle);
    static void SetPercentage(void *addonData, void* handle, int iPercentage);
    static int GetPercentage(void *addonData, void* handle);
    static void ShowProgressBar(void *addonData, void* handle, bool bOnOff);
    static void SetProgressMax(void *addonData, void* handle, int iMax);
    static void SetProgressAdvance(void *addonData, void* handle, int nSteps);
    static bool Abort(void *addonData, void* handle);
  };

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
