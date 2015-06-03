/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
#include <Platinum/Source/Platinum/Platinum.h>

#include "UPnPInternal.h"
#include "UPnP.h"
#include "UPnPServer.h"
#include "URL.h"
#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "TextureDatabase.h"
#include "ThumbLoader.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "utils/LangCodeExpander.h"

#include <algorithm>

using namespace MUSIC_INFO;
using namespace XFILE;

namespace UPNP
{

/*----------------------------------------------------------------------
|  GetClientQuirks
+---------------------------------------------------------------------*/
EClientQuirks GetClientQuirks(const PLT_HttpRequestContext* context)
{
  if(context == NULL)
      return ECLIENTQUIRKS_NONE;

  unsigned int quirks = 0;
  const NPT_String* user_agent = context->GetRequest().GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_USER_AGENT);
  const NPT_String* server     = context->GetRequest().GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_SERVER);

  if (user_agent) {
      if (user_agent->Find("XBox", 0, true) >= 0 ||
          user_agent->Find("Xenon", 0, true) >= 0)
          quirks |= ECLIENTQUIRKS_ONLYSTORAGEFOLDER | ECLIENTQUIRKS_BASICVIDEOCLASS;

      if (user_agent->Find("Windows-Media-Player", 0, true) >= 0)
          quirks |= ECLIENTQUIRKS_UNKNOWNSERIES;

  }
  if (server) {
      if (server->Find("Xbox", 0, true) >= 0)
          quirks |= ECLIENTQUIRKS_ONLYSTORAGEFOLDER | ECLIENTQUIRKS_BASICVIDEOCLASS;
  }

  return (EClientQuirks)quirks;
}

/*----------------------------------------------------------------------
|  GetMediaControllerQuirks
+---------------------------------------------------------------------*/
EMediaControllerQuirks GetMediaControllerQuirks(const PLT_DeviceData *device)
{
    if (device == NULL)
        return EMEDIACONTROLLERQUIRKS_NONE;

    unsigned int quirks = 0;

    if (device->m_Manufacturer.Find("Samsung Electronics") >= 0)
        quirks |= EMEDIACONTROLLERQUIRKS_X_MKV;

    return (EMediaControllerQuirks)quirks;
}

/*----------------------------------------------------------------------
|   GetMimeType
+---------------------------------------------------------------------*/
NPT_String
GetMimeType(const char* filename,
            const PLT_HttpRequestContext* context /* = NULL */)
{
    NPT_String ext = URIUtils::GetExtension(filename).c_str();
    ext.TrimLeft('.');
    ext = ext.ToLowercase();

    return PLT_MimeType::GetMimeTypeFromExtension(ext, context);
}

/*----------------------------------------------------------------------
|   GetMimeType
+---------------------------------------------------------------------*/
NPT_String
GetMimeType(const CFileItem& item,
            const PLT_HttpRequestContext* context /* = NULL */)
{
    std::string path = item.GetPath();
    if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().empty()) {
        path = item.GetVideoInfoTag()->GetPath();
    } else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().empty()) {
        path = item.GetMusicInfoTag()->GetURL();
    }

    if (URIUtils::IsStack(path))
        path = XFILE::CStackDirectory::GetFirstStackedFile(path);

    NPT_String ext = URIUtils::GetExtension(path).c_str();
    ext.TrimLeft('.');
    ext = ext.ToLowercase();

    NPT_String mime;

    /* We always use Platinum mime type first
       as it is defined to map extension to DLNA compliant mime type
       or custom according to context (who asked for it) */
    if (!ext.IsEmpty()) {
        mime = PLT_MimeType::GetMimeTypeFromExtension(ext, context);
        if (mime == "application/octet-stream") mime = "";
    }

    /* if Platinum couldn't map it, default to XBMC mapping */
    if (mime.IsEmpty()) {
        NPT_String mime = item.GetMimeType().c_str();
        if (mime == "application/octet-stream") mime = "";
    }

    /* fallback to generic mime type if not found */
    if (mime.IsEmpty()) {
        if (item.IsVideo() || item.IsVideoDb() )
            mime = "video/" + ext;
        else if (item.IsAudio() || item.IsMusicDb() )
            mime = "audio/" + ext;
        else if (item.IsPicture() )
            mime = "image/" + ext;
        else if (item.IsSubtitle())
            mime = "text/" + ext;
    }

    /* nothing we can figure out */
    if (mime.IsEmpty()) {
        mime = "application/octet-stream";
    }

    return mime;
}

/*----------------------------------------------------------------------
|   GetProtocolInfo
+---------------------------------------------------------------------*/
const NPT_String
GetProtocolInfo(const CFileItem&              item,
                const char*                   protocol,
                const PLT_HttpRequestContext* context /* = NULL */)
{
    NPT_String proto = protocol;

    /* fixup the protocol just in case nothing was passed */
    if (proto.IsEmpty()) {
        proto = item.GetURL().GetProtocol().c_str();
    }

    /*
       map protocol to right prefix and use xbmc-get for
       unsupported UPnP protocols for other xbmc clients
       TODO: add rtsp ?
    */
    if (proto == "http") {
        proto = "http-get";
    } else {
        proto = "xbmc-get";
    }

    /* we need a valid extension to retrieve the mimetype for the protocol info */
    NPT_String mime = GetMimeType(item, context);
    proto += ":*:" + mime + ":" + PLT_ProtocolInfo::GetDlnaExtension(mime, context);
    return proto;
}

  /*----------------------------------------------------------------------
   |   CResourceFinder
   +---------------------------------------------------------------------*/
CResourceFinder::CResourceFinder(const char* protocol, const char* content)
  : m_Protocol(protocol)
  , m_Content(content)
{
}

bool CResourceFinder::operator()(const PLT_MediaItemResource& resource) const {
    if (m_Content.IsEmpty())
        return (resource.m_ProtocolInfo.GetProtocol().Compare(m_Protocol, true) == 0);
    else
        return ((resource.m_ProtocolInfo.GetProtocol().Compare(m_Protocol, true) == 0)
              && resource.m_ProtocolInfo.GetContentType().StartsWith(m_Content, true));
}

/*----------------------------------------------------------------------
|   PopulateObjectFromTag
+---------------------------------------------------------------------*/
NPT_Result
PopulateObjectFromTag(CMusicInfoTag&         tag,
                      PLT_MediaObject&       object,
                      NPT_String*            file_path,
                      PLT_MediaItemResource* resource,
                      EClientQuirks          quirks,
                      UPnPService            service /* = UPnPServiceNone */)
{
    if (!tag.GetURL().empty() && file_path)
      *file_path = tag.GetURL().c_str();

    std::vector<std::string> genres = tag.GetGenre();
    for (unsigned int index = 0; index < genres.size(); index++)
      object.m_Affiliation.genres.Add(genres.at(index).c_str());
    object.m_Title = tag.GetTitle().c_str();
    object.m_Affiliation.album = tag.GetAlbum().c_str();
    for (unsigned int index = 0; index < tag.GetArtist().size(); index++)
    {
      object.m_People.artists.Add(tag.GetArtist().at(index).c_str());
      object.m_People.artists.Add(tag.GetArtist().at(index).c_str(), "Performer");
    }
    object.m_People.artists.Add(StringUtils::Join(!tag.GetAlbumArtist().empty() ? tag.GetAlbumArtist() : tag.GetArtist(), g_advancedSettings.m_musicItemSeparator).c_str(), "AlbumArtist");
    if(tag.GetAlbumArtist().empty())
        object.m_Creator = StringUtils::Join(tag.GetArtist(), g_advancedSettings.m_musicItemSeparator).c_str();
    else
        object.m_Creator = StringUtils::Join(tag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator).c_str();
    object.m_MiscInfo.original_track_number = tag.GetTrackNumber();
    if(tag.GetDatabaseId() >= 0) {
      object.m_ReferenceID = NPT_String::Format("musicdb://songs/%i%s", tag.GetDatabaseId(), URIUtils::GetExtension(tag.GetURL()).c_str());
    }
    if (object.m_ReferenceID == object.m_ObjectID)
        object.m_ReferenceID = "";

    object.m_MiscInfo.last_time = tag.GetLastPlayed().GetAsW3CDateTime().c_str();
    object.m_MiscInfo.play_count = tag.GetPlayCount();

    if (resource) resource->m_Duration = tag.GetDuration();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PopulateObjectFromTag
+---------------------------------------------------------------------*/
NPT_Result
PopulateObjectFromTag(CVideoInfoTag&         tag,
                      PLT_MediaObject&       object,
                      NPT_String*            file_path,
                      PLT_MediaItemResource* resource,
                      EClientQuirks          quirks,
                      UPnPService            service /* = UPnPServiceNone */)
{
    if (!tag.m_strFileNameAndPath.empty() && file_path)
      *file_path = tag.m_strFileNameAndPath.c_str();

    if (tag.m_iDbId != -1 ) {
        if (tag.m_type == MediaTypeMusicVideo) {
          object.m_ObjectClass.type = "object.item.videoItem.musicVideoClip";
          object.m_Creator = StringUtils::Join(tag.m_artist, g_advancedSettings.m_videoItemSeparator).c_str();
          for (std::vector<std::string>::const_iterator itArtist = tag.m_artist.begin(); itArtist != tag.m_artist.end(); ++itArtist)
              object.m_People.artists.Add(itArtist->c_str());
          object.m_Affiliation.album = tag.m_strAlbum.c_str();
          object.m_Title = tag.m_strTitle.c_str();
          object.m_Date = CDateTime(tag.m_iYear, 1, 1, 0, 0, 0).GetAsW3CDate().c_str();
          object.m_ReferenceID = NPT_String::Format("videodb://musicvideos/titles/%i", tag.m_iDbId);
        } else if (tag.m_type == MediaTypeMovie) {
          object.m_ObjectClass.type = "object.item.videoItem.movie";
          object.m_Title = tag.m_strTitle.c_str();
          object.m_Date = CDateTime(tag.m_iYear, 1, 1, 0, 0, 0).GetAsW3CDate().c_str();
          object.m_ReferenceID = NPT_String::Format("videodb://movies/titles/%i", tag.m_iDbId);
        } else {
          object.m_ObjectClass.type = "object.item.videoItem.videoBroadcast";
          object.m_Recorded.program_title  = "S" + ("0" + NPT_String::FromInteger(tag.m_iSeason)).Right(2);
          object.m_Recorded.program_title += "E" + ("0" + NPT_String::FromInteger(tag.m_iEpisode)).Right(2);
          object.m_Recorded.program_title += (" : " + tag.m_strTitle).c_str();
          object.m_Recorded.series_title = tag.m_strShowTitle.c_str();
          int season = tag.m_iSeason > 1 ? tag.m_iSeason : 1;
          object.m_Recorded.episode_number = season * 100 + tag.m_iEpisode;
          object.m_Title = object.m_Recorded.series_title + " - " + object.m_Recorded.program_title;
          object.m_Date = tag.m_firstAired.GetAsW3CDate().c_str();
          if(tag.m_iSeason != -1)
              object.m_ReferenceID = NPT_String::Format("videodb://tvshows/0/%i", tag.m_iDbId);
        }
    }

    if(quirks & ECLIENTQUIRKS_BASICVIDEOCLASS)
        object.m_ObjectClass.type = "object.item.videoItem";

    if(object.m_ReferenceID == object.m_ObjectID)
        object.m_ReferenceID = "";

    for (unsigned int index = 0; index < tag.m_studio.size(); index++)
        object.m_People.publisher.Add(tag.m_studio[index].c_str());

    object.m_XbmcInfo.date_added = tag.m_dateAdded.GetAsW3CDate().c_str();
    object.m_XbmcInfo.rating = tag.m_fRating;
    object.m_XbmcInfo.votes = tag.m_strVotes.c_str();
    object.m_XbmcInfo.unique_identifier = tag.m_strIMDBNumber.c_str();

    for (unsigned int index = 0; index < tag.m_genre.size(); index++)
      object.m_Affiliation.genres.Add(tag.m_genre.at(index).c_str());

    for(CVideoInfoTag::iCast it = tag.m_cast.begin();it != tag.m_cast.end();it++) {
        object.m_People.actors.Add(it->strName.c_str(), it->strRole.c_str());
    }

    for (unsigned int index = 0; index < tag.m_director.size(); index++)
      object.m_People.directors.Add(tag.m_director[index].c_str());

    for (unsigned int index = 0; index < tag.m_writingCredits.size(); index++)
      object.m_People.authors.Add(tag.m_writingCredits[index].c_str());

    object.m_Description.description = tag.m_strTagLine.c_str();
    object.m_Description.long_description = tag.m_strPlot.c_str();
    object.m_Description.rating = tag.m_strMPAARating.c_str();
    object.m_MiscInfo.last_position = (NPT_UInt32)tag.m_resumePoint.timeInSeconds;
    object.m_MiscInfo.last_time = tag.m_lastPlayed.GetAsW3CDateTime().c_str();
    object.m_MiscInfo.play_count = tag.m_playCount;
    if (resource) {
        resource->m_Duration = tag.GetDuration();
        if (tag.HasStreamDetails()) {
            const CStreamDetails &details = tag.m_streamDetails;
            resource->m_Resolution = NPT_String::FromInteger(details.GetVideoWidth()) + "x" + NPT_String::FromInteger(details.GetVideoHeight());
            resource->m_NbAudioChannels = details.GetAudioChannels();
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BuildObject
+---------------------------------------------------------------------*/
PLT_MediaObject*
BuildObject(CFileItem&                    item,
            NPT_String&                   file_path,
            bool                          with_count,
            NPT_Reference<CThumbLoader>&  thumb_loader,
            const PLT_HttpRequestContext* context /* = NULL */,
            CUPnPServer*                  upnp_server /* = NULL */,
            UPnPService                   upnp_service /* = UPnPServiceNone */)
{
    PLT_MediaItemResource resource;
    PLT_MediaObject*      object = NULL;
    std::string thumb;

    CLog::Log(LOGDEBUG, "UPnP: Building didl for object '%s'", item.GetPath().c_str());

    EClientQuirks quirks = GetClientQuirks(context);

    // get list of ip addresses
    NPT_List<NPT_IpAddress> ips;
    NPT_HttpUrl rooturi;
    NPT_CHECK_LABEL(PLT_UPnPMessageHelper::GetIPAddresses(ips), failure);

    // if we're passed an interface where we received the request from
    // move the ip to the top
    if (context && context->GetLocalAddress().GetIpAddress().ToString() != "0.0.0.0") {
        rooturi = NPT_HttpUrl(context->GetLocalAddress().GetIpAddress().ToString(), context->GetLocalAddress().GetPort(), "/");
        ips.Remove(context->GetLocalAddress().GetIpAddress());
        ips.Insert(ips.GetFirstItem(), context->GetLocalAddress().GetIpAddress());
    } else if(upnp_server) {
        rooturi = NPT_HttpUrl("localhost", upnp_server->GetPort(), "/");
    }

    if (!item.m_bIsFolder) {
        object = new PLT_MediaItem();
        object->m_ObjectID = item.GetPath().c_str();

        /* Setup object type */
        if (item.IsMusicDb() || item.IsAudio()) {
            object->m_ObjectClass.type = "object.item.audioItem.musicTrack";

            if (item.HasMusicInfoTag()) {
                CMusicInfoTag *tag = (CMusicInfoTag*)item.GetMusicInfoTag();
                PopulateObjectFromTag(*tag, *object, &file_path, &resource, quirks, upnp_service);
            }
        } else if (item.IsVideoDb() || item.IsVideo()) {
            object->m_ObjectClass.type = "object.item.videoItem";

            if(quirks & ECLIENTQUIRKS_UNKNOWNSERIES)
                object->m_Affiliation.album = "[Unknown Series]";

            if (item.HasVideoInfoTag()) {
                CVideoInfoTag *tag = (CVideoInfoTag*)item.GetVideoInfoTag();
                PopulateObjectFromTag(*tag, *object, &file_path, &resource, quirks, upnp_service);
            }
        } else if (item.IsPicture()) {
            object->m_ObjectClass.type = "object.item.imageItem.photo";
        } else {
            object->m_ObjectClass.type = "object.item";
        }

        // duration of zero is invalid
        if (resource.m_Duration == 0) resource.m_Duration = -1;

        // Set the resource file size
        resource.m_Size = item.m_dwSize;
        if(resource.m_Size == 0)
          resource.m_Size = (NPT_LargeSize)-1;

        // set date
        if (object->m_Date.IsEmpty() && item.m_dateTime.IsValid()) {
            object->m_Date = item.m_dateTime.GetAsW3CDate().c_str();
        }

        if (upnp_server) {
            upnp_server->AddSafeResourceUri(object, rooturi, ips, file_path, GetProtocolInfo(item, "http", context));
        }

        // if the item is remote, add a direct link to the item
        if (URIUtils::IsRemote((const char*)file_path)) {
            resource.m_ProtocolInfo = PLT_ProtocolInfo(GetProtocolInfo(item, item.GetURL().GetProtocol().c_str(), context));
            resource.m_Uri = file_path;

            // if the direct link can be served directly using http, then push it in front
            // otherwise keep the xbmc-get resource last and let a compatible client look for it
            if (resource.m_ProtocolInfo.ToString().StartsWith("xbmc", true)) {
                object->m_Resources.Add(resource);
            } else {
                object->m_Resources.Insert(object->m_Resources.GetFirstItem(), resource);
            }
        }

        // copy across the known metadata
        for(unsigned i=0; i<object->m_Resources.GetItemCount(); i++) {
            object->m_Resources[i].m_Size       = resource.m_Size;
            object->m_Resources[i].m_Duration   = resource.m_Duration;
            object->m_Resources[i].m_Resolution = resource.m_Resolution;
        }

        // Some upnp clients expect all audio items to have parent root id 4
#ifdef WMP_ID_MAPPING
        object->m_ParentID = "4";
#endif
    } else {
        PLT_MediaContainer* container = new PLT_MediaContainer;
        object = container;

        /* Assign a title and id for this container */
        container->m_ObjectID = item.GetPath().c_str();
        container->m_ObjectClass.type = "object.container";
        container->m_ChildrenCount = -1;

        /* this might be overkill, but hey */
        if (item.IsMusicDb()) {
            MUSICDATABASEDIRECTORY::NODE_TYPE node = CMusicDatabaseDirectory::GetDirectoryType(item.GetPath());
            switch(node) {
                case MUSICDATABASEDIRECTORY::NODE_TYPE_ARTIST: {
                      container->m_ObjectClass.type += ".person.musicArtist";
                      CMusicInfoTag *tag = (CMusicInfoTag*)item.GetMusicInfoTag();
                      if (tag) {
                          container->m_People.artists.Add(
                              CorrectAllItemsSortHack(StringUtils::Join(tag->GetArtist(), g_advancedSettings.m_musicItemSeparator)).c_str(), "Performer");
                          container->m_People.artists.Add(
                              CorrectAllItemsSortHack(StringUtils::Join(!tag->GetAlbumArtist().empty() ? tag->GetAlbumArtist() : tag->GetArtist(), g_advancedSettings.m_musicItemSeparator)).c_str(), "AlbumArtist");
                      }
#ifdef WMP_ID_MAPPING
                      // Some upnp clients expect all artists to have parent root id 107
                      container->m_ParentID = "107";
#endif
                  }
                  break;
                case MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM:
                case MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_COMPILATIONS:
                case MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_ADDED:
                case MUSICDATABASEDIRECTORY::NODE_TYPE_YEAR_ALBUM: {
                      container->m_ObjectClass.type += ".album.musicAlbum";
                      // for Sonos to be happy
                      CMusicInfoTag *tag = (CMusicInfoTag*)item.GetMusicInfoTag();
                      if (tag) {
                          container->m_People.artists.Add(
                              CorrectAllItemsSortHack(StringUtils::Join(tag->GetArtist(), g_advancedSettings.m_musicItemSeparator)).c_str(), "Performer");
                          container->m_People.artists.Add(
                              CorrectAllItemsSortHack(StringUtils::Join(!tag->GetAlbumArtist().empty() ? tag->GetAlbumArtist() : tag->GetArtist(), g_advancedSettings.m_musicItemSeparator)).c_str(), "AlbumArtist");
                          container->m_Affiliation.album = CorrectAllItemsSortHack(tag->GetAlbum()).c_str();
                      }
#ifdef WMP_ID_MAPPING
                      // Some upnp clients expect all albums to have parent root id 7
                      container->m_ParentID = "7";
#endif
                  }
                  break;
                case MUSICDATABASEDIRECTORY::NODE_TYPE_GENRE:
                  container->m_ObjectClass.type += ".genre.musicGenre";
                  break;
                default:
                  break;
            }
        } else if (item.IsVideoDb()) {
            VIDEODATABASEDIRECTORY::NODE_TYPE node = CVideoDatabaseDirectory::GetDirectoryType(item.GetPath());
            CVideoInfoTag &tag = *(CVideoInfoTag*)item.GetVideoInfoTag();
            switch(node) {
                case VIDEODATABASEDIRECTORY::NODE_TYPE_GENRE:
                  container->m_ObjectClass.type += ".genre.movieGenre";
                  break;
                case VIDEODATABASEDIRECTORY::NODE_TYPE_ACTOR:
                  container->m_ObjectClass.type += ".person.videoArtist";
                  container->m_Creator = StringUtils::Join(tag.m_artist, g_advancedSettings.m_videoItemSeparator).c_str();
                  container->m_Title   = tag.m_strTitle.c_str();
                  break;
                case VIDEODATABASEDIRECTORY::NODE_TYPE_SEASONS:
                case VIDEODATABASEDIRECTORY::NODE_TYPE_TITLE_TVSHOWS:
                  container->m_ObjectClass.type += ".album.videoAlbum";
                  container->m_Recorded.series_title = tag.m_strShowTitle.c_str();
                  container->m_Recorded.episode_number = tag.m_iEpisode;
                  container->m_MiscInfo.play_count = tag.m_playCount;
                  container->m_Title = tag.m_strTitle.c_str();
                  if (!tag.m_premiered.IsValid() && tag.m_iYear)
                    container->m_Date = CDateTime(tag.m_iYear, 1, 1, 0, 0, 0).GetAsW3CDateTime().c_str();
                  else
                    container->m_Date = tag.m_premiered.GetAsW3CDate().c_str();

                  for (unsigned int index = 0; index < tag.m_genre.size(); index++)
                    container->m_Affiliation.genres.Add(tag.m_genre.at(index).c_str());

                  for(CVideoInfoTag::iCast it = tag.m_cast.begin();it != tag.m_cast.end();it++) {
                      container->m_People.actors.Add(it->strName.c_str(), it->strRole.c_str());
                  }

                  for (unsigned int index = 0; index < tag.m_director.size(); index++)
                    container->m_People.directors.Add(tag.m_director[index].c_str());
                  for (unsigned int index = 0; index < tag.m_writingCredits.size(); index++)
                    container->m_People.authors.Add(tag.m_writingCredits[index].c_str());

                  container->m_Description.description = tag.m_strTagLine.c_str();
                  container->m_Description.long_description = tag.m_strPlot.c_str();

                  break;
                default:
                  container->m_ObjectClass.type += ".storageFolder";
                  break;
            }
        } else if (item.IsPlayList() || item.IsSmartPlayList()) {
            container->m_ObjectClass.type += ".playlistContainer";
        }

        if(quirks & ECLIENTQUIRKS_ONLYSTORAGEFOLDER) {
          container->m_ObjectClass.type = "object.container.storageFolder";
        }

        /* Get the number of children for this container */
        if (with_count && upnp_server) {
            if (object->m_ObjectID.StartsWith("virtualpath://")) {
                NPT_LargeSize count = 0;
                NPT_CHECK_LABEL(NPT_File::GetSize(file_path, count), failure);
                container->m_ChildrenCount = (NPT_Int32)count;
            } else {
                /* this should be a standard path */
                // TODO - get file count of this directory
            }
        }
    }

    // set a title for the object
    if (object->m_Title.IsEmpty()) {
        if (!item.GetLabel().empty()) {
            std::string title = item.GetLabel();
            if (item.IsPlayList() || !item.m_bIsFolder) URIUtils::RemoveExtension(title);
            object->m_Title = title.c_str();
        }
    }

    if (upnp_server) {
        // determine the correct artwork for this item
        if (!thumb_loader.IsNull())
            thumb_loader->LoadItem(&item);

        // finally apply the found artwork
        thumb = item.GetArt("thumb");
        if (!thumb.empty()) {
            PLT_AlbumArtInfo art;
            art.uri = upnp_server->BuildSafeResourceUri(
                rooturi,
                (*ips.GetFirstItem()).ToString(),
                CTextureUtils::GetWrappedImageURL(thumb).c_str());

            // Set DLNA profileID by extension, defaulting to JPEG.
            if (URIUtils::HasExtension(thumb, ".png")) {
                art.dlna_profile = "PNG_TN";
            } else {
                art.dlna_profile = "JPEG_TN";
            }
            object->m_ExtraInfo.album_arts.Add(art);
        }

        for (CGUIListItem::ArtMap::const_iterator itArtwork = item.GetArt().begin(); itArtwork != item.GetArt().end(); ++itArtwork) {
            if (!itArtwork->first.empty() && !itArtwork->second.empty()) {
                std::string wrappedUrl = CTextureUtils::GetWrappedImageURL(itArtwork->second);
                object->m_XbmcInfo.artwork.Add(itArtwork->first.c_str(),
                  upnp_server->BuildSafeResourceUri(rooturi, (*ips.GetFirstItem()).ToString(), wrappedUrl.c_str()));
                upnp_server->AddSafeResourceUri(object, rooturi, ips, wrappedUrl.c_str(), ("xbmc.org:*:" + itArtwork->first + ":*").c_str());
            }
        }
    }

    // look for and add external subtitle if we are processing a video file and
    // we are being called by a UPnP player or renderer or the user has chosen
    // to look for external subtitles
    if (upnp_server != NULL && item.IsVideo() &&
       (upnp_service == UPnPPlayer || upnp_service == UPnPRenderer ||
        CSettings::Get().GetBool("services.upnplookforexternalsubtitles")))
    {
        // find any available external subtitles
        std::vector<std::string> filenames;
        std::vector<std::string> subtitles;
        CUtil::ScanForExternalSubtitles(file_path.GetChars(), filenames);

        std::string ext;
        for (unsigned int i = 0; i < filenames.size(); i++)
        {
            ext = URIUtils::GetExtension(filenames[i]).c_str();
            ext = ext.substr(1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            /* Hardcoded check for extension is not the best way, but it can't be allowed to pass all
               subtitle extension (ex. rar or zip). There are the most popular extensions support by UPnP devices.*/
            if (ext == "txt" || ext == "srt" || ext == "ssa" || ext == "ass" || ext == "sub" || ext == "smi")
            {
                subtitles.push_back(filenames[i]);
            }
        }

        std::string subtitlePath;

        if (subtitles.size() == 1)
        {
            subtitlePath = subtitles[0];
        }
        else if (!subtitles.empty())
        {
            /* trying to find subtitle with prefered language settings */
            std::string preferredLanguage = (CSettings::Get().GetSetting("locale.subtitlelanguage"))->ToString();
            std::string preferredLanguageCode;
            g_LangCodeExpander.ConvertToISO6392T(preferredLanguage, preferredLanguageCode);

            for (unsigned int i = 0; i < subtitles.size(); i++)
            {
                ExternalStreamInfo info;
                CUtil::GetExternalStreamDetailsFromFilename(file_path.GetChars(), subtitles[i], info);

                if (preferredLanguageCode == info.language)
                {
                    subtitlePath = subtitles[i];
                    break;
                }
            }
            /* if not found subtitle with prefered language, get the first one */
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
            upnp_server->AddSafeResourceUri(object, rooturi, ips, NPT_String(subtitlePath.c_str()), protocolInfo);
            // add subtitle resource with smi/caption protocol info (some devices)
            PLT_ProtocolInfo protInfo = PLT_ProtocolInfo(protocolInfo);
            protocolInfo = protInfo.GetProtocol() + ":" + protInfo.GetMask() + ":smi/caption:" + protInfo.GetExtra();
            upnp_server->AddSafeResourceUri(object, rooturi, ips, NPT_String(subtitlePath.c_str()), protocolInfo);

            ext = URIUtils::GetExtension(subtitlePath).c_str();
            ext = ext.substr(1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

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

    return object;

failure:
    delete object;
    return NULL;
}

/*----------------------------------------------------------------------
|   CUPnPServer::CorrectAllItemsSortHack
+---------------------------------------------------------------------*/
const std::string&
CorrectAllItemsSortHack(const std::string &item)
{
    // This is required as in order for the "* All Albums" etc. items to sort
    // correctly, they must have fake artist/album etc. information generated.
    // This looks nasty if we attempt to render it to the GUI, thus this (further)
    // workaround
    if ((item.size() == 1 && item[0] == 0x01) || (item.size() > 1 && ((unsigned char) item[1]) == 0xff))
        return StringUtils::Empty;

    return item;
}

int
PopulateTagFromObject(CMusicInfoTag&         tag,
                      PLT_MediaObject&       object,
                      PLT_MediaItemResource* resource /* = NULL */,
                      UPnPService            service /* = UPnPServiceNone */)
{
    tag.SetTitle((const char*)object.m_Title);
    tag.SetArtist((const char*)object.m_Creator);
    for(PLT_PersonRoles::Iterator it = object.m_People.artists.GetFirstItem(); it; it++) {
        if     (it->role == "")            tag.SetArtist((const char*)it->name);
        else if(it->role == "Performer")   tag.SetArtist((const char*)it->name);
        else if(it->role == "AlbumArtist") tag.SetAlbumArtist((const char*)it->name);
    }
    tag.SetTrackNumber(object.m_MiscInfo.original_track_number);

    for (NPT_List<NPT_String>::Iterator it = object.m_Affiliation.genres.GetFirstItem(); it; it++) {
        // ignore single "Unknown" genre inserted by Platinum
        if (it == object.m_Affiliation.genres.GetFirstItem() && object.m_Affiliation.genres.GetItemCount() == 1 &&
            *it == "Unknown")
            break;

        tag.SetGenre((const char*) *it);
    }

    tag.SetAlbum((const char*)object.m_Affiliation.album);
    CDateTime last;
    last.SetFromW3CDateTime((const char*)object.m_MiscInfo.last_time);
    tag.SetLastPlayed(last);
    tag.SetPlayCount(object.m_MiscInfo.play_count);
    if(resource)
        tag.SetDuration(resource->m_Duration);
    tag.SetLoaded();
    return NPT_SUCCESS;
}

int
PopulateTagFromObject(CVideoInfoTag&         tag,
                      PLT_MediaObject&       object,
                      PLT_MediaItemResource* resource /* = NULL */,
                      UPnPService            service /* = UPnPServiceNone */)
{
    CDateTime date;
    date.SetFromW3CDate((const char*)object.m_Date);

    if(!object.m_Recorded.program_title.IsEmpty())
    {
        tag.m_type = MediaTypeEpisode;
        int episode;
        int season;
        int title = object.m_Recorded.program_title.Find(" : ");
        if(sscanf(object.m_Recorded.program_title, "S%2dE%2d", &season, &episode) == 2 && title >= 0) {
            tag.m_strTitle = object.m_Recorded.program_title.SubString(title + 3);
            tag.m_iEpisode = episode;
            tag.m_iSeason  = season;
        } else {
            tag.m_strTitle = object.m_Recorded.program_title;
            tag.m_iSeason  = object.m_Recorded.episode_number / 100;
            tag.m_iEpisode = object.m_Recorded.episode_number % 100;
        }
        tag.m_firstAired = date;
    }
    else if (!object.m_Recorded.series_title.IsEmpty()) {
        tag.m_type= MediaTypeSeason;
        tag.m_strTitle = object.m_Title; // because could be TV show Title, or Season 1 etc
        tag.m_iSeason  = object.m_Recorded.episode_number / 100;
        tag.m_iEpisode = object.m_Recorded.episode_number % 100;
        tag.m_premiered = date;
    }
    else if(object.m_ObjectClass.type == "object.item.videoItem.musicVideoClip") {
        tag.m_type = MediaTypeMusicVideo;
        for (unsigned int index = 0; index < object.m_People.artists.GetItemCount(); index++)
            tag.m_artist.push_back(object.m_People.artists.GetItem(index)->name.GetChars());
        tag.m_strAlbum = object.m_Affiliation.album;
    }
    else
    {
        tag.m_type         = MediaTypeMovie;
        tag.m_strTitle     = object.m_Title;
        tag.m_premiered    = date;
    }

    if (date.IsValid())
      tag.m_iYear = date.GetYear();

    for (unsigned int index = 0; index < object.m_People.publisher.GetItemCount(); index++)
        tag.m_studio.push_back(object.m_People.publisher.GetItem(index)->GetChars());

    tag.m_dateAdded.SetFromW3CDate((const char*)object.m_XbmcInfo.date_added);
    tag.m_fRating = object.m_XbmcInfo.rating;
    tag.m_strVotes = object.m_XbmcInfo.votes;
    tag.m_strIMDBNumber = object.m_XbmcInfo.unique_identifier;

    for (unsigned int index = 0; index < object.m_Affiliation.genres.GetItemCount(); index++)
    {
      // ignore single "Unknown" genre inserted by Platinum
      if (index == 0 && object.m_Affiliation.genres.GetItemCount() == 1 &&
          *object.m_Affiliation.genres.GetItem(index) == "Unknown")
          break;

      tag.m_genre.push_back(object.m_Affiliation.genres.GetItem(index)->GetChars());
    }
    for (unsigned int index = 0; index < object.m_People.directors.GetItemCount(); index++)
      tag.m_director.push_back(object.m_People.directors.GetItem(index)->name.GetChars());
    for (unsigned int index = 0; index < object.m_People.authors.GetItemCount(); index++)
      tag.m_writingCredits.push_back(object.m_People.authors.GetItem(index)->name.GetChars());
    for (unsigned int index = 0; index < object.m_People.actors.GetItemCount(); index++)
    {
      SActorInfo info;
      info.strName = object.m_People.actors.GetItem(index)->name;
      info.strRole = object.m_People.actors.GetItem(index)->role;
      tag.m_cast.push_back(info);
    }
    tag.m_strTagLine  = object.m_Description.description;
    tag.m_strPlot     = object.m_Description.long_description;
    tag.m_strMPAARating = object.m_Description.rating;
    tag.m_strShowTitle = object.m_Recorded.series_title;
    tag.m_lastPlayed.SetFromW3CDateTime((const char*)object.m_MiscInfo.last_time);
    tag.m_playCount = object.m_MiscInfo.play_count;

    if(resource)
    {
      if (resource->m_Duration)
        tag.m_duration = resource->m_Duration;
      if (object.m_MiscInfo.last_position > 0 )
      {
        tag.m_resumePoint.totalTimeInSeconds = resource->m_Duration;
        tag.m_resumePoint.timeInSeconds = object.m_MiscInfo.last_position;
      }
      if (!resource->m_Resolution.IsEmpty())
      {
        int width, height;
        if (sscanf(resource->m_Resolution, "%dx%d", &width, &height) == 2)
        {
          CStreamDetailVideo* detail = new CStreamDetailVideo;
          detail->m_iWidth = width;
          detail->m_iHeight = height;
          detail->m_iDuration = tag.m_duration;
          tag.m_streamDetails.AddStream(detail);
        }
      }
      if (resource->m_NbAudioChannels > 0)
      {
        CStreamDetailAudio* detail = new CStreamDetailAudio;
        detail->m_iChannels = resource->m_NbAudioChannels;
        tag.m_streamDetails.AddStream(detail);
      }
    }
    return NPT_SUCCESS;
}

CFileItemPtr BuildObject(PLT_MediaObject* entry,
                         UPnPService      upnp_service /* = UPnPServiceNone */)
{
  NPT_String ObjectClass = entry->m_ObjectClass.type.ToLowercase();

  CFileItemPtr pItem(new CFileItem((const char*)entry->m_Title));
  pItem->SetLabelPreformated(true);
  pItem->m_strTitle = (const char*)entry->m_Title;
  pItem->m_bIsFolder = entry->IsContainer();

  // if it's a container, format a string as upnp://uuid/object_id
  if (pItem->m_bIsFolder) {

    // look for metadata
    if( ObjectClass.StartsWith("object.container.album.videoalbum") ) {
      pItem->SetLabelPreformated(false);
      UPNP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *entry, NULL, upnp_service);

    } else if( ObjectClass.StartsWith("object.container.album.photoalbum")) {
      //CPictureInfoTag* tag = pItem->GetPictureInfoTag();

    } else if( ObjectClass.StartsWith("object.container.album") ) {
      pItem->SetLabelPreformated(false);
      UPNP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *entry, NULL, upnp_service);
    }

  } else {
    bool audio = false
       , image = false
       , video = false;
    // set a general content type
    const char* content = NULL;
    if (ObjectClass.StartsWith("object.item.videoitem")) {
      pItem->SetMimeType("video/octet-stream");
      content = "video";
      video = true;
    }
    else if(ObjectClass.StartsWith("object.item.audioitem")) {
      pItem->SetMimeType("audio/octet-stream");
      content = "audio";
      audio = true;
    }
    else if(ObjectClass.StartsWith("object.item.imageitem")) {
      pItem->SetMimeType("image/octet-stream");
      content = "image";
      image = true;
    }

    // attempt to find a valid resource (may be multiple)
    PLT_MediaItemResource resource, *res = NULL;
    if(NPT_SUCCEEDED(NPT_ContainerFind(entry->m_Resources,
                                       CResourceFinder("http-get", content), resource))) {

      // set metadata
      if (resource.m_Size != (NPT_LargeSize)-1) {
        pItem->m_dwSize  = resource.m_Size;
      }
      res = &resource;
    }
    // look for metadata
    if(video) {
        pItem->SetLabelPreformated(false);
        UPNP::PopulateTagFromObject(*pItem->GetVideoInfoTag(), *entry, res, upnp_service);

    } else if(audio) {
        pItem->SetLabelPreformated(false);
        UPNP::PopulateTagFromObject(*pItem->GetMusicInfoTag(), *entry, res, upnp_service);

    } else if(image) {
        //CPictureInfoTag* tag = pItem->GetPictureInfoTag();

    }
  }

  // look for date?
  if(entry->m_Description.date.GetLength()) {
    SYSTEMTIME time = {};
    sscanf(entry->m_Description.date, "%hu-%hu-%huT%hu:%hu:%hu",
           &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
    pItem->m_dateTime = time;
  }

  // if there is a thumbnail available set it here
  if(entry->m_ExtraInfo.album_arts.GetItem(0))
    // only considers first album art
    pItem->SetArt("thumb", (const char*) entry->m_ExtraInfo.album_arts.GetItem(0)->uri);
  else if(entry->m_Description.icon_uri.GetLength())
    pItem->SetArt("thumb", (const char*) entry->m_Description.icon_uri);

  for (unsigned int index = 0; index < entry->m_XbmcInfo.artwork.GetItemCount(); index++)
      pItem->SetArt(entry->m_XbmcInfo.artwork.GetItem(index)->type.GetChars(),
                    entry->m_XbmcInfo.artwork.GetItem(index)->url.GetChars());

  // set the watched overlay, as this will not be set later due to
  // content set on file item list
  if (pItem->HasVideoInfoTag()) {
    int episodes = pItem->GetVideoInfoTag()->m_iEpisode;
    int played   = pItem->GetVideoInfoTag()->m_playCount;
    const std::string& type = pItem->GetVideoInfoTag()->m_type;
    bool watched(false);
    if (type == MediaTypeTvShow || type == MediaTypeSeason) {
      pItem->SetProperty("totalepisodes", episodes);
      pItem->SetProperty("numepisodes", episodes);
      pItem->SetProperty("watchedepisodes", played);
      pItem->SetProperty("unwatchedepisodes", episodes - played);
      watched = (episodes && played >= episodes);
      pItem->GetVideoInfoTag()->m_playCount = watched ? 1 : 0;
    }
    else if (type == MediaTypeEpisode || type == MediaTypeMovie)
      watched = (played > 0);
    pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, watched);
  }
  return pItem;
}

struct ResourcePrioritySort
{
  ResourcePrioritySort(const PLT_MediaObject* entry)
  {
    if (entry->m_ObjectClass.type.StartsWith("object.item.audioItem"))
        m_content = "audio";
    else if (entry->m_ObjectClass.type.StartsWith("object.item.imageItem"))
        m_content = "image";
    else if (entry->m_ObjectClass.type.StartsWith("object.item.videoItem"))
        m_content = "video";
  }

  int  GetPriority(const PLT_MediaItemResource& res) const
  {
    int prio = 0;

    if (m_content != "" && res.m_ProtocolInfo.GetContentType().StartsWith(m_content))
        prio += 400;

    NPT_Url url(res.m_Uri);
    if (URIUtils::IsHostOnLAN((const char*)url.GetHost(), false))
        prio += 300;

    if (res.m_ProtocolInfo.GetProtocol() == "xbmc-get")
        prio += 200;
    else if (res.m_ProtocolInfo.GetProtocol() == "http-get")
        prio += 100;

    return prio;
  }

  int operator()(const PLT_MediaItemResource& lh, const PLT_MediaItemResource& rh) const
  {
    if(GetPriority(lh) < GetPriority(rh))
        return 1;
    else
        return 0;
  }

  NPT_String m_content;
};

bool GetResource(const PLT_MediaObject* entry, CFileItem& item)
{
  PLT_MediaItemResource resource;

  // store original path so we remember it
  item.SetProperty("original_listitem_url",  item.GetPath());
  item.SetProperty("original_listitem_mime", item.GetMimeType());

  // get a sorted list based on our preference
  NPT_List<PLT_MediaItemResource> sorted;
  for (NPT_Cardinal i = 0; i < entry->m_Resources.GetItemCount(); ++i) {
      sorted.Add(entry->m_Resources[i]);
  }
  sorted.Sort(ResourcePrioritySort(entry));

  if(sorted.GetItemCount() == 0)
    return false;

  resource = *sorted.GetFirstItem();

  // if it's an item, path is the first url to the item
  // we hope the server made the first one reachable for us
  // (it could be a format we dont know how to play however)
  item.SetPath((const char*) resource.m_Uri);

  // look for content type in protocol info
  if (resource.m_ProtocolInfo.IsValid()) {
    CLog::Log(LOGDEBUG, "CUPnPDirectory::GetResource - resource protocol info '%s'",
              (const char*)(resource.m_ProtocolInfo.ToString()));

    if (resource.m_ProtocolInfo.GetContentType().Compare("application/octet-stream") != 0) {
      item.SetMimeType((const char*)resource.m_ProtocolInfo.GetContentType());
    }
  } else {
    CLog::Log(LOGERROR, "CUPnPDirectory::GetResource - invalid protocol info '%s'",
              (const char*)(resource.m_ProtocolInfo.ToString()));
  }

  // look for subtitles
  unsigned subs = 0;
  for(unsigned r = 0; r < entry->m_Resources.GetItemCount(); r++)
  {
    const PLT_MediaItemResource& res  = entry->m_Resources[r];
    const PLT_ProtocolInfo&      info = res.m_ProtocolInfo;
    static const char* allowed[] = { "text/srt"
      , "text/ssa"
      , "text/sub"
      , "text/idx" };
    for(unsigned type = 0; type < ARRAY_SIZE(allowed); type++)
    {
      if(info.Match(PLT_ProtocolInfo("*", "*", allowed[type], "*")))
      {
        std::string prop = StringUtils::Format("subtitle:%d", ++subs);
        item.SetProperty(prop, (const char*)res.m_Uri);
        break;
      }
    }
  }
  return true;
}

CFileItemPtr GetFileItem(const NPT_String& uri, const NPT_String& meta)
{
    PLT_MediaObjectListReference list;
    PLT_MediaObject*             object = NULL;
    CFileItemPtr                 item;

    if (NPT_SUCCEEDED(PLT_Didl::FromDidl(meta, list))) {
        list->Get(0, object);
    }

    if (object) {
        item = BuildObject(object);
    }

    if (item) {
        item->SetPath((const char*)uri);
        GetResource(object, *item);
    } else {
        item.reset(new CFileItem((const char*)uri, false));
    }
    return item;
}

} /* namespace UPNP */

