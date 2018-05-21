#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

class CFileItemList;

class CGUIDialogPictureInfo :
      public CGUIDialog
{
public:
  CGUIDialogPictureInfo(void);
  ~CGUIDialogPictureInfo(void) override;
  void SetPicture(CFileItem *item);
  void FrameMove() override;

protected:
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnAction(const CAction& action) override;
  void UpdatePictureInfo();

  CFileItemList* m_pictureInfo;
  std::string    m_currentPicture;
};
