/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "KeyboardLayout.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CSetting;
struct StringSettingOption;

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \ingroup keyboard
 */
using KeyboardLayouts = std::map<std::string, CKeyboardLayout>;

/*!
 * \ingroup keyboard
 */
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
} // namespace KEYBOARD
} // namespace KODI
