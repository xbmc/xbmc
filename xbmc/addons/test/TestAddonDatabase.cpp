/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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

#include "addons/AddonDatabase.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"

#include "gtest/gtest.h"
#include <set>

using namespace ADDON;


class AddonDatabaseTest : public ::testing::Test
{
protected:
  DatabaseSettings settings;
  CAddonDatabase database;

  void SetUp() override
  {
    settings.type = "sqlite3";
    settings.name = "test";
    settings.host = CSpecialProtocol::TranslatePath("special://temp/");

    database.Connect("test", settings, true);

    std::set<std::string> installed{"repository.a", "repository.b"};
    database.SyncInstalled(installed, installed, std::set<std::string>());

    VECADDONS addons;
    CreateAddon(addons, "foo.bar", "1.0.0");
    database.UpdateRepositoryContent("repository.a", AddonVersion("1.0.0"), "test", addons);

    addons.clear();
    CreateAddon(addons, "foo.baz", "1.1.0");
    database.UpdateRepositoryContent("repository.b", AddonVersion("1.0.0"), "test", addons);
  }

  void CreateAddon(VECADDONS& addons, std::string id, std::string version)
  {
    CAddonBuilder builder;
    builder.SetId(id);
    builder.SetVersion(AddonVersion(version));
    addons.push_back(builder.Build());
  }
};


TEST_F(AddonDatabaseTest, TestFindById)
{
  VECADDONS addons;
  EXPECT_TRUE(database.FindByAddonId("foo.baz", addons));
  EXPECT_EQ(1, addons.size());
  EXPECT_EQ(addons.at(0)->ID(), "foo.baz");
  EXPECT_EQ(addons.at(0)->Version().asString(), "1.1.0");
  EXPECT_EQ(addons.at(0)->Origin(), "repository.b");
}

TEST_F(AddonDatabaseTest, TestFindByNonExistingId)
{
  VECADDONS addons;
  EXPECT_TRUE(database.FindByAddonId("does.not.exist", addons));
  EXPECT_EQ(0, addons.size());
}
