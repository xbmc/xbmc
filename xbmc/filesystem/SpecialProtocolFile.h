/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"

namespace XFILE
{
class CSpecialProtocolFile : public COverrideFile
{
public:
  CSpecialProtocolFile(void);
  ~CSpecialProtocolFile(void) override;

protected:
  std::string TranslatePath(const CURL& url) override;
};
}
