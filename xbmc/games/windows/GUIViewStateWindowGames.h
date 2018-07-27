/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

namespace KODI
{
namespace GAME
{
  class CGUIViewStateWindowGames : public CGUIViewState
  {
  public:
    explicit CGUIViewStateWindowGames(const CFileItemList& items);

    virtual ~CGUIViewStateWindowGames() = default;

    // implementation of CGUIViewState
    virtual std::string GetLockType() override;
    virtual std::string GetExtensions() override;
    virtual VECSOURCES& GetSources() override;

  protected:
    // implementation of CGUIViewState
    virtual void SaveViewState() override;
  };
}
}
