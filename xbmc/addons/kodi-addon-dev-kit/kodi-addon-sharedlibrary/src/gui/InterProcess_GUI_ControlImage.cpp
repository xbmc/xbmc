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

#include "InterProcess_GUI_ControlImage.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlImage::Control_Image_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Image.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlImage::Control_Image_SetFileName(void* window, const std::string& strFileName, const bool useCache)
{
  g_interProcess.m_Callbacks->GUI.Control.Image.SetFileName(g_interProcess.m_Handle->addonData, window, strFileName.c_str(), useCache);
}

void CKODIAddon_InterProcess_GUI_ControlImage::Control_Image_SetColorDiffuse(void* window, uint32_t colorDiffuse)
{
  g_interProcess.m_Callbacks->GUI.Control.Image.SetColorDiffuse(g_interProcess.m_Handle->addonData, window, colorDiffuse);
}

}; /* extern "C" */
