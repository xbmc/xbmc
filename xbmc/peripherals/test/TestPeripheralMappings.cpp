/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "peripherals/Peripherals.h"

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include <gtest/gtest.h>
#include <tinyxml2.h>

using namespace std::chrono_literals;

namespace PERIPHERALS
{
class TestPeripheralMappings : public testing::Test
{
protected:
  using Settings = std::map<std::string, PeripheralDeviceSetting>;

  //! Reads the <setting> children of \p xml, or nothing if that does not finish within a second.
  static std::optional<Settings> SettingsFrom(const std::string& xml)
  {
    auto done = std::make_shared<std::promise<Settings>>();
    std::future<Settings> result = done->get_future();
    std::thread(
        [xml, done]
        {
          tinyxml2::XMLDocument doc;
          doc.Parse(xml.c_str());
          Settings settings;
          CPeripherals::GetSettingsFromMappingsFile(doc.RootElement(), settings);
          done->set_value(std::move(settings));
        })
        .detach();

    if (result.wait_for(1s) != std::future_status::ready)
      return std::nullopt;
    return result.get();
  }
};
} // namespace PERIPHERALS

using PERIPHERALS::TestPeripheralMappings;

TEST_F(TestPeripheralMappings, SkipsASettingWithoutAKey)
{
  const auto settings = SettingsFrom(R"(<peripheral>
                                          <setting type="bool" value="1" label="1"/>
                                          <setting key="enabled" type="bool" value="1" label="2"/>
                                        </peripheral>)");

  ASSERT_TRUE(settings.has_value()) << "reading the settings did not finish";
  EXPECT_EQ(1u, settings->size());
  EXPECT_TRUE(settings->contains("enabled"));
}

TEST_F(TestPeripheralMappings, ReadsTheOrderOfASetting)
{
  const auto settings = SettingsFrom(R"(<peripheral>
                                          <setting key="first" type="bool" value="1" label="1" order="2"/>
                                          <setting key="second" type="bool" value="1" label="2" order="1"/>
                                        </peripheral>)");

  ASSERT_TRUE(settings.has_value()) << "reading the settings did not finish";
  ASSERT_TRUE(settings->contains("first"));
  ASSERT_TRUE(settings->contains("second"));
  EXPECT_EQ(2, settings->at("first").m_order);
  EXPECT_EQ(1, settings->at("second").m_order);
}
