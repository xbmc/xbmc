/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileItemListModifier.h"

namespace PVR
{

class CPVRFileItemListModifier : public IFileItemListModifier
{
public:
  CPVRFileItemListModifier() = default;
  ~CPVRFileItemListModifier() override = default;

  bool CanModify(const CFileItemList& items) const override;
  bool Modify(CFileItemList& items) const override;
};

} // namespace PVR
