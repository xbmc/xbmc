/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "utils/XBMCTinyXML2.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

using namespace ADDON;

namespace
{
AddonInfoPtr PlainHttpRepository(const std::string& version)
{
  const std::string xml{R"(<addon id="repository.test.plainhttp" name="Test" version=")" + version +
                        R"(" provider-name="Team Kodi">
  <extension point="xbmc.addon.repository">
    <dir>
      <info>http://example.com/addons.xml</info>
      <checksum>http://example.com/addons.xml.md5</checksum>
      <datadir>http://example.com/</datadir>
    </dir>
  </extension>
  <extension point="xbmc.addon.metadata"><platform>all</platform></extension>
</addon>)"};
  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  return CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{});
}

//! How many lines logged while building these repositories warn that one uses plain HTTP
long PlainHttpWarnings(const std::vector<AddonInfoPtr>& repositories)
{
  const auto logger{spdlog::get("general")};
  EXPECT_NE(nullptr, logger);
  if (!logger)
  {
    return 0;
  }

  const auto sink{std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(1000)};
  logger->sinks().push_back(sink);
  for (const auto& info : repositories)
  {
    CRepository repository(info);
  }
  logger->sinks().pop_back();

  const auto lines{sink->last_formatted()};
  return std::ranges::count_if(
      lines, [](const std::string& line)
      { return line.find("repository.test.plainhttp uses plain HTTP") != std::string::npos; });
}
} // namespace

TEST(TestRepository, AProblemWithARepositoryIsLoggedOncePerVersion)
{
  // The versions already logged are remembered for the life of the process, so a repeated run
  // needs versions of its own
  static int run{0};
  ++run;
  const auto version1{PlainHttpRepository("1.0." + std::to_string(run))};
  const auto version2{PlainHttpRepository("2.0." + std::to_string(run))};

  EXPECT_EQ(2, PlainHttpWarnings({version1, version1, version2, version2}));
  EXPECT_EQ(0, PlainHttpWarnings({version1, version2}));
}
