/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>

#include "addons/Skin.h"
#include "settings/lib/ISubSettings.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CSkinSettings : public ISubSettings
{
public:
  static CSkinSettings& GetInstance();

  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;
  void Clear() override;

  void MigrateSettings(const ADDON::SkinPtr& skin);

  int TranslateString(const std::string &setting);
  const std::string& GetString(int setting) const;
  void SetString(int setting, const std::string &label);

  int TranslateBool(const std::string &setting);
  bool GetBool(int setting) const;
  void SetBool(int setting, bool set);

  void Reset(const std::string &setting);
  void Reset();

protected:
  CSkinSettings();
  CSkinSettings(const CSkinSettings&) = delete;
  CSkinSettings& operator=(CSkinSettings const&) = delete;
  ~CSkinSettings() override;

private:
  CCriticalSection m_critical;
  std::set<ADDON::CSkinSettingPtr> m_settings;
};
