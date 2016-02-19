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

  struct CKODIAddon_InterProcess_GUI_ControlEdit
  {
    void Control_Edit_SetVisible(void* control, bool visible);
    void Control_Edit_SetEnabled(void* control, bool enabled);
    void Control_Edit_SetLabel(void* control, const std::string& label);
    std::string Control_Edit_GetLabel(void* control) const;
    void Control_Edit_SetText(void* control, const std::string& label);
    std::string Control_Edit_GetText(void* control) const;
    void Control_Edit_SetCursorPosition(void* control, unsigned int iPosition);
    unsigned int Control_Edit_GetCursorPosition(void* control);
    void Control_Edit_SetInputType(void* control, AddonGUIInputType type, const std::string& heading);
  };

}; /* extern "C" */
