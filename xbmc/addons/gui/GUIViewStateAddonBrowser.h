/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateAddonBrowser : public CGUIViewState
{
public:
  explicit CGUIViewStateAddonBrowser(const CFileItemList& items);

protected:
  void SaveViewState() override;
  std::string GetExtensions() override;
};
