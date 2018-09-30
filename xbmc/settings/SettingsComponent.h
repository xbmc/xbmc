/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAppParamParser;
class CAdvancedSettings;

class CSettingsComponent
{
public:
  CSettingsComponent();
  virtual ~CSettingsComponent();
  void Init(const CAppParamParser &params);
  void Deinit();

  std::shared_ptr<CAdvancedSettings> GetAdvancedSettings();

protected:
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
};
