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

#include "InterProcess_GUI_ControlFadeLabel.h"
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

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_SetVisible(void* window, bool visible)
{
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_AddLabel(void* window, const std::string& text)
{
}

std::string CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_GetLabel(void* window)
{
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_SetScrolling(void* window, bool scroll)
{
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_Reset(void* window)
{
}


}; /* extern "C" */
