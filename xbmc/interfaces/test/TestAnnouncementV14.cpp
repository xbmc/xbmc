/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "interfaces/AnnouncementEvents.h"
#include "interfaces/AnnouncementV14.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "settings/lib/SettingLevel.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace ANNOUNCEMENT;
using namespace std::chrono_literals;

/*
 * JSON-RPC v14 clients receive these as notifications and Python add-ons as onNotification data,
 * so these expectations are a contract: nothing may move, be renamed, be removed or change type.
 */
namespace
{
constexpr auto AUDIO = KODI::MEDIA::Streams::Audio;
constexpr auto VIDEO = KODI::MEDIA::Streams::Video;
constexpr auto VIDEO_AND_AUDIO = KODI::MEDIA::Streams::VideoAndAudio;

//! Compared as JSON text, the form the clients receive; the parser reads numbers as unsigned.
void ExpectNotification(const Announcement& announcement,
                        AnnouncementFlag flag,
                        const std::string& message,
                        const std::string& expected)
{
  const std::vector<V14::Notification> notifications = V14::NotificationsOf(announcement);
  ASSERT_EQ(1U, notifications.size());
  EXPECT_EQ(flag, notifications[0].flag);
  EXPECT_EQ("xbmc", notifications[0].sender);
  EXPECT_EQ(message, notifications[0].message);

  CVariant parsed;
  ASSERT_TRUE(CJSONVariantParser::Parse(expected, parsed)) << "invalid test JSON: " << expected;
  std::string expectedJson;
  std::string actualJson;
  CJSONVariantWriter::Write(parsed, expectedJson, true);
  CJSONVariantWriter::Write(notifications[0].data, actualJson, true);
  EXPECT_EQ(expectedJson, actualJson);
}

template<typename FlagEvent>
void Expect(const FlagEvent& event, const std::string& message, const std::string& expected)
{
  ExpectNotification(event, FlagEvent::FLAG, message, expected);
}

void ExpectItem(const std::shared_ptr<CFileItem>& item, const std::string& expected)
{
  Expect(PlaylistEvent{EVENT::PLAYLIST::Add{KODI::PLAYLIST::Type::Video, 0, item}}, "OnAdd",
         R"({"item":)" + expected + R"(,"playlistid":1,"position":0})");
}
} // unnamed namespace

TEST(TestAnnouncementV14, PlayerTransport)
{
  using namespace EVENT::PLAYER;
  Expect(PlayerEvent{Play{nullptr, 1, VIDEO}}, "OnPlay", R"({"player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{Play{nullptr, 1, VIDEO_AND_AUDIO}}, "OnPlay",
         R"({"player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{Play{nullptr, 1, AUDIO}}, "OnPlay", R"({"player":{"playerid":0,"speed":1}})");
  Expect(PlayerEvent{AVStart{nullptr, AUDIO}}, "OnAVStart",
         R"({"player":{"playerid":0,"speed":1}})");
  Expect(PlayerEvent{AVChange{nullptr, VIDEO}}, "OnAVChange",
         R"({"player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{Pause{nullptr, VIDEO}}, "OnPause", R"({"player":{"playerid":1,"speed":0}})");
  Expect(PlayerEvent{Resume{nullptr, VIDEO}}, "OnResume", R"({"player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{SpeedChanged{nullptr, 4, VIDEO}}, "OnSpeedChanged",
         R"({"player":{"playerid":1,"speed":4}})");
  Expect(PlayerEvent{Seek{nullptr, 1, 3723004ms, 1000ms, VIDEO}}, "OnSeek",
         R"({"player":{"playerid":1,
             "seekoffset":{"hours":0,"milliseconds":0,"minutes":0,"seconds":1},
             "speed":1,
             "time":{"hours":1,"milliseconds":4,"minutes":2,"seconds":3}}})");
}

TEST(TestAnnouncementV14, PlayerStop)
{
  using namespace EVENT::PLAYER;
  Expect(PlayerEvent{Stop{nullptr, false, std::nullopt}}, "OnStop", R"({"end":false})");
  Expect(PlayerEvent{Stop{nullptr, true, std::nullopt}}, "OnStop", R"({"end":true})");
  // only the slideshow names its streams
  Expect(PlayerEvent{Stop{nullptr, true, VIDEO}}, "OnStop",
         R"({"end":true,"player":{"playerid":2}})");
}

TEST(TestAnnouncementV14, ASlideIsPlayedByThePicturePlayer)
{
  using namespace EVENT::PLAYER;
  const auto slide = std::make_shared<CFileItem>("/pictures/photo.jpg", false);
  slide->GetPictureInfoTag();
  const std::string item = R"("item":{"file":"/pictures/photo.jpg","type":"picture"})";

  Expect(PlayerEvent{Play{slide, 0, VIDEO}}, "OnPlay",
         "{" + item + R"(,"player":{"playerid":2,"speed":0}})");
  Expect(PlayerEvent{Pause{slide, VIDEO}}, "OnPause",
         "{" + item + R"(,"player":{"playerid":2,"speed":0}})");
  Expect(PlayerEvent{Stop{slide, true, VIDEO}}, "OnStop",
         R"({"end":true,)" + item + R"(,"player":{"playerid":2}})");
}

TEST(TestAnnouncementV14, PlayerPropertiesAreAnnouncedAsOnPropertyChanged)
{
  using namespace EVENT::PLAYER;
  Expect(PlayerEvent{PropertiesChanged{.streams = VIDEO, .partyMode = true}}, "OnPropertyChanged",
         R"({"player":{"playerid":1},"property":{"partymode":true}})");
  Expect(PlayerEvent{PropertiesChanged{.streams = AUDIO, .partyMode = false}}, "OnPropertyChanged",
         R"({"player":{"playerid":0},"property":{"partymode":false}})");

  CVariant stream;
  stream["index"] = 1;
  stream["language"] = "eng";
  Expect(PlayerEvent{PropertiesChanged{
             .streams = VIDEO, .subtitleEnabled = true, .currentSubtitle = stream}},
         "OnPropertyChanged",
         R"({"player":{"playerid":1},
             "property":{"currentsubtitle":{"index":1,"language":"eng"},"subtitleenabled":true}})");
  Expect(PlayerEvent{PropertiesChanged{.streams = VIDEO, .currentAudioStream = stream}},
         "OnPropertyChanged",
         R"({"player":{"playerid":1},
             "property":{"currentaudiostream":{"index":1,"language":"eng"}}})");
  Expect(PlayerEvent{PropertiesChanged{.streams = VIDEO, .currentVideoStream = stream}},
         "OnPropertyChanged",
         R"({"player":{"playerid":1},
             "property":{"currentvideostream":{"index":1,"language":"eng"}}})");

  // A subtitle stream that changed to one without details names no property
  Expect(PlayerEvent{PropertiesChanged{.streams = VIDEO}}, "OnPropertyChanged",
         R"({"player":{"playerid":1}})");
}

TEST(TestAnnouncementV14, VolumeIsAnnouncedByTheApplication)
{
  using namespace EVENT::PLAYER;
  ExpectNotification(PlayerEvent{PropertiesChanged{.volume = 42, .muted = false}}, Application,
                     "OnVolumeChanged", R"({"muted":false,"volume":42})");
  ExpectNotification(PlayerEvent{PropertiesChanged{.volume = 0, .muted = true}}, Application,
                     "OnVolumeChanged", R"({"muted":true,"volume":0})");
}

TEST(TestAnnouncementV14, ShuffleAndRepeatAreAnnouncedAsPropertiesOfThePlayer)
{
  using namespace EVENT::PLAYLIST;
  using KODI::PLAYLIST::Repeat;
  using KODI::PLAYLIST::Type;
  ExpectNotification(PlaylistEvent{PropertiesChanged{Type::Audio, false, std::nullopt}}, Player,
                     "OnPropertyChanged",
                     R"({"player":{"playerid":0},"property":{"shuffled":false}})");
  ExpectNotification(PlaylistEvent{PropertiesChanged{std::nullopt, true, std::nullopt}}, Player,
                     "OnPropertyChanged",
                     R"({"player":{"playerid":2},"property":{"shuffled":true}})");
  ExpectNotification(PlaylistEvent{PropertiesChanged{Type::Audio, std::nullopt, Repeat::Off}},
                     Player, "OnPropertyChanged",
                     R"({"player":{"playerid":0},"property":{"repeat":"off"}})");
  ExpectNotification(PlaylistEvent{PropertiesChanged{Type::Audio, std::nullopt, Repeat::One}},
                     Player, "OnPropertyChanged",
                     R"({"player":{"playerid":0},"property":{"repeat":"one"}})");
  ExpectNotification(PlaylistEvent{PropertiesChanged{Type::Video, std::nullopt, Repeat::All}},
                     Player, "OnPropertyChanged",
                     R"({"player":{"playerid":1},"property":{"repeat":"all"}})");
}

TEST(TestAnnouncementV14, PlayerOther)
{
  using namespace EVENT::PLAYER;
  Expect(PlayerEvent{Commercial{"01:30"}}, "OnCommercial", R"("01:30")");
  Expect(PlayerEvent{ToggleSkipCommercials{true}}, "OnToggleSkipCommercials", "true");
  Expect(PlayerEvent{PlaybackFailed{}}, "OnPlaybackFailed", "null");
  Expect(PlayerEvent{ProcessInfo{}}, "OnProcessInfo", "null");
  Expect(PlayerEvent{Menu{}}, "OnMenu", "null");
  Expect(PlayerEvent{BlurayMenuError{}}, "OnBlurayMenuError", "null");
  Expect(PlayerEvent{BlurayEncryptedError{}}, "OnBlurayEncryptedError", "null");
  Expect(PlayerEvent{SourceSlow{}}, "SourceSlow", "null");
}

TEST(TestAnnouncementV14, Playlist)
{
  using namespace EVENT::PLAYLIST;
  using KODI::PLAYLIST::Type;
  Expect(PlaylistEvent{Add{Type::Video, 3, nullptr}}, "OnAdd", R"({"playlistid":1,"position":3})");
  Expect(PlaylistEvent{Remove{Type::Audio, 0}}, "OnRemove", R"({"playlistid":0,"position":0})");
  Expect(PlaylistEvent{Clear{Type::Audio}}, "OnClear", R"({"playlistid":0})");
  // the slideshow's pictures
  Expect(PlaylistEvent{Clear{std::nullopt}}, "OnClear", R"({"playlistid":2})");
}

TEST(TestAnnouncementV14, GUI)
{
  using namespace EVENT::GUI;
  Expect(GUIEvent{ScreensaverActivated{}}, "OnScreensaverActivated", "null");
  Expect(GUIEvent{ScreensaverDeactivated{true}}, "OnScreensaverDeactivated",
         R"({"shuttingdown":true})");
  Expect(GUIEvent{ScreensaverDeactivated{false}}, "OnScreensaverDeactivated",
         R"({"shuttingdown":false})");
  Expect(GUIEvent{DPMSActivated{}}, "OnDPMSActivated", "null");
  Expect(GUIEvent{DPMSDeactivated{}}, "OnDPMSDeactivated", "null");
  Expect(GUIEvent{SkinUnloading{}}, "OnSkinUnloading", "null");
  Expect(GUIEvent{SkinLoaded{}}, "OnSkinLoaded", "null");
  Expect(GUIEvent{SkinLoadFailed{}}, "OnSkinLoadFailed", "null");
  Expect(GUIEvent{WindowFocused{}}, "WindowFocused", "null");
  Expect(GUIEvent{WindowUnfocused{}}, "WindowUnfocused", "null");
}

TEST(TestAnnouncementV14, System)
{
  using namespace EVENT::SYSTEM;
  Expect(SystemEvent{Quit{64}}, "OnQuit", R"({"exitcode":64})");
  Expect(SystemEvent{Restart{}}, "OnRestart", "null");
  Expect(SystemEvent{EVENT::SYSTEM::Sleep{}}, "OnSleep", "null");
  Expect(SystemEvent{Wake{}}, "OnWake", "null");
  Expect(SystemEvent{LowBattery{}}, "OnLowBattery", "null");
}

TEST(TestAnnouncementV14, LibraryScansAndCleans)
{
  using namespace EVENT::LIBRARY;
  Expect(VideoLibraryEvent{ScanStarted{}}, "OnScanStarted", "null");
  Expect(AudioLibraryEvent{ScanFinished{}}, "OnScanFinished", "null");
  Expect(VideoLibraryEvent{CleanStarted{}}, "OnCleanStarted", "null");
  Expect(AudioLibraryEvent{CleanFinished{}}, "OnCleanFinished", "null");
  Expect(VideoLibraryEvent{Refresh{}}, "OnRefresh", "null");
}

TEST(TestAnnouncementV14, LibraryUpdate)
{
  using namespace EVENT::LIBRARY;
  Expect(VideoLibraryEvent{Update{.type = KODI::MEDIA::TYPE::MOVIE, .id = 7}}, "OnUpdate",
         R"({"id":7,"type":"movie"})");
  Expect(AudioLibraryEvent{Update{
             .type = KODI::MEDIA::TYPE::SONG, .id = 2, .transaction = true, .added = true}},
         "OnUpdate", R"({"added":true,"id":2,"transaction":true,"type":"song"})");
  Expect(VideoLibraryEvent{Update{.type = KODI::MEDIA::TYPE::NONE, .id = -1}}, "OnUpdate",
         R"({"id":-1,"type":""})");
  Expect(VideoLibraryEvent{Update{.otherType = "actor", .id = 3}}, "OnUpdate",
         R"({"id":3,"type":"actor"})");

  // An item names itself
  CVideoInfoTag tag;
  tag.m_iDbId = 7;
  tag.SetMediaType(KODI::MEDIA::TYPE::EPISODE);
  const auto item = std::make_shared<CFileItem>(tag);
  Expect(VideoLibraryEvent{Update{.item = item, .transaction = true, .playCount = 2}}, "OnUpdate",
         R"({"item":{"id":7,"type":"episode"},"playcount":2,"transaction":true})");
  Expect(VideoLibraryEvent{Update{.item = item, .added = true}}, "OnUpdate",
         R"({"added":true,"item":{"id":7,"type":"episode"}})");
  Expect(VideoLibraryEvent{Update{.item = item}}, "OnUpdate",
         R"({"item":{"id":7,"type":"episode"}})");
}

TEST(TestAnnouncementV14, LibraryRemoveAndExport)
{
  using namespace EVENT::LIBRARY;
  Expect(VideoLibraryEvent{Remove{KODI::MEDIA::TYPE::EPISODE, 5, false}}, "OnRemove",
         R"({"id":5,"type":"episode"})");
  Expect(AudioLibraryEvent{Remove{KODI::MEDIA::TYPE::ARTIST, 9, true}}, "OnRemove",
         R"({"id":9,"transaction":true,"type":"artist"})");

  Expect(VideoLibraryEvent{Export{"/export/", "/export/videodb.xml", 3}}, "OnExport",
         R"({"failcount":3,"file":"/export/videodb.xml","root":"/export/"})");
  Expect(AudioLibraryEvent{Export{.file = "/export/kodi_musicdb.xml"}}, "OnExport",
         R"({"file":"/export/kodi_musicdb.xml"})");
  Expect(VideoLibraryEvent{Export{}}, "OnExport", "null");
}

TEST(TestAnnouncementV14, Input)
{
  using namespace EVENT::INPUT;
  using enum KODI::DIALOGS::NUMERIC_MODE;
  Expect(InputEvent{Requested{.title = "Search", .value = "abc"}}, "OnInputRequested",
         R"({"title":"Search","type":"keyboard","value":"abc"})");
  Expect(InputEvent{Requested{.hidden = true, .title = "PIN", .value = ""}}, "OnInputRequested",
         R"({"title":"PIN","type":"password","value":""})");
  Expect(InputEvent{Requested{.numeric = PASSWORD, .value = "12"}}, "OnInputRequested",
         R"({"type":"numericpassword","value":"12"})");
  Expect(InputEvent{Requested{.numeric = NUMBER, .title = "Count", .value = "7"}},
         "OnInputRequested", R"({"title":"Count","type":"number","value":"7"})");
  Expect(InputEvent{Requested{.numeric = DATE, .value = "01/02/2026"}}, "OnInputRequested",
         R"({"type":"date","value":"01/02/2026"})");
  Expect(InputEvent{Requested{.numeric = TIME, .value = "12:30"}}, "OnInputRequested",
         R"({"type":"time","value":"12:30"})");
  Expect(InputEvent{Requested{.numeric = TIME_SECONDS, .value = "90"}}, "OnInputRequested",
         R"({"type":"seconds","value":"90"})");
  Expect(InputEvent{Requested{.numeric = IP_ADDRESS, .value = "10.0.0.1"}}, "OnInputRequested",
         R"({"type":"ip","value":"10.0.0.1"})");
  Expect(InputEvent{Finished{}}, "OnInputFinished", "null");
}

TEST(TestAnnouncementV14, PVR)
{
  using namespace EVENT::PVR;
  Expect(PVREvent{RadioTrafficAnnouncement{true}}, "RDSRadioTA", R"({"on":true})");
  Expect(PVREvent{RadioClock{"Thu, 08 Oct 2026 18:00:00 GMT"}}, "RDSRadioRTC",
         R"({"dateTime":"Thu, 08 Oct 2026 18:00:00 GMT"})");
  Expect(PVREvent{RadioClock{""}}, "RDSRadioRTC", R"({"dateTime":""})");
  Expect(PVREvent{RadioTrafficMessage{"Radio 1", 0x1234, 0x80, 1, 0x0203, 0x0405}}, "RDSRadioTMC",
         R"({"channel":"Radio 1","flags":128,"ident":4660,"x":1,"y":515,"z":1029})");
}

TEST(TestAnnouncementV14, InfoSourcesAndFiles)
{
  Expect(InfoEvent{EVENT::INFO::Changed{}}, "OnChanged", "null");

  using namespace EVENT::SOURCES;
  Expect(SourcesEvent{Added{"upnp://"}}, "OnAdded", R"("upnp://")");
  Expect(SourcesEvent{Removed{"upnp://"}}, "OnRemoved", R"("upnp://")");
  Expect(SourcesEvent{Updated{"zeroconf://"}}, "OnUpdated", R"("zeroconf://")");

  using namespace EVENT::FILES;
  Expect(FilesEvent{PlayCountChanged{"smb://host/film.mkv", 2}}, "OnPlayCountChanged",
         R"({"file":"smb://host/film.mkv","playcount":2})");
  Expect(FilesEvent{ResumePointChanged{"smb://host/film.mkv", 90.5, 5400.0}},
         "OnResumePointChanged",
         R"({"file":"smb://host/film.mkv","resume":{"position":90.5,"total":5400.0}})");
}

TEST(TestAnnouncementV14, AnOtherEventIsItsSendersOwn)
{
  CVariant data;
  data["key"] = "value";
  const std::vector<V14::Notification> notifications =
      V14::NotificationsOf(OtherEvent{"script.example", "Hello", data});

  ASSERT_EQ(1U, notifications.size());
  EXPECT_EQ(Other, notifications[0].flag);
  EXPECT_EQ("script.example", notifications[0].sender);
  EXPECT_EQ("Hello", notifications[0].message);
  EXPECT_EQ(data, notifications[0].data);

  EXPECT_TRUE(V14::NotificationsOf(OtherEvent{"script.example", "Hello", {}})[0].data.isNull());
}

TEST(TestAnnouncementV14, EventsV14DoesNotKnowAreNotAnnounced)
{
  using namespace EVENT::PLAYER;
  EXPECT_TRUE(V14::NotificationsOf(PlayerEvent{ContentGeometryChange{}}).empty());
  EXPECT_TRUE(V14::NotificationsOf(PlayerEvent{AudioFormatHold{}}).empty());
  EXPECT_TRUE(V14::NotificationsOf(SettingsEvent{EVENT::SETTINGS::LevelChanged{}}).empty());
}

TEST(TestAnnouncementV14, ALibraryVideoIsNamedByItsId)
{
  CVideoInfoTag tag;
  tag.m_iDbId = 7;
  tag.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  tag.m_strTitle = "Film";

  ExpectItem(std::make_shared<CFileItem>(tag), R"({"id":7,"type":"movie"})");
}

TEST(TestAnnouncementV14, AMovieOutsideTheLibraryCarriesItsTitleAndYear)
{
  CVideoInfoTag tag;
  tag.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  tag.m_strTitle = "Film";
  tag.SetYear(1999);

  ExpectItem(std::make_shared<CFileItem>(tag), R"({"title":"Film","type":"movie","year":1999})");
}

TEST(TestAnnouncementV14, AnEpisodeOutsideTheLibraryCarriesItsShow)
{
  CVideoInfoTag tag;
  tag.SetMediaType(KODI::MEDIA::TYPE::EPISODE);
  tag.m_strTitle = "Pilot";
  tag.m_strShowTitle = "The Show";
  tag.m_iSeason = 1;
  tag.m_iEpisode = 2;

  ExpectItem(std::make_shared<CFileItem>(tag),
             R"({"episode":2,"season":1,"showtitle":"The Show","title":"Pilot","type":"episode"})");
}

TEST(TestAnnouncementV14, AMusicVideoOutsideTheLibraryCarriesItsAlbumAndArtists)
{
  CVideoInfoTag tag;
  tag.SetMediaType(KODI::MEDIA::TYPE::MUSIC_VIDEO);
  tag.m_strTitle = "Clip";
  tag.m_strAlbum = "Album";
  tag.m_artist = {"First", "Second"};

  ExpectItem(std::make_shared<CFileItem>(tag),
             R"({"album":"Album","artist":"First / Second","title":"Clip","type":"musicvideo"})");
}

TEST(TestAnnouncementV14, AVideoWithoutATitleIsNamedByItsLabel)
{
  CVideoInfoTag tag;
  tag.SetMediaType(KODI::MEDIA::TYPE::MOVIE);
  const auto item = std::make_shared<CFileItem>(tag);
  item->SetLabel("film.mkv");

  ExpectItem(item, R"({"title":"film.mkv","type":"movie"})");
}

TEST(TestAnnouncementV14, ALibrarySongIsNamedByItsId)
{
  MUSIC_INFO::CMusicInfoTag tag;
  tag.SetDatabaseId(9, "song");
  tag.SetTitle("Song");

  ExpectItem(std::make_shared<CFileItem>(tag), R"({"id":9,"type":"song"})");
}

TEST(TestAnnouncementV14, ASongOutsideTheLibraryCarriesItsTrackAlbumAndArtist)
{
  MUSIC_INFO::CMusicInfoTag tag;
  tag.SetTitle("Song");
  tag.SetTrackNumber(3);
  tag.SetAlbum("Album");
  tag.SetArtist("Artist");

  ExpectItem(std::make_shared<CFileItem>(tag),
             R"({"album":"Album","artist":["Artist"],"title":"Song","track":3,"type":"song"})");
}

TEST(TestAnnouncementV14, AVideoFileWithoutDetailsIsAMovieNamedByItsLabel)
{
  const auto item = std::make_shared<CFileItem>("/videos/clip.mkv", false);
  item->SetLabel("clip.mkv");

  ExpectItem(item, R"({"title":"clip.mkv","type":"movie"})");
}

TEST(TestAnnouncementV14, APictureIsNamedByItsFile)
{
  const auto item = std::make_shared<CFileItem>("/pictures/photo.jpg", false);
  item->GetPictureInfoTag();

  ExpectItem(item, R"({"file":"/pictures/photo.jpg","type":"picture"})");
}

TEST(TestAnnouncementV14, AnythingElseIsUnknown)
{
  ExpectItem(std::make_shared<CFileItem>("/files/readme.txt", false), R"({"type":"unknown"})");
}

TEST(TestAnnouncementV14, AChannelNamesItsPlayer)
{
  // TV rather than radio, and no group name: either would reach PVR services tests do not have.
  const auto member{std::make_shared<PVR::CPVRChannelGroupMember>()};
  member->SetChannel(std::make_shared<PVR::CPVRChannel>(false));
  const auto item = std::make_shared<CFileItem>(member);

  ExpectItem(item, R"({"channeltype":"tv","id":-1,"title":"","type":"channel"})");

  // The player a channel plays in replaces the one the event names
  Expect(PlayerEvent{EVENT::PLAYER::Play{item, 1, AUDIO}}, "OnPlay",
         R"({"item":{"channeltype":"tv","id":-1,"title":"","type":"channel"},
             "player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{EVENT::PLAYER::Stop{item, true, std::nullopt}}, "OnStop",
         R"({"end":true,"item":{"channeltype":"tv","id":-1,"title":"","type":"channel"}})");
}

TEST(TestAnnouncementV14, AnItemJoinsTheDataOfItsEvent)
{
  CVideoInfoTag tag;
  tag.m_iDbId = 7;
  tag.SetMediaType(KODI::MEDIA::TYPE::EPISODE);
  const auto item = std::make_shared<CFileItem>(tag);

  Expect(PlayerEvent{EVENT::PLAYER::Play{item, 1, VIDEO}}, "OnPlay",
         R"({"item":{"id":7,"type":"episode"},"player":{"playerid":1,"speed":1}})");
  Expect(PlayerEvent{EVENT::PLAYER::Stop{item, false, std::nullopt}}, "OnStop",
         R"({"end":false,"item":{"id":7,"type":"episode"}})");
}
