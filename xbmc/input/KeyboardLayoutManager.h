/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "input/KeyboardLayout.h"

class CSetting;

typedef std::map<std::string, CKeyboardLayout> KeyboardLayouts;

class CKeyboardLayoutManager
{
public:
  virtual ~CKeyboardLayoutManager();

  static CKeyboardLayoutManager& GetInstance();

  bool Load(const std::string& path = "");
  void Unload();

  const KeyboardLayouts& GetLayouts() const { return m_layouts; }
  bool GetLayout(const std::string& name, CKeyboardLayout& layout) const;

  static void SettingOptionsKeyboardLayoutsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void* data);

private:
  CKeyboardLayoutManager() = default;
  CKeyboardLayoutManager(const CKeyboardLayoutManager&) = delete;
  CKeyboardLayoutManager const& operator=(CKeyboardLayoutManager const&) = delete;

  KeyboardLayouts m_layouts;
};
