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

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, gui/General.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{
namespace General
{

void Lock()
{
  g_interProcess.m_Callbacks->GUI.General.Lock();
}

void Unlock()
{
  g_interProcess.m_Callbacks->GUI.General.Unlock();
}

int GetScreenHeight()
{
  return g_interProcess.m_Callbacks->GUI.General.GetScreenHeight();
}

int GetScreenWidth()
{
  return g_interProcess.m_Callbacks->GUI.General.GetScreenWidth();
}

int GetVideoResolution()
{
  return g_interProcess.m_Callbacks->GUI.General.GetVideoResolution();
}

int GetCurrentWindowDialogId()
{
  return g_interProcess.m_Callbacks->GUI.General.GetCurrentWindowDialogId();
}

int GetCurrentWindowId()
{
  return g_interProcess.m_Callbacks->GUI.General.GetCurrentWindowId();
}

} /* namespace General */
} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
