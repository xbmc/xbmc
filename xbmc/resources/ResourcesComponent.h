/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CLocalizeStrings;

class CResourcesComponent
{
public:
  CResourcesComponent();
  virtual ~CResourcesComponent();
  void Init();
  void Deinit();

  CLocalizeStrings& GetLocalizeStrings();

protected:
  // members are pointers in order to avoid includes
  std::unique_ptr<CLocalizeStrings> m_localizeStrings;
};
