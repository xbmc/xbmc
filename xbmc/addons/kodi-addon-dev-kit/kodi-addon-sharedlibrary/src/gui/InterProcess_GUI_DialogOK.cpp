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

#include "InterProcess_GUI_DialogOK.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_DialogOK::Dialogs_OK_ShowAndGetInputSingleText(
          const std::string&      heading,
          const std::string&      text)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.OK.ShowAndGetInputSingleText(heading.c_str(), text.c_str());
}

void CKODIAddon_InterProcess_GUI_DialogOK::Dialogs_OK_ShowAndGetInputLineText(
          const std::string&      heading,
          const std::string&      line0,
          const std::string&      line1,
          const std::string&      line2)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.OK.ShowAndGetInputLineText(heading.c_str(), line0.c_str(), line1.c_str(), line2.c_str());
}

}; /* extern "C" */
