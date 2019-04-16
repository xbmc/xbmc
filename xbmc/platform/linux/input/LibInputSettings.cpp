/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibInputSettings.h"

#include "LibInputHandler.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>

const std::string CLibInputSettings::SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT = "input.libinputkeyboardlayout";
static std::vector<StringSettingOption> layouts;

namespace
{
  inline bool LayoutSort(const StringSettingOption& i, const StringSettingOption& j)
  {
    return (i.value > j.value);
  }
} // unnamed namespace

CLibInputSettings::CLibInputSettings(CLibInputHandler *handler) :
  m_libInputHandler(handler)
{
  std::set<std::string> settingSet;
  settingSet.insert(SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->GetSettingsManager()->RegisterCallback(this, settingSet);
  settings->GetSettingsManager()->RegisterSettingOptionsFiller("libinputkeyboardlayout", SettingOptionsKeyboardLayoutsFiller);

  /* load the keyboard layouts from xkeyboard-config */
  std::string xkbFile("/usr/share/X11/xkb/rules/base.xml");

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(xkbFile))
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: unable to open: %s", xkbFile.c_str());
    return;
  }

  const TiXmlElement* rootElement = xmlDoc.RootElement();
  if (!rootElement)
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: missing or invalid XML root element in: %s", xkbFile.c_str());
    return;
  }

  if (rootElement->ValueStr() != "xkbConfigRegistry")
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML root element %s in: %s", rootElement->Value(), xkbFile.c_str());
    return;
  }

  const TiXmlElement* layoutListElement = rootElement->FirstChildElement("layoutList");
  if (!layoutListElement)
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element %s in: %s", layoutListElement->Value(), xkbFile.c_str());
    return;
  }

  const TiXmlElement* layoutElement = layoutListElement->FirstChildElement("layout");
  while (layoutElement)
  {
    const TiXmlElement* configElement = layoutElement->FirstChildElement("configItem");
    if (!configElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element %s in: %s", layoutListElement->Value(), xkbFile.c_str());
      return;
    }

    const TiXmlElement* nameElement = configElement->FirstChildElement("name");
    if (!nameElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element %s in: %s", configElement->Value(), xkbFile.c_str());
      return;
    }

    const TiXmlElement* descriptionElement = configElement->FirstChildElement("description");
    if (!descriptionElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element %s in: %s", configElement->Value(), xkbFile.c_str());
      return;
    }

    std::string layout = nameElement->GetText();
    std::string layoutDescription = descriptionElement->GetText();

    if (!layout.empty() && !layoutDescription.empty())
      layouts.emplace_back(StringSettingOption(layoutDescription, layout));

    layoutElement = layoutElement->NextSiblingElement();
  }

  std::sort(layouts.begin(), layouts.end(), LayoutSort);
}

CLibInputSettings::~CLibInputSettings()
{
  CSettingsComponent *settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("libinputkeyboardlayout");
  settings->GetSettingsManager()->UnregisterCallback(this);
}

void CLibInputSettings::SettingOptionsKeyboardLayoutsFiller(std::shared_ptr<const CSetting> setting, std::vector<StringSettingOption> &list, std::string &current, void *data)
{
  list = layouts;
}

void CLibInputSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT)
  {
    std::string layout = std::dynamic_pointer_cast<const CSettingString>(setting)->GetValue();
    m_libInputHandler->SetKeymap(layout);
  }
}
