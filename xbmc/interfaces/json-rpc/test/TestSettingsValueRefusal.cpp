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
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/Setting.h"
#include "utils/Variant.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace JSONRPC;

namespace
{

CVariant Write(const std::string& id, const CVariant& value)
{
  CVariant params{CVariant::VariantTypeObject};
  params["setting"] = id;
  params["value"] = value;
  return params;
}

std::shared_ptr<CSettingBool> ToggleSetting()
{
  return std::static_pointer_cast<CSettingBool>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_DEBUG_SHOWLOGINFO));
}

/*!
 \brief A change handler that declines every change and notes whether it was confirmed
 */
class CDecliningHandler : public ISettingCallback
{
public:
  explicit CDecliningHandler(const std::string& id)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, {id});
  }
  ~CDecliningHandler() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);
  }

  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override
  {
    m_confirmed = CDisplaySettings::IsChangeConfirmed();
    return false;
  }

  bool WasConfirmed() const { return m_confirmed; }

private:
  bool m_confirmed{false};
};

} // unnamed namespace

/*!
 A value the setting does not offer is a client-side error, which is a different answer from
 a change the application would not keep.
 */
TEST(TestSettingsValueRefusal, AValueTheSettingDoesNotOfferIsInvalidParams)
{
  CVariant result;
  EXPECT_EQ(CSettingsOperations::SetSettingValue(
                "", nullptr, nullptr, Write(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE, 99),
                result),
            InvalidParams);
}

/*!
 Change callbacks are dispatched only once the settings manager is marked loaded, which the
 test environment never does, so these two run only against a loaded settings store.
 */
TEST(TestSettingsValueRefusal, DISABLED_AChangeAHandlerDeclinesIsUnavailable)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->IsLoaded())
    GTEST_SKIP() << "change callbacks are not dispatched until the settings are loaded";

  const auto setting{ToggleSetting()};
  ASSERT_NE(setting, nullptr) << "debug.showloginfo is not defined in this environment";
  const bool before{setting->GetValue()};
  CDecliningHandler handler{CSettings::SETTING_DEBUG_SHOWLOGINFO};

  CVariant result;
  EXPECT_EQ(CSettingsOperations::SetSettingValue(
                "", nullptr, nullptr, Write(CSettings::SETTING_DEBUG_SHOWLOGINFO, !before), result),
            Unavailable);
  EXPECT_EQ(before, setting->GetValue());
}

TEST(TestSettingsValueRefusal, DISABLED_ConfirmedReachesTheChangeHandler)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->IsLoaded())
    GTEST_SKIP() << "change callbacks are not dispatched until the settings are loaded";

  const auto setting{ToggleSetting()};
  ASSERT_NE(setting, nullptr) << "debug.showloginfo is not defined in this environment";
  const bool before{setting->GetValue()};

  {
    CDecliningHandler handler{CSettings::SETTING_DEBUG_SHOWLOGINFO};
    CVariant result;
    CSettingsOperations::SetSettingValue(
        "", nullptr, nullptr, Write(CSettings::SETTING_DEBUG_SHOWLOGINFO, !before), result);
    EXPECT_FALSE(handler.WasConfirmed());
  }

  {
    CDecliningHandler handler{CSettings::SETTING_DEBUG_SHOWLOGINFO};
    CVariant params{Write(CSettings::SETTING_DEBUG_SHOWLOGINFO, !before)};
    params["confirmed"] = true;
    CVariant result;
    CSettingsOperations::SetSettingValue("", nullptr, nullptr, params, result);
    EXPECT_TRUE(handler.WasConfirmed());
  }

  EXPECT_FALSE(CDisplaySettings::IsChangeConfirmed());
}
