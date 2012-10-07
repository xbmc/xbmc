/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DirectoryNodeYear.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"

using namespace XFILE::PICTUREDATABASEDIRECTORY;

bool CDirectoryNodeYear::GetContent(CFileItemList& items) const
{
  if (CDirectoryNode::GetContent(items))
  {
    // Re-label year 0 as "Unknown"
    for (int i = 0; i < items.Size(); i++)
      if (items[i]->GetLabel() == "0")
        { items[i]->SetLabel(g_localizeStrings.Get(13205)); break; }
    return true;
  }
  return false;
}
