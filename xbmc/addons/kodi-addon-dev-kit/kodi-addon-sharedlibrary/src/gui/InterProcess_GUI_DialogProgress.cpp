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

#include "InterProcess_GUI_DialogProgress.h"
#include "InterProcess.h"

extern "C"
{

GUIHANDLE CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_New()
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.New(g_interProcess.m_Handle->addonData);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Delete(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Delete(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Open(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Open(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetHeading(GUIHANDLE handle, const std::string& title)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetHeading(g_interProcess.m_Handle->addonData, handle, title.c_str());
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetLine(GUIHANDLE handle, unsigned int iLine, const std::string& line)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetLine(g_interProcess.m_Handle->addonData, handle, iLine, line.c_str());
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetCanCancel(GUIHANDLE handle, bool bCanCancel)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetCanCancel(g_interProcess.m_Handle->addonData, handle, bCanCancel);
}

bool CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_IsCanceled(GUIHANDLE handle) const
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.IsCanceled(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetPercentage(GUIHANDLE handle, int iPercentage)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetPercentage(g_interProcess.m_Handle->addonData, handle, iPercentage);
}

int CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_GetPercentage(GUIHANDLE handle) const
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.GetPercentage(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_ShowProgressBar(GUIHANDLE handle, bool bOnOff)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.ShowProgressBar(g_interProcess.m_Handle->addonData, handle, bOnOff);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetProgressMax(GUIHANDLE handle, int iMax)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetProgressMax(g_interProcess.m_Handle->addonData, handle, iMax);
}

void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetProgressAdvance(GUIHANDLE handle, int nSteps)
{
  g_interProcess.m_Callbacks->GUI.Dialogs.Progress.SetProgressAdvance(g_interProcess.m_Handle->addonData, handle, nSteps);
}

bool CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Abort(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.Dialogs.Progress.Abort(g_interProcess.m_Handle->addonData, handle);
}

}; /* extern "C" */
