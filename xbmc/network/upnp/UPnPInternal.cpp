/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UPnPInternal.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "ThumbLoader.h"
#include "UPnPServer.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "imagefiles/ImageFileURL.h"
#include "language/LanguageTag.h"
#include "music/MusicDbPaths.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/ArtTypes.h"
#include "utils/Base64.h"
#include "utils/ContentUtils.h"
#include "utils/ItemProperties.h"
#include "utils/Set.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDbPaths.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <Platinum/Source/Platinum/Platinum.h>

using namespace KODI;
using namespace MUSIC_INFO;
using namespace XFILE;

namespace
{
std::optional<std::string> GetImageDLNAProfile(const std::string& imgPath)
{
  if (URIUtils::HasExtension(imgPath, ".png"))
  {
    return "PNG_TN";
  }
  else if (URIUtils::HasExtension(imgPath, ".jpg") || URIUtils::HasExtension(imgPath, ".jpeg"))
  {
    return "JPEG_TN";
  }
  return std::nullopt;
}

constexpr char GENERIC_MIME_TYPE[] = "application/octet-stream";

//! The extension of \p path in lower case, without its dot
std::string LowerExtension(const std::string& path)
{
  std::string extension{URIUtils::GetExtension(path)};
  if (!extension.empty())
    extension.erase(0, 1);
  StringUtils::ToLower(extension);
  return extension;
}

//! The general kind of content an object of \p objectClass holds: video, audio or image
const char* ContentOfClass(const NPT_String& objectClass)
{
  if (objectClass.StartsWith("object.item.videoItem", true))
    return "video";
  if (objectClass.StartsWith("object.item.audioItem", true))
    return "audio";
  if (objectClass.StartsWith("object.item.imageItem", true))
    return "image";
  return nullptr;
}

template<typename List>
void AddAll(List& list, const std::vector<std::string>& values)
{
  for (const std::string& value : values)
    list.Add(value.c_str());
}

std::vector<std::string> StringsOf(const NPT_List<NPT_String>& list)
{
  std::vector<std::string> values;
  for (auto it = list.GetFirstItem(); it; ++it)
    values.emplace_back(it->GetChars());
  return values;
}

std::vector<std::string> NamesOf(const PLT_PersonRoles& people)
{
  std::vector<std::string> names;
  for (auto it = people.GetFirstItem(); it; ++it)
    names.emplace_back(it->name.GetChars());
  return names;
}

//! The genres of \p object, without the lone "Unknown" Platinum gives an object that has none
std::vector<std::string> GenresOf(const PLT_MediaObject& object)
{
  const NPT_List<NPT_String>& genres{object.m_Affiliation.genres};
  if (genres.GetItemCount() == 1 && *genres.GetFirstItem() == "Unknown")
    return {};
  return StringsOf(genres);
}

std::string AlbumArtistOrArtist(const CMusicInfoTag& tag)
{
  return tag.GetAlbumArtistString().empty() ? tag.GetArtistString() : tag.GetAlbumArtistString();
}

void AddArtistRoles(PLT_PersonRoles& roles,
                    const std::string& performers,
                    const std::string& albumArtist)
{
  roles.Add(performers.c_str(), "Performer");
  roles.Add(albumArtist.c_str(), "AlbumArtist");
}

//! The date a show or season first aired, or the start of its year when only that is known
CDateTime PremieredOrYear(const CVideoInfoTag& tag)
{
  if (!tag.m_premiered.IsValid() && tag.GetYear() > 0)
    return CDateTime(tag.GetYear(), 1, 1, 0, 0, 0);
  return tag.m_premiered;
}

//! Whether \p id is one of the integer ids used for fixed containers, such as the root, which are
//! not encoded
bool IsIntegerObjectId(const std::string& id)
{
  return StringUtils::IsInteger(id);
}
} // namespace

namespace UPNP
{

// the original version of content type here,eg: text/srt,which was defined 10 years ago (year 2013,commit 56519bec #L1158-L1161 )
// is not a standard mime type. according to the specs of UPNP
// http://upnp.org/specs/av/UPnP-av-ConnectionManager-v3-Service.pdf chapter "A.1.1 ProtocolInfo Definition"
// "The <contentFormat> part for HTTP GET is described by a MIME type RFC https://www.ietf.org/rfc/rfc1341.txt"
// all the pre-defined "text/*" MIME by IANA is here https://www.iana.org/assignments/media-types/media-types.xhtml#text
// there is not any subtitle MIME for now (year 2022), we used to use text/srt|ssa|sub|idx, but,
// kodi support SUP subtitle now, and SUP subtitle is not really a text(see below), to keep it
// compatible, we suggest only to match the extension
//
// main purpose of this array is to share supported real subtitle formats when kodi act as a UPNP
// server or UPNP/DLNA media render
constexpr auto SupportedSubFormats = make_set<std::string_view>({
    "txt",
    "srt",
    "ssa",
    "ass",
    "sub",
    "smi",
    "vtt",
    // "sup" subtitle is not a real TEXT,
    // and there is no real STD subtitle RFC of DLNA,
    // so we only match the extension of the "fake" content type
    "sup",
    "idx",
});

// Map defining extensions for mimetypes not available in Platinum mimetype map
// or that the application wants to override. These definitions take precedence
// over all other possible mime type definitions.
constexpr NPT_HttpFileRequestHandler_DefaultFileTypeMapEntry kodiPlatinumMimeTypeExtensions[] = {
    {"m2ts", "video/vnd.dlna.mpeg-tts"}};

EClientQuirks GetClientQuirks(const PLT_HttpRequestContext* context)
{
  if (context == NULL)
    return ECLIENTQUIRKS_NONE;

  unsigned int quirks = 0;
  const NPT_String* user_agent =
      context->GetRequest().GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_USER_AGENT);
  const NPT_String* server =
      context->GetRequest().GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_SERVER);

  if (user_agent)
  {
    if (user_agent->Find("XBox", 0, true) >= 0 || user_agent->Find("Xenon", 0, true) >= 0)
      quirks |= ECLIENTQUIRKS_ONLYSTORAGEFOLDER | ECLIENTQUIRKS_BASICVIDEOCLASS;

    if (user_agent->Find("Windows-Media-Player", 0, true) >= 0)
      quirks |= ECLIENTQUIRKS_UNKNOWNSERIES;
  }
  if (server)
  {
    if (server->Find("Xbox", 0, true) >= 0)
      quirks |= ECLIENTQUIRKS_ONLYSTORAGEFOLDER | ECLIENTQUIRKS_BASICVIDEOCLASS;
  }

  return (EClientQuirks)quirks;
}

EMediaControllerQuirks GetMediaControllerQuirks(const PLT_DeviceData* device)
{
  if (device == NULL)
    return EMEDIACONTROLLERQUIRKS_NONE;

  unsigned int quirks = 0;

  if (device->m_Manufacturer.Find("Samsung Electronics") >= 0)
    quirks |= EMEDIACONTROLLERQUIRKS_X_MKV;

  return (EMediaControllerQuirks)quirks;
}

NPT_String GetMimeType(const char* filename, const PLT_HttpRequestContext* context /* = NULL */)
{
  return PLT_MimeType::GetMimeTypeFromExtension(LowerExtension(filename).c_str(), context);
}

NPT_String GetMimeType(const CFileItem& item, const PLT_HttpRequestContext* context /* = NULL */)
{
  std::string path = item.GetPath();
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().empty())
  {
    path = item.GetVideoInfoTag()->GetPath();
  }
  else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().empty())
  {
    path = item.GetMusicInfoTag()->GetURL();
  }

  if (URIUtils::IsStack(path))
    path = XFILE::CStackDirectory::GetFirstStackedFile(path);

  const NPT_String ext{LowerExtension(path).c_str()};

  NPT_String mime;

  if (!ext.IsEmpty())
  {
    /* We look first to our extensions/overrides of libplatinum mimetypes. If not found, fallback to
         Platinum definitions.
      */
    const auto kodiOverrideMimeType = std::find_if(
        std::begin(kodiPlatinumMimeTypeExtensions), std::end(kodiPlatinumMimeTypeExtensions),
        [&](const auto& mimeTypeEntry) { return mimeTypeEntry.extension == ext; });
    if (kodiOverrideMimeType != std::end(kodiPlatinumMimeTypeExtensions))
    {
      mime = kodiOverrideMimeType->mime_type;
    }
    else
    {
      /* Give priority to Platinum mime types as they are defined to map extension to DLNA compliant mime types
               or custom types according to context (who asked for it)
        */
      mime = PLT_MimeType::GetMimeTypeFromExtension(ext, context);
      if (mime == GENERIC_MIME_TYPE)
        mime = "";
    }
  }

  /* if Platinum couldn't map it, default to Kodi internal mapping */
  if (mime.IsEmpty())
  {
    mime = item.GetMimeType().c_str();
    if (mime == GENERIC_MIME_TYPE)
      mime = "";
  }

  /* fallback to generic mime type if not found */
  if (mime.IsEmpty())
  {
    if (VIDEO::IsVideo(item) || VIDEO::IsVideoDb(item))
      mime = "video/" + ext;
    else if (MUSIC::IsAudio(item) || MUSIC::IsMusicDb(item))
      mime = "audio/" + ext;
    else if (item.IsPicture())
      mime = "image/" + ext;
    else if (VIDEO::IsSubtitle(item))
      mime = "text/" + ext;
  }

  /* nothing we can figure out */
  if (mime.IsEmpty())
    mime = GENERIC_MIME_TYPE;

  return mime;
}

const NPT_String GetProtocolInfo(const CFileItem& item,
                                 const char* protocol,
                                 const PLT_HttpRequestContext* context /* = NULL */)
{
  const std::string source{protocol && *protocol ? protocol : item.GetURL().GetProtocol()};

  // UPnP clients get http; other protocols are offered as xbmc-get for other Kodi clients
  //! @todo add rtsp ?
  NPT_String proto{source == "http" ? "http-get" : "xbmc-get"};

  const NPT_String mime{GetMimeType(item, context)};
  proto += ":*:" + mime + ":" + PLT_ProtocolInfo::GetDlnaExtension(mime, context);
  return proto;
}

void AddAlternateMimeResources(PLT_MediaObject& object)
{
  // Content types in use under two names; a renderer matching on one cannot select the other.
  static constexpr std::pair<const char*, const char*> alternates[] = {
      {"audio/x-flac", "audio/flac"},
      {"audio/x-ms-wma", "audio/wma"},
  };

  const NPT_Cardinal count = object.m_Resources.GetItemCount();
  for (NPT_Cardinal i = 0; i < count; i++)
  {
    const NPT_String type = object.m_Resources[i].m_ProtocolInfo.GetContentType();
    for (const auto& [name, alternate] : alternates)
    {
      const char* to = type == name ? alternate : type == alternate ? name : nullptr;
      if (!to)
        continue;

      PLT_MediaItemResource resource = object.m_Resources[i];
      NPT_String protocolInfo = resource.m_ProtocolInfo.ToString();
      protocolInfo.Replace(":" + type + ":", ":" + NPT_String(to) + ":");
      resource.m_ProtocolInfo = PLT_ProtocolInfo(protocolInfo);
      object.m_Resources.Add(resource);
    }
  }
}

CResourceFinder::CResourceFinder(const char* protocol, const char* content)
  : m_Protocol(protocol), m_Content(content)
{
}

bool CResourceFinder::operator()(const PLT_MediaItemResource& resource) const
{
  return resource.m_ProtocolInfo.GetProtocol().Compare(m_Protocol, true) == 0 &&
         (m_Content.IsEmpty() ||
          resource.m_ProtocolInfo.GetContentType().StartsWith(m_Content, true));
}

NPT_Result PopulateObjectFromTag(CMusicInfoTag& tag,
                                 PLT_MediaObject& object,
                                 NPT_String* file_path,
                                 PLT_MediaItemResource* resource,
                                 EClientQuirks quirks,
                                 UPnPService service /* = UPnPServiceNone */)
{
  if (!tag.GetURL().empty() && file_path)
    *file_path = tag.GetURL().c_str();

  AddAll(object.m_Affiliation.genres, tag.GetGenre());
  object.m_Title = tag.GetTitle().c_str();
  object.m_Affiliation.album = tag.GetAlbum().c_str();
  for (const std::string& artist : tag.GetArtist())
  {
    object.m_People.artists.Add(artist.c_str());
    object.m_People.artists.Add(artist.c_str(), "Performer");
  }
  object.m_People.artists.Add(AlbumArtistOrArtist(tag).c_str(), "AlbumArtist");
  object.m_Creator = AlbumArtistOrArtist(tag).c_str();
  object.m_MiscInfo.original_track_number = tag.GetTrackNumber();
  if (tag.GetDatabaseId() >= 0)
  {
    object.m_ReferenceID =
        EncodeObjectId(StringUtils::Format("{}{}{}", MUSIC::DB_PATH::SONGS, tag.GetDatabaseId(),
                                           URIUtils::GetExtension(tag.GetURL())));
  }
  if (object.m_ReferenceID == object.m_ObjectID)
    object.m_ReferenceID = "";

  object.m_MiscInfo.last_time = tag.GetLastPlayed().GetAsW3CDateTime().c_str();
  object.m_MiscInfo.play_count = tag.GetPlayCount();

  if (resource)
    resource->m_Duration = tag.GetDuration();

  return NPT_SUCCESS;
}

NPT_Result PopulateObjectFromTag(CVideoInfoTag& tag,
                                 PLT_MediaObject& object,
                                 NPT_String* file_path,
                                 PLT_MediaItemResource* resource,
                                 EClientQuirks quirks,
                                 UPnPService service /* = UPnPServiceNone */)
{
  if (!tag.m_strFileNameAndPath.empty() && file_path)
    *file_path = tag.m_strFileNameAndPath.c_str();

  if (tag.m_iDbId != -1)
  {
    if (tag.m_type == MediaTypeMusicVideo)
    {
      object.m_ObjectClass.type = "object.item.videoItem.musicVideoClip";
      object.m_Creator =
          StringUtils::Join(
              tag.m_artist,
              CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator)
              .c_str();
      AddAll(object.m_People.artists, tag.m_artist);
      object.m_Affiliation.album = tag.m_strAlbum.c_str();
      object.m_Title = tag.m_strTitle.c_str();
      object.m_Date = tag.GetPremiered().GetAsW3CDate().c_str();
      object.m_ReferenceID =
          EncodeObjectId(StringUtils::Format("{}{}", VIDEO::DB_PATH::MUSICVIDEO_TITLES, tag.m_iDbId));
    }
    else if (tag.m_type == MediaTypeMovie)
    {
      object.m_ObjectClass.type = "object.item.videoItem.movie";
      object.m_Title = tag.m_strTitle.c_str();
      object.m_Date = tag.GetPremiered().GetAsW3CDate().c_str();
      object.m_ReferenceID =
          EncodeObjectId(StringUtils::Format("{}{}", VIDEO::DB_PATH::MOVIE_TITLES, tag.m_iDbId));
    }
    else
    {
      object.m_Recorded.series_title = tag.m_strShowTitle.c_str();

      if (tag.m_type == MediaTypeTvShow)
      {
        object.m_ObjectClass.type = "object.container.album.videoAlbum.videoBroadcastShow";
        object.m_Title = tag.m_strTitle.c_str();
        object.m_Recorded.episode_number = tag.m_iEpisode;
        object.m_Recorded.episode_count = tag.m_iEpisode;
        object.m_Date = PremieredOrYear(tag).GetAsW3CDate().c_str();
        object.m_ReferenceID =
            EncodeObjectId(StringUtils::Format("{}{}", VIDEO::DB_PATH::TVSHOW_TITLES, tag.m_iDbId));
      }
      else if (tag.m_type == MediaTypeSeason)
      {
        object.m_ObjectClass.type = "object.container.album.videoAlbum.videoBroadcastSeason";
        object.m_Title = tag.m_strTitle.c_str();
        object.m_Recorded.episode_season = tag.m_iSeason;
        object.m_Recorded.episode_count = tag.m_iEpisode;
        object.m_Date = PremieredOrYear(tag).GetAsW3CDate().c_str();
        object.m_ReferenceID = EncodeObjectId(
            StringUtils::Format(
            "{}{}/{}", VIDEO::DB_PATH::TVSHOW_TITLES, tag.m_iIdShow, tag.m_iSeason));
      }
      else
      {
        object.m_ObjectClass.type = "object.item.videoItem.videoBroadcast";
        object.m_Recorded.program_title =
            StringUtils::Format("S{:02}E{:02} : {}", tag.m_iSeason, tag.m_iEpisode, tag.m_strTitle)
                .c_str();
        object.m_Recorded.episode_number = tag.m_iEpisode;
        object.m_Recorded.episode_season = tag.m_iSeason;
        object.m_Title = object.m_Recorded.series_title + " - " + object.m_Recorded.program_title;
        object.m_ReferenceID = EncodeObjectId(StringUtils::Format("{}{}/{}/{}", VIDEO::DB_PATH::TVSHOW_TITLES, tag.m_iIdShow, tag.m_iSeason, tag.m_iDbId));
        object.m_Date = tag.m_firstAired.GetAsW3CDate().c_str();
      }
    }
  }

  if (quirks & ECLIENTQUIRKS_BASICVIDEOCLASS)
    object.m_ObjectClass.type = "object.item.videoItem";

  if (object.m_ReferenceID == object.m_ObjectID)
    object.m_ReferenceID = "";

  AddAll(object.m_People.publisher, tag.m_studio);

  object.m_XbmcInfo.date_added = tag.m_dateAdded.GetAsW3CDate().c_str();
  object.m_XbmcInfo.rating = tag.GetRating().rating;
  object.m_XbmcInfo.votes = tag.GetRating().votes;
  object.m_XbmcInfo.unique_identifier = tag.GetUniqueID().c_str();
  AddAll(object.m_XbmcInfo.countries, tag.m_country);
  object.m_XbmcInfo.user_rating = tag.m_iUserRating;

  AddAll(object.m_Affiliation.genres, tag.m_genre);

  for (const SActorInfo& actor : tag.m_cast)
    object.m_People.actors.Add(actor.strName.c_str(), actor.strRole.c_str());
  AddAll(object.m_People.directors, tag.m_director);
  AddAll(object.m_People.authors, tag.m_writingCredits);

  object.m_Description.description = tag.m_strTagLine.c_str();
  object.m_Description.long_description = tag.m_strPlot.c_str();
  object.m_Description.rating = tag.m_strMPAARating.c_str();
  object.m_MiscInfo.last_position = (NPT_UInt32)tag.GetResumePoint().timeInSeconds;
  object.m_XbmcInfo.last_playerstate = tag.GetResumePoint().playerState.c_str();
  object.m_MiscInfo.last_time = tag.m_lastPlayed.GetAsW3CDateTime().c_str();
  object.m_MiscInfo.play_count = tag.GetPlayCount();
  if (resource)
  {
    resource->m_Duration = tag.GetDuration();
    if (tag.HasStreamDetails())
    {
      const CStreamDetails& details = tag.m_streamDetails;
      resource->m_Resolution = NPT_String::FromInteger(details.GetVideoWidth()) + "x" +
                               NPT_String::FromInteger(details.GetVideoHeight());
      resource->m_NbAudioChannels = details.GetAudioChannels();
    }
  }

  return NPT_SUCCESS;
}

PLT_MediaObject* BuildObject(CFileItem& item,
                             NPT_String& file_path,
                             bool with_count,
                             NPT_Reference<CThumbLoader>& thumb_loader,
                             const PLT_HttpRequestContext* context /* = NULL */,
                             CUPnPServer* upnp_server /* = NULL */,
                             UPnPService upnp_service /* = UPnPServiceNone */)
{
  static Logger logger = CServiceBroker::GetLogging().GetLogger("UPNP::BuildObject");

  PLT_MediaItemResource resource;
  PLT_MediaObject* object = NULL;
  std::string thumb;

  logger->debug("Building didl for plain object '{}' (encoded value: '{}')", item.GetPath(),
                EncodeObjectId(item.GetPath()).GetChars());

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return nullptr;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return nullptr;

  EClientQuirks quirks = GetClientQuirks(context);

  // get list of ip addresses
  NPT_List<NPT_IpAddress> ips;
  NPT_HttpUrl rooturi;
  NPT_CHECK_LABEL(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

  // if we're passed an interface where we received the request from
  // move the ip to the top
  if (context && context->GetLocalAddress().GetIpAddress().ToString() != "0.0.0.0")
  {
    rooturi = NPT_HttpUrl(context->GetLocalAddress().GetIpAddress().ToString(),
                          context->GetLocalAddress().GetPort(), "/");
    ips.Remove(context->GetLocalAddress().GetIpAddress());
    ips.Insert(ips.GetFirstItem(), context->GetLocalAddress().GetIpAddress());
  }
  else if (upnp_server)
  {
    rooturi = NPT_HttpUrl("localhost", upnp_server->GetPort(), "/");
  }

  if (!item.IsFolder())
  {
    object = new PLT_MediaItem();
    object->m_ObjectID = EncodeObjectId(item.GetPath());

    /* Setup object type */
    if (MUSIC::IsMusicDb(item) || MUSIC::IsAudio(item))
    {
      object->m_ObjectClass.type = "object.item.audioItem.musicTrack";

      if (item.HasMusicInfoTag())
      {
        CMusicInfoTag* tag = item.GetMusicInfoTag();
        PopulateObjectFromTag(*tag, *object, &file_path, &resource, quirks, upnp_service);
      }
    }
    else if (VIDEO::IsVideoDb(item) || VIDEO::IsVideo(item))
    {
      object->m_ObjectClass.type = "object.item.videoItem";

      if (quirks & ECLIENTQUIRKS_UNKNOWNSERIES)
        object->m_Affiliation.album = "[Unknown Series]";

      if (item.HasVideoInfoTag())
      {
        CVideoInfoTag* tag = item.GetVideoInfoTag();
        PopulateObjectFromTag(*tag, *object, &file_path, &resource, quirks, upnp_service);
      }
    }
    else if (item.IsPicture())
    {
      object->m_ObjectClass.type = "object.item.imageItem.photo";
    }
    else
    {
      object->m_ObjectClass.type = "object.item";
    }

    // duration of zero is invalid
    if (resource.m_Duration == 0)
      resource.m_Duration = -1;

    // Set the resource file size
    resource.m_Size = item.GetSize();
    if (resource.m_Size == 0)
      resource.m_Size = (NPT_LargeSize)-1;

    // set date
    if (object->m_Date.IsEmpty())
    {
      const CDateTime& dateTime{item.GetDateTime()};
      if (dateTime.IsValid())
        object->m_Date = dateTime.GetAsW3CDate().c_str();
    }

    if (upnp_server)
    {
      upnp_server->AddSafeResourceUri(object, rooturi, ips, file_path,
                                      GetProtocolInfo(item, "http", context));
    }

    // if the item is remote, add a direct link to the item
    if (URIUtils::IsRemote((const char*)file_path))
    {
      resource.m_ProtocolInfo =
          PLT_ProtocolInfo(GetProtocolInfo(item, item.GetURL().GetProtocol().c_str(), context));
      resource.m_Uri = file_path;

      // if the direct link can be served directly using http, then push it in front
      // otherwise keep the xbmc-get resource last and let a compatible client look for it
      if (resource.m_ProtocolInfo.ToString().StartsWith("xbmc", true))
      {
        object->m_Resources.Add(resource);
      }
      else
      {
        object->m_Resources.Insert(object->m_Resources.GetFirstItem(), resource);
      }
    }

    // copy across the known metadata
    for (unsigned i = 0; i < object->m_Resources.GetItemCount(); i++)
    {
      object->m_Resources[i].m_Size = resource.m_Size;
      object->m_Resources[i].m_Duration = resource.m_Duration;
      object->m_Resources[i].m_Resolution = resource.m_Resolution;
    }
  }
  else
  {
    PLT_MediaContainer* container = new PLT_MediaContainer;
    object = container;

    /* Assign a title and id for this container */
    container->m_ObjectID = EncodeObjectId(item.GetPath());
    container->m_ObjectClass.type = "object.container";
    container->m_ChildrenCount = -1;

    /* this might be overkill, but hey */
    if (MUSIC::IsMusicDb(item))
    {
      const auto node = CMusicDatabaseDirectory::GetDirectoryType(item.GetPath());
      switch (node)
      {
        case MUSICDATABASEDIRECTORY::NodeType::ARTIST:
        case MUSICDATABASEDIRECTORY::NodeType::ALBUM:
        case MUSICDATABASEDIRECTORY::NodeType::ALBUM_RECENTLY_ADDED:
        {
          const bool artist{node == MUSICDATABASEDIRECTORY::NodeType::ARTIST};
          container->m_ObjectClass.type += artist ? ".person.musicArtist" : ".album.musicAlbum";
          // albums carry their artists for Sonos
          if (const CMusicInfoTag* tag = item.GetMusicInfoTag(); tag)
          {
            AddArtistRoles(container->m_People.artists,
                           CorrectAllItemsSortHack(tag->GetArtistString()),
                           CorrectAllItemsSortHack(AlbumArtistOrArtist(*tag)));
            if (!artist)
              container->m_Affiliation.album = CorrectAllItemsSortHack(tag->GetAlbum()).c_str();
          }
          break;
        }
        case MUSICDATABASEDIRECTORY::NodeType::GENRE:
          container->m_ObjectClass.type += ".genre.musicGenre";
          break;
        default:
          break;
      }
    }
    else if (VIDEO::IsVideoDb(item))
    {
      const auto node = CVideoDatabaseDirectory::GetDirectoryType(item.GetPath());
      CVideoInfoTag& tag = *item.GetVideoInfoTag();
      switch (node)
      {
        case VIDEODATABASEDIRECTORY::NodeType::GENRE:
          container->m_ObjectClass.type += ".genre.movieGenre";
          break;
        case VIDEODATABASEDIRECTORY::NodeType::ACTOR:
          container->m_ObjectClass.type += ".person.videoArtist";
          container->m_Creator =
              StringUtils::Join(tag.m_artist,
                                settingsComponent->GetAdvancedSettings()->m_videoItemSeparator)
                  .c_str();
          container->m_Title = tag.m_strTitle.c_str();
          break;
        case VIDEODATABASEDIRECTORY::NodeType::SEASONS:
        case VIDEODATABASEDIRECTORY::NodeType::TITLE_TVSHOWS:
          container->m_ObjectClass.type += node == VIDEODATABASEDIRECTORY::NodeType::SEASONS
                                               ? ".album.videoAlbum.videoBroadcastSeason"
                                               : ".album.videoAlbum.videoBroadcastShow";
          PopulateObjectFromTag(tag, *container, &file_path, &resource, quirks);
          break;
        default:
          container->m_ObjectClass.type += ".storageFolder";
          break;
      }
    }
    else if (PLAYLIST::IsPlayList(item) || PLAYLIST::IsSmartPlayList(item))
    {
      container->m_ObjectClass.type += ".playlistContainer";
    }

    if (quirks & ECLIENTQUIRKS_ONLYSTORAGEFOLDER)
    {
      container->m_ObjectClass.type = "object.container.storageFolder";
    }

    /* Get the number of children for this container */
    if (with_count && upnp_server)
    {
      const NPT_String decodedObjectId = DecodeObjectId(object->m_ObjectID.GetChars());
      if (StringUtils::StartsWithNoCase(decodedObjectId.GetChars(), "virtualpath://"))
      {
        NPT_LargeSize count = 0;
        NPT_CHECK_LABEL(NPT_File::GetSize(file_path, count), failure);
        container->m_ChildrenCount = (NPT_Int32)count;
      }
      else
      {
        /* this should be a standard path */
        //! @todo - get file count of this directory
      }
    }
  }

  // set a title for the object
  if (object->m_Title.IsEmpty())
  {
    if (!item.GetLabel().empty())
    {
      std::string title = item.GetLabel();
      if (PLAYLIST::IsPlayList(item) || !item.IsFolder())
        URIUtils::RemoveExtension(title);
      object->m_Title = title.c_str();
    }
  }

  if (upnp_server)
  {
    // determine the correct artwork for this item
    if (!thumb_loader.IsNull())
      thumb_loader->LoadItem(&item);

    // we have to decide the best art type to serve to the client - use ContentUtils
    // to get it since it depends on the mediatype of the item being served
    thumb = ContentUtils::GetPreferredArtImage(item);

    if (!thumb.empty())
    {
      PLT_AlbumArtInfo art;

      // Get DLNA profileId for the image
      std::optional<std::string> imageProfile = GetImageDLNAProfile(thumb);
      if (imageProfile.has_value())
      {
        art.dlna_profile = imageProfile.value().c_str();
      }
      else
      {
        // unknown image format (might be a embed thumb previously extracted and cached - e.g. mp3, flac, mvk, etc)
        bool needsRecaching;
        const std::string cachedImagePath =
            CServiceBroker::GetTextureCache()->CheckCachedImage(thumb, needsRecaching);
        if (!cachedImagePath.empty())
        {
          imageProfile = GetImageDLNAProfile(cachedImagePath);
          if (imageProfile.has_value())
          {
            thumb = cachedImagePath;
            art.dlna_profile = imageProfile.value().c_str();
          }
        }
      }

      // Always default to JPEG if the profile could not be found
      if (art.dlna_profile.IsEmpty())
      {
        art.dlna_profile = "JPEG_TN";
      }

      std::string wrappedUrl = IMAGE_FILES::URLFromFile(thumb);
      art.uri = upnp_server->BuildSafeResourceUri(rooturi, (*ips.GetFirstItem()).ToString(),
                                                  wrappedUrl.c_str());

      object->m_ExtraInfo.album_arts.Add(art);
    }

    for (const auto& itArtwork : item.GetArt())
    {
      if (!itArtwork.first.empty() && !itArtwork.second.empty())
      {
        std::string wrappedUrl = IMAGE_FILES::URLFromFile(itArtwork.second);
        object->m_XbmcInfo.artwork.Add(
            itArtwork.first.c_str(),
            upnp_server->BuildSafeResourceUri(rooturi, (*ips.GetFirstItem()).ToString(),
                                              wrappedUrl.c_str()));
        upnp_server->AddSafeResourceUri(object, rooturi, ips, wrappedUrl.c_str(),
                                        ("xbmc.org:*:" + itArtwork.first + ":*").c_str());
      }
    }
  }

  // look for and add external subtitle if we are processing a video file and
  // we are being called by a UPnP player or renderer or the user has chosen
  // to look for external subtitles
  if (upnp_server != nullptr && VIDEO::IsVideo(item) &&
      (upnp_service == UPnPPlayer || upnp_service == UPnPRenderer ||
       settings->GetBool(CSettings::SETTING_SERVICES_UPNPLOOKFOREXTERNALSUBTITLES)))
  {
    // find any available external subtitles
    std::vector<std::string> filenames;
    std::vector<std::string> subtitles;
    CUtil::ScanForExternalSubtitles(file_path.GetChars(), filenames);

    // Only the formats UPnP devices commonly support, which leaves out archives such as rar or zip
    for (const std::string& filename : filenames)
    {
      if (SupportedSubFormats.contains(LowerExtension(filename)))
        subtitles.push_back(filename);
    }

    std::string subtitlePath;

    if (subtitles.size() == 1)
    {
      subtitlePath = subtitles[0];
    }
    else if (!subtitles.empty())
    {
      std::string preferredLanguage{"en"};

      /* trying to find subtitle with preferred language settings */
      auto setting = settings->GetSetting("locale.subtitlelanguage");
      if (!setting)
        CLog::Log(LOGERROR, "Failed to load setting for: {}", "locale.subtitlelanguage");
      else
        preferredLanguage = setting->ToString();

      const KODI::LANGUAGE::CLanguageTag preferredTag{
          KODI::LANGUAGE::CLanguageTag::Parse(preferredLanguage)};

      for (const std::string& subtitle : subtitles)
      {
        if (CUtil::GetExternalStreamDetailsFromFilename(file_path.GetChars(), subtitle)
                .language.Matches(preferredTag))
        {
          subtitlePath = subtitle;
          break;
        }
      }
      /* if not found subtitle with preferred language, get the first one */
      if (subtitlePath.empty())
      {
        subtitlePath = subtitles[0];
      }
    }

    if (!subtitlePath.empty())
    {
      /* subtitles are added as 2 resources, 2 sec resources and 1 addon to video resource, to be compatible with
               the most of the devices; all UPnP devices take the last one it could handle,
               and skip ones it doesn't "understand" */
      // add subtitle resource with standard protocolInfo
      NPT_String protocolInfo = GetProtocolInfo(CFileItem(subtitlePath, false), "http", context);
      upnp_server->AddSafeResourceUri(object, rooturi, ips, NPT_String(subtitlePath.c_str()),
                                      protocolInfo);
      // add subtitle resource with smi/caption protocol info (some devices)
      PLT_ProtocolInfo protInfo = PLT_ProtocolInfo(protocolInfo);
      protocolInfo =
          protInfo.GetProtocol() + ":" + protInfo.GetMask() + ":smi/caption:" + protInfo.GetExtra();
      upnp_server->AddSafeResourceUri(object, rooturi, ips, NPT_String(subtitlePath.c_str()),
                                      protocolInfo);

      const std::string ext{LowerExtension(subtitlePath)};

      NPT_String subtitle_uri = object->m_Resources[object->m_Resources.GetItemCount() - 1].m_Uri;

      // add subtitle to video resource (the first one) (for some devices)
      object->m_Resources[0].m_CustomData["xmlns:pv"] = "http://www.pv.com/pvns/";
      object->m_Resources[0].m_CustomData["pv:subtitleFileUri"] = subtitle_uri;
      object->m_Resources[0].m_CustomData["pv:subtitleFileType"] = ext.c_str();

      // for samsung devices
      PLT_SecResource sec_res;
      sec_res.name = "CaptionInfoEx";
      sec_res.value = subtitle_uri;
      sec_res.attributes["type"] = ext.c_str();
      object->m_SecResources.Add(sec_res);
      sec_res.name = "CaptionInfo";
      object->m_SecResources.Add(sec_res);

      // adding subtitle uri for movie md5, for later use in http response
      NPT_String movie_md5 = object->m_Resources[0].m_Uri;
      movie_md5 = movie_md5.Right(movie_md5.GetLength() - movie_md5.Find("/%25/") - 5);
      upnp_server->AddSubtitleUriForSecResponse(movie_md5, subtitle_uri);
    }
  }

  AddAlternateMimeResources(*object);

  return object;

failure:
  delete object;
  return NULL;
}

const std::string& CorrectAllItemsSortHack(const std::string& item)
{
  // This is required as in order for the "* All Albums" etc. items to sort
  // correctly, they must have fake artist/album etc. information generated.
  // This looks nasty if we attempt to render it to the GUI, thus this (further)
  // workaround
  if ((item.size() == 1 && item[0] == 0x01) ||
      (item.size() > 1 && ((unsigned char)item[1]) == 0xff))
    return StringUtils::Empty;

  return item;
}

int PopulateTagFromObject(CMusicInfoTag& tag,
                          PLT_MediaObject& object,
                          PLT_MediaItemResource* resource /* = NULL */,
                          UPnPService service /* = UPnPServiceNone */)
{
  tag.SetTitle((const char*)object.m_Title);
  tag.SetArtist((const char*)object.m_Creator);

  // An artist is listed under no role and again as a performer
  std::vector<std::string> artists;
  std::vector<std::string> albumArtists;
  for (auto it = object.m_People.artists.GetFirstItem(); it; ++it)
  {
    std::vector<std::string>& names{it->role == "AlbumArtist" ? albumArtists : artists};
    if ((it->role.IsEmpty() || it->role == "Performer" || it->role == "AlbumArtist") &&
        std::ranges::find(names, it->name.GetChars()) == names.end())
      names.emplace_back(it->name.GetChars());
  }
  if (!artists.empty())
    tag.SetArtist(artists, true);
  if (!albumArtists.empty())
    tag.SetAlbumArtist(albumArtists, true);
  tag.SetTrackNumber(object.m_MiscInfo.original_track_number);
  tag.SetGenre(GenresOf(object));

  tag.SetAlbum((const char*)object.m_Affiliation.album);
  CDateTime last;
  last.SetFromW3CDateTime((const char*)object.m_MiscInfo.last_time);
  tag.SetLastPlayed(last);
  tag.SetPlayCount(object.m_MiscInfo.play_count);
  if (resource)
    tag.SetDuration(resource->m_Duration);
  tag.SetLoaded();
  return NPT_SUCCESS;
}

int PopulateTagFromObject(CVideoInfoTag& tag,
                          PLT_MediaObject& object,
                          PLT_MediaItemResource* resource /* = NULL */,
                          UPnPService service /* = UPnPServiceNone */)
{
  CDateTime date;
  date.SetFromW3CDate((const char*)object.m_Date);

  if (!object.m_Recorded.program_title.IsEmpty() ||
      object.m_ObjectClass.type == "object.item.videoItem.videoBroadcast")
  {
    tag.m_type = MediaTypeEpisode;
    tag.m_strShowTitle = object.m_Recorded.series_title;
    if (date.IsValid())
      tag.m_firstAired = date;

    int title = object.m_Recorded.program_title.Find(" : ");
    if (title >= 0)
      tag.m_strTitle = object.m_Recorded.program_title.SubString(title + 3);
    else
      tag.m_strTitle = object.m_Recorded.program_title;

    int episode;
    int season;
    if (object.m_Recorded.episode_number > 0 && object.m_Recorded.episode_season < (NPT_UInt32)-1)
    {
      tag.m_iEpisode = object.m_Recorded.episode_number;
      tag.m_iSeason = object.m_Recorded.episode_season;
    }
    else if (sscanf(object.m_Recorded.program_title, "S%2dE%2d", &season, &episode) == 2 &&
             title >= 0)
    {
      tag.m_iEpisode = episode;
      tag.m_iSeason = season;
    }
    else
    {
      tag.m_iSeason = object.m_Recorded.episode_number / 100;
      tag.m_iEpisode = object.m_Recorded.episode_number % 100;
    }
  }
  else
  {
    tag.m_strTitle = object.m_Title;
    if (date.IsValid())
      tag.m_premiered = date;

    if (!object.m_Recorded.series_title.IsEmpty())
    {
      if (object.m_ObjectClass.type == "object.container.album.videoAlbum.videoBroadcastSeason")
      {
        tag.m_type = MediaTypeSeason;
        tag.m_iSeason = object.m_Recorded.episode_season;
        tag.m_strShowTitle = object.m_Recorded.series_title;
      }
      else
      {
        tag.m_type = MediaTypeTvShow;
        tag.m_strShowTitle = object.m_Title;
      }

      if (object.m_Recorded.episode_count > 0)
        tag.m_iEpisode = object.m_Recorded.episode_count;
      else
        tag.m_iEpisode = object.m_Recorded.episode_number;
    }
    else if (object.m_ObjectClass.type == "object.item.videoItem.musicVideoClip")
    {
      tag.m_type = MediaTypeMusicVideo;

      if (object.m_People.artists.GetItemCount() > 0)
        tag.m_artist = NamesOf(object.m_People.artists);
      else if (!object.m_Creator.IsEmpty() && object.m_Creator != "Unknown")
        tag.m_artist = StringUtils::Split(
            object.m_Creator.GetChars(),
            CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
      tag.m_strAlbum = object.m_Affiliation.album;
    }
    else
      tag.m_type = MediaTypeMovie;

    tag.m_strTitle = object.m_Title;
    if (date.IsValid())
      tag.SetPremiered(date);
  }

  tag.m_studio = StringsOf(object.m_People.publisher);

  tag.m_dateAdded.SetFromW3CDate((const char*)object.m_XbmcInfo.date_added);
  tag.SetRating(object.m_XbmcInfo.rating, object.m_XbmcInfo.votes);
  tag.SetUniqueID(object.m_XbmcInfo.unique_identifier.GetChars());
  tag.m_country = StringsOf(object.m_XbmcInfo.countries);
  tag.m_iUserRating = object.m_XbmcInfo.user_rating;

  tag.m_genre = GenresOf(object);
  tag.m_director = NamesOf(object.m_People.directors);
  tag.m_writingCredits = NamesOf(object.m_People.authors);
  for (auto it = object.m_People.actors.GetFirstItem(); it; ++it)
  {
    SActorInfo info;
    info.strName = it->name;
    info.strRole = it->role;
    tag.m_cast.push_back(info);
  }
  tag.m_strTagLine = object.m_Description.description;
  tag.m_strPlot = object.m_Description.long_description;
  tag.m_strMPAARating = object.m_Description.rating;
  tag.m_strShowTitle = object.m_Recorded.series_title;
  tag.m_lastPlayed.SetFromW3CDateTime((const char*)object.m_MiscInfo.last_time);
  tag.SetPlayCount(object.m_MiscInfo.play_count);

  if (resource)
  {
    if (resource->m_Duration)
      tag.SetDuration(resource->m_Duration);
    if (object.m_MiscInfo.last_position > 0)
    {
      tag.SetResumePoint(object.m_MiscInfo.last_position, resource->m_Duration,
                         object.m_XbmcInfo.last_playerstate.GetChars());
    }
    if (!resource->m_Resolution.IsEmpty())
    {
      int width, height;
      if (sscanf(resource->m_Resolution, "%dx%d", &width, &height) == 2)
      {
        CStreamDetailVideo* detail = new CStreamDetailVideo;
        detail->m_iWidth = width;
        detail->m_iHeight = height;
        detail->m_iDuration = tag.GetDuration();
        tag.m_streamDetails.AddStream(detail);
      }
    }
    if (resource->m_NbAudioChannels > 0)
    {
      CStreamDetailAudio* detail = new CStreamDetailAudio;
      detail->m_iChannels = resource->m_NbAudioChannels;
      tag.m_streamDetails.AddStream(detail);
    }
    tag.m_streamDetails.SetSources(CStreamDetail::EXTERNAL);
  }
  return NPT_SUCCESS;
}

std::shared_ptr<CFileItem> BuildObject(PLT_MediaObject* entry,
                                       UPnPService upnp_service /* = UPnPServiceNone */)
{
  NPT_String ObjectClass = entry->m_ObjectClass.type.ToLowercase();

  CFileItemPtr pItem(new CFileItem((const char*)entry->m_Title));
  pItem->SetLabelPreformatted(true);
  pItem->SetTitle(static_cast<const char*>(entry->m_Title));
  pItem->SetFolder(entry->IsContainer());

  // if it's a container, format a string as upnp://uuid/object_id
  if (pItem->IsFolder())
  {

    // look for metadata
    if (ObjectClass.StartsWith("object.container.album.videoalbum"))
    {
      pItem->SetLabelPreformatted(false);
      UPNP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *entry, NULL, upnp_service);
    }
    else if (ObjectClass.StartsWith("object.container.album.photoalbum"))
    {
      //CPictureInfoTag* tag = pItem->GetPictureInfoTag();
    }
    else if (ObjectClass.StartsWith("object.container.album"))
    {
      pItem->SetLabelPreformatted(false);
      UPNP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *entry, NULL, upnp_service);
    }
  }
  else
  {
    const char* content{ContentOfClass(entry->m_ObjectClass.type)};
    const std::string_view contentView{content ? content : ""};
    const bool video{contentView == "video"};
    const bool audio{contentView == "audio"};
    const bool image{contentView == "image"};
    if (content)
      pItem->SetMimeType(StringUtils::Format("{}/octet-stream", content));

    // attempt to find a valid resource (may be multiple)
    PLT_MediaItemResource resource, *res = NULL;
    if (NPT_SUCCEEDED(
            NPT_ContainerFind(entry->m_Resources, CResourceFinder("http-get", content), resource)))
    {

      // set metadata
      if (resource.m_Size != (NPT_LargeSize)-1)
      {
        pItem->SetSize(resource.m_Size);
      }
      res = &resource;
    }
    // look for metadata
    if (video)
    {
      pItem->SetLabelPreformatted(false);
      UPNP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *entry, res, upnp_service);
    }
    else if (audio)
    {
      pItem->SetLabelPreformatted(false);
      UPNP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *entry, res, upnp_service);
    }
    else if (image)
    {
      //! @todo fill pictureinfotag?
      GetResource(entry, *pItem);
    }
  }

  // look for date?
  if (entry->m_Date.GetLength())
  {
    CDateTime date;
    date.SetFromW3CDateTime((const char*)entry->m_Date);
    if (date.IsValid())
      pItem->SetDateTime(date);
  }

  // if there is a thumbnail available set it here
  if (entry->m_ExtraInfo.album_arts.GetItem(0))
    // only considers first album art
    pItem->SetArt(ART::TYPE::THUMB, (const char*)entry->m_ExtraInfo.album_arts.GetItem(0)->uri);
  else if (entry->m_Description.icon_uri.GetLength())
    pItem->SetArt(ART::TYPE::THUMB, (const char*)entry->m_Description.icon_uri);

  for (unsigned int index = 0; index < entry->m_XbmcInfo.artwork.GetItemCount(); index++)
    pItem->SetArt(entry->m_XbmcInfo.artwork.GetItem(index)->type.GetChars(),
                  entry->m_XbmcInfo.artwork.GetItem(index)->url.GetChars());

  // set the watched overlay, as this will not be set later due to
  // content set on file item list
  if (pItem->HasVideoInfoTag())
  {
    int episodes = pItem->GetVideoInfoTag()->m_iEpisode;
    int played = pItem->GetVideoInfoTag()->GetPlayCount();
    const std::string& type = pItem->GetVideoInfoTag()->m_type;
    bool watched(false);
    if (type == MediaTypeTvShow || type == MediaTypeSeason)
    {
      pItem->SetProperty("totalepisodes", episodes);
      pItem->SetProperty("numepisodes", episodes);
      pItem->SetProperty("watchedepisodes", played);
      pItem->SetProperty("unwatchedepisodes", episodes - played);
      pItem->SetProperty("watchedepisodepercent", episodes > 0 ? played * 100 / episodes : 0);
      watched = (episodes && played >= episodes);
      pItem->GetVideoInfoTag()->SetPlayCount(watched ? 1 : 0);
    }
    else if (type == MediaTypeEpisode || type == MediaTypeMovie)
      watched = (played > 0);
    pItem->SetOverlayImage(watched ? CGUIListItem::ICON_OVERLAY_WATCHED
                                   : CGUIListItem::ICON_OVERLAY_UNWATCHED);
  }
  return pItem;
}

struct ResourcePrioritySort
{
  explicit ResourcePrioritySort(const PLT_MediaObject* entry)
  {
    if (const char* content = ContentOfClass(entry->m_ObjectClass.type); content)
      m_content = content;
  }

  int GetPriority(const PLT_MediaItemResource& res) const
  {
    int prio = 0;

    if (m_content != "" && res.m_ProtocolInfo.GetContentType().StartsWith(m_content))
      prio += 400;

    NPT_Url url(res.m_Uri);
    if (URIUtils::IsHostOnLAN(url.GetHost().GetChars(), LanCheckMode::ONLY_LOCAL_SUBNET))
      prio += 300;

    if (res.m_ProtocolInfo.GetProtocol() == "xbmc-get")
      prio += 200;
    else if (res.m_ProtocolInfo.GetProtocol() == "http-get")
      prio += 100;

    return prio;
  }

  int operator()(const PLT_MediaItemResource& lh, const PLT_MediaItemResource& rh) const
  {
    if (GetPriority(lh) < GetPriority(rh))
      return 1;
    else
      return 0;
  }

  NPT_String m_content;
};

bool GetResource(const PLT_MediaObject* entry, CFileItem& item)
{
  static Logger logger = CServiceBroker::GetLogging().GetLogger("CUPnPDirectory::GetResource");

  PLT_MediaItemResource resource;

  // store original path so we remember it
  item.SetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL, item.GetPath());
  item.SetProperty("original_listitem_mime", item.GetMimeType());

  // get a sorted list based on our preference
  NPT_List<PLT_MediaItemResource> sorted;
  for (NPT_Cardinal i = 0; i < entry->m_Resources.GetItemCount(); ++i)
  {
    sorted.Add(entry->m_Resources[i]);
  }
  sorted.Sort(ResourcePrioritySort(entry));

  if (sorted.GetItemCount() == 0)
    return false;

  resource = *sorted.GetFirstItem();

  // if it's an item, path is the first url to the item
  // we hope the server made the first one reachable for us
  // (it could be a format we dont know how to play however)
  item.SetDynPath((const char*)resource.m_Uri);

  // look for content type in protocol info
  if (resource.m_ProtocolInfo.IsValid())
  {
    logger->debug("resource protocol info '{}'", (const char*)(resource.m_ProtocolInfo.ToString()));

    if (resource.m_ProtocolInfo.GetContentType().Compare(GENERIC_MIME_TYPE) != 0)
    {
      item.SetMimeType((const char*)resource.m_ProtocolInfo.GetContentType());
    }

    // if this is an image fill the thumb of the item
    if (StringUtils::StartsWithNoCase(resource.m_ProtocolInfo.GetContentType().GetChars(), "image"))
    {
      item.SetArt(ART::TYPE::THUMB, std::string_view(resource.m_Uri));
    }
  }
  else
  {
    logger->error("invalid protocol info '{}'", (const char*)(resource.m_ProtocolInfo.ToString()));
  }

  // look for subtitles
  unsigned subIdx = 0;

  for (unsigned r = 0; r < entry->m_Resources.GetItemCount(); r++)
  {
    const PLT_MediaItemResource& res = entry->m_Resources[r];
    const PLT_ProtocolInfo& info = res.m_ProtocolInfo;

    const std::string type = info.GetContentType().Split("/").GetLastItem()->GetChars();
    if (SupportedSubFormats.contains(type))
    {
      ++subIdx;
      logger->info("adding subtitle: #{}, type '{}', URI '{}'", subIdx, type, res.m_Uri.GetChars());

      std::string prop = StringUtils::Format("subtitle:{}", subIdx);
      item.SetProperty(prop, (const char*)res.m_Uri);
    }
  }
  return true;
}

std::shared_ptr<CFileItem> GetFileItem(const NPT_String& uri, const NPT_String& meta)
{
  PLT_MediaObjectListReference list;
  PLT_MediaObject* object = NULL;
  CFileItemPtr item;

  if (NPT_SUCCEEDED(PLT_Didl::FromDidl(meta, list)))
  {
    list->Get(0, object);
  }

  if (object)
  {
    item = BuildObject(object);
  }

  if (item)
  {
    item->SetPath((const char*)uri);
    GetResource(object, *item);
  }
  else
  {
    item = std::make_shared<CFileItem>((const char*)uri, false);
  }
  return item;
}

NPT_String EncodeObjectId(const std::string& id)
{
  if (id.empty())
  {
    CLog::LogF(LOGWARNING, "Failed to encode object id, provided object id is empty");
    return {};
  }
  if (IsIntegerObjectId(id))
    return id.c_str();

  return Base64::Encode(id).c_str();
}

NPT_String DecodeObjectId(const std::string& id)
{
  if (id.empty())
  {
    CLog::LogF(LOGWARNING, "Failed to decode object id, provided object id is empty");
    return {};
  }
  if (IsIntegerObjectId(id))
    return id.c_str();
  // if the provided object id is a url (contains :// and thus not valid in the base64 dictionary)
  // we are trying to decode a plain vfs path. In such cases, return the id as is for backward
  // compatibility with external clients/tools relying on plain vfs paths
  if (URIUtils::IsURL(id))
  {
    return id.c_str();
  }
  // otherwise try to decode the path
  const std::string decodedObjectId = Base64::Decode(id);
  if (decodedObjectId.empty())
  {
    CLog::LogF(LOGERROR, "Failed to decode object id {}, not properly Base64 encoded", id);
  }

  return decodedObjectId.c_str();
}

} /* namespace UPNP */
