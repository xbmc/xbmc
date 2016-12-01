#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "settings/lib/Setting.h"

class CSettingPath : public CSettingString
{
public:
  CSettingPath(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingPath(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingPath(const std::string &id, const CSettingPath &setting);
  virtual ~CSettingPath() { }

  virtual SettingPtr Clone(const std::string &id) const override;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetValue(const std::string &value) override;

  bool Writable() const { return m_writable; }
  void SetWritable(bool writable) { m_writable = writable; }
  const std::vector<std::string>& GetSources() const { return m_sources; }
  void SetSources(const std::vector<std::string> &sources) { m_sources = sources; }
  bool HideExtension() const { return m_hideExtension; }
  void SetHideExtension(bool hideExtension) { m_hideExtension = hideExtension; }
  const std::string& GetMasking() const { return m_masking; }
  void SetMasking(const std::string& masking) { m_masking = masking; }

private:
  using CSettingString::copy;
  void copy(const CSettingPath &setting);

  bool m_writable;
  std::vector<std::string> m_sources;
  bool m_hideExtension;
  std::string m_masking;
};
