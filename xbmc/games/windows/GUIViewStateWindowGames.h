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
/*!
 * \ingroup games
 */
class CGUIViewStateWindowGames : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowGames(const CFileItemList& items);

  ~CGUIViewStateWindowGames() override = default;

  // implementation of CGUIViewState
  std::string GetLockType() override;
  std::string GetExtensions() override;
  VECSOURCES& GetSources() override;

protected:
  // implementation of CGUIViewState
  void SaveViewState() override;
};
} // namespace GAME
} // namespace KODI
