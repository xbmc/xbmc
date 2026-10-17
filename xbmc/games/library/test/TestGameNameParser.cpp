/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/GameNameParser.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameNameParser, ReadsANoIntroName)
{
  const ParsedGameName parsed = CGameNameParser::Parse(
      "Legend of Zelda, The - A Link to the Past (USA, Europe) (En,Fr,De) (Rev 1).sfc");
  EXPECT_EQ(parsed.title, "Legend of Zelda, The - A Link to the Past");
  EXPECT_EQ(parsed.displayTitle, "The Legend of Zelda - A Link to the Past");
  EXPECT_EQ(parsed.regions, (std::vector<std::string>{"USA", "Europe"}));
  EXPECT_EQ(parsed.languages, (std::vector<std::string>{"en", "fr", "de"}));
  EXPECT_EQ(parsed.revision, "Rev 1");
  EXPECT_EQ(parsed.status, ReleaseStatus::RETAIL);
  EXPECT_TRUE(parsed.unknownTags.empty());
}

TEST(TestGameNameParser, DropsBothExtensionsOfACompressedTarArchive)
{
  EXPECT_EQ(CGameNameParser::Parse("Game (USA).tar.gz").title, "Game");
  EXPECT_EQ(CGameNameParser::Parse("Game (USA).tar.xz").title, "Game");
  EXPECT_EQ(CGameNameParser::Parse("Game (USA).TAR.BZ2").title, "Game");
}

TEST(TestGameNameParser, ReadsARedumpDiscAndItsStatus)
{
  const ParsedGameName parsed =
      CGameNameParser::Parse("Final Fantasy VII (Japan) (Disc 2 of 3) (Beta).cue");
  EXPECT_EQ(parsed.title, "Final Fantasy VII");
  EXPECT_EQ(parsed.regions, (std::vector<std::string>{"Japan"}));
  EXPECT_EQ(parsed.disc, 2);
  EXPECT_EQ(parsed.discs, 3);
  EXPECT_EQ(parsed.status, ReleaseStatus::BETA);
  EXPECT_EQ(CGameNameParser::Parse("Sonic CD (Europe) (Proto).cue").status,
            ReleaseStatus::PROTOTYPE);
  EXPECT_EQ(CGameNameParser::Parse("Tetris (World) (Unl).gb").licence, Licence::UNLICENSED);
}

TEST(TestGameNameParser, ReadsGoodToolsFlags)
{
  const ParsedGameName verified = CGameNameParser::Parse("Super Mario Bros. (W) [!].nes");
  EXPECT_EQ(verified.title, "Super Mario Bros.");
  EXPECT_EQ(verified.regions, (std::vector<std::string>{"World"}));
  EXPECT_TRUE(verified.verified);
  EXPECT_TRUE(CGameNameParser::Parse("Super Mario Bros. (W) [h1].nes").hack);
  EXPECT_TRUE(CGameNameParser::Parse("Super Mario Bros. (W) [b1].nes").bad);
}

TEST(TestGameNameParser, ReadsTheLanguageOfAFanTranslation)
{
  const ParsedGameName french = CGameNameParser::Parse("Super Mario Bros. (W) [T+Fre].nes");
  EXPECT_TRUE(french.translated);
  EXPECT_EQ(french.translation, "fr");
  EXPECT_EQ(CGameNameParser::Parse("Mother (J) [T+Ger1.00_Translator].nes").translation, "de");
  EXPECT_EQ(CGameNameParser::Parse("Mother (J) [T-Spa].nes").translation, "es");
  EXPECT_EQ(CGameNameParser::Parse("Mother (J) [T+Por].nes").translation, "pt");
  EXPECT_EQ(CGameNameParser::Parse("Mother (J) [T+Pol].nes").translation, "pl");

  // Still a translation when the language isn't one it knows
  const ParsedGameName unknown = CGameNameParser::Parse("Mother (J) [T+Xyz].nes");
  EXPECT_TRUE(unknown.translated);
  EXPECT_TRUE(unknown.translation.empty());
  EXPECT_FALSE(CGameNameParser::Parse("Mother (J).nes").translated);
}

TEST(TestGameNameParser, TellsADataDiskFromTheGamesOwn)
{
  EXPECT_TRUE(CGameNameParser::Parse("Ashe (Data disk).d88").dataDisk);
  EXPECT_TRUE(CGameNameParser::Parse("Abyss (User disk) {V1 mode}.d88").dataDisk);
  EXPECT_TRUE(CGameNameParser::Parse("Arcush (Scenario disk).d88").dataDisk);
  EXPECT_TRUE(CGameNameParser::Parse("Fushigi no Umi no Nadia (Music Disk) [FD].zip").dataDisk);
  EXPECT_FALSE(CGameNameParser::Parse("Ashe (Game disk).d88").dataDisk);
  EXPECT_FALSE(CGameNameParser::Parse("Ys (Disk A).d88").dataDisk);
  EXPECT_EQ(CGameNameParser::Parse("Ashe (Data disk).d88").title, "Ashe");
}

TEST(TestGameNameParser, TellsAnotherDumpFromTheCleanOne)
{
  EXPECT_TRUE(CGameNameParser::Parse("Super Mario Bros. Special [Set 1].d88").alternate);
  EXPECT_TRUE(CGameNameParser::Parse("Super Mario Bros. Special [Alt 2].d88").alternate);
  EXPECT_TRUE(CGameNameParser::Parse("Thunder Force (SR) [Set 2] [bad sectors].d88").bad);
  EXPECT_TRUE(CGameNameParser::Parse("Gradius (Kai hack).d88").hack);
  EXPECT_TRUE(CGameNameParser::Parse("Thexder (invincibility hack) [Set 1].d88").hack);
  const ParsedGameName clean = CGameNameParser::Parse("Thexder.d88");
  EXPECT_FALSE(clean.alternate || clean.bad || clean.hack);
  EXPECT_EQ(CGameNameParser::Parse("Thexder (invincibility hack) [Set 1].d88").title, "Thexder");
}

TEST(TestGameNameParser, LeavesTheLoadingInstructionsOutOfTheTitle)
{
  EXPECT_EQ(CGameNameParser::Parse("Apploon {V1 mode, MON R G8F00}.t88").title, "Apploon");
  EXPECT_EQ(CGameNameParser::Parse("Melt Down  {4MHz}.d88").title, "Melt Down");
  EXPECT_EQ(CGameNameParser::Parse("Abyss (User disk) {V1 mode}.d88").title, "Abyss");
}

TEST(TestGameNameParser, SeparatesTheWordsOfAWhdLoadName)
{
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("SecretOfMonkeyIsland_v3.4_1625"),
            "Secret Of Monkey Island");
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("StuntCarRacerTNT_v1.3"), "Stunt Car Racer TNT");
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("SabreTeam_v1.1.lha"), "Sabre Team");
}

TEST(TestGameNameParser, KeepsAnAcronymWhole)
{
  // The split goes before the last capital of a run, not through the middle of it
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("UFOEnemyUnknown_v1.0_AGA_0157"),
            "UFO Enemy Unknown");
}

TEST(TestGameNameParser, SeparatesADigitFromTheWordBeforeIt)
{
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("Turrican3_v1.4_2633"), "Turrican 3");
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("AbandonedPlaces2_v2.0"), "Abandoned Places 2");
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("A10TankKiller_v2.0_3Disk"), "A10 Tank Killer");
}

TEST(TestGameNameParser, DropsATagThatNeverCloses)
{
  // Extracted sets routinely hit a 64 character filename limit, which cuts a
  // name mid-tag. What follows the stray bracket is half an attribute, not part
  // of the title, and keeping it would stop the whole set from matching.
  EXPECT_EQ(CGameNameParser::Parse("Bulldog (1988)(Ariolasoft UK LTD)(Tape 2.tap").title,
            "Bulldog");
  EXPECT_EQ(CGameNameParser::Parse("Samurai Trilogy (1988)(Ariolasoft UK LT2.tap").title,
            "Samurai Trilogy");
  EXPECT_EQ(CGameNameParser::Parse("Auf Wiedersehen Monty (1988)(Ariolasoft .tap").title,
            "Auf Wiedersehen Monty");
  EXPECT_EQ(CGameNameParser::Parse("Elite [cr TWD.d64").title, "Elite");
}

TEST(TestGameNameParser, LeavesEveryOtherConventionAlone)
{
  // A catalogue name has spaces, so it must never be taken for a WHDLoad slave
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("Sonic The Hedgehog (USA, Europe).md"), "");
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("Savage (1988)(Probe Software).tap"), "");
  // no version, so not a slave name
  EXPECT_EQ(CGameNameParser::ParseWhdLoadName("Turrican3.lha"), "");
}

TEST(TestGameNameParser, ReadsAWhdLoadNameThroughParse)
{
  const ParsedGameName parsed = CGameNameParser::Parse("BodyBlows_v1.2_AGA_1322");
  EXPECT_EQ(parsed.displayTitle, "Body Blows");
}

TEST(TestGameNameParser, StillReadsATosecName)
{
  const ParsedGameName parsed = CGameNameParser::Parse("Savage (1988)(Probe Software).tap");
  EXPECT_EQ(parsed.displayTitle, "Savage");
}

TEST(TestGameNameParser, CreditsAPublisherWhoseNameStartsLikeATag)
{
  const ParsedGameName parsed = CGameNameParser::Parse("Savage (1988)(Demonware).adf");
  EXPECT_EQ(parsed.status, ReleaseStatus::RETAIL);
  EXPECT_EQ(parsed.publisher, "Demonware");
  EXPECT_EQ(CGameNameParser::Parse("Savage (Demo 2).adf").status, ReleaseStatus::DEMO);
}

TEST(TestGameNameParser, LeavesADiscNumberTooLongToBeOne)
{
  const ParsedGameName parsed = CGameNameParser::Parse("Game (Disc 99999999999).cue");
  EXPECT_EQ(parsed.disc, 0);
  EXPECT_EQ(parsed.title, "Game");
}

TEST(TestGameNameParser, TellsATosecCountryFromALanguage)
{
  const ParsedGameName german = CGameNameParser::Parse("Game (1990)(Publisher)(DE).adf");
  EXPECT_EQ(german.regions, (std::vector<std::string>{"Germany"}));
  EXPECT_TRUE(german.languages.empty());

  const ParsedGameName inGerman = CGameNameParser::Parse("Game (1990)(Publisher)(de).adf");
  EXPECT_TRUE(inGerman.regions.empty());
  EXPECT_EQ(inGerman.languages, (std::vector<std::string>{"de"}));

  EXPECT_EQ(CGameNameParser::Parse("Game (Europe) (En,Fr,De).sfc").languages,
            (std::vector<std::string>{"en", "fr", "de"}));
}

TEST(TestGameNameParser, ReadsATosecNameWithNoPublisher)
{
  const ParsedGameName parsed = CGameNameParser::Parse("Game (1990)(-)(PAL).adf");
  EXPECT_TRUE(parsed.publisher.empty());
  EXPECT_TRUE(parsed.unknownTags.empty());
}

TEST(TestGameNameParser, CreditsNoCompanyFromATosecName)
{
  EXPECT_TRUE(
      CGameNameParser::Credits(CGameNameParser::Parse("Game (1993)(Team17)(AGA).adf")).empty());
  EXPECT_TRUE(CGameNameParser::Credits(CGameNameParser::Parse("Game (1993)(-)(AGA).adf")).empty());
}

TEST(TestGameNameParser, ReadsATosecCountry)
{
  const ParsedGameName parsed = CGameNameParser::Parse("1942 (1986)(Elite)(GB).tap");
  EXPECT_EQ(parsed.regions, (std::vector<std::string>{"United Kingdom"}));
  EXPECT_EQ(parsed.publisher, "Elite");
  EXPECT_TRUE(CGameNameParser::Credits(parsed).empty());
  EXPECT_EQ(CGameNameParser::Parse("Galaga (1983)(Namco)(SE).tap").regions,
            (std::vector<std::string>{"Sweden"}));
}

TEST(TestGameNameParser, ReadsATosecCountryWhoseCodeIsALanguagesToo)
{
  const ParsedGameName canada = CGameNameParser::Parse("Game (1990)(Publisher)(CA).adf");
  EXPECT_EQ(canada.regions, (std::vector<std::string>{"Canada"}));
  EXPECT_TRUE(canada.languages.empty());
  EXPECT_EQ(CGameNameParser::Parse("Game (1990)(Publisher)(AS).adf").regions,
            (std::vector<std::string>{"Asia"}));
}

TEST(TestGameNameParser, ReadsATosecTranslation)
{
  const ParsedGameName parsed = CGameNameParser::Parse("Game (1990)(Publisher)[tr de].adf");
  EXPECT_TRUE(parsed.translated);
  EXPECT_EQ(parsed.translation, "de");
}

TEST(TestGameNameParser, ReadsAPrototypesBuildDate)
{
  const ParsedGameName parsed =
      CGameNameParser::Parse("40 Winks (Europe) (En,Es,It) (Proto) (1999-10-07).z64");
  EXPECT_EQ(parsed.year, 1999);
  EXPECT_EQ(parsed.status, ReleaseStatus::PROTOTYPE);
  EXPECT_FALSE(parsed.tosec);
}

TEST(TestGameNameParser, CreditsNoCompanyForWhatACartridgeWorksWith)
{
  EXPECT_TRUE(
      CGameNameParser::Parse("Balloon Fight GB (Japan) (SGB Enhanced, GB Compatible) (NP).zip")
          .unknownTags.empty());
  EXPECT_TRUE(CGameNameParser::Parse("0002 Need for Speed - Underground 2 (US)(M5).zip")
                  .unknownTags.empty());
}

TEST(TestGameNameParser, ReadsAGoodToolsCountryOnlyWhereGoodToolsWritesIt)
{
  EXPECT_EQ(
      CGameNameParser::Parse("Corpse Killer - Graveyard Edition (USA) (Disc 1) (R).cue").regions,
      (std::vector<std::string>{"USA"}));
}

TEST(TestGameNameParser, RestoresAnArticleTheSetsMove)
{
  EXPECT_EQ(CGameNameParser::DisplayTitle("Magische Eenhoorn, De"), "De Magische Eenhoorn");
  EXPECT_EQ(CGameNameParser::DisplayTitle("Genio de las Palabras, El"), "El Genio de las Palabras");
}

TEST(TestGameNameParser, KeepsATitleThatStartsWithANumber)
{
  EXPECT_EQ(CGameNameParser::Parse("1943 Kai (Japan).zip").title, "1943 Kai");
  EXPECT_EQ(CGameNameParser::Parse("007 - The World Is Not Enough (USA).z64").title,
            "007 - The World Is Not Enough");
  EXPECT_EQ(CGameNameParser::Parse("2002 FIFA World Cup (USA) (En,Es).cue").title,
            "2002 FIFA World Cup");
  EXPECT_EQ(CGameNameParser::Parse("1942 (1986)(Elite)(GB).tap").title, "1942");
  EXPECT_EQ(CGameNameParser::Parse("1943 Kai (1991)(Namco)(JP).pce").title, "1943 Kai");
  EXPECT_EQ(CGameNameParser::Parse("2010 - The Graphic Action Game.col").title,
            "2010 - The Graphic Action Game");
  EXPECT_EQ(CGameNameParser::Parse("1942 (US).zip").title, "1942");
  EXPECT_EQ(CGameNameParser::Parse("1943 Kai (JP).zip").title, "1943 Kai");
  EXPECT_EQ(CGameNameParser::Parse("2002 FIFA World Cup (US).zip").title, "2002 FIFA World Cup");
}

TEST(TestGameNameParser, KeepsTheDotsInAFoldersName)
{
  EXPECT_EQ(CGameNameParser::Parse("G.R", false).title, "G.R");
  EXPECT_EQ(CGameNameParser::Parse("Flight Simulator Ver.5.0", false).title,
            "Flight Simulator Ver.5.0");
  EXPECT_EQ(CGameNameParser::Parse("G.R").title, "G");
}

TEST(TestGameNameParser, CreditsTheCompanyInATagOfItsOwn)
{
  EXPECT_EQ(CGameNameParser::Credits(CGameNameParser::Parse("Tetris (Doujin - Noripy)", false)),
            std::vector<std::string>{"Noripy"});
  EXPECT_EQ(CGameNameParser::Credits(
                CGameNameParser::Parse("Teki wa Kaizoku - Kaizokuban (D-Photon - Victor)", false)),
            (std::vector<std::string>{"D-Photon", "Victor"}));
  EXPECT_TRUE(CGameNameParser::Credits(CGameNameParser::Parse("Sonic (USA, Europe).md")).empty());
  EXPECT_TRUE(
      CGameNameParser::Credits(CGameNameParser::Parse("Elite (1985)(Firebird)[a].d64")).empty());
  // A flag in square brackets describes the dump
  EXPECT_TRUE(CGameNameParser::Credits(CGameNameParser::Parse("Artemis [HD].zip")).empty());
  EXPECT_TRUE(CGameNameParser::Credits(CGameNameParser::Parse("Mirage [extras].zip")).empty());
  // A company whose name starts like a disc label is still a company
  EXPECT_EQ(CGameNameParser::Credits(CGameNameParser::Parse("Mirage (Discovery)", false)),
            std::vector<std::string>{"Discovery"});
}

TEST(TestGameNameParser, TellsCompaniesApart)
{
  EXPECT_TRUE(CGameNameParser::SameCompany("Micro Cabin", "Microcabin"));
  EXPECT_TRUE(CGameNameParser::SameCompany("Riverhill Soft", "Riverhillsoft"));
  EXPECT_TRUE(CGameNameParser::SameCompany("Onion Soft", "Doujin - Onion Soft"));
  EXPECT_FALSE(CGameNameParser::SameCompany("Noripy", "BPS"));
  EXPECT_FALSE(CGameNameParser::SameCompany("Apollo Technica", "Tsukumo"));
}

TEST(TestGameNameParser, ReadsANumberTooLargeToBeOne)
{
  EXPECT_NO_THROW(CGameNameParser::Parse("Game (U) [h999999999999].nes"));
  EXPECT_TRUE(CGameNameParser::Parse("Game (U) [h999999999999].nes").hack);
  EXPECT_NO_THROW(
      CGameNameParser::Parse("Game (1990)(Publisher)(M999999999999)[a99999999999].adf"));
  EXPECT_TRUE(
      CGameNameParser::Parse("Game (1990)(Publisher)(M999999999999)[a99999999999].adf").alternate);
}

TEST(TestGameNameParser, ReadsEachConventionsCodeForACountry)
{
  EXPECT_EQ(CGameNameParser::Parse("Game (D) [!].nes").regions,
            (std::vector<std::string>{"Netherlands"}));
  EXPECT_EQ(CGameNameParser::Parse("Game (Nl) [!].nes").regions,
            (std::vector<std::string>{"Netherlands"}));
  EXPECT_EQ(CGameNameParser::Parse("0822 Every Day DS Brain Training (KS).zip").regions,
            (std::vector<std::string>{"Korea"}));
}

TEST(TestGameNameParser, RestoresTheArticleOfATosecTitle)
{
  EXPECT_EQ(CGameNameParser::Parse("Legend of TOSEC, The (1986)(Publisher).adf").displayTitle,
            "The Legend of TOSEC");
}
