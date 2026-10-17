/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/naming/TosecNaming.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;
using namespace GAME::TOSEC;

TEST(TestTosecNaming, ReadsEveryField)
{
  const std::optional<Name> name = CTosecNaming::Parse(
      "Legend of TOSEC, The v1.03b (demo-kiosk) (1986-06-21)(Devstudio - Publisher)(A500)(PAL)"
      "(DE-GB)(de-en)(SW-R)(beta)(Disk 1 of 2)(Character Disk)"
      "[cr PDX][h Hacker][tr fr Translator][a2][!][aka House of TOSEC]");
  ASSERT_TRUE(name.has_value());
  EXPECT_EQ(name->title, "Legend of TOSEC, The");
  EXPECT_EQ(name->version, "v1.03b");
  EXPECT_EQ(name->demo, Demo::KIOSK);
  EXPECT_EQ(name->date.year, 1986);
  EXPECT_EQ(name->date.month, 6);
  EXPECT_EQ(name->date.day, 21);
  EXPECT_EQ(name->publishers, (std::vector<std::string>{"Devstudio", "Publisher"}));
  EXPECT_EQ(name->system, "A500");
  EXPECT_EQ(name->video, "PAL");
  EXPECT_EQ(name->countries, (std::vector<std::string>{"DE", "GB"}));
  EXPECT_EQ(name->languages, (std::vector<std::string>{"de", "en"}));
  EXPECT_EQ(name->copyright, Copyright::SHAREWARE_REGISTERED);
  EXPECT_EQ(name->status, DevelopmentStatus::BETA);
  EXPECT_EQ(name->media.type, "Disk");
  EXPECT_EQ(name->media.number, "1");
  EXPECT_EQ(name->media.total, "2");
  EXPECT_EQ(name->mediaLabel, "Character Disk");
  ASSERT_EQ(name->dumpFlags.size(), 5u);
  EXPECT_EQ(name->dumpFlags[0].type, DumpFlagType::CRACKED);
  EXPECT_EQ(name->dumpFlags[0].detail, "PDX");
  EXPECT_EQ(name->dumpFlags[2].type, DumpFlagType::TRANSLATED);
  EXPECT_EQ(name->dumpFlags[2].detail, "fr Translator");
  EXPECT_EQ(name->dumpFlags[3].type, DumpFlagType::ALTERNATE_DUMP);
  EXPECT_EQ(name->dumpFlags[3].number, 2);
  EXPECT_EQ(name->dumpFlags[4].type, DumpFlagType::VERIFIED);
  EXPECT_EQ(name->moreInfo, (std::vector<std::string>{"aka House of TOSEC"}));
  EXPECT_TRUE(name->unknownTags.empty());
}

TEST(TestTosecNaming, ReadsADateWithUnknownParts)
{
  EXPECT_EQ(CTosecNaming::ParseDate("198x")->year, 0);
  EXPECT_EQ(CTosecNaming::ParseDate("19xx-12-2x")->month, 12);
  EXPECT_EQ(CTosecNaming::ParseDate("2001-01")->year, 2001);
  EXPECT_FALSE(CTosecNaming::ParseDate("1x86").has_value());
  EXPECT_FALSE(CTosecNaming::ParseDate("Proto").has_value());
}

TEST(TestTosecNaming, TellsACountryFromALanguageByItsCase)
{
  const std::optional<Name> canada = CTosecNaming::Parse("Game (1990)(Publisher)(CA)");
  ASSERT_TRUE(canada.has_value());
  EXPECT_EQ(canada->countries, (std::vector<std::string>{"CA"}));
  EXPECT_TRUE(canada->languages.empty());

  const std::optional<Name> catalan = CTosecNaming::Parse("Game (1990)(Publisher)(ca)");
  ASSERT_TRUE(catalan.has_value());
  EXPECT_TRUE(catalan->countries.empty());
  EXPECT_EQ(catalan->languages, (std::vector<std::string>{"ca"}));

  EXPECT_EQ(CTosecNaming::CountryName("AS"), "Asia");
  EXPECT_EQ(CTosecNaming::CountryName("EU"), "Europe");
  EXPECT_TRUE(CTosecNaming::CountryName("de").empty());
}

TEST(TestTosecNaming, CountsLanguagesItDoesNotName)
{
  EXPECT_EQ(CTosecNaming::Parse("Game (1990)(Publisher)(M3)")->languageCount, 3);
}

TEST(TestTosecNaming, KeepsTheFieldsBeforeOneCutShort)
{
  const std::optional<Name> name = CTosecNaming::Parse("Bulldog (1988)(Ariolasoft UK LTD)(Tape 2");
  ASSERT_TRUE(name.has_value());
  EXPECT_EQ(name->title, "Bulldog");
  EXPECT_EQ(name->publishers, (std::vector<std::string>{"Ariolasoft UK LTD"}));
  EXPECT_TRUE(name->media.type.empty());

  EXPECT_EQ(CTosecNaming::Parse("Samurai Trilogy (1988)(Ariolasoft UK LT2")->title,
            "Samurai Trilogy");
}

TEST(TestTosecNaming, ReadsAPublisherItDoesNotKnow)
{
  EXPECT_TRUE(CTosecNaming::Parse("Game (1990)(-)(PAL)")->publishers.empty());
}

TEST(TestTosecNaming, ReadsATapeSide)
{
  const Media media = CTosecNaming::Parse("Game (1985)(Publisher)(Tape 2 of 2 Side B)")->media;
  EXPECT_EQ(media.number, "2");
  EXPECT_EQ(media.total, "2");
  EXPECT_EQ(media.side, "B");
}

TEST(TestTosecNaming, ReadsTheWaysSetsBendTheConvention)
{
  const std::optional<Name> catacomb =
      CTosecNaming::Parse("Catacomb Episode 1 - Abyss (v1.3) (1992)(Softdisk Publishing)");
  ASSERT_TRUE(catacomb.has_value());
  EXPECT_EQ(catacomb->version, "v1.3");
  EXPECT_EQ(catacomb->publishers, (std::vector<std::string>{"Softdisk Publishing"}));

  const std::optional<Name> catacomb3d = CTosecNaming::Parse(
      "Catacomb 3D - The Descent (aka A New Dimension) (1991)(Softdisk Publishing)");
  ASSERT_TRUE(catacomb3d.has_value());
  EXPECT_EQ(catacomb3d->unknownTags, (std::vector<std::string>{"aka A New Dimension"}));

  EXPECT_EQ(CTosecNaming::Parse("Tunnel B1 (1996) (Acclaim Entertainment Inc)")->publishers,
            (std::vector<std::string>{"Acclaim Entertainment Inc"}));
  EXPECT_EQ(CTosecNaming::Parse("Aliens (1987)(Electric Dreams Software)2")->title, "Aliens");

  // No publisher, a language in its place
  const std::optional<Name> thesee =
      CTosecNaming::Parse("Il Faut Fuir Thesee (1984)(Fr)(Disk 1 of 1 Side A)");
  ASSERT_TRUE(thesee.has_value());
  EXPECT_TRUE(thesee->publishers.empty());
  EXPECT_EQ(thesee->languages, (std::vector<std::string>{"fr"}));
  EXPECT_EQ(thesee->media.side, "A");

  // A capitalised code is a language where it can be one, a country where not
  EXPECT_EQ(CTosecNaming::Parse("Game (1990)(L.K. Avalon)(Pl)")->languages,
            (std::vector<std::string>{"pl"}));
  EXPECT_EQ(CTosecNaming::Parse("Game (1983)(Commodore)(Jp)")->countries,
            (std::vector<std::string>{"JP"}));
  EXPECT_EQ(CTosecNaming::Parse("Game (1991)(Future)(UK)")->countries,
            (std::vector<std::string>{"UK"}));
  EXPECT_EQ(CTosecNaming::Parse("Game (19Xx)(-)")->date.year, 0);

  // A disk number after the dump flags
  EXPECT_EQ(
      CTosecNaming::Parse("Game (1985)(Electronic Arts)[cr](Disk 2 of 3)[fantasy]")->media.number,
      "2");
}

TEST(TestTosecNaming, LeavesANameNotWrittenTheTosecWay)
{
  // A No-Intro prototype gives the date of its build
  EXPECT_FALSE(CTosecNaming::Parse("3-D Zapper (USA) (Proto) (1982-12-15)").has_value());
  EXPECT_FALSE(
      CTosecNaming::Parse("Superman (Europe) (Proto) (2001-01-12) (GB Compatible)").has_value());
  EXPECT_FALSE(CTosecNaming::Parse("Blasteroids (Image Works)(1990)").has_value());
  EXPECT_FALSE(CTosecNaming::Parse("Choose An Enemy (1991)").has_value());
  EXPECT_FALSE(CTosecNaming::Parse("Super Mario Bros. (W) [!]").has_value());
}
