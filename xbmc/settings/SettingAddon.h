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

#include "settings/lib/Setting.h"
#include "addons/IAddon.h"

class CSettingAddon : public CSettingString
{
public:
  CSettingAddon(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  CSettingAddon(const std::string &id, const CSettingAddon &setting);
  virtual ~CSettingAddon() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  ADDON::TYPE GetAddonType() const { return m_addonType; }
  void SetAddonType(ADDON::TYPE addonType) { m_addonType = addonType; }

private:
  void copy(const CSettingAddon &setting);

  ADDON::TYPE m_addonType;
};
