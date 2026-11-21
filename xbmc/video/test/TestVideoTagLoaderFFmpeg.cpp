/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/Scraper.h"
#include "cores/FFmpeg.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/EmbeddedArt.h"
#include "utils/URIUtils.h"
#include "video/VideoEmbeddedImageFileLoader.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderFFmpeg.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
constexpr const char* SCRAPER_ID = "metadata.test.videotagloaderffmpeg";

constexpr const char* SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videotagloaderffmpeg" name="Test scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

constexpr const char* URL = "http://imdb.invalid/title/tt0111161/";

//! The threads the stand-in for python ran the scraper on
struct SRuns
{
  std::mutex lock;
  std::vector<int> threads;
};

//! Stands in for the python interpreter running a movie scraper, which recognises a url to
//! imdb.invalid in an nfo, as the IMDb scraper recognises its own
class CNfoUrlInvoker : public ILanguageInvoker
{
public:
  CNfoUrlInvoker(ILanguageInvocationHandler* handler, SRuns& runs)
    : ILanguageInvoker(handler),
      m_runs(runs)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    {
      std::unique_lock lock(m_runs.lock);
      m_runs.threads.push_back(GetId());
    }

    // As a plugin is run: its url, handle and options
    const int handle{std::stoi(arguments[1])};
    if (CURL{arguments[0] + arguments[2]}.GetOption("nfo").find(URL) != std::string::npos)
    {
      CFileItem url{URL, false};
      XFILE::CPluginDirectory::AddItem(handle, &url, 1);
    }
    XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);

    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool abort) override { return true; }

private:
  SRuns& m_runs;
};

class CNfoUrlHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CNfoUrlInvoker>(this, m_runs);
  }

  SRuns m_runs;
};

//! A png of one pixel
constexpr uint8_t POSTER[]{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
                           0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                           0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00,
                           0x0D, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
                           0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49,
                           0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

//! An mp4 or mkv holding a subtitle track with no samples, and the tags a test gives it. An mp4's
//! tags are written as named metadata, which keeps a track as the text given, rather than as the
//! number the iTunes track tag holds.
class TestVideoTagLoaderFFmpeg : public ::testing::Test
{
protected:
  void SetUp() override { m_useTags = Settings()->GetBool(CSettings::SETTING_MYVIDEOS_USETAGS); }

  void TearDown() override
  {
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_USETAGS, m_useTags);
    XFILE::CFile::Delete(m_path);
    XFILE::CFile::Delete(m_m4v);
    XFILE::CFile::Delete(m_nfo);
    XFILE::CFile::Delete(m_mkv);
    if (m_registered)
    {
      std::unique_lock lock(m_handler.m_runs.lock);
      const auto end{std::chrono::steady_clock::now() + 5s};
      for (const int id : m_handler.m_runs.threads)
      {
        while (Scripts().IsRunning(id) && std::chrono::steady_clock::now() < end)
          std::this_thread::sleep_for(1ms);
        EXPECT_FALSE(Scripts().IsRunning(id));
      }
      Scripts().UnregisterLanguageInvocationHandler(&m_handler);
      CServiceBroker::GetAddonMgr().UnloadAddon(SCRAPER_ID);
    }
    XFILE::CDirectory::RemoveRecursive(m_scraper);
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
  }

  static CScriptInvocationManager& Scripts() { return CScriptInvocationManager::GetInstance(); }

  bool WriteMp4(const char* track) { return WriteMp4(track, m_path); }

  bool WriteMp4(const char* track, const std::string& path)
  {
    AVFormatContext* context{nullptr};
    if (avformat_alloc_output_context2(&context, nullptr, "mp4", path.c_str()) < 0)
      return false;

    AVStream* stream{avformat_new_stream(context, nullptr)};
    stream->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
    stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
    stream->time_base = {1, 1000};
    av_dict_set(&context->metadata, "title", "Film", 0);
    av_dict_set(&context->metadata, "track", track, 0);

    AVDictionary* options{nullptr};
    av_dict_set(&options, "movflags", "use_metadata_tags", 0);
    const bool written{avio_open(&context->pb, path.c_str(), AVIO_FLAG_WRITE) >= 0 &&
                       avformat_write_header(context, &options) >= 0 &&
                       av_write_trailer(context) >= 0};
    av_dict_free(&options);
    avio_closep(&context->pb);
    avformat_free_context(context);
    return written;
  }

  //! An mp4 with a title and a poster, written as iTunes tags, as only they hold a poster
  bool WriteMp4WithPoster()
  {
    AVFormatContext* context{nullptr};
    if (avformat_alloc_output_context2(&context, nullptr, "mp4", m_path.c_str()) < 0)
      return false;

    AVStream* stream{avformat_new_stream(context, nullptr)};
    stream->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
    stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
    stream->time_base = {1, 1000};
    AVStream* poster{avformat_new_stream(context, nullptr)};
    poster->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    poster->codecpar->codec_id = AV_CODEC_ID_PNG;
    poster->codecpar->width = 1;
    poster->codecpar->height = 1;
    poster->disposition = AV_DISPOSITION_ATTACHED_PIC;
    av_dict_set(&context->metadata, "title", "Film", 0);

    uint8_t picture[sizeof(POSTER)];
    std::memcpy(picture, POSTER, sizeof(POSTER));
    AVPacket* packet{av_packet_alloc()};
    packet->data = picture;
    packet->size = sizeof(picture);
    packet->stream_index = poster->index;
    packet->pts = 0;
    packet->dts = 0;
    packet->flags = AV_PKT_FLAG_KEY;
    const bool written{avio_open(&context->pb, m_path.c_str(), AVIO_FLAG_WRITE) >= 0 &&
                       avformat_write_header(context, nullptr) >= 0 &&
                       av_write_frame(context, packet) >= 0 && av_write_trailer(context) >= 0};
    av_packet_free(&packet);
    avio_closep(&context->pb);
    avformat_free_context(context);
    return written;
  }

  //! An mkv with a tag, or with an attachment holding an nfo to override a scraper's details
  bool WriteMkv(const char* key, const char* value, const std::string& overrideNfo = {})
  {
    AVFormatContext* context{nullptr};
    if (avformat_alloc_output_context2(&context, nullptr, "matroska", m_mkv.c_str()) < 0)
      return false;

    AVStream* stream{avformat_new_stream(context, nullptr)};
    stream->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
    stream->codecpar->codec_id = AV_CODEC_ID_SUBRIP;
    stream->time_base = {1, 1000};
    if (key)
      av_dict_set(&context->metadata, key, value, 0);

    if (!overrideNfo.empty())
    {
      AVStream* attachment{avformat_new_stream(context, nullptr)};
      attachment->codecpar->codec_type = AVMEDIA_TYPE_ATTACHMENT;
      attachment->codecpar->extradata =
          static_cast<uint8_t*>(av_mallocz(overrideNfo.size() + AV_INPUT_BUFFER_PADDING_SIZE));
      std::memcpy(attachment->codecpar->extradata, overrideNfo.data(), overrideNfo.size());
      attachment->codecpar->extradata_size = static_cast<int>(overrideNfo.size());
      av_dict_set(&attachment->metadata, "filename", "kodi-override-metadata", 0);
      av_dict_set(&attachment->metadata, "mimetype", "text/xml", 0);
    }

    // A subtitle, as the demuxer reading an mkv without a cluster takes the ids of its seek head
    // for elements, and carries on looking for one
    uint8_t text[]{'F', 'i', 'l', 'm'};
    AVPacket* packet{av_packet_alloc()};
    packet->data = text;
    packet->size = sizeof(text);
    packet->pts = 0;
    packet->dts = 0;
    packet->duration = 1000;
    const bool written{avio_open(&context->pb, m_mkv.c_str(), AVIO_FLAG_WRITE) >= 0 &&
                       avformat_write_header(context, nullptr) >= 0 &&
                       av_write_frame(context, packet) >= 0 && av_write_trailer(context) >= 0};
    av_packet_free(&packet);
    avio_closep(&context->pb);
    avformat_free_context(context);
    return written;
  }

  //! The scraper a scan of the mkv's folder would use. Its script is run by the stand-in for
  //! python, which the tests don't have.
  ADDON::ScraperPtr Scraper()
  {
    if (Scripts().HasLanguageInvoker("scraper.py"))
      return {};
    Scripts().RegisterLanguageInvocationHandler(&m_handler, ".py");
    m_registered = true;

    static_cast<void>(
        XFILE::CDirectory::Create(CSpecialProtocol::TranslatePath("special://home/addons/")));
    if (!XFILE::CDirectory::Create(m_scraper))
      return {};
    for (const auto& [name, content] :
         {std::pair{"addon.xml", SCRAPER_ADDON}, std::pair{"scraper.py", ""}})
    {
      XFILE::CFile file;
      if (!file.OpenForWrite(URIUtils::AddFileToFolder(m_scraper, name), true))
        return {};
      file.Write(content, std::strlen(content));
    }
    ADDON::AddonPtr addon;
    if (!CServiceBroker::GetAddonMgr().FindAddon(SCRAPER_ID, ADDON::ORIGIN_SYSTEM,
                                                 ADDON::CAddonVersion{"1.0.0"}) ||
        !CServiceBroker::GetAddonMgr().GetAddon(SCRAPER_ID, addon, ADDON::OnlyEnabled::CHOICE_YES))
      return {};
    return std::dynamic_pointer_cast<ADDON::CScraper>(addon);
  }

  const std::string m_path{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderFFmpeg.mp4")};
  const std::string m_m4v{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderFFmpeg.m4v")};
  const std::string m_nfo{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderFFmpeg.nfo")};
  const std::string m_mkv{
      CSpecialProtocol::TranslatePath("special://temp/TestVideoTagLoaderFFmpeg.mkv")};
  const std::string m_scraper{
      CSpecialProtocol::TranslatePath(std::string{"special://home/addons/"} + SCRAPER_ID + "/")};
  bool m_useTags{false};
  CNfoUrlHandler m_handler;
  bool m_registered{false};
};
} // namespace

TEST_F(TestVideoTagLoaderFFmpeg, AnMp4TrackTagIsReadAsANumber)
{
  ASSERT_TRUE(WriteMp4("3/12"));
  const CFileItem item{m_path, false};
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::TITLE, loader.Load(tag, false, nullptr));

  EXPECT_EQ("Film", tag.m_strTitle);
  EXPECT_EQ(3, tag.m_iTrack);
}

// An m4v is an mp4 by another name, whose tags are read as an mp4's are
TEST_F(TestVideoTagLoaderFFmpeg, AnM4vIsReadAsAnMp4)
{
  ASSERT_TRUE(WriteMp4("3/12", m_m4v));
  const CFileItem item{m_m4v, false};
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::TITLE, loader.Load(tag, false, nullptr));

  EXPECT_EQ("Film", tag.m_strTitle);
  EXPECT_EQ(3, tag.m_iTrack);
}

TEST_F(TestVideoTagLoaderFFmpeg, AnMp4TrackTagThatIsNotANumberIsPassedOver)
{
  ASSERT_TRUE(WriteMp4("Side A"));
  const CFileItem item{m_path, false};
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::TITLE, loader.Load(tag, false, nullptr));

  EXPECT_EQ("Film", tag.m_strTitle);
  EXPECT_EQ(-1, tag.m_iTrack);
}

// The url an mkv's tag names is given to the scraper to recognise, not downloaded as an nfo
TEST_F(TestVideoTagLoaderFFmpeg, AnMkvsUrlTagIsGivenToItsScraper)
{
  ASSERT_TRUE(WriteMkv("IMDBURL", URL));
  const ADDON::ScraperPtr scraper{Scraper()};
  ASSERT_TRUE(scraper);
  const CFileItem item{m_mkv, false};
  CVideoTagLoaderFFmpeg loader{item, scraper, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::URL, loader.Load(tag, false, nullptr));
  ASSERT_TRUE(loader.ScraperUrl().HasUrls());
  EXPECT_EQ(URL, loader.ScraperUrl().GetFirstUrlByType().m_url);
}

// The nfo an mkv holds to override a scraper's details is read, with the url it names, both as the
// file is read and again over the scraper's details
TEST_F(TestVideoTagLoaderFFmpeg, AnMkvsOverrideNfoIsRead)
{
  ASSERT_TRUE(
      WriteMkv(nullptr, nullptr, std::string{"<movie><title>Override</title></movie>\n"} + URL));
  const ADDON::ScraperPtr scraper{Scraper()};
  ASSERT_TRUE(scraper);
  const CFileItem item{m_mkv, false};
  CVideoTagLoaderFFmpeg loader{item, scraper, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::COMBINED, loader.Load(tag, false, nullptr));
  ASSERT_TRUE(loader.ScraperUrl().HasUrls());
  EXPECT_EQ(URL, loader.ScraperUrl().GetFirstUrlByType().m_url);
  EXPECT_EQ("Override", tag.m_strTitle);

  CVideoInfoTag scraped;
  scraped.SetTitle("Scraped");
  scraped.SetPlot("From the scraper");
  EXPECT_EQ(CInfoScanner::InfoType::COMBINED, loader.Load(scraped, true, nullptr));
  EXPECT_EQ("Override", scraped.m_strTitle);
  EXPECT_EQ("From the scraper", scraped.m_strPlot);
}

// An mkv's override nfo naming no url is still one to override a scraper's details with, rather
// than a full nfo of its own
TEST_F(TestVideoTagLoaderFFmpeg, AnMkvsOverrideNfoNamingNoUrlIsAnOverride)
{
  ASSERT_TRUE(WriteMkv(nullptr, nullptr, "<movie><title>Override</title></movie>"));
  const ADDON::ScraperPtr scraper{Scraper()};
  ASSERT_TRUE(scraper);
  const CFileItem item{m_mkv, false};
  CVideoTagLoaderFFmpeg loader{item, scraper, false};
  ASSERT_TRUE(loader.HasInfo());

  CVideoInfoTag tag;
  EXPECT_EQ(CInfoScanner::InfoType::OVERRIDE, loader.Load(tag, false, nullptr));
  EXPECT_FALSE(loader.ScraperUrl().HasUrls());
  EXPECT_EQ("Override", tag.m_strTitle);
}

// The poster an mp4 holds is loaded with an nfo beside it, which holds none, as when a library
// exported to nfos has its thumbnails cleared
TEST_F(TestVideoTagLoaderFFmpeg, AnMp4sPosterLoadsWithAnNfoBesideIt)
{
  // As when the library was scanned and the poster found
  Settings()->SetBool(CSettings::SETTING_MYVIDEOS_USETAGS, true);
  ASSERT_TRUE(WriteMp4WithPoster());
  XFILE::CFile nfo;
  ASSERT_TRUE(nfo.OpenForWrite(m_nfo, true));
  const std::string movie{"<movie><title>Film</title></movie>"};
  nfo.Write(movie.data(), movie.size());
  nfo.Close();

  EmbeddedArt art;
  ASSERT_TRUE(KODI::VIDEO::CVideoEmbeddedImageFileLoader::GetEmbeddedArt(m_path, "poster", art));
  EXPECT_EQ("poster", art.m_type);
  ASSERT_EQ(sizeof(POSTER), art.m_size);
  EXPECT_EQ(0, std::memcmp(POSTER, art.m_data.data(), sizeof(POSTER)));
}

// A file that is gone holds no poster, as when its library item's poster is looked for after
TEST_F(TestVideoTagLoaderFFmpeg, AFileThatIsGoneHoldsNoPoster)
{
  EmbeddedArt art;
  EXPECT_FALSE(KODI::VIDEO::CVideoEmbeddedImageFileLoader::GetEmbeddedArt(m_path, "poster", art));
}

// A lookup that finds no image says so, though the art it is given holds one found before
TEST_F(TestVideoTagLoaderFFmpeg, ALookupFindingNoImageSaysSoThoughItsArtHoldsOne)
{
  Settings()->SetBool(CSettings::SETTING_MYVIDEOS_USETAGS, true);
  ASSERT_TRUE(WriteMp4WithPoster());
  EmbeddedArt art;
  ASSERT_TRUE(KODI::VIDEO::CVideoEmbeddedImageFileLoader::GetEmbeddedArt(m_path, "poster", art));

  EXPECT_FALSE(KODI::VIDEO::CVideoEmbeddedImageFileLoader::GetEmbeddedArt(m_path, "fanart", art));
}
