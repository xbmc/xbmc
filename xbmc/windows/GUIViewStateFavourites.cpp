/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIViewStateFavourites.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/GraphicContext.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "view/ViewState.h"

CGUIViewStateFavourites::CGUIViewStateFavourites(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Date | Label, Date
  SetSortMethod(SortByLabel);

  LoadViewState(items.GetPath(), WINDOW_FAVOURITES);
}

void CGUIViewStateFavourites::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_FAVOURITES);
}

