/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AnnouncementV14.h"

#include "FileItem.h"
#include "interfaces/AnnouncementEvents.h"
#include "interfaces/json-rpc/JSONUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "pvr/channels/PVRChannel.h"
#include "utils/DatabaseUtils.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <optional>
#include <type_traits>
#include <variant>

namespace ANNOUNCEMENT::V14
{

namespace
{
using KODI::PLAYLIST::Id;

constexpr char ON_PROPERTY_CHANGED[] = "OnPropertyChanged";
constexpr char ON_VOLUME_CHANGED[] = "OnVolumeChanged";

template<class... Ts>
struct Overloaded : Ts...
{
  using Ts::operator()...;
};

int PlayerIdOf(KODI::MEDIA::Streams streams)
{
  return static_cast<int>(KODI::MEDIA::HasVideo(streams) ? Id::TYPE_VIDEO : Id::TYPE_MUSIC);
}

int PlayerIdOf(KODI::MEDIA::Streams streams, const std::shared_ptr<const CFileItem>& item)
{
  if (EVENT::PLAYER::IsPicture(item.get()))
    return static_cast<int>(Id::TYPE_PICTURE);
  return PlayerIdOf(streams);
}

int PlaylistIdOf(const std::optional<KODI::PLAYLIST::Type>& list)
{
  if (!list)
    return static_cast<int>(Id::TYPE_PICTURE);
  return static_cast<int>(*list == KODI::PLAYLIST::Type::Video ? Id::TYPE_VIDEO : Id::TYPE_MUSIC);
}

CVariant SpeedAndPlayer(int speed, int playerId)
{
  CVariant data;
  data["player"]["speed"] = speed;
  data["player"]["playerid"] = playerId;
  return data;
}

const char* InputNameOf(const EVENT::INPUT::Requested& requested)
{
  if (!requested.numeric)
    return requested.hidden ? "password" : "keyboard";

  using enum KODI::DIALOGS::NUMERIC_MODE;
  switch (*requested.numeric)
  {
    case TIME:
      return "time";
    case DATE:
      return "date";
    case IP_ADDRESS:
      return "ip";
    case PASSWORD:
      return "numericpassword";
    case NUMBER:
      return "number";
    case TIME_SECONDS:
      break;
  }
  return "seconds";
}

std::optional<CVariant> PropertyDataOf(const EVENT::PLAYER::PropertiesChanged& changed)
{
  if (!changed.streams)
    return std::nullopt;

  CVariant data;
  data["player"]["playerid"] = PlayerIdOf(*changed.streams);
  if (changed.partyMode)
    data["property"]["partymode"] = *changed.partyMode;
  if (changed.subtitleEnabled)
    data["property"]["subtitleenabled"] = *changed.subtitleEnabled;
  if (changed.currentSubtitle)
    data["property"]["currentsubtitle"] = *changed.currentSubtitle;
  if (changed.currentAudioStream)
    data["property"]["currentaudiostream"] = *changed.currentAudioStream;
  if (changed.currentVideoStream)
    data["property"]["currentvideostream"] = *changed.currentVideoStream;
  return data;
}

std::optional<CVariant> VolumeDataOf(const EVENT::PLAYER::PropertiesChanged& changed)
{
  if (!changed.volume && !changed.muted)
    return std::nullopt;

  CVariant data(CVariant::VariantTypeObject);
  if (changed.volume)
    data["volume"] = *changed.volume;
  if (changed.muted)
    data["muted"] = *changed.muted;
  return data;
}

//! The data of a player event that is announced as itself; empty for one that is not.
std::optional<CVariant> DataOfEvent(const PlayerEvent& event)
{
  using namespace EVENT::PLAYER;
  using Data = std::optional<CVariant>;
  return std::visit(
      Overloaded{
          [](const Play& e) -> Data
          { return SpeedAndPlayer(e.speed, PlayerIdOf(e.streams, e.item)); },
          [](const AVStart& e) -> Data { return SpeedAndPlayer(1, PlayerIdOf(e.streams, e.item)); },
          [](const AVChange& e) -> Data
          { return SpeedAndPlayer(1, PlayerIdOf(e.streams, e.item)); },
          [](const Pause& e) -> Data { return SpeedAndPlayer(0, PlayerIdOf(e.streams, e.item)); },
          [](const Resume& e) -> Data { return SpeedAndPlayer(1, PlayerIdOf(e.streams, e.item)); },
          [](const SpeedChanged& e) -> Data
          { return SpeedAndPlayer(e.speed, PlayerIdOf(e.streams, e.item)); },
          [](const Seek& e) -> Data
          {
            CVariant data = SpeedAndPlayer(e.speed, PlayerIdOf(e.streams, e.item));
            JSONRPC::CJSONUtils::MillisecondsToTimeObject(static_cast<int>(e.time.count()),
                                                          data["player"]["time"]);
            JSONRPC::CJSONUtils::MillisecondsToTimeObject(static_cast<int>(e.seekOffset.count()),
                                                          data["player"]["seekoffset"]);
            return data;
          },
          [](const Stop& e) -> Data
          {
            CVariant data(CVariant::VariantTypeObject);
            // only the slideshow names its streams
            if (e.streams)
              data["player"]["playerid"] = static_cast<int>(Id::TYPE_PICTURE);
            data["end"] = e.end;
            return data;
          },
          [](const PlaybackFailed&) -> Data { return CVariant{}; },
          [](const PropertiesChanged&) -> Data { return std::nullopt; },
          [](const ContentGeometryChange&) -> Data { return std::nullopt; },
          [](const Commercial& e) -> Data { return CVariant{e.time}; },
          [](const ToggleSkipCommercials& e) -> Data { return CVariant{e.skip}; },
          [](const ProcessInfo&) -> Data { return CVariant{}; }, [](const Menu&) -> Data
          { return CVariant{}; }, [](const BlurayMenuError&) -> Data { return CVariant{}; },
          [](const BlurayEncryptedError&) -> Data { return CVariant{}; },
          [](const SourceSlow&) -> Data { return CVariant{}; },
          [](const AudioFormatHold&) -> Data { return std::nullopt; }},
      static_cast<const PlayerEvent::variant&>(event));
}

//! The data of a playlist event that is announced as itself; empty for one that is not.
std::optional<CVariant> DataOfEvent(const PlaylistEvent& event)
{
  using namespace EVENT::PLAYLIST;
  using Data = std::optional<CVariant>;
  return std::visit(Overloaded{[](const Add& e) -> Data
                               {
                                 CVariant data;
                                 data["playlistid"] = PlaylistIdOf(e.playList);
                                 data["position"] = e.position;
                                 return data;
                               },
                               [](const Remove& e) -> Data
                               {
                                 CVariant data;
                                 data["playlistid"] = PlaylistIdOf(e.playList);
                                 data["position"] = e.position;
                                 return data;
                               },
                               [](const Clear& e) -> Data
                               {
                                 CVariant data;
                                 data["playlistid"] = PlaylistIdOf(e.playList);
                                 return data;
                               },
                               [](const PropertiesChanged&) -> Data { return std::nullopt; }},
                    static_cast<const PlaylistEvent::variant&>(event));
}

//! A playlist's shuffle and repeat are announced as properties of its player.
CVariant PropertyDataOf(const EVENT::PLAYLIST::PropertiesChanged& changed)
{
  CVariant data;
  data["player"]["playerid"] = PlaylistIdOf(changed.playList);
  if (changed.shuffled)
    data["property"]["shuffled"] = *changed.shuffled;
  if (changed.repeat)
    data["property"]["repeat"] = std::string{KODI::PLAYLIST::NameOf(*changed.repeat)};
  return data;
}

CVariant DataOfEvent(const GUIEvent& event)
{
  if (const auto* deactivated = std::get_if<EVENT::GUI::ScreensaverDeactivated>(&event))
  {
    CVariant data(CVariant::VariantTypeObject);
    data["shuttingdown"] = deactivated->shuttingDown;
    return data;
  }
  return CVariant{};
}

CVariant DataOfEvent(const SystemEvent& event)
{
  if (const auto* quit = std::get_if<EVENT::SYSTEM::Quit>(&event))
  {
    CVariant data(CVariant::VariantTypeObject);
    data["exitcode"] = quit->exitCode;
    return data;
  }
  return CVariant{};
}

CVariant DataOfEvent(const LibraryEvent& event)
{
  using namespace EVENT::LIBRARY;
  CVariant data;
  if (const auto* update = std::get_if<Update>(&event))
  {
    if (!update->item)
    {
      data["type"] =
          update->otherType.empty() ? KODI::MEDIA::NameOf(update->type) : update->otherType;
      data["id"] = update->id;
    }
    if (update->transaction)
      data["transaction"] = true;
    if (update->added)
      data["added"] = true;
    if (update->playCount)
      data["playcount"] = *update->playCount;
  }
  else if (const auto* remove = std::get_if<Remove>(&event))
  {
    data["type"] = KODI::MEDIA::NameOf(remove->type);
    data["id"] = remove->id;
    if (remove->transaction)
      data["transaction"] = true;
  }
  else if (const auto* exported = std::get_if<Export>(&event))
  {
    if (exported->root)
      data["root"] = *exported->root;
    if (exported->file)
      data["file"] = *exported->file;
    if (exported->failCount)
      data["failcount"] = *exported->failCount;
  }
  return data;
}

CVariant DataOfEvent(const InputEvent& event)
{
  CVariant data;
  if (const auto* requested = std::get_if<EVENT::INPUT::Requested>(&event))
  {
    data["type"] = InputNameOf(*requested);
    if (requested->title)
      data["title"] = *requested->title;
    data["value"] = requested->value;
  }
  return data;
}

CVariant DataOfEvent(const PVREvent& event)
{
  using namespace EVENT::PVR;
  CVariant data(CVariant::VariantTypeObject);
  std::visit(Overloaded{[&data](const RadioTrafficAnnouncement& e) { data["on"] = e.on; },
                        [&data](const RadioClock& e) { data["dateTime"] = e.dateTime; },
                        [&data](const RadioTrafficMessage& e)
                        {
                          data["channel"] = e.channel;
                          data["ident"] = e.ident;
                          data["flags"] = e.flags;
                          data["x"] = e.x;
                          data["y"] = e.y;
                          data["z"] = e.z;
                        }},
             static_cast<const PVREvent::variant&>(event));
  return data;
}

CVariant DataOfEvent(const InfoEvent&)
{
  return CVariant{};
}

CVariant DataOfEvent(const SourcesEvent& event)
{
  return std::visit([](const auto& e) { return CVariant{e.path}; },
                    static_cast<const SourcesEvent::variant&>(event));
}

CVariant DataOfEvent(const FilesEvent& event)
{
  using namespace EVENT::FILES;
  CVariant data(CVariant::VariantTypeObject);
  std::visit(Overloaded{[&data](const PlayCountChanged& e)
                        {
                          data["playcount"] = e.playCount;
                          data["file"] = e.file;
                        },
                        [&data](const ResumePointChanged& e)
                        {
                          data["resume"]["position"] = e.position;
                          data["resume"]["total"] = e.total;
                          data["file"] = e.file;
                        }},
             static_cast<const FilesEvent::variant&>(event));
  return data;
}

//! Settings events are not announced.
std::optional<CVariant> DataOfEvent(const SettingsEvent&)
{
  return std::nullopt;
}

CVariant DataOfEvent(const OtherEvent& event)
{
  return event.data;
}

void CopyPVRTagInfoToObject(const PVR::CPVRChannel& channel, bool copyPlayerId, CVariant& object)
{
  auto& objItem = object["item"];

  objItem["type"] = "channel";
  objItem["title"] = channel.ChannelName();
  objItem["channeltype"] = channel.IsRadio() ? "radio" : "tv";

  if (copyPlayerId)
  {
    object["player"]["playerid"] =
        static_cast<int>(channel.IsRadio() ? Id::TYPE_MUSIC : Id::TYPE_VIDEO);
  }

  objItem["id"] = channel.ChannelID();
}

void CopyVideoTagInfoToObject(const CFileItem& item, CVariant& object)
{
  const CVideoInfoTag& tag = *item.GetVideoInfoTag();

  auto& objItem = object["item"];
  const int id = tag.GetDatabaseId();

  if (!tag.m_type.empty())
    objItem["type"] = tag.m_type;
  else
    objItem["type"] = KODI::MEDIA::NameOf(
        DatabaseUtils::MediaTypeFromVideoContentType(item.GetVideoContentType()));

  if (id <= 0)
  {
    std::string title = tag.m_strTitle;
    if (title.empty())
      title = item.GetLabel();
    objItem["title"] = title;

    using enum VideoDbContentType;
    switch (item.GetVideoContentType())
    {
      case MOVIES:
        if (tag.HasYear())
          objItem["year"] = tag.GetYear();
        break;
      case EPISODES:
        if (tag.m_iEpisode >= 0)
          objItem["episode"] = tag.m_iEpisode;
        if (tag.m_iSeason >= 0)
          objItem["season"] = tag.m_iSeason;
        if (!tag.m_strShowTitle.empty())
          objItem["showtitle"] = tag.m_strShowTitle;
        break;
      case MUSICVIDEOS:
        if (!tag.m_strAlbum.empty())
          objItem["album"] = tag.m_strAlbum;
        if (!tag.m_artist.empty())
          objItem["artist"] = StringUtils::Join(tag.m_artist, " / ");
        break;
      default:
        break;
    }
  }
  else
  {
    objItem["id"] = id;
  }
}

void CopyMusicTagInfoToObject(const CFileItem& item, CVariant& object)
{
  const MUSIC_INFO::CMusicInfoTag& tag = *item.GetMusicInfoTag();

  auto& objItem = object["item"];
  const int id = tag.GetDatabaseId();
  objItem["type"] = KODI::MEDIA::NameOf(KODI::MEDIA::TYPE::SONG);

  if (id <= 0)
  {
    objItem["title"] = tag.GetTitle();
    if (objItem["title"].empty())
      objItem["title"] = item.GetLabel();

    if (tag.GetTrackNumber() > 0)
      objItem["track"] = tag.GetTrackNumber();
    if (!tag.GetAlbum().empty())
      objItem["album"] = tag.GetAlbum();
    if (!tag.GetArtist().empty())
      objItem["artist"] = tag.GetArtist();
  }
  else
  {
    objItem["id"] = id;
  }
}

CVariant CreateDataObjectFromItem(const CFileItem& item, const CVariant& data)
{
  CVariant object;
  if (data.isNull() || data.isObject())
    object = data;
  else
    object = CVariant::VariantTypeObject;

  if (item.HasPVRChannelInfoTag())
  {
    const bool copyPlayerId = data.isMember("player") && data["player"].isMember("playerid");
    CopyPVRTagInfoToObject(*item.GetPVRChannelInfoTag(), copyPlayerId, object);
  }
  else if (item.HasVideoInfoTag() && !item.HasPVRRecordingInfoTag())
  {
    CopyVideoTagInfoToObject(item, object);
  }
  else if (item.HasMusicInfoTag())
  {
    CopyMusicTagInfoToObject(item, object);
  }
  else if (KODI::VIDEO::IsVideo(item))
  {
    // video item but has no video info tag.
    object["item"]["type"] = "movie";
    object["item"]["title"] = item.GetLabel();
  }
  else if (item.HasPictureInfoTag())
  {
    object["item"]["type"] = "picture";
    object["item"]["file"] = item.GetPath();
  }
  else
    object["item"]["type"] = "unknown";

  return object;
}

CVariant WithItemOf(const Announcement& announcement, const CVariant& data)
{
  const std::shared_ptr<const CFileItem> item = ItemOf(announcement);
  return item ? CreateDataObjectFromItem(*item, data) : data;
}

} // unnamed namespace

std::vector<Notification> NotificationsOf(const Announcement& announcement)
{
  std::vector<Notification> notifications;
  const std::string& sender = SenderOf(announcement);

  if (const auto* player = std::get_if<PlayerEvent>(&announcement))
  {
    if (const auto* changed = std::get_if<EVENT::PLAYER::PropertiesChanged>(player))
    {
      if (const std::optional<CVariant> volume = VolumeDataOf(*changed); volume)
        notifications.push_back({Application, sender, ON_VOLUME_CHANGED, *volume});
      if (const std::optional<CVariant> property = PropertyDataOf(*changed); property)
        notifications.push_back({Player, sender, ON_PROPERTY_CHANGED, *property});
      return notifications;
    }
  }
  else if (const auto* playlist = std::get_if<PlaylistEvent>(&announcement))
  {
    if (const auto* changed = std::get_if<EVENT::PLAYLIST::PropertiesChanged>(playlist))
    {
      notifications.push_back({Player, sender, ON_PROPERTY_CHANGED, PropertyDataOf(*changed)});
      return notifications;
    }
  }

  const std::optional<CVariant> data =
      std::visit([](const auto& event) -> std::optional<CVariant> { return DataOfEvent(event); },
                 static_cast<const Announcement::variant&>(announcement));
  if (data)
    notifications.push_back(
        {FlagOf(announcement), sender, MessageOf(announcement), WithItemOf(announcement, *data)});
  return notifications;
}

} // namespace ANNOUNCEMENT::V14
