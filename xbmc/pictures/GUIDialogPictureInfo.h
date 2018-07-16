/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
