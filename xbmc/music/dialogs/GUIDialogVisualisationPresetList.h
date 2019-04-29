/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIDialog.h"

class CGUIVisualisationControl;
class CFileItemList;

class CGUIDialogVisualisationPresetList : public CGUIDialogSelect
{
public:
  CGUIDialogVisualisationPresetList();
  bool OnMessage(CGUIMessage &message) override;

protected:
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnSelect(int idx) override;

private:
  void ClearVisualisation();
  void SetVisualisation(CGUIVisualisationControl *addon);
  CGUIVisualisationControl* m_viz = nullptr;
};
