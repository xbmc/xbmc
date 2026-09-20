/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/Setting.h"

#include <string_view>
#include <vector>

class CFileExtensionProvider;
class TiXmlNode;

class CSettingPath : public CSettingString
{
public:
  CSettingPath(std::string_view id, CSettingsManager* settingsManager = nullptr);
  CSettingPath(std::string_view id,
               int label,
               const std::string& value,
               CSettingsManager* settingsManager = nullptr);
  CSettingPath(std::string_view id, const CSettingPath& setting);
  ~CSettingPath() override = default;

  SettingPtr Clone(std::string_view id) const override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;
  bool SetValue(const std::string &value) override;

  bool Writable() const { return m_writable; }
  void SetWritable(bool writable) { m_writable = writable; }
  const std::vector<std::string>& GetSources() const { return m_sources; }
  void SetSources(const std::vector<std::string> &sources) { m_sources = sources; }
  bool HideExtension() const { return m_hideExtension; }
  void SetHideExtension(bool hideExtension) { m_hideExtension = hideExtension; }
  std::string GetMasking(const CFileExtensionProvider& fileExtensionProvider) const;
  void SetMasking(std::string_view masking) { m_masking = masking; }

private:
  using CSettingString::copy;
  void copy(const CSettingPath &setting);

  bool m_writable = true;
  std::vector<std::string> m_sources;
  bool m_hideExtension = false;
  std::string m_masking;
};
