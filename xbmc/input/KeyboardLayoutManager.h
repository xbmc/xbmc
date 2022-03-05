/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/KeyboardLayout.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CSetting;
struct StringSettingOption;

typedef std::map<std::string, CKeyboardLayout> KeyboardLayouts;

class CKeyboardLayoutManager
{
public:
  CKeyboardLayoutManager() = default;
  virtual ~CKeyboardLayoutManager();

  bool Load(const std::string& path = "");
  void Unload();

  const KeyboardLayouts& GetLayouts() const { return m_layouts; }
  bool GetLayout(const std::string& name, CKeyboardLayout& layout) const;

  static void SettingOptionsKeyboardLayoutsFiller(const std::shared_ptr<const CSetting>& setting,
                                                  std::vector<StringSettingOption>& list,
                                                  std::string& current,
                                                  void* data);

private:
  CKeyboardLayoutManager(const CKeyboardLayoutManager&) = delete;
  CKeyboardLayoutManager const& operator=(CKeyboardLayoutManager const&) = delete;

  KeyboardLayouts m_layouts;
};
