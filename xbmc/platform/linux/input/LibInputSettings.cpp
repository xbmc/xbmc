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
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>

const std::string CLibInputSettings::SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT = "input.libinputkeyboardlayout";
static std::vector<StringSettingOption> layouts;

namespace
{
  inline bool LayoutSort(const StringSettingOption& i, const StringSettingOption& j)
  {
    return (i.value < j.value);
  }
} // unnamed namespace

CLibInputSettings::CLibInputSettings(CLibInputHandler *handler) :
  m_libInputHandler(handler)
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  auto settingsManager = settings->GetSettingsManager();
  if (!settingsManager)
    return;

  auto setting = settings->GetSetting(SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
  if (!setting)
  {
    CLog::Log(LOGERROR, "Failed to load setting for: {}", SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
    return;
  }

  setting->SetVisible(true);

  std::set<std::string> settingSet;
  settingSet.insert(SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
  settingsManager->RegisterCallback(this, settingSet);
  settingsManager->RegisterSettingOptionsFiller("libinputkeyboardlayout",
                                                SettingOptionsKeyboardLayoutsFiller);

  /* load the keyboard layouts from xkeyboard-config */
  std::string xkbFile("/usr/share/X11/xkb/rules/base.xml");

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(xkbFile))
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: unable to open: {}", xkbFile);
    return;
  }

  const auto* rootElement = xmlDoc.RootElement();
  if (!rootElement)
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: missing or invalid XML root element in: {}", xkbFile);
    return;
  }

  if (strcmp(rootElement->Value(), "xkbConfigRegistry") != 0)
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML root element {} in: {}",
              rootElement->Value(), xkbFile);
    return;
  }

  const auto* layoutListElement = rootElement->FirstChildElement("layoutList");
  if (!layoutListElement)
  {
    CLog::Log(LOGWARNING, "CLibInputSettings: missing XML child element {} in: {}", "layoutList",
              xkbFile);
    return;
  }

  const auto* layoutElement = layoutListElement->FirstChildElement("layout");
  while (layoutElement)
  {
    const auto* configElement = layoutElement->FirstChildElement("configItem");
    if (!configElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element {} in: {}",
                layoutListElement->Value(), xkbFile);
      return;
    }

    const auto* nameElement = configElement->FirstChildElement("name");
    if (!nameElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element {} in: {}",
                configElement->Value(), xkbFile);
      return;
    }

    const auto* descriptionElement = configElement->FirstChildElement("description");
    if (!descriptionElement)
    {
      CLog::Log(LOGWARNING, "CLibInputSettings: unexpected XML child element {} in: {}",
                configElement->Value(), xkbFile);
      return;
    }

    std::string layout = nameElement->GetText();
    std::string layoutDescription = descriptionElement->GetText();

    if (!layout.empty() && !layoutDescription.empty())
      layouts.emplace_back(layoutDescription, layout);

    layoutElement = layoutElement->NextSiblingElement();
  }

  std::sort(layouts.begin(), layouts.end(), LayoutSort);
}

CLibInputSettings::~CLibInputSettings()
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("libinputkeyboardlayout");
  settings->GetSettingsManager()->UnregisterCallback(this);
}

void CLibInputSettings::SettingOptionsKeyboardLayoutsFiller(
    const std::shared_ptr<const CSetting>& setting,
    std::vector<StringSettingOption>& list,
    std::string& current,
    void* data)
{
  list = layouts;
}

void CLibInputSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
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
