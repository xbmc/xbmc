/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "utils/XBMCTinyXML2.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
namespace fs = std::filesystem;

std::string FileUrl(const fs::path& path)
{
  return path.string();
}

fs::path WriteRepoDir(const fs::path& root, const std::string& name, const std::string& digest)
{
  const fs::path dir = root / name;
  fs::create_directories(dir);

  {
    std::ofstream index(dir / "addons.xml");
    index << "<addons><addon id=\"" << name << ".test\" name=\"" << name
          << "\" version=\"1.0.0\" provider-name=\"Team Kodi\">"
          << "<extension point=\"xbmc.python.module\" library=\"lib.so\"/>"
          << "<extension point=\"kodi.addon.metadata\"><platform>all</platform></extension>"
          << "</addon></addons>";
  }
  {
    std::ofstream sum(dir / "addons.xml.md5", std::ios::binary);
    sum << digest << "\n";
  }
  return dir;
}

AddonInfoPtr TwoDirRepository(const fs::path& dira, const fs::path& dirb)
{
  const std::string xml{R"(<addon id="repository.test.twodir" name="Test" version="1.0.0" provider-name="Team Kodi">
  <extension point="xbmc.addon.repository">
    <dir>
      <info>)" + FileUrl(dira / "addons.xml") + R"(</info>
      <checksum>)" + FileUrl(dira / "addons.xml.md5") + R"(</checksum>
      <datadir>)" + FileUrl(dira) + R"(</datadir>
    </dir>
    <dir>
      <info>)" + FileUrl(dirb / "addons.xml") + R"(</info>
      <checksum>)" + FileUrl(dirb / "addons.xml.md5") + R"(</checksum>
      <datadir>)" + FileUrl(dirb) + R"(</datadir>
    </dir>
  </extension>
  <extension point="xbmc.addon.metadata"><platform>all</platform></extension>
</addon>)"};
  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  return CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{});
}

std::vector<std::string> AddonIds(const std::vector<AddonInfoPtr>& addons)
{
  std::vector<std::string> ids;
  for (const auto& addon : addons)
    ids.push_back(addon->ID());
  return ids;
}
} // namespace

TEST(TestRepository, FetchIfChangedRecoversDirectoryAfterOutage)
{
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path root = fs::temp_directory_path() / ("kodi-test-repo-" + std::to_string(stamp));
  fs::remove_all(root);
  ASSERT_TRUE(fs::create_directories(root));

  const fs::path dira = WriteRepoDir(root, "dira", "digestA");
  const fs::path dirb = WriteRepoDir(root, "dirb", "digestB");

  const AddonInfoPtr repoInfo = TwoDirRepository(dira, dirb);
  ASSERT_NE(nullptr, repoInfo);
  CRepository repository(repoInfo);

  std::string checksum;
  std::vector<AddonInfoPtr> addons;
  int recheckAfter = 0;

  // first check: both directories fetched
  ASSERT_EQ(CRepository::FetchStatus::OK,
            repository.FetchIfChanged("", checksum, addons, recheckAfter));
  ASSERT_EQ("digestA\ndigestB", checksum);
  ASSERT_EQ((std::vector<std::string>{"dira.test", "dirb.test"}), AddonIds(addons));

  // dira becomes unreachable while dirb changes
  fs::remove(dira / "addons.xml.md5");
  {
    std::ofstream sum(dirb / "addons.xml.md5", std::ios::binary);
    sum << "digestB2\n";
  }

  const std::string storedChecksum = checksum;
  checksum.clear();
  addons.clear();
  ASSERT_EQ(CRepository::FetchStatus::OK,
            repository.FetchIfChanged(storedChecksum, checksum, addons, recheckAfter));
  // the stored checksum records an empty digest for the unreachable directory
  ASSERT_EQ("\ndigestB2", checksum);
  ASSERT_EQ((std::vector<std::string>{"dirb.test"}), AddonIds(addons));
  const std::string outageChecksum = checksum;

  // dira responds again, unchanged: its content must be fetched, not skipped
  WriteRepoDir(root, "dira", "digestA");

  checksum.clear();
  addons.clear();
  ASSERT_EQ(CRepository::FetchStatus::OK,
            repository.FetchIfChanged(outageChecksum, checksum, addons, recheckAfter));
  ASSERT_EQ("digestA\ndigestB2", checksum);
  ASSERT_EQ((std::vector<std::string>{"dira.test", "dirb.test"}), AddonIds(addons));

  fs::remove_all(root);
}
