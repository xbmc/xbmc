/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIListItem.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabel.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/GUIInfoProvider.h"
#include "guilib/guiinfo/GUIInfoProviders.h"
#include "smarthome/guiinfo/IPowerMeterHUD.h"
#include "smarthome/guiinfo/ISystemHealthHUD.h"
#include "smarthome/guiinfo/IVehicleHUD.h"
#include "smarthome/guiinfo/SmartHomeGuiInfo.h"
#include "smarthome/guiinfo/SmartHomeProperty.h"
#include "utils/Variant.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI::SMART_HOME;

namespace
{
class TestGUIComponent : public CGUIComponent
{
public:
  TestGUIComponent() : CGUIComponent(false)
  {
    m_guiInfoManager = std::make_unique<CGUIInfoManager>();
    CServiceBroker::RegisterGUI(this);
  }
  ~TestGUIComponent() override { CServiceBroker::UnregisterGUI(); }
};

class TestPropertyProvider : public KODI::GUILIB::GUIINFO::CGUIInfoProvider
{
public:
  bool InitCurrentItem(CFileItem*) override { return false; }

  bool GetLabel(std::string& value,
                const CFileItem*,
                int,
                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                std::string*) const override
  {
    if (info.GetInfo() == SMARTHOME_PROPERTY)
    {
      EXPECT_EQ("station", info.GetData3());
      EXPECT_EQ("traction_voltage", info.GetData5());
      EXPECT_EQ("float32", info.GetData6());
      EXPECT_TRUE(info.GetData7().empty() || info.GetData7() == "{:.1f} V");
      return FormatSmartHomeProperty(7.843f, info.GetData7(), value);
    }
    if (info.GetInfo() == SMARTHOME_POWER_METER_VOLTAGE ||
        info.GetInfo() == SMARTHOME_POWER_METER_CURRENT ||
        info.GetInfo() == SMARTHOME_POWER_METER_POWER ||
        info.GetInfo() == SMARTHOME_POWER_METER_CURRENT_SHARE)
    {
      EXPECT_EQ("station", info.GetData3());
      EXPECT_EQ("power_meter_0", info.GetData5());
      double number = 4.567;
      if (info.GetInfo() == SMARTHOME_POWER_METER_VOLTAGE)
        number = 7.843;
      else if (info.GetInfo() == SMARTHOME_POWER_METER_CURRENT)
        number = 1.234;
      else if (info.GetInfo() == SMARTHOME_POWER_METER_CURRENT_SHARE)
      {
        EXPECT_EQ("power_meter_1", info.GetData6());
        number = CalculateCurrentShare(1.3, 2.0);
      }
      return FormatSmartHomeNumber(number, info.GetData7(), value);
    }
    if (info.GetInfo() == SYSTEM_BUILD_VERSION)
    {
      value = "22.0";
      return true;
    }
    return false;
  }

  bool GetInt(int&, const CGUIListItem*, int, const KODI::GUILIB::GUIINFO::CGUIInfo&) const override
  {
    return false;
  }

  bool GetBool(bool&,
               const CGUIListItem*,
               int,
               const KODI::GUILIB::GUIINFO::CGUIInfo&) const override
  {
    return false;
  }
};

class TestSystemHealthHUD : public ISystemHealthHUD
{
public:
  bool IsActive(const std::string&, std::chrono::milliseconds) override { return false; }
  CTemperature CPUTemperature(const std::string&) override { return {}; }
  float CPUUtilization(const std::string&) override { return 0.0f; }
  double CPUFrequencyHz(const std::string&) override { return 0.0; }
  float MemoryUtilization(const std::string&) override { return 0.0f; }
  unsigned int BatteryCharge(const std::string&) override { return 0; }
  float BatteryLoad(const std::string&) override { return 0.0f; }
  std::optional<SmartHomePropertyValue> Property(const std::string&,
                                                 const std::string&,
                                                 SmartHomePropertyType) override
  {
    return std::nullopt;
  }
};

class TestVehicleHUD : public IVehicleHUD
{
public:
  float ForwardSpeed(const std::string&) override { return 0.0f; }
  float ForwardSpeedStdDev(const std::string&) override { return 0.0f; }
  float Roll(const std::string&) override { return 0.0f; }
  float RollStdDev(const std::string&) override { return 0.0f; }
  float Pitch(const std::string&) override { return 0.0f; }
  float PitchStdDev(const std::string&) override { return 0.0f; }
  float Yaw(const std::string&) override { return 0.0f; }
  float YawStdDev(const std::string&) override { return 0.0f; }
  float Tilt(const std::string&) override { return 0.0f; }
  float TiltStdDev(const std::string&) override { return 0.0f; }
};

class TestPowerMeterHUD : public IPowerMeterHUD
{
public:
  std::optional<PowerMeterReading> PowerMeter(const std::string&, const std::string&) override
  {
    return std::nullopt;
  }

  std::optional<double> CurrentShare(const std::string& systemName,
                                     const std::string& powerMeterName,
                                     const std::string& otherPowerMeterName) override
  {
    EXPECT_EQ("station", systemName);
    EXPECT_EQ("power_meter_0", powerMeterName);
    EXPECT_EQ("power_meter_1", otherPowerMeterName);
    return share;
  }

  std::optional<double> share;
};

} // namespace

TEST(TestSmartHomeProperty, Translation)
{
  CGUIInfoManager infoManager;
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).Property(traction_voltage,float32)"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).Property(traction_voltage,float32,{:.1f} V)"));
  EXPECT_NE(0, infoManager.TranslateString("SmartHome.System(station).IsActive(2)"));
}

TEST(TestSmartHomeProperty, ListItemPropertyFloat)
{
  CGUIInfoManager infoManager;
  CGUIListItem item;
  item.SetProperty("smarthome.current_balance", CVariant{47.625f});

  const int info = infoManager.TranslateString("ListItem.Property(smarthome.current_balance)");
  ASSERT_NE(0, info);

  float value = 0.0f;
  EXPECT_TRUE(infoManager.GetFloat(value, info, 0, &item));
  EXPECT_FLOAT_EQ(47.625f, value);
}

TEST(TestSmartHomeProperty, InvalidTranslation)
{
  CGUIInfoManager infoManager;
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System().Property(value,float32)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(,float32)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(value)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).Property(value,Float32)"));
  EXPECT_EQ(0,
            infoManager.TranslateString("SmartHome.System(station).Property(value,float32,{},x)"));
  EXPECT_EQ(0,
            infoManager.TranslateString("SmartHome.System(station).Property(value,float32,{:.1f)"));
}

TEST(TestSmartHomeProperty, PowerMeterTranslation)
{
  CGUIInfoManager infoManager;
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(power_meter_0).Voltage"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(power_meter_0).Current"));
  EXPECT_NE(
      0, infoManager.TranslateString("SmartHome.System(station).PowerMeter(power_meter_0).Power"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(power_meter_0).Voltage({:.1f} V)"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(power_meter_0).Current({:.2f} A)"));
  EXPECT_NE(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(power_meter_0).Power({:.1f} W)"));
  EXPECT_NE(0,
            infoManager.TranslateString(
                "SmartHome.System(station).PowerMeter(power_meter_0).CurrentShare(power_meter_1)"));
  EXPECT_NE(0, infoManager.TranslateString("SmartHome.System(station).PowerMeter(power_meter_0)."
                                           "CurrentShare(power_meter_1,{:.0f} %)"));
}

TEST(TestSmartHomeProperty, InvalidPowerMeterTranslation)
{
  CGUIInfoManager infoManager;
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System().PowerMeter(meter).Voltage"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).PowerMeter().Voltage"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).PowerMeter(meter).Status"));
  EXPECT_EQ(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(meter).Voltage({},extra)"));
  EXPECT_EQ(
      0, infoManager.TranslateString("SmartHome.System(station).PowerMeter(meter).Voltage({:.1f)"));
  EXPECT_EQ(0, infoManager.TranslateString("SmartHome.System(station).PowerMeter.Voltage"));
  EXPECT_EQ(
      0, infoManager.TranslateString("SmartHome.System(station).PowerMeter(meter).CurrentShare()"));
  EXPECT_EQ(0, infoManager.TranslateString(
                   "SmartHome.System(station).PowerMeter(meter).CurrentShare(peer,{},extra)"));
}

TEST(TestSmartHomeProperty, PowerMeterRendering)
{
  TestGUIComponent gui;
  TestPropertyProvider provider;
  gui.GetInfoManager().GetInfoProviders().RegisterProvider(&provider, false);

  KODI::GUILIB::GUIINFO::CGUIInfoLabel voltage(
      "$INFO[SmartHome.System(station).PowerMeter(power_meter_0).Voltage({:.1f} V)]");
  KODI::GUILIB::GUIINFO::CGUIInfoLabel current(
      "$INFO[SmartHome.System(station).PowerMeter(power_meter_0).Current({:.2f} A)]");
  KODI::GUILIB::GUIINFO::CGUIInfoLabel power(
      "$INFO[SmartHome.System(station).PowerMeter(power_meter_0).Power({:.1f} W)]");
  KODI::GUILIB::GUIINFO::CGUIInfoLabel share("$INFO[SmartHome.System(station).PowerMeter(power_"
                                             "meter_0).CurrentShare(power_meter_1,{:.0f} %)]");
  KODI::GUILIB::GUIINFO::CGUIInfoLabel preciseShare(
      "$INFO[SmartHome.System(station).PowerMeter(power_meter_0).CurrentShare(power_meter_1,{:.1f} "
      "%)]");
  EXPECT_EQ("7.8 V", voltage.GetLabel(0));
  EXPECT_EQ("1.23 A", current.GetLabel(0));
  EXPECT_EQ("4.6 W", power.GetLabel(0));
  EXPECT_EQ("39 %", share.GetLabel(0));
  EXPECT_EQ("39.4 %", preciseShare.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel unformatted(
      "$INFO[SmartHome.System(station).PowerMeter(power_meter_0).Voltage]");
  EXPECT_EQ("7.843", unformatted.GetLabel(0));
  gui.GetInfoManager().GetInfoProviders().UnregisterProvider(&provider);
}

TEST(TestSmartHomeProperty, PowerMeterCurrentShare)
{
  EXPECT_NEAR(39.3939, CalculateCurrentShare(1.3, 2.0), 0.0001);
  EXPECT_NEAR(60.6061, CalculateCurrentShare(2.0, 1.3), 0.0001);
  EXPECT_DOUBLE_EQ(50.0, CalculateCurrentShare(0.10, 0.12));
  EXPECT_DOUBLE_EQ(50.0, CalculateCurrentShare(0.12, 0.10));
  EXPECT_DOUBLE_EQ(50.0, CalculateCurrentShare(0.10, 0.15));
  EXPECT_DOUBLE_EQ(45.0, CalculateCurrentShare(0.20, 0.30));
  EXPECT_DOUBLE_EQ(55.0, CalculateCurrentShare(0.30, 0.20));
  EXPECT_DOUBLE_EQ(40.0, CalculateCurrentShare(0.30, 0.45));
  EXPECT_DOUBLE_EQ(60.0, CalculateCurrentShare(0.45, 0.30));
  EXPECT_DOUBLE_EQ(50.0, CalculateCurrentShare(1.3, 1.3));

  const double negativeShare = CalculateCurrentShare(-1.3, 2.0);
  EXPECT_NEAR(39.3939, negativeShare, 0.0001);
  EXPECT_NEAR(60.6061, CalculateCurrentShare(2.0, -1.3), 0.0001);

  for (const double share : {CalculateCurrentShare(0.0, 1.0), CalculateCurrentShare(0.20, 0.30),
                             CalculateCurrentShare(1.0, 0.0), negativeShare})
  {
    EXPECT_GE(share, 0.0);
    EXPECT_LE(share, 100.0);
  }
}

TEST(TestSmartHomeProperty, PowerMeterCurrentShareInteger)
{
  CGUIInfoManager infoManager;
  TestSystemHealthHUD systemHealthHud;
  TestVehicleHUD vehicleHud;
  TestPowerMeterHUD powerMeterHud;
  CSmartHomeGuiInfo provider(infoManager, systemHealthHud, vehicleHud, powerMeterHud);
  const KODI::GUILIB::GUIINFO::CGUIInfo info(SMARTHOME_POWER_METER_CURRENT_SHARE, "station",
                                             "power_meter_0", "power_meter_1", "");

  int value = -1;
  for (const auto& [share, expected] : std::array<std::pair<double, int>, 5>{
           {{39.3939, 39}, {60.6061, 61}, {50.0, 50}, {-1.0, 0}, {101.0, 100}}})
  {
    powerMeterHud.share = share;
    EXPECT_TRUE(provider.GetInt(value, nullptr, 0, info));
    EXPECT_EQ(expected, value);
  }

  powerMeterHud.share = std::nullopt;
  EXPECT_FALSE(provider.GetInt(value, nullptr, 0, info));

  const KODI::GUILIB::GUIINFO::CGUIInfo otherInfo(SMARTHOME_POWER_METER_CURRENT, "station",
                                                  "power_meter_0", "", "");
  powerMeterHud.share = 50.0;
  EXPECT_FALSE(provider.GetInt(value, nullptr, 0, otherInfo));
}

TEST(TestSmartHomeProperty, InfoLabelNestedCommasAndOuterAffixes)
{
  TestGUIComponent gui;
  TestPropertyProvider provider;
  gui.GetInfoManager().GetInfoProviders().RegisterProvider(&provider, false);

  KODI::GUILIB::GUIINFO::CGUIInfoLabel label("$INFO[SmartHome.System(station).Property(traction_"
                                             "voltage,float32,{:.1f} V),prefix:, postfix]");
  EXPECT_EQ("prefix:7.8 V postfix", label.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel defaultFormat(
      "$INFO[SmartHome.System(station).Property(traction_voltage,float32)]");
  EXPECT_EQ("7.843", defaultFormat.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel existing("$INFO[System.BuildVersion,prefix:, postfix]");
  EXPECT_EQ("prefix:22.0 postfix", existing.GetLabel(0));

  KODI::GUILIB::GUIINFO::CGUIInfoLabel malformed(
      "$INFO[SmartHome.System(station).Property(traction_voltage,float32,prefix:,postfix]");
  EXPECT_TRUE(malformed.IsConstant());

  gui.GetInfoManager().GetInfoProviders().UnregisterProvider(&provider);
}

TEST(TestSmartHomeProperty, ScalarTypes)
{
  constexpr std::array<std::string_view, 12> types{"bool",   "int8",    "int16",   "int32",
                                                   "int64",  "uint8",   "uint16",  "uint32",
                                                   "uint64", "float32", "float64", "string"};
  for (const auto type : types)
    EXPECT_TRUE(SmartHomePropertyTypeFromString(type).has_value()) << type;

  EXPECT_FALSE(SmartHomePropertyTypeFromString("Byte").has_value());
  EXPECT_FALSE(SmartHomePropertyTypeFromString("array").has_value());
}

TEST(TestSmartHomeProperty, Formatting)
{
  std::string result;
  EXPECT_TRUE(FormatSmartHomeProperty(7.843f, "{:.1f} V", result));
  EXPECT_EQ("7.8 V", result);
  EXPECT_TRUE(FormatSmartHomeProperty(uint32_t{42}, {}, result));
  EXPECT_EQ("42", result);
  EXPECT_FALSE(FormatSmartHomeProperty(7.843f, "{:.1f", result));
  EXPECT_TRUE(result.empty());
}
