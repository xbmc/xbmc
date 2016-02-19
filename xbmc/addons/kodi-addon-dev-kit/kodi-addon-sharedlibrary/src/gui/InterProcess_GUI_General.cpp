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

#include "InterProcess_GUI_General.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_General::GUIGeneral_Lock()
{
  g_interProcess.m_Callbacks->GUI.General.Lock();
}

void CKODIAddon_InterProcess_GUI_General::GUIGeneral_Unlock()
{
  g_interProcess.m_Callbacks->GUI.General.Unlock();
}

int CKODIAddon_InterProcess_GUI_General::GUIGeneral_GetScreenHeight()
{
  return g_interProcess.m_Callbacks->GUI.General.GetScreenHeight();
}

int CKODIAddon_InterProcess_GUI_General::GUIGeneral_GetScreenWidth()
{
  return g_interProcess.m_Callbacks->GUI.General.GetScreenWidth();
}

int CKODIAddon_InterProcess_GUI_General::GUIGeneral_GetVideoResolution()
{
  return g_interProcess.m_Callbacks->GUI.General.GetVideoResolution();
}

int CKODIAddon_InterProcess_GUI_General::GUIGeneral_GetCurrentWindowDialogId()
{
  return g_interProcess.m_Callbacks->GUI.General.GetCurrentWindowDialogId();
}

int CKODIAddon_InterProcess_GUI_General::GUIGeneral_GetCurrentWindowId()
{
  return g_interProcess.m_Callbacks->GUI.General.GetCurrentWindowId();
}

}; /* extern "C" */
