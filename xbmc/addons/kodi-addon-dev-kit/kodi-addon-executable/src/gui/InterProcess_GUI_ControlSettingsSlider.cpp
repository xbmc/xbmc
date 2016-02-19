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

#include "InterProcess_GUI_ControlSettingsSlider.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetVisible(void* window, bool visible)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetEnabled(void* window, bool enabled)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetText(void* window, const std::string& label)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_Reset(void* window)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntRange(void* window, int start, int end)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntValue(void* window, int value)
{
}

int CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetIntValue(void* window) const
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntInterval(void* window, int interval)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetPercentage(void* window, float percent)
{
}

float CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetPercentage(void* window) const
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatRange(void* window, float start, float end)
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatValue(void* window, float value)
{
}

float CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetFloatValue(void* window) const
{
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatInterval(void* window, float interval)
{
}

}; /* extern "C" */
