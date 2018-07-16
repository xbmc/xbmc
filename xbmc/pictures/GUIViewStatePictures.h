/*
 *  Copyright (C) 2005-2013 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowPictures : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowPictures(const CFileItemList& items);

  std::string GetLockType() override;
  std::string GetExtensions() override;
  VECSOURCES& GetSources() override;

protected:
  void SaveViewState() override;
};

