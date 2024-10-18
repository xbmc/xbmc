/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowPrograms : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowPrograms(const CFileItemList& items);

protected:
  void SaveViewState() override;
  std::string GetLockType() override;
  std::string GetExtensions() override;
  std::vector<CMediaSource>& GetSources() override;
};

