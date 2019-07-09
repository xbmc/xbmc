/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include <gtest/gtest.h>

class CSettings;
class CSettingsComponent;

class TestBasicEnvironment : public testing::Environment
{
public:
  TestBasicEnvironment();

  void SetUp() override;
  void TearDown() override;
private:
  void SetUpError();
  std::string m_tempPath;
  std::unique_ptr<CSettingsComponent> m_pSettingsComponent;
};
