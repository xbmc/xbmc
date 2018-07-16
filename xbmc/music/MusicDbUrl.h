/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "DbUrl.h"

class CVariant;

class CMusicDbUrl : public CDbUrl
{
public:
  CMusicDbUrl();
  ~CMusicDbUrl() override;

protected:
  bool parse() override;
  bool validateOption(const std::string &key, const CVariant &value) override;
};
