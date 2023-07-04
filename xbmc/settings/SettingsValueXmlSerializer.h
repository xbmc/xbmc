/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsValueSerializer.h"

#include <memory>

class CSetting;
class CSettingCategory;
class CSettingGroup;
class CSettingSection;

namespace tinyxml2
{
class XMLNode;
}

class CSettingsValueXmlSerializer : public ISettingsValueSerializer
{
public:
  CSettingsValueXmlSerializer() = default;
  ~CSettingsValueXmlSerializer() = default;

  // implementation of ISettingsValueSerializer
  std::string SerializeValues(const CSettingsManager* settingsManager) const override;

private:
  void SerializeSection(tinyxml2::XMLNode* parent,
                        const std::shared_ptr<CSettingSection>& section) const;
  void SerializeCategory(tinyxml2::XMLNode* parent,
                         const std::shared_ptr<CSettingCategory>& category) const;
  void SerializeGroup(tinyxml2::XMLNode* parent, const std::shared_ptr<CSettingGroup>& group) const;
  void SerializeSetting(tinyxml2::XMLNode* parent, const std::shared_ptr<CSetting>& setting) const;
};
