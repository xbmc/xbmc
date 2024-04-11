/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"

namespace XFILE
{

class CDVDFile : public COverrideFile
{
public:
  CDVDFile();
  ~CDVDFile() override;

protected:
  std::string TranslatePath(const CURL& url) override;
};
} // namespace XFILE
