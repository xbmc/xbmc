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
  CSkinSettings(const CSkinSettings&);
  CSkinSettings& operator=(CSkinSettings const&);
  ~CSkinSettings() override;

private:
  CCriticalSection m_critical;
  std::set<ADDON::CSkinSettingPtr> m_settings;
};