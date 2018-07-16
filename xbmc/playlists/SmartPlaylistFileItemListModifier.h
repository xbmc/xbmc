/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "IFileItemListModifier.h"

class CSmartPlaylistFileItemListModifier : public IFileItemListModifier
{
public:
  CSmartPlaylistFileItemListModifier() = default;
  ~CSmartPlaylistFileItemListModifier() override = default;

  bool CanModify(const CFileItemList &items) const override;
  bool Modify(CFileItemList &items) const override;

private:
  static std::string GetUrlOption(const std::string &path, const std::string &option);
};
