/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"

namespace ADDON
{

class CUISoundsResource : public CResource
{
public:
  explicit CUISoundsResource(const AddonInfoPtr& addonInfo);

  bool IsAllowed(const std::string &file) const override;
  bool IsInUse() const override;
  void OnPostInstall(bool update, bool modal) override;
};

}
