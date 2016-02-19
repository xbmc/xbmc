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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_DialogExtendedProgress
  {
    GUIHANDLE Dialogs_ExtendedProgress_New(const std::string& title);
    void Dialogs_ExtendedProgress_Delete(GUIHANDLE handle);
    std::string Dialogs_ExtendedProgress_Title(GUIHANDLE handle) const;
    void Dialogs_ExtendedProgress_SetTitle(GUIHANDLE handle, const std::string& title);
    std::string Dialogs_ExtendedProgress_Text(GUIHANDLE handle) const;
    void Dialogs_ExtendedProgress_SetText(GUIHANDLE handle, const std::string& text);
    bool Dialogs_ExtendedProgress_IsFinished(GUIHANDLE handle) const;
    void Dialogs_ExtendedProgress_MarkFinished(GUIHANDLE handle);
    float Dialogs_ExtendedProgress_Percentage(GUIHANDLE handle) const;
    void Dialogs_ExtendedProgress_SetPercentage(GUIHANDLE handle, float fPercentage);
    void Dialogs_ExtendedProgress_SetProgress(GUIHANDLE handle, int currentItem, int itemCount);
  };

}; /* extern "C" */
