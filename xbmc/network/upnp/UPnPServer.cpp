/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UPnPServer.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "UPnPInternal.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/MusicLibraryQueue.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"
#include "video/VideoThumbLoader.h"
#include "view/GUIViewState.h"
#include "xbmc/interfaces/AnnouncementManager.h"

#include <memory>

#include <Platinum/Source/Platinum/Platinum.h>

NPT_SET_LOCAL_LOGGER("xbmc.upnp.server")

using namespace ANNOUNCEMENT;
using namespace XFILE;
using KODI::UTILITY::CDigest;

namespace UPNP
{

NPT_UInt32 CUPnPServer::m_MaxReturnedItems = 0;

const char* audio_containers[] = { "musicdb://genres/", "musicdb://artists/", "musicdb://albums/",
                                   "musicdb://songs/", "musicdb://recentlyaddedalbums/", "musicdb://years/",
                                   "musicdb://singles/" };

const char* video_containers[] = { "library://video/movies/titles.xml/", "library://video/tvshows/titles.xml/",
                                   "videodb://recentlyaddedmovies/", "videodb://recentlyaddedepisodes/"  };

/*----------------------------------------------------------------------
|   CUPnPServer::CUPnPServer
+---------------------------------------------------------------------*/
CUPnPServer::CUPnPServer(const char* friendly_name, const char* uuid /*= NULL*/, int port /*= 0*/)
  : PLT_MediaConnect(friendly_name, false, uuid, port),
    PLT_FileMediaConnectDelegate("/", "/"),
    m_scanning(CMusicLibraryQueue::GetInstance().IsScanningLibrary() ||
               CVideoLibraryQueue::GetInstance().IsScanningLibrary()),
    m_logger(CServiceBroker::GetLogging().GetLogger(
        StringUtils::Format("CUPnPServer[{}]", friendly_name)))
{
}

CUPnPServer::~CUPnPServer()
{
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

/*----------------------------------------------------------------------
|   CUPnPServer::ProcessGetSCPD
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::ProcessGetSCPD(PLT_Service*                  service,
                            NPT_HttpRequest&              request,
                            const NPT_HttpRequestContext& context,
                            NPT_HttpResponse&             response)
{
  // needed because PLT_MediaConnect only allows Xbox360 & WMP to search
  return PLT_MediaServer::ProcessGetSCPD(service, request, context, response);
}

/*----------------------------------------------------------------------
|   CUPnPServer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::SetupServices()
{
    PLT_MediaConnect::SetupServices();
    PLT_Service* service = NULL;
    NPT_Result result = FindServiceById("urn:upnp-org:serviceId:ContentDirectory", service);
    if (service)
    {
      service->SetStateVariable("SearchCapabilities", "upnp:class");
      service->SetStateVariable("SortCapabilities", "res@duration,res@size,res@bitrate,dc:date,dc:title,dc:size,upnp:album,upnp:artist,upnp:albumArtist,upnp:episodeNumber,upnp:genre,upnp:originalTrackNumber,upnp:rating,upnp:episodeCount,upnp:episodeSeason,xbmc:rating,xbmc:dateadded,xbmc:votes");
    }

    m_scanning = true;
    OnScanCompleted(AudioLibrary);
    m_scanning = true;
    OnScanCompleted(VideoLibrary);

    // now safe to start passing on new notifications
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);

    return result;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnScanCompleted
+---------------------------------------------------------------------*/
void
CUPnPServer::OnScanCompleted(int type)
{
    if (type == AudioLibrary) {
        for (const char* const audio_container : audio_containers)
            UpdateContainer(audio_container);
    }
    else if (type == VideoLibrary) {
        for (const char* const video_container : video_containers)
            UpdateContainer(video_container);
    }
    else
        return;
    m_scanning = false;
    PropagateUpdates();
}

/*----------------------------------------------------------------------
|   CUPnPServer::UpdateContainer
+---------------------------------------------------------------------*/
void
CUPnPServer::UpdateContainer(const std::string& id)
{
    std::map<std::string, std::pair<bool, unsigned long> >::iterator itr = m_UpdateIDs.find(id);
    unsigned long count = 0;
    if (itr != m_UpdateIDs.end())
        count = ++itr->second.second;
    m_UpdateIDs[id] = std::make_pair(true, count);
    PropagateUpdates();
}

/*----------------------------------------------------------------------
|   CUPnPServer::PropagateUpdates
+---------------------------------------------------------------------*/
void
CUPnPServer::PropagateUpdates()
{
    PLT_Service* service = NULL;
    NPT_String current_ids;
    std::string buffer;
    std::map<std::string, std::pair<bool, unsigned long> >::iterator itr;

    if (m_scanning || !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNPANNOUNCE))
        return;

    NPT_CHECK_LABEL(FindServiceById("urn:upnp-org:serviceId:ContentDirectory", service), failed);

    // we pause, and we must retain any changes which have not been
    // broadcast yet
    NPT_CHECK_LABEL(service->PauseEventing(), failed);
    NPT_CHECK_LABEL(service->GetStateVariableValue("ContainerUpdateIDs", current_ids), failed);
    buffer = (const char*)current_ids;
    if (!buffer.empty())
        buffer.append(",");

    // only broadcast ids with modified bit set
    for (itr = m_UpdateIDs.begin(); itr != m_UpdateIDs.end(); ++itr) {
        if (itr->second.first) {
          buffer.append(StringUtils::Format("{},{},", itr->first, itr->second.second));
          itr->second.first = false;
        }
    }

    // set the value, Platinum will clear ContainerUpdateIDs after sending
    NPT_CHECK_LABEL(service->SetStateVariable("ContainerUpdateIDs", buffer.substr(0,buffer.size()-1).c_str(), true), failed);
    NPT_CHECK_LABEL(service->IncStateVariable("SystemUpdateID"), failed);

    service->PauseEventing(false);
    return;

failed:
    // should attempt to start eventing on a failure
    if (service) service->PauseEventing(false);
    m_logger->error("Unable to propagate updates");
}

/*----------------------------------------------------------------------
|   CUPnPServer::SetupIcons
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::SetupIcons()
{
    NPT_String file_root = CSpecialProtocol::TranslatePath("special://xbmc/media/").c_str();
    AddIcon(
        PLT_DeviceIcon("image/png", 256, 256, 8, "/icon256x256.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 120, 120, 8, "/icon120x120.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 48, 48, 8, "/icon48x48.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 32, 32, 8, "/icon32x32.png"),
        file_root);
    AddIcon(
        PLT_DeviceIcon("image/png", 16, 16, 8, "/icon16x16.png"),
        file_root);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPServer::BuildSafeResourceUri
+---------------------------------------------------------------------*/
NPT_String CUPnPServer::BuildSafeResourceUri(const NPT_HttpUrl &rooturi,
                                             const char* host,
                                             const char* file_path)
{
    CURL url(file_path);
    std::string md5;
    std::string mapped_file_path(file_path);

    // determine the filename to provide context to md5'd urls
    std::string filename;
    if (url.IsProtocol("image"))
    {
      filename = URIUtils::GetFileName(url.GetHostName());
      // Remove trailing / for Platinum/Neptune to recognize the file extension and use the correct mime type when serving the image
      URIUtils::RemoveSlashAtEnd(mapped_file_path);
    }
    else
      filename = URIUtils::GetFileName(mapped_file_path);

    filename = CURL::Encode(filename);
    md5 = CDigest::Calculate(CDigest::Type::MD5, mapped_file_path);
    md5 += "/" + filename;
    { NPT_AutoLock lock(m_FileMutex);
      NPT_CHECK(m_FileMap.Put(md5.c_str(), mapped_file_path.c_str()));
    }
    return PLT_FileMediaServer::BuildSafeResourceUri(rooturi, host, md5.c_str());
}

/*----------------------------------------------------------------------
|   CUPnPServer::Build
+---------------------------------------------------------------------*/
PLT_MediaObject* CUPnPServer::Build(const std::shared_ptr<CFileItem>& item,
                                    bool with_count,
                                    const PLT_HttpRequestContext& context,
                                    NPT_Reference<CThumbLoader>& thumb_loader,
                                    const char* parent_id /* = NULL */)
{
    PLT_MediaObject* object = NULL;
    NPT_String       path = item->GetPath().c_str();

    //HACK: temporary disabling count as it thrashes HDD
    with_count = false;

    m_logger->debug("Preparing upnp object for item '{}'", (const char*)path);

    if (path.StartsWith("virtualpath://upnproot")) {
        path.TrimRight("/");
        item->m_bIsFolder =  true;
        if (path.StartsWith("virtualpath://")) {
            object = new PLT_MediaContainer;
            object->m_Title = item->GetLabel().c_str();
            object->m_ObjectClass.type = "object.container";
            object->m_ObjectID = EncodeObjectId(path.GetChars());

            // root
            object->m_ObjectID = EncodeObjectId("0");
            object->m_ParentID = EncodeObjectId("-1");
            // root has 5 children

            //This is dead code because of the HACK a few lines up setting with_count to false
            //if (with_count) {
            //    ((PLT_MediaContainer*)object)->m_ChildrenCount = 5;
            //}
        } else {
            goto failure;
        }

    } else {
        // db path handling
        NPT_String file_path, share_name;
        file_path = item->GetPath().c_str();
        share_name = "";

        if (path.StartsWith("musicdb://")) {
            if (path == "musicdb://" ) {
                item->SetLabel("Music Library");
                item->SetLabelPreformatted(true);
                item->m_bIsFolder = true;
            } else {
                if (!item->HasMusicInfoTag()) {
                    MUSICDATABASEDIRECTORY::CQueryParams params;
                    MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo((const char*)path, params);

                    CMusicDatabase db;
                    if (!db.Open() ) return NULL;

                    if (params.GetSongId() >= 0 ) {
                        CSong song;
                        if (db.GetSong(params.GetSongId(), song))
                            item->GetMusicInfoTag()->SetSong(song);
                    }
                    else if (params.GetAlbumId() >= 0 ) {
                        item->m_bIsFolder = true;
                        CAlbum album;
                        if (db.GetAlbum(params.GetAlbumId(), album, false))
                            item->GetMusicInfoTag()->SetAlbum(album);
                    }
                    else if (params.GetArtistId() >= 0 ) {
                        item->m_bIsFolder = true;
                        CArtist artist;
                        if (db.GetArtist(params.GetArtistId(), artist, false))
                            item->GetMusicInfoTag()->SetArtist(artist);
                    }
                }

                // all items appart from songs (artists, albums, etc) are folders
                if (!item->HasMusicInfoTag() || item->GetMusicInfoTag()->GetType() != MediaTypeSong)
                {
                    item->m_bIsFolder = true;
                }

                if (item->GetLabel().empty()) {
                    /* if no label try to grab it from node type */
                    std::string label;
                    if (CMusicDatabaseDirectory::GetLabel((const char*)path, label)) {
                        item->SetLabel(label);
                        item->SetLabelPreformatted(true);
                    }
                }
            }
        } else if (file_path.StartsWith("library://") || file_path.StartsWith("videodb://")) {
            if (path == "library://video/" ) {
                item->SetLabel("Video Library");
                item->SetLabelPreformatted(true);
                item->m_bIsFolder = true;
            } else {
                if (!item->HasVideoInfoTag()) {
                    VIDEODATABASEDIRECTORY::CQueryParams params;
                    VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo((const char*)path, params);

                    CVideoDatabase db;
                    if (!db.Open() ) return NULL;

                    if (params.GetMovieId() >= 0 )
                        db.GetMovieInfo((const char*)path, *item->GetVideoInfoTag(), params.GetMovieId());
                    else if (params.GetMVideoId() >= 0 )
                        db.GetMusicVideoInfo((const char*)path, *item->GetVideoInfoTag(), params.GetMVideoId());
                    else if (params.GetEpisodeId() >= 0 )
                        db.GetEpisodeInfo((const char*)path, *item->GetVideoInfoTag(), params.GetEpisodeId());
                    else if (params.GetTvShowId() >= 0)
                    {
                        if (params.GetSeason() >= 0)
                        {
                            int idSeason = db.GetSeasonId(params.GetTvShowId(), params.GetSeason());
                            if (idSeason >= 0)
                                db.GetSeasonInfo(idSeason, *item->GetVideoInfoTag());
                        }
                        else
                            db.GetTvShowInfo((const char*)path, *item->GetVideoInfoTag(), params.GetTvShowId());
                    }
                }

                if (item->GetVideoInfoTag()->m_type == MediaTypeTvShow || item->GetVideoInfoTag()->m_type == MediaTypeSeason) {
                    // for tvshows and seasons, iEpisode and playCount are
                    // invalid
                    item->m_bIsFolder = true;
                    item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("totalepisodes").asInteger();
                    item->GetVideoInfoTag()->SetPlayCount(static_cast<int>(item->GetProperty("watchedepisodes").asInteger()));
                }
                // if this is an item in the library without a playable path it most be a folder
                else if (item->GetVideoInfoTag()->m_strFileNameAndPath.empty())
                {
                    item->m_bIsFolder = true;
                }

                // try to grab title from tag
                if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strTitle.empty()) {
                    item->SetLabel(item->GetVideoInfoTag()->m_strTitle);
                    item->SetLabelPreformatted(true);
                }

                // try to grab it from the folder
                if (item->GetLabel().empty()) {
                    std::string label;
                    if (CVideoDatabaseDirectory::GetLabel((const char*)path, label)) {
                        item->SetLabel(label);
                        item->SetLabelPreformatted(true);
                    }
                }
            }
        }
        // playlists are folders
        else if (item->IsPlayList())
        {
            item->m_bIsFolder = true;
        }
        // audio and not a playlist -> song, so it's not a folder
        else if (item->IsAudio())
        {
            item->m_bIsFolder = false;
        }
        // any other type of item -> delegate to CDirectory
        else
        {
            item->m_bIsFolder = CDirectory::Exists(item->GetPath());
        }

        // not a virtual path directory, new system
        object = BuildObject(*item.get(), file_path, with_count, thumb_loader, &context, this, UPnPContentDirectory);

        // set parent id if passed, otherwise it should have been determined
        if (object && parent_id) {
            object->m_ParentID = EncodeObjectId(parent_id);
        }
    }

    if (object) {
        // remap Root virtualpath://upnproot/ to id "0"
        const NPT_String encodedRootObjectId = EncodeObjectId("virtualpath://upnproot/");
        if (StringUtils::EqualsNoCase(object->m_ObjectID, encodedRootObjectId))
            object->m_ObjectID = EncodeObjectId("0");

        // remap Parent Root virtualpath://upnproot/ to id "0"
        if (StringUtils::EqualsNoCase(object->m_ParentID, encodedRootObjectId))
            object->m_ParentID = EncodeObjectId("0");
    }

    return object;

failure:
    delete object;
    return NULL;
}

/*----------------------------------------------------------------------
|   CUPnPServer::Announce
+---------------------------------------------------------------------*/
void CUPnPServer::Announce(AnnouncementFlag flag,
                           const std::string& sender,
                           const std::string& message,
                           const CVariant& data)
{
    NPT_String path;
    int item_id;
    std::string item_type;

    if (sender != CAnnouncementManager::ANNOUNCEMENT_SENDER)
      return;

    if (message != "OnUpdate" && message != "OnRemove" && message != "OnScanStarted" &&
        message != "OnScanFinished")
      return;

    if (data.isNull()) {
      if (message == "OnScanStarted" || message == "OnCleanStarted")
      {
        m_scanning = true;
        }
        else if (message == "OnScanFinished" || message == "OnCleanFinished")
        {
          OnScanCompleted(flag);
        }
    }
    else {
        // handle both updates & removals
        if (!data["item"].isNull()) {
            item_id = (int)data["item"]["id"].asInteger();
            item_type = data["item"]["type"].asString();
        }
        else {
            item_id = (int)data["id"].asInteger();
            item_type = data["type"].asString();
        }

        // we always update 'recently added' nodes along with the specific container,
        // as we don't differentiate 'updates' from 'adds' in RPC interface
        if (flag == VideoLibrary) {
            if(item_type == MediaTypeEpisode) {
                CVideoDatabase db;
                if (!db.Open()) return;
                int show_id = db.GetTvShowForEpisode(item_id);
                int season_id = db.GetSeasonForEpisode(item_id);
                UpdateContainer(StringUtils::Format("videodb://tvshows/titles/{}/", show_id));
                UpdateContainer(StringUtils::Format("videodb://tvshows/titles/{}/{}/?tvshowid={}",
                                                    show_id, season_id, show_id));
                UpdateContainer("videodb://recentlyaddedepisodes/");
            }
            else if(item_type == MediaTypeTvShow) {
                UpdateContainer("library://video/tvshows/titles.xml/");
                UpdateContainer("videodb://recentlyaddedepisodes/");
            }
            else if(item_type == MediaTypeMovie) {
                UpdateContainer("library://video/movies/titles.xml/");
                UpdateContainer("videodb://recentlyaddedmovies/");
            }
            else if(item_type == MediaTypeMusicVideo) {
                UpdateContainer("library://video/musicvideos/titles.xml/");
                UpdateContainer("videodb://recentlyaddedmusicvideos/");
            }
        }
        else if (flag == AudioLibrary && item_type == MediaTypeSong) {
            // we also update the 'songs' container is maybe a performance drop too
            // high? would need to check if slow clients even cache at all anyway
            CMusicDatabase db;
            CAlbum album;
            if (!db.Open()) return;
            if (db.GetAlbumFromSong(item_id, album)) {
              UpdateContainer(StringUtils::Format("musicdb://albums/{}", album.idAlbum));
              UpdateContainer("musicdb://songs/");
              UpdateContainer("musicdb://recentlyaddedalbums/");
            }
        }
    }
}

/*----------------------------------------------------------------------
|   TranslateWMPObjectId
+---------------------------------------------------------------------*/
static NPT_String TranslateWMPObjectId(NPT_String id, const Logger& logger)
{
    if (id == "0") {
        id = "virtualpath://upnproot/";
    } else if (id == "15") {
        // Xbox 360 asking for videos
        id = "library://video/";
    } else if (id == "16") {
        // Xbox 360 asking for photos
    } else if (id == "107") {
        // Sonos uses 107 for artists root container id
        id = "musicdb://artists/";
    } else if (id == "7") {
        // Sonos uses 7 for albums root container id
        id = "musicdb://albums/";
    } else if (id == "4") {
        // Sonos uses 4 for tracks root container id
        id = "musicdb://songs/";
    }

    logger->debug("Translated id to '{}'", (const char*)id);
    return id;
}

NPT_Result
ObjectIDValidate(const NPT_String& id)
{
    if (CFileUtils::RemoteAccessAllowed(id.GetChars()))
        return NPT_SUCCESS;
    return NPT_ERROR_NO_SUCH_FILE;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnBrowseMetadata
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::OnBrowseMetadata(PLT_ActionReference&          action,
                              const char*                   object_id,
                              const char*                   filter,
                              NPT_UInt32                    starting_index,
                              NPT_UInt32                    requested_count,
                              const char*                   sort_criteria,
                              const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(sort_criteria);
    NPT_COMPILER_UNUSED(requested_count);
    NPT_COMPILER_UNUSED(starting_index);

    NPT_String                     didl;
    NPT_Reference<PLT_MediaObject> object;
    CFileItemPtr                   item;
    NPT_Reference<CThumbLoader>    thumb_loader;

    m_logger->info("Received UPnP Browse Metadata request for encoded object '{}' (plain value: '{}')", object_id, DecodeObjectId(object_id).GetChars());

    NPT_String id = TranslateWMPObjectId(DecodeObjectId(object_id), m_logger);

    if(NPT_FAILED(ObjectIDValidate(id))) {
        action->SetError(701, "Incorrect ObjectID.");
        return NPT_FAILURE;
    }

    if (id.StartsWith("virtualpath://")) {
        id.TrimRight("/");
        if (id == "virtualpath://upnproot") {
            id += "/";
            item = std::make_shared<CFileItem>((const char*)id, true);
            item->SetLabel("Root");
            item->SetLabelPreformatted(true);
            object = Build(item, true, context, thumb_loader);
            object->m_ParentID = EncodeObjectId("-1");
        } else {
            return NPT_FAILURE;
        }
    } else {
        item = std::make_shared<CFileItem>((const char*)id, false);

        // attempt to determine the parent of this item
        std::string parent;
        if (URIUtils::IsVideoDb((const char*)id) || URIUtils::IsMusicDb((const char*)id) || StringUtils::StartsWithNoCase((const char*)id, "library://video/")) {
            if (!URIUtils::GetParentPath((const char*)id, parent)) {
                parent = "0";
            }
        }
        else {
            // non-library objects - playlists / sources
            //
            // we could instead store the parents in a hash during every browse
            // or could handle this in URIUtils::GetParentPath() possibly,
            // however this is quicker to implement and subsequently purge when a
            // better solution presents itself
            std::string child_id((const char*)id);
            if      (StringUtils::StartsWithNoCase(child_id, "special://musicplaylists/"))          parent = "musicdb://";
            else if (StringUtils::StartsWithNoCase(child_id, "special://videoplaylists/"))          parent = "library://video/";
            else if (StringUtils::StartsWithNoCase(child_id, "sources://video/"))                   parent = "library://video/";
            else if (StringUtils::StartsWithNoCase(child_id, "special://profile/playlists/music/")) parent = "special://musicplaylists/";
            else if (StringUtils::StartsWithNoCase(child_id, "special://profile/playlists/video/")) parent = "special://videoplaylists/";
            else parent = "sources://video/"; // this can only match video sources
        }

        if (item->IsVideoDb()) {
            thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
        }
        else if (item->IsMusicDb()) {
            thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());
        }
        if (!thumb_loader.IsNull()) {
            thumb_loader->OnLoaderStart();
        }
        object = Build(item, true, context, thumb_loader, parent.empty()?NULL:parent.c_str());
    }

    if (object.IsNull()) {
        /* error */
        NPT_LOG_WARNING_1("CUPnPServer::OnBrowseMetadata - Object null (%s)", object_id);
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    NPT_String tmp;
    NPT_CHECK(PLT_Didl::ToDidl(*object.AsPointer(), filter, tmp));

    /* add didl header and footer */
    didl = didl_header + tmp + didl_footer;

    NPT_CHECK(action->SetArgumentValue("Result", didl));
    NPT_CHECK(action->SetArgumentValue("NumberReturned", "1"));
    NPT_CHECK(action->SetArgumentValue("TotalMatches", "1"));

    // update ID may be wrong here, it should be the one of the container?
    NPT_CHECK(action->SetArgumentValue("UpdateId", "0"));

    //! @todo We need to keep track of the overall SystemUpdateID of the CDS

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnBrowseDirectChildren
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::OnBrowseDirectChildren(PLT_ActionReference&          action,
                                    const char*                   object_id,
                                    const char*                   filter,
                                    NPT_UInt32                    starting_index,
                                    NPT_UInt32                    requested_count,
                                    const char*                   sort_criteria,
                                    const PLT_HttpRequestContext& context)
{
    CFileItemList items;
    const NPT_String decodedObjectId = DecodeObjectId(object_id);
    m_logger->info("Received Browse DirectChildren request for encoded object '{}' (plain value: '{}'), with sort criteria {}",
                   object_id, decodedObjectId.GetChars(), sort_criteria);

    NPT_String parent_id = TranslateWMPObjectId(decodedObjectId, m_logger);

    if(NPT_FAILED(ObjectIDValidate(parent_id))) {
        action->SetError(701, "Incorrect ObjectID.");
        return NPT_FAILURE;
    }

    items.SetPath(std::string(parent_id));

    // guard against loading while saving to the same cache file
    // as CArchive currently performs no locking itself
    bool load;
    { NPT_AutoLock lock(m_CacheMutex);
      load = items.Load();
    }

    if (!load) {
        // cache anything that takes more than a second to retrieve
        auto start = std::chrono::steady_clock::now();

        if (parent_id.StartsWith("virtualpath://upnproot")) {
            CFileItemPtr item;

            // music library
            item = std::make_shared<CFileItem>("musicdb://", true);
            item->SetLabel("Music Library");
            item->SetLabelPreformatted(true);
            items.Add(item);

            // video library
            item = std::make_shared<CFileItem>("library://video/", true);
            item->SetLabel("Video Library");
            item->SetLabelPreformatted(true);
            items.Add(item);

            items.Sort(SortByLabel, SortOrderAscending);
        } else {
            // this is the only way to hide unplayable items in the 'files'
            // view as we cannot tell what context (eg music vs video) the
            // request came from
            std::string supported = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions() + "|"
                                  + CServiceBroker::GetFileExtensionProvider().GetVideoExtensions() + "|"
                                  + CServiceBroker::GetFileExtensionProvider().GetMusicExtensions() + "|"
                                  + CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
            CDirectory::GetDirectory((const char*)parent_id, items, supported, DIR_FLAG_DEFAULTS);
            DefaultSortItems(items);
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if (items.CacheToDiscAlways() || (items.CacheToDiscIfSlow() && duration.count() > 1000))
        {
          NPT_AutoLock lock(m_CacheMutex);
          items.Save();
        }
    }

    // as there's no library://music support, manually add playlists and music
    // video nodes
    if (items.GetPath() == "musicdb://") {
      CFileItemPtr playlists(new CFileItem("special://musicplaylists/", true));
      playlists->SetLabel(g_localizeStrings.Get(136));
      items.Add(playlists);

      CVideoDatabase database;
      database.Open();
      if (database.HasContent(VideoDbContentType::MUSICVIDEOS))
      {
        CFileItemPtr mvideos(new CFileItem("library://video/musicvideos/", true));
        mvideos->SetLabel(g_localizeStrings.Get(20389));
        items.Add(mvideos);
      }
    }

    // Don't pass parent_id if action is Search not BrowseDirectChildren, as
    // we want the engine to determine the best parent id, not necessarily the one
    // passed
    NPT_String action_name = action->GetActionDesc().GetName();
    return BuildResponse(
        action,
        items,
        filter,
        starting_index,
        requested_count,
        sort_criteria,
        context,
        (action_name.Compare("Search", true)==0)?NULL:parent_id.GetChars());
}

/*----------------------------------------------------------------------
|   CUPnPServer::BuildResponse
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::BuildResponse(PLT_ActionReference&          action,
                           CFileItemList&                items,
                           const char*                   filter,
                           NPT_UInt32                    starting_index,
                           NPT_UInt32                    requested_count,
                           const char*                   sort_criteria,
                           const PLT_HttpRequestContext& context,
                           const char*                   parent_id /* = NULL */)
{
    NPT_COMPILER_UNUSED(sort_criteria);

    m_logger->debug("Building UPnP response with filter '{}', starting @ {} with {} requested",
                    filter, starting_index, requested_count);

    // we will reuse this ThumbLoader for all items
    NPT_Reference<CThumbLoader> thumb_loader;

    if (URIUtils::IsVideoDb(items.GetPath()) ||
        StringUtils::StartsWithNoCase(items.GetPath(), "library://video/") ||
        StringUtils::StartsWithNoCase(items.GetPath(), "special://profile/playlists/video/")) {

        thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
    }
    else if (URIUtils::IsMusicDb(items.GetPath()) ||
        StringUtils::StartsWithNoCase(items.GetPath(), "special://profile/playlists/music/")) {

        thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());
    }
    if (!thumb_loader.IsNull()) {
        thumb_loader->OnLoaderStart();
    }

    // this isn't pretty but needed to properly hide the addons node from clients
    if (StringUtils::StartsWith(items.GetPath(), "library")) {
        for (int i=0; i<items.Size(); i++) {
            if (StringUtils::StartsWith(items[i]->GetPath(), "addons") ||
                StringUtils::EndsWith(items[i]->GetPath(), "/addons.xml/"))
                items.Remove(i);
        }
    }

    // won't return more than UPNP_MAX_RETURNED_ITEMS items at a time to keep things smooth
    // 0 requested means as many as possible
    NPT_UInt32 max_count  = (requested_count == 0)?m_MaxReturnedItems:std::min((unsigned long)requested_count, (unsigned long)m_MaxReturnedItems);
    NPT_UInt32 stop_index = std::min((unsigned long)(starting_index + max_count), (unsigned long)items.Size()); // don't return more than we can

    NPT_Cardinal count = 0;
    NPT_Cardinal total = items.Size();
    NPT_String didl = didl_header;
    PLT_MediaObjectReference object;
    for (unsigned long i=starting_index; i<stop_index; ++i) {
        object = Build(items[i], true, context, thumb_loader, parent_id);
        if (object.IsNull()) {
            // don't tell the client this item ever existed
            --total;
            continue;
        }

        NPT_String tmp;
        NPT_CHECK(PLT_Didl::ToDidl(*object.AsPointer(), filter, tmp));

        // Neptunes string growing is dead slow for small additions
        if (didl.GetCapacity() < tmp.GetLength() + didl.GetLength()) {
            didl.Reserve((tmp.GetLength() + didl.GetLength())*2);
        }
        didl += tmp;
        ++count;
    }

    didl += didl_footer;

    m_logger->debug("Returning UPnP response with {} items out of {} total matches", count, total);

    NPT_CHECK(action->SetArgumentValue("Result", didl));
    NPT_CHECK(action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(count)));
    NPT_CHECK(action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total)));
    NPT_CHECK(action->SetArgumentValue("UpdateId", "0"));
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FindSubCriteria
+---------------------------------------------------------------------*/
static
NPT_String
FindSubCriteria(NPT_String criteria, const char* name)
{
    NPT_String result;
    int search = criteria.Find(name);
    if (search >= 0) {
        criteria = criteria.Right(criteria.GetLength() - search - NPT_StringLength(name));
        criteria.TrimLeft(" ");
        if (criteria.GetLength()>0 && criteria[0] == '=') {
            criteria.TrimLeft("= ");
            if (criteria.GetLength()>0 && criteria[0] == '\"') {
                search = criteria.Find("\"", 1);
                if (search > 0) result = criteria.SubString(1, search-1);
            }
        }
    }
    return result;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnSearchContainer
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::OnSearchContainer(PLT_ActionReference&          action,
                               const char*                   object_id,
                               const char*                   search_criteria,
                               const char*                   filter,
                               NPT_UInt32                    starting_index,
                               NPT_UInt32                    requested_count,
                               const char*                   sort_criteria,
                               const PLT_HttpRequestContext& context)
{
  NPT_String id = DecodeObjectId(object_id);
  m_logger->debug("Received Search request for encoded object '{}' (plain value: '{}') with search '{}'",
                  object_id, id.GetChars(), search_criteria);

  NPT_String searchClass = NPT_String(search_criteria);
  if (id.StartsWith("musicdb://")) {
      // we browse for all tracks given a genre, artist or album
      if (searchClass.Find("object.item.audioItem") >= 0) {
          if (!id.EndsWith("/")) id += "/";
          NPT_Cardinal count = id.SubString(10).Split("/").GetItemCount();
          // remove extra empty node count
          count = count?count-1:0;

          // genre
          if (id.StartsWith("musicdb://genres/")) {
              // all tracks of all genres
              if (count == 1)
                  id += "-1/-1/-1/";
              // all tracks of a specific genre
              else if (count == 2)
                  id += "-1/-1/";
              // all tracks of a specific genre of a specific artist
              else if (count == 3)
                  id += "-1/";
          }
          else if (id.StartsWith("musicdb://artists/")) {
              // all tracks by all artists
              if (count == 1)
                  id += "-1/-1/";
              // all tracks of a specific artist
              else if (count == 2)
                  id += "-1/";
          }
          else if (id.StartsWith("musicdb://albums/")) {
              // all albums ?
              if (count == 1) id += "-1/";
          }
      }
      return OnBrowseDirectChildren(action, id, filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.item.audioItem") >= 0) {
        // look for artist, album & genre filters
        NPT_String genre = FindSubCriteria(searchClass, "upnp:genre");
        NPT_String album = FindSubCriteria(searchClass, "upnp:album");
        NPT_String artist = FindSubCriteria(searchClass, "upnp:artist");
        // sonos looks for microsoft specific stuff
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:artistPerformer");
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:artistAlbumArtist");
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:authorComposer");

        CMusicDatabase database;
        database.Open();

        if (genre.GetLength() > 0) {
            // all tracks by genre filtered by artist and/or album
            std::string strPath = StringUtils::Format(
                "musicdb://genres/{}/{}/{}/", database.GetGenreByName((const char*)genre),
                database.GetArtistByName((const char*)artist), // will return -1 if no artist
                database.GetAlbumByName((const char*)album)); // will return -1 if no album

            return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index, requested_count, sort_criteria, context);
        } else if (artist.GetLength() > 0) {
            // all tracks by artist name filtered by album if passed
            std::string strPath = StringUtils::Format(
                "musicdb://artists/{}/{}/", database.GetArtistByName((const char*)artist),
                database.GetAlbumByName((const char*)album)); // will return -1 if no album

            return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index, requested_count, sort_criteria, context);
        } else if (album.GetLength() > 0) {
            // all tracks by album name
            std::string strPath = StringUtils::Format("musicdb://albums/{}/",
                                                      database.GetAlbumByName((const char*)album));

            return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index, requested_count, sort_criteria, context);
        }

        // browse all songs
        return OnBrowseDirectChildren(action, "musicdb://songs/", filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.container.album.musicAlbum") >= 0) {
        // sonos filters by genre
        NPT_String genre = FindSubCriteria(searchClass, "upnp:genre");

        // 360 hack: artist/albums using search
        NPT_String artist = FindSubCriteria(searchClass, "upnp:artist");
        // sonos looks for microsoft specific stuff
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:artistPerformer");
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:artistAlbumArtist");
        artist = artist.GetLength()?artist:FindSubCriteria(searchClass, "microsoft:authorComposer");

        CMusicDatabase database;
        database.Open();

        if (genre.GetLength() > 0) {
          std::string strPath = StringUtils::Format(
              "musicdb://genres/{}/{}/", database.GetGenreByName((const char*)genre),
              database.GetArtistByName((const char*)artist)); // no artist should return -1
          return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index,
                                        requested_count, sort_criteria, context);
        } else if (artist.GetLength() > 0) {
          std::string strPath = StringUtils::Format("musicdb://artists/{}/",
                                                    database.GetArtistByName((const char*)artist));
          return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index,
                                        requested_count, sort_criteria, context);
        }

        // all albums
        return OnBrowseDirectChildren(action, "musicdb://albums/", filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.container.person.musicArtist") >= 0) {
        // Sonos filters by genre
        NPT_String genre = FindSubCriteria(searchClass, "upnp:genre");
        if (genre.GetLength() > 0) {
            CMusicDatabase database;
            database.Open();
            std::string strPath = StringUtils::Format("musicdb://genres/{}/",
                                                      database.GetGenreByName((const char*)genre));
            return OnBrowseDirectChildren(action, strPath.c_str(), filter, starting_index, requested_count, sort_criteria, context);
        }
        return OnBrowseDirectChildren(action, "musicdb://artists/", filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.container.genre.musicGenre") >= 0) {
        return OnBrowseDirectChildren(action, "musicdb://genres/", filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.container.playlistContainer") >= 0) {
        return OnBrowseDirectChildren(action, "special://musicplaylists/", filter, starting_index, requested_count, sort_criteria, context);
    } else if (searchClass.Find("object.container.album.videoAlbum.videoBroadcastShow") >= 0) {
      CVideoDatabase database;
      if (!database.Open()) {
        action->SetError(800, "Internal Error");
        return NPT_SUCCESS;
      }

      CFileItemList items;
      if (!database.GetTvShowsByWhere("videodb://tvshows/titles/?local", CDatabase::Filter(),
        items, SortDescription(), GetRequiredVideoDbDetails(NPT_String(filter)))) {
        action->SetError(800, "Internal Error");
        return NPT_SUCCESS;
      }

      items.SetPath("videodb://tvshows/titles/");
      return BuildResponse(action, items, filter, starting_index, requested_count, sort_criteria, context, NULL);
    } else if (searchClass.Find("object.container.album.videoAlbum.videoBroadcastSeason") >= 0) {
      CVideoDatabase database;
      if (!database.Open()) {
        action->SetError(800, "Internal Error");
        return NPT_SUCCESS;
      }

      CFileItemList items;
      if (!database.GetSeasonsByWhere("videodb://tvshows/titles/-1/?local", CDatabase::Filter(), items, true)) {
        action->SetError(800, "Internal Error");
        return NPT_SUCCESS;
      }

      items.SetPath("videodb://tvshows/titles/-1/");
      return BuildResponse(action, items, filter, starting_index, requested_count, sort_criteria, context, NULL);
    } else if (searchClass.Find("object.item.videoItem") >= 0) {
      CFileItemList items, allItems;

      CVideoDatabase database;
      if (!database.Open()) {
        action->SetError(800, "Internal Error");
        return NPT_SUCCESS;
      }

      bool allVideoItems = searchClass.Compare("object.item.videoItem") == 0;

      // determine the required videodb details to be retrieved
      int requiredVideoDbDetails = GetRequiredVideoDbDetails(NPT_String(filter));

      if (allVideoItems || searchClass.Find("object.item.videoItem.movie") >= 0)
      {
        if (!database.GetMoviesByWhere("videodb://movies/titles/?local", CDatabase::Filter(), items, SortDescription(), requiredVideoDbDetails)) {
          action->SetError(800, "Internal Error");
          return NPT_SUCCESS;
        }

        allItems.Append(items);
        items.Clear();

        if (!allVideoItems)
          allItems.SetPath("videodb://movies/titles/");
      }

      if (allVideoItems || searchClass.Find("object.item.videoItem.videoBroadcast") >= 0)
      {
        if (!database.GetEpisodesByWhere("videodb://tvshows/titles/?local", CDatabase::Filter(), items, true, SortDescription(), requiredVideoDbDetails)) {
          action->SetError(800, "Internal Error");
          return NPT_SUCCESS;
        }

        allItems.Append(items);
        items.Clear();

        if (!allVideoItems)
          allItems.SetPath("videodb://tvshows/titles/");
      }

      if (allVideoItems || searchClass.Find("object.item.videoItem.musicVideoClip") >= 0)
      {
        if (!database.GetMusicVideosByWhere("videodb://musicvideos/titles/?local", CDatabase::Filter(), items, true, SortDescription(), requiredVideoDbDetails)) {
          action->SetError(800, "Internal Error");
          return NPT_SUCCESS;
        }

        allItems.Append(items);
        items.Clear();

        if (!allVideoItems)
          allItems.SetPath("videodb://musicvideos/titles/");
      }

      if (allVideoItems)
        allItems.SetPath("videodb://movies/titles/");

      return BuildResponse(action, allItems, filter, starting_index, requested_count, sort_criteria, context, NULL);
  } else if (searchClass.Find("object.item.imageItem") >= 0) {
      CFileItemList items;
      return BuildResponse(action, items, filter, starting_index, requested_count, sort_criteria, context, NULL);
  }

  return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   CUPnPServer::OnUpdateObject
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::OnUpdateObject(PLT_ActionReference&             action,
                            const char*                      object_id,
                            NPT_Map<NPT_String,NPT_String>&  current_vals,
                            NPT_Map<NPT_String,NPT_String>&  new_vals,
                            const PLT_HttpRequestContext&    context)
{
    std::string path(CURL::Decode(object_id));
    CFileItem updated;
    updated.SetPath(path);
    m_logger->info("OnUpdateObject: {} from {}", path,
                   (const char*)context.GetRemoteAddress().GetIpAddress().ToString());

    NPT_String playCount, position, lastPlayed;
    int err;
    const char* msg = NULL;
    bool updatelisting(false);

    // we pause eventing as multiple announces may happen in this operation
    PLT_Service* service = NULL;
    NPT_CHECK_LABEL(FindServiceById("urn:upnp-org:serviceId:ContentDirectory", service), error);
    NPT_CHECK_LABEL(service->PauseEventing(), error);

    if (updated.IsVideoDb()) {
        CVideoDatabase db;
        NPT_CHECK_LABEL(!db.Open(), error);

        // must first determine type of file from object id
        VIDEODATABASEDIRECTORY::CQueryParams params;
        VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(path.c_str(), params);

        int id = -1;
        VideoDbContentType content_type;
        if ((id = params.GetMovieId()) >= 0 )
          content_type = VideoDbContentType::MOVIES;
        else if ((id = params.GetEpisodeId()) >= 0 )
          content_type = VideoDbContentType::EPISODES;
        else if ((id = params.GetMVideoId()) >= 0 )
          content_type = VideoDbContentType::MUSICVIDEOS;
        else {
            err = 701;
            msg = "No such object";
            goto failure;
        }

        std::string file_path;
        db.GetFilePathById(id, file_path, content_type);
        CVideoInfoTag tag;
        db.LoadVideoInfo(file_path, tag);
        updated.SetFromVideoInfoTag(tag);
        m_logger->info("Translated to {}", file_path);

        position = new_vals["lastPlaybackPosition"];
        playCount = new_vals["playCount"];
        lastPlayed = new_vals["lastPlaybackTime"];


        if (!position.IsEmpty()
              && position.Compare(current_vals["lastPlaybackPosition"]) != 0) {
            NPT_UInt32 resume;
            NPT_CHECK_LABEL(position.ToInteger32(resume), args);

            if (resume <= 0)
                db.ClearBookMarksOfFile(file_path, CBookmark::RESUME);
            else {
                CBookmark bookmark;
                bookmark.timeInSeconds = resume;
                bookmark.totalTimeInSeconds = resume + 100; // not required to be correct
                bookmark.playerState = new_vals["lastPlayerState"];

                db.AddBookMarkToFile(file_path, bookmark, CBookmark::RESUME);
            }
            if (playCount.IsEmpty()) {
              CVariant data;
              data["id"] = updated.GetVideoInfoTag()->m_iDbId;
              data["type"] = updated.GetVideoInfoTag()->m_type;
              CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary,
                                                                 "OnUpdate", data);
            }
            updatelisting = true;
        }

        if (!playCount.IsEmpty()
              && playCount.Compare(current_vals["playCount"]) != 0) {

            NPT_UInt32 count;
            NPT_CHECK_LABEL(playCount.ToInteger32(count), args);

            CDateTime lastPlayedObj;
            if (!lastPlayed.IsEmpty() &&
                lastPlayed.Compare(current_vals["lastPlaybackTime"]) != 0)
                lastPlayedObj.SetFromW3CDateTime(lastPlayed.GetChars());

            db.SetPlayCount(updated, count, lastPlayedObj);
            updatelisting = true;
        }

        // we must load the changed settings before propagating to local UI
        if (updatelisting) {
            db.LoadVideoInfo(file_path, tag);
            updated.SetFromVideoInfoTag(tag);
            //! TODO: we should find a way to avoid obtaining the artwork just to
            // update the playcount or similar properties. Maybe a flag in the GUI
            // update message to inform we should only update the playback properties
            // without touching other parts of the item.
            CVideoThumbLoader().FillLibraryArt(updated);
        }

    } else if (updated.IsMusicDb()) {
      //! @todo implement this

    } else {
        err = 701;
        msg = "No such object";
        goto failure;
    }

    if (updatelisting) {
        updated.SetPath(path);
        if (updated.IsVideoDb())
             CUtil::DeleteVideoDatabaseDirectoryCache();
        else if (updated.IsMusicDb())
             CUtil::DeleteMusicDatabaseDirectoryCache();

        CFileItemPtr msgItem(new CFileItem(updated));
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE_ITEM, GUI_MSG_FLAG_UPDATE_LIST, msgItem);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
    }

    NPT_CHECK_LABEL(service->PauseEventing(false), error);
    return NPT_SUCCESS;

args:
    err = 402;
    msg = "Invalid args";
    goto failure;

error:
    err = 501;
    msg = "Internal error";

failure:
  m_logger->error("OnUpdateObject failed with err {}: {}", err, msg);
  action->SetError(err, msg);
  service->PauseEventing(false);
  return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   CUPnPServer::ServeFile
+---------------------------------------------------------------------*/
NPT_Result
CUPnPServer::ServeFile(const NPT_HttpRequest&              request,
                       const NPT_HttpRequestContext& context,
                       NPT_HttpResponse&             response,
                       const NPT_String&             md5)
{
    // Translate hash to filename
    NPT_String file_path(md5), *file_path2;
    { NPT_AutoLock lock(m_FileMutex);
      if(NPT_SUCCEEDED(m_FileMap.Get(md5, file_path2))) {
        file_path = *file_path2;
        m_logger->debug("Received request to serve '{}' = '{}'", (const char*)md5,
                        (const char*)file_path);
      } else {
        m_logger->debug("Received request to serve unknown md5 '{}'", (const char*)md5);
        response.SetStatus(404, "File Not Found");
        return NPT_SUCCESS;
      }
    }

    // File requested
    NPT_HttpUrl rooturi(context.GetLocalAddress().GetIpAddress().ToString(), context.GetLocalAddress().GetPort(), "/");

    if (file_path.Left(8).Compare("stack://", true) == 0) {

        NPT_List<NPT_String> files = file_path.SubString(8).Split(" , ");
        if (files.GetItemCount() == 0) {
            response.SetStatus(404, "File Not Found");
            return NPT_SUCCESS;
        }

        NPT_String output;
        output.Reserve(file_path.GetLength()*2);
        output += "#EXTM3U\r\n";

        NPT_List<NPT_String>::Iterator url = files.GetFirstItem();
        for (;url;url++) {
            output += ("#EXTINF:-1," + URIUtils::GetFileName((const char*)*url)).c_str();
            output += "\r\n";
            output += BuildSafeResourceUri(
                          rooturi,
                          context.GetLocalAddress().GetIpAddress().ToString(),
                          *url);
            output += "\r\n";
        }

        PLT_HttpHelper::SetBody(response, (const char*)output, output.GetLength());
        response.GetHeaders().SetHeader("Content-Disposition", "inline; filename=\"stack.m3u\"");
        return NPT_SUCCESS;
    }

    if (URIUtils::IsURL(static_cast<const char*>(file_path)))
    {
      CURL url(CTextureUtils::UnwrapImageURL(static_cast<const char*>(file_path)));
      std::string disp = "inline; filename=\"" + URIUtils::GetFileName(url) + "\"";
      response.GetHeaders().SetHeader("Content-Disposition", disp.c_str());
    }

    // set getCaptionInfo.sec - sets subtitle uri for Samsung devices
    const NPT_String* captionInfoHeader = request.GetHeaders().GetHeaderValue("getCaptionInfo.sec");
    if (captionInfoHeader)
    {
      NPT_String *sub_uri, movie;
      movie = "subtitle://" + md5;

      NPT_AutoLock lock(m_FileMutex);
      if (NPT_SUCCEEDED(m_FileMap.Get(movie, sub_uri)))
      {
        response.GetHeaders().SetHeader("CaptionInfo.sec", sub_uri->GetChars(), false);
      }
    }

    return PLT_HttpServer::ServeFile(request,
                                       context,
                                       response,
                                       file_path);
}

void
CUPnPServer::DefaultSortItems(CFileItemList& items)
{
  CGUIViewState* viewState = CGUIViewState::GetViewState(items.IsVideoDb() ? WINDOW_VIDEO_NAV : -1, items);
  if (viewState)
  {
    SortDescription sorting = viewState->GetSortMethod();
    items.Sort(sorting.sortBy, sorting.sortOrder, sorting.sortAttributes);
    delete viewState;
  }
}

NPT_Result CUPnPServer::AddSubtitleUriForSecResponse(const NPT_String& movie_md5,
                                                     const NPT_String& subtitle_uri)
{
  /* using existing m_FileMap to store subtitle uri for movie,
     adding subtitle:// prefix, because there is already entry for movie md5 with movie path */
  NPT_String movie = "subtitle://" + movie_md5;

  NPT_AutoLock lock(m_FileMutex);
  NPT_CHECK(m_FileMap.Put(movie, subtitle_uri));

  return NPT_SUCCESS;
}

int CUPnPServer::GetRequiredVideoDbDetails(const NPT_String& filter)
{
  int details = VideoDbDetailsRating;
  if (filter.Find("res@resolution") >= 0 || filter.Find("res@nrAudioChannels") >= 0)
    details |= VideoDbDetailsStream;
  if (filter.Find("upnp:actor") >= 0)
    details |= VideoDbDetailsCast;

  return details;
}

} /* namespace UPNP */

