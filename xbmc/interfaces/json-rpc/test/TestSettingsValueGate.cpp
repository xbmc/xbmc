/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "interfaces/json-rpc/JSONRPCUtils.h"
#include "interfaces/json-rpc/SettingsOperations.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/Variant.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

CVariant Request(const std::string& id)
{
  CVariant params{CVariant::VariantTypeObject};
  params["setting"] = id;
  return params;
}

/*!
 \brief A setting that exists in every build, with no dependencies of its own
 */
std::shared_ptr<CSetting> ToggleSetting()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
      CSettings::SETTING_DEBUG_SHOWLOGINFO);
}

/*!
 \brief Restores a setting's visibility and enabled state when a test leaves scope
 */
class CSettingStateGuard
{
public:
  explicit CSettingStateGuard(std::shared_ptr<CSetting> setting)
    : m_setting{std::move(setting)},
      m_visible{m_setting->GetVisible()},
      m_enabled{m_setting->IsEnabled()}
  {
  }
  ~CSettingStateGuard()
  {
    m_setting->SetVisible(m_visible);
    m_setting->SetEnabled(m_enabled);
  }

private:
  std::shared_ptr<CSetting> m_setting;
  bool m_visible;
  bool m_enabled;
};

} // unnamed namespace

/*!
 A setting id that names nothing is not-found, which is a different answer from a
 request that did not validate.
 */
TEST(TestSettingsValueGate, AnUnknownSettingIsNotFound)
{
  CVariant result;
  EXPECT_EQ(CSettingsOperations::GetSettingValue("", nullptr, nullptr, Request("no.such.setting"),
                                                 result),
            NotFound);
  EXPECT_EQ(CSettingsOperations::SetSettingValue("", nullptr, nullptr, Request("no.such.setting"),
                                                 result),
            NotFound);
  EXPECT_EQ(CSettingsOperations::ResetSettingValue("", nullptr, nullptr, Request("no.such.setting"),
                                                   result),
            NotFound);
}

/*!
 Visibility is a GUI predicate that says nothing about whether the value is real, so a
 read answers for a hidden setting.
 */
TEST(TestSettingsValueGate, AHiddenSettingCanBeRead)
{
  const auto setting{ToggleSetting()};
  ASSERT_NE(setting, nullptr) << "debug.showloginfo is not defined in this environment";
  CSettingStateGuard guard{setting};

  setting->SetVisible(false);
  ASSERT_FALSE(setting->IsVisible());

  CVariant result;
  EXPECT_EQ(CSettingsOperations::GetSettingValue(
                "", nullptr, nullptr, Request(CSettings::SETTING_DEBUG_SHOWLOGINFO), result),
            OK);
  EXPECT_TRUE(result["value"].isBoolean());
}

TEST(TestSettingsValueGate, AHiddenSettingCanBeWritten)
{
  const auto setting{ToggleSetting()};
  ASSERT_NE(setting, nullptr) << "debug.showloginfo is not defined in this environment";
  CSettingStateGuard guard{setting};

  setting->SetVisible(false);

  // writing the value already in force changes nothing and proves the gate is open
  CVariant params{Request(CSettings::SETTING_DEBUG_SHOWLOGINFO)};
  params["value"] = std::static_pointer_cast<CSettingBool>(setting)->GetValue();

  CVariant result;
  EXPECT_EQ(CSettingsOperations::SetSettingValue("", nullptr, nullptr, params, result), OK);
}

/*!
 A disabled setting is one the interface would not let a person change either, so a write
 is refused as Unavailable (exists, but not now); a read still answers.
 */
TEST(TestSettingsValueGate, ADisabledSettingRefusesWritesAsUnavailable)
{
  const auto setting{ToggleSetting()};
  ASSERT_NE(setting, nullptr) << "debug.showloginfo is not defined in this environment";
  CSettingStateGuard guard{setting};

  setting->SetEnabled(false);
  ASSERT_FALSE(setting->IsEnabled());

  CVariant params{Request(CSettings::SETTING_DEBUG_SHOWLOGINFO)};
  params["value"] = true;

  CVariant result;
  EXPECT_EQ(CSettingsOperations::SetSettingValue("", nullptr, nullptr, params, result),
            Unavailable);
  EXPECT_EQ(CSettingsOperations::ResetSettingValue("", nullptr, nullptr, params, result),
            Unavailable);
  EXPECT_EQ(CSettingsOperations::GetSettingValue("", nullptr, nullptr, params, result), OK);
}
