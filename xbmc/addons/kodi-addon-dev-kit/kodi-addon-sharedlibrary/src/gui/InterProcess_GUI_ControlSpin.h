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

  struct CKODIAddon_InterProcess_GUI_ControlSpin
  {
    void Control_Spin_SetVisible(void* window, bool visible);
    void Control_Spin_SetEnabled(void* window, bool enabled);
    void Control_Spin_SetText(void* window, const std::string& label);
    void Control_Spin_Reset(void* window);
    void Control_Spin_SetType(void* window, int type);
    void Control_Spin_AddStringLabel(void* window, const std::string& label, const std::string& value);
    void Control_Spin_AddIntLabel(void* window, const std::string& label, int value);
    void Control_Spin_SetStringValue(void* window, const std::string& value);
    std::string Control_Spin_GetStringValue(void* window) const;
    void Control_Spin_SetIntRange(void* window, int start, int end);
    void Control_Spin_SetIntValue(void* window, int value);
    int Control_Spin_GetIntValue(void* window) const;
    void Control_Spin_SetFloatRange(void* window, float start, float end);
    void Control_Spin_SetFloatValue(void* window, float value);
    float Control_Spin_GetFloatValue(void* window) const;
    void Control_Spin_SetFloatInterval(void* window, float interval);
  };

}; /* extern "C" */
