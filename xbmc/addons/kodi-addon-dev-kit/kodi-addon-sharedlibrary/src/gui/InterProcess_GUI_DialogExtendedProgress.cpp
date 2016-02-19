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

#include "InterProcess_GUI_DialogExtendedProgress.h"
#include "InterProcess.h"

extern "C"
{

GUIHANDLE CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_New(const std::string& title)
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.New(g_interProcess.m_Handle->addonData, title.c_str());
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Delete(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Delete(g_interProcess.m_Handle->addonData, handle);
}

std::string CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Title(GUIHANDLE handle) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Title(g_interProcess.m_Handle->addonData, handle, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetTitle(GUIHANDLE handle, const std::string& title)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetTitle(g_interProcess.m_Handle->addonData, handle, title.c_str());
}

std::string CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Text(GUIHANDLE handle) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Text(g_interProcess.m_Handle->addonData, handle, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetText(GUIHANDLE handle, const std::string& text)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetText(g_interProcess.m_Handle->addonData, handle, text.c_str());
}

bool CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_IsFinished(GUIHANDLE handle) const
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.IsFinished(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_MarkFinished(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.MarkFinished(g_interProcess.m_Handle->addonData, handle);
}

float CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Percentage(GUIHANDLE handle) const
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.Percentage(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetPercentage(GUIHANDLE handle, float fPercentage)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetPercentage(g_interProcess.m_Handle->addonData, handle, fPercentage);
}

void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetProgress(GUIHANDLE handle, int currentItem, int itemCount)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.ExtendedProgress.SetProgress(g_interProcess.m_Handle->addonData, handle, currentItem, itemCount);
}

}; /* extern "C" */
