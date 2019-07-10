/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileItemListModifier.h"

#include <set>

class CFileItemListModification : public IFileItemListModifier
{
public:
  ~CFileItemListModification() override;

  static CFileItemListModification& GetInstance();

  bool CanModify(const CFileItemList &items) const override;
  bool Modify(CFileItemList &items) const override;

private:
  CFileItemListModification();
  CFileItemListModification(const CFileItemListModification&) = delete;
  CFileItemListModification& operator=(CFileItemListModification const&) = delete;

  std::set<IFileItemListModifier*> m_modifiers;
};
