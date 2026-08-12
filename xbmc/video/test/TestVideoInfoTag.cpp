/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/LanguageTag.h"
#include "utils/SortUtils.h"
#include "utils/StreamDetails.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/VideoInfoTag.h"

#include <map>
#include <string>

#include <gtest/gtest.h>

using KODI::UTILS::CLanguageTag;

TEST(TestVideoInfoTag, ReadTVShowSeasons)
{
  const std::string document =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <tvshow>
         <namedseason number="1">season 1</namedseason>
         <namedseason number="2"></namedseason>
         <namedseason number="3"></namedseason>
         <namedseason number="4">season 4</namedseason>
         <seasonplot number="3">plot 3</seasonplot>
         <seasonplot number="4">plot 4</seasonplot>
         <seasonplot number="5"></seasonplot>
         </tvshow>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", ""}}, {3, {"", "plot 3"}}, {4, {"season 4", "plot 4"}}};

  EXPECT_EQ(details.m_seasons, reference);
}

TEST(TestVideoInfoTag, ReadStreamDetailFlags)
{
  const std::string document =
      R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
         <movie>
         <fileinfo>
         <streamdetails>
         <audio><codec>dts</codec><language>eng</language><channels>6</channels>
           <flags><flag>default</flag><flag>original</flag></flags></audio>
         <audio><codec>ac3</codec><language>fre</language><channels>2</channels></audio>
         <audio><codec>aac</codec><language>ger</language><channels>2</channels>
           <flags><flag>  Forced  </flag><flag>notaflag</flag><flag></flag></flags></audio>
         <subtitle><language>eng</language><flags><flag>forced</flag></flags></subtitle>
         <subtitle><language>fre</language></subtitle>
         <subtitle><language>ger</language><flags></flags></subtitle>
         </streamdetails>
         </fileinfo>
         </movie>)";

  CXBMCTinyXML doc;
  doc.Parse(document, TIXML_ENCODING_UNKNOWN);

  CVideoInfoTag details;
  EXPECT_TRUE(details.Load(doc.RootElement(), true, false));

  const CStreamDetails& streams = details.m_streamDetails;
  ASSERT_EQ(3, streams.GetAudioStreamCount());
  ASSERT_EQ(3, streams.GetSubtitleStreamCount());

  EXPECT_EQ(StreamFlags::FLAG_DEFAULT | StreamFlags::FLAG_ORIGINAL, streams.GetAudioFlags(1));
  EXPECT_EQ(StreamFlags::FLAG_FORCED, streams.GetSubtitleFlags(1));

  // An NFO written before flags existed has no <flags> element at all, which must
  // read back as no flags rather than leaving the field uninitialised.
  EXPECT_EQ(StreamFlags::FLAG_NONE, streams.GetAudioFlags(2));
  EXPECT_EQ(StreamFlags::FLAG_NONE, streams.GetSubtitleFlags(2));

  // Names are trimmed and case-insensitive; a name this build doesn't know, and an empty
  // one, are skipped rather than failing the whole stream.
  EXPECT_EQ(StreamFlags::FLAG_FORCED, streams.GetAudioFlags(3));

  // An empty <flags> block is a positive statement that the stream has no flags.
  EXPECT_EQ(StreamFlags::FLAG_NONE, streams.GetSubtitleFlags(3));
}

TEST(TestVideoInfoTag, WriteStreamDetailFlags)
{
  // Flags survive an export/import cycle, so a library rebuilt from exported NFOs
  // keeps them.
  AudioStreamInfo audioInfo;
  audioInfo.codecName = "dts";
  audioInfo.language = CLanguageTag::Parse("eng");
  audioInfo.channels = 6;
  audioInfo.flags =
      static_cast<StreamFlags>(StreamFlags::FLAG_DEFAULT | StreamFlags::FLAG_ORIGINAL);

  SubtitleStreamInfo subtitleInfo;
  subtitleInfo.language = CLanguageTag::Parse("eng");
  subtitleInfo.flags = StreamFlags::FLAG_FORCED;

  CVideoInfoTag details;
  details.m_streamDetails.AddStream(new CStreamDetailAudio(audioInfo, CStreamDetail::MEDIA));
  details.m_streamDetails.AddStream(new CStreamDetailSubtitle(subtitleInfo, CStreamDetail::MEDIA));
  details.m_streamDetails.DetermineBestStreams();

  CXBMCTinyXML xmlDoc;
  ASSERT_TRUE(details.Save(&xmlDoc, "movie"));

  CVideoInfoTag reloaded;
  ASSERT_TRUE(reloaded.Load(xmlDoc.RootElement(), true, false));

  EXPECT_EQ(audioInfo.flags, reloaded.m_streamDetails.GetAudioFlags(1));
  EXPECT_EQ(subtitleInfo.flags, reloaded.m_streamDetails.GetSubtitleFlags(1));
}

// Trick to make protected methods accessible for testing
class CVideoInfoTagTest : public CVideoInfoTag
{
public:
  bool ForwardSaveTvShowSeasons(TiXmlNode* node) { return SaveTvShowSeasons(node); }
};

TEST(TestVideoInfoTag, SaveTVShowSeasons)
{
  const std::map<int, CVideoInfoTag::SeasonAttributes> reference = {
      {1, {"season 1", "plot 1"}}, {2, {"", "plot 2"}}, {3, {"season 3", ""}}, {4, {"", ""}}};

  const std::string referenceXml = R"(<namedseason number="1">season 1</namedseason>
<seasonplot number="1">plot 1</seasonplot>
<seasonplot number="2">plot 2</seasonplot>
<namedseason number="3">season 3</namedseason>
)";

  CVideoInfoTagTest details;
  details.SetSeasons(reference);

  CXBMCTinyXML xmlDoc;
  details.ForwardSaveTvShowSeasons(&xmlDoc);

  EXPECT_EQ(referenceXml, XMLUtils::NodeStringSerialization(xmlDoc.RootElement(),
                                                            XMLUtils::SerializationFormat::PRETTY));
}

TEST(TestVideoInfoTag, SetUniqueIDs)
{
  // initial state: no default, empty list.
  CVideoInfoTag details;
  std::map<std::string, std::string, std::less<>> reference = {};

  EXPECT_EQ(details.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // usual flow: initialize from initial state with a list.
  // entries with blank type or uniqueid are ignored
  std::map<std::string, std::string, std::less<>> test = {
      {"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}, {"", "123456"}, {"foo", ""}};
  reference = {{"imdb", "tt4577466"}, {"tmdb", "64043"}, {"tvdb", "299350"}};

  details.SetUniqueIDs(test);
  details.SetUniqueID("64043", "tmdb", true);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // current update behavior, not sure why:
  // the former default type and value from the previous list of uniqueids are added back when
  // omitted from the new list - instead of reverting to "unknown" default and setting the list as provided.
  test = {{"imdb", "tt4577466"}, {"tvdb", "299350"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // setting a blank list clears all except the previous default
  test = {};
  reference = {{"tmdb", "64043"}};
  details.SetUniqueIDs(test);

  EXPECT_EQ(details.GetDefaultUniqueID(), "tmdb");
  EXPECT_EQ(details.GetUniqueIDs(), reference);

  // except when there is no explicit default, then setting a blank list clears the list.
  CVideoInfoTag details2;
  details2.SetUniqueIDs(reference);
  details2.SetUniqueIDs(test);

  EXPECT_EQ(details2.GetDefaultUniqueID(), "unknown");
  EXPECT_EQ(details2.GetUniqueIDs(), test);
}

struct TestOriginalLanguage
{
  std::string input;
  std::string expected;
  CVideoInfoTag::LanguageTagSource source = CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL;
  bool status = true;
};

std::ostream& operator<<(std::ostream& os, const TestOriginalLanguage& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestOriginalLanguage OriginalLanguageTests[] = {
    {"en", "en", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"foobarbaz", "foobarbaz", CVideoInfoTag::LanguageTagSource::SOURCE_INTERNAL},
    {"en", "en"}, // ISO 639-1
    {"eng", "en"}, // ISO 639-2
    {"fra", "fr"}, // ISO 639-2/T
    {"fre", "fr"}, // ISO 639-2/B
    {"en-US", "en-US"}, // BCP 47 lang-region
    {"zh-guoyu", "zh-guoyu"}, // Grandfathered BCP 47
    // Future: expected to be rewritten to the preferred language defined in the registry
    // Other tests for canonicalization will be needed as well
    {"english", "en"}, // English name
    {"foobarbaz", "", CVideoInfoTag::LanguageTagSource::SOURCE_EXTERNAL, false}, // Unknown English name
};
// clang-format on

class OriginalLanguageTester : public testing::Test,
                               public testing::WithParamInterface<TestOriginalLanguage>
{
};

TEST_P(OriginalLanguageTester, SetOriginalLanguage)
{
  auto& param = GetParam();

  CVideoInfoTag tag;
  bool status = tag.SetOriginalLanguage(param.input, param.source);
  EXPECT_EQ(param.status, status);
  if (status)
  {
    // { required to quiet clang warning about dangling else
    EXPECT_EQ(param.expected, tag.GetOriginalLanguage());
  }
}

INSTANTIATE_TEST_SUITE_P(TestVideoInfoTag,
                         OriginalLanguageTester,
                         testing::ValuesIn(OriginalLanguageTests));

// Sorting a list by an audio field must order it by the stream the list displays, which is the
// stream playback will start with, not by the technically best stream that is not shown.
class AudioSortKeyTester : public testing::Test
{
protected:
  void SetUp() override
  {
    m_settingOriginal = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_LOCALE_AUDIOLANGUAGE);
    m_audioLanguageOriginal = g_langInfo.GetAudioLanguage(false).AsBcp47();
  }

  void TearDown() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(
        CSettings::SETTING_LOCALE_AUDIOLANGUAGE, m_settingOriginal);
    g_langInfo.SetAudioLanguage(m_audioLanguageOriginal);
  }

  static void PreferLanguage(const std::string& language)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(
        CSettings::SETTING_LOCALE_AUDIOLANGUAGE, language);
    g_langInfo.SetAudioLanguage(language);
  }

  // A German TrueHD 7.1 track that outranks an English AC3 5.1 one on quality alone
  static CVideoInfoTag MakeTagWithTwoAudioStreams()
  {
    CVideoInfoTag tag;
    for (const auto& [language, codec, channels] :
         {std::tuple{"ger", "truehd", 8}, std::tuple{"eng", "ac3", 6}})
    {
      auto* audio = new CStreamDetailAudio();
      audio->m_strLanguage = language;
      audio->m_strCodec = codec;
      audio->m_iChannels = channels;
      audio->SetSource(CStreamDetail::MEDIA);
      tag.m_streamDetails.AddStream(audio);
    }
    tag.m_streamDetails.DetermineBestStreams();
    return tag;
  }

  std::string m_settingOriginal;
  std::string m_audioLanguageOriginal;
};

TEST_F(AudioSortKeyTester, OrdersByThePreferredLanguageStream)
{
  const CVideoInfoTag tag{MakeTagWithTwoAudioStreams()};

  // The technically best stream is the German one, so that is what the sort key used to be
  ASSERT_EQ("truehd", tag.m_streamDetails.GetAudioCodec());

  PreferLanguage("eng");

  SortItem sortable;
  tag.ToSortable(sortable, Field::AUDIO_CODEC);
  EXPECT_EQ("ac3", sortable[Field::AUDIO_CODEC].asString());

  tag.ToSortable(sortable, Field::AUDIO_CHANNELS);
  EXPECT_EQ(6, sortable[Field::AUDIO_CHANNELS].asInteger());

  tag.ToSortable(sortable, Field::AUDIO_LANGUAGE);
  EXPECT_EQ("eng", sortable[Field::AUDIO_LANGUAGE].asString());
}

TEST_F(AudioSortKeyTester, FallsBackToTheBestStreamWithoutALanguagePreference)
{
  const CVideoInfoTag tag{MakeTagWithTwoAudioStreams()};

  PreferLanguage("mediadefault");

  SortItem sortable;
  tag.ToSortable(sortable, Field::AUDIO_CODEC);
  EXPECT_EQ("truehd", sortable[Field::AUDIO_CODEC].asString());
}
