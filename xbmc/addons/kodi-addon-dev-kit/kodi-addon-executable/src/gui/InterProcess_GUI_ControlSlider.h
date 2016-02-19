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

  struct CKODIAddon_InterProcess_GUI_ControlSlider
  {
    void Control_Slider_SetVisible(void* window, bool visible);
    void Control_Slider_SetEnabled(void* window, bool enabled);
    std::string Control_Slider_GetDescription(void* window) const;
    void Control_Slider_SetIntRange(void* window, int start, int end);
    void Control_Slider_SetIntValue(void* window, int value);
    int Control_Slider_GetIntValue(void* window) const;
    void Control_Slider_SetIntInterval(void* window, int interval);
    void Control_Slider_SetPercentage(void* window, float percent);
    float Control_Slider_GetPercentage(void* window) const;
    void Control_Slider_SetFloatRange(void* window, float start, float end);
    void Control_Slider_SetFloatValue(void* window, float value);
    float Control_Slider_GetFloatValue(void* window) const;
    void Control_Slider_SetFloatInterval(void* window, float interval);
  };

}; /* extern "C" */
