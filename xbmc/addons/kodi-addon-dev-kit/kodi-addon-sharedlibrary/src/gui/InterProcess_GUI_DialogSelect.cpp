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

#include "InterProcess_GUI_DialogSelect.h"
#include "InterProcess.h"

extern "C"
{

int CKODIAddon_InterProcess_GUI_DialogSelect::Dialogs_Select_Show(
          const std::string&              heading,
          const std::vector<std::string>& entries,
          int                             selected)
{
  unsigned int size = entries.size();
  const char** cEntries = (const char**)malloc(size*sizeof(const char**));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].c_str();
  }
  int ret = g_interProcess.m_Callbacks->GUI.Dialogs.Select.Open(heading.c_str(), cEntries, size, selected);
  free(cEntries);
  return ret;
}

}; /* extern "C" */
