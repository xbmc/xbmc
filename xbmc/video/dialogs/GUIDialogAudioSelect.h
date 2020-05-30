#pragma once

/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIDialog.h"

#include <string>

class CFileItem;
class CFileItemList;

class CGUIDialogAudioSelect : public CGUIDialog
{
public:
  CGUIDialogAudioSelect(void);
  ~CGUIDialogAudioSelect(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void OnInitWindow() override;

protected:
  bool ShowAudioSelect();
};
