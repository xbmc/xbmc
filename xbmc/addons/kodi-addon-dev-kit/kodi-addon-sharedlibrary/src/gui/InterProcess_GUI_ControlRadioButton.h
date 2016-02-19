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

  struct CKODIAddon_InterProcess_GUI_ControlRadioButton
  {
    void Control_RadioButton_SetVisible(void* window, bool visible);
    void Control_RadioButton_SetEnabled(void* window, bool enabled);
    void Control_RadioButton_SetLabel(void* window, const std::string& label);
    std::string Control_RadioButton_GetLabel(void* window) const;
    void Control_RadioButton_SetSelected(void* window, bool yesNo);
    bool Control_RadioButton_IsSelected(void* window) const;
  };

}; /* extern "C" */
