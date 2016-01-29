#pragma once

/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
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

#include "GUIMediaWindow.h"

class CGUIWindowFavourites : public CGUIMediaWindow
{
public:
  CGUIWindowFavourites(void);
  virtual ~CGUIWindowFavourites(void);

  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;

  virtual bool OnSelect(int item) override;
  virtual void OnDeleteItem(int iItem) override;
  virtual void OnRenameItem(int iItem);
  virtual void OnSetThumb(int iItem);
};

