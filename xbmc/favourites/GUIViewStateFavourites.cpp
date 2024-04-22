/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStateFavourites.h"

#include "FileItemList.h"
#include "guilib/WindowIDs.h"

CGUIViewStateFavourites::CGUIViewStateFavourites(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByUserPreference, 19349,
                LABEL_MASKS("%L", "", "%L", "")); // Label, empty | Label, empty
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "", "%L", "")); // Label, empty | Label, empty

  SetSortMethod(SortByUserPreference);

  LoadViewState(items.GetPath(), WINDOW_FAVOURITES);
}

void CGUIViewStateFavourites::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_FAVOURITES);
}
