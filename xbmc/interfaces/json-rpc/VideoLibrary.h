/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"
#include "JSONRPC.h"
#include "XBDateTime.h"
#include "utils/Artwork.h"
#include "utils/DatabaseUtils.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;
class CVideoDatabase;
class CVariant;

namespace JSONRPC
{
  class CVideoLibrary : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetMovies(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetMovieDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetMovieSets(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetMovieSetDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetTVShows(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetTVShowDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSeasons(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetSeasonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetEpisodes(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetEpisodeDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetMusicVideos(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetMusicVideoDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetRecentlyAddedMovies(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecentlyAddedEpisodes(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetRecentlyAddedMusicVideos(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetInProgressTVShows(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS GetGenres(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetTags(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS GetAvailableArtTypes(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAvailableArt(const std::string& method, ITransportLayer* transport, IClient* client, const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS SetMovieDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetMovieSetDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetTVShowDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetSeasonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetEpisodeDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetMusicVideoDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Refresh(const std::string& method,
                                  ITransportLayer* transport,
                                  IClient* client,
                                  const CVariant& parameterObject,
                                  CVariant& result);

    // Deprecated in favour of Refresh, which also reaches movie sets and seasons
    static JSONRPC_STATUS RefreshMovie(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RefreshTVShow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RefreshEpisode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RefreshMusicVideo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS RemoveMovie(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RemoveTVShow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RemoveEpisode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS RemoveMusicVideo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSONRPC_STATUS Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS SetSourceContent(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result);
    static JSONRPC_STATUS Export(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSONRPC_STATUS Clean(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static bool FillFileItem(
        const std::string& strFilename,
        std::shared_ptr<CFileItem>& item,
        const CVariant& parameterObject = CVariant(CVariant::VariantTypeArray));
    static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);

  protected:
    /*! \brief Add how a file was played to an item that already says what it is.
     \param fileDetails the tag filled from the files table
     \param details the tag to add it to, left otherwise untouched
    */
    static void ApplyPlaybackState(const CVideoInfoTag& fileDetails, CVideoInfoTag& details);

    struct PlaybackUpdate
    {
      int playCount;
      CDateTime lastPlayed;
    };

    /*! \brief The playback state a show-level update leaves one of its episodes with.
     \param show the show's tag, carrying the values the update asked for
     \param updatePlaycount whether the update named a playcount
     \param updateLastplayed whether the update named a lastplayed
     \param episode the episode as the library holds it
     \return what to store, or nothing when the episode is left as it is
    */
    static std::optional<PlaybackUpdate> EpisodePlaybackUpdate(const CVideoInfoTag& show,
                                                               bool updatePlaycount,
                                                               bool updateLastplayed,
                                                               const CVideoInfoTag& episode);

  public:
    static void UpdateResumePoint(const CVariant &parameterObject, CVideoInfoTag &details, CVideoDatabase &videodatabase);

    /*! \brief Provided the JSON-RPC parameter object compute the VideoDbDetails mask
    * \param parameterObject the JSON parameter mask
    * \return the mask value for the requested properties
    */
    static int GetDetailsFromJsonParameters(const CVariant& parameterObject);

  private:
    static int RequiresAdditionalDetails(const MediaType& mediaType, const CVariant &parameterObject);
    static JSONRPC_STATUS HandleItems(const char *idProperty, const char *resultName, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool limit = true);
    static JSONRPC_STATUS RemoveVideo(const CVariant &parameterObject);

    /*! \brief Queue a refresh of the library item an identifier names
     \param identifier the object carrying the item's library id
     \param parameterObject the call's parameters, for the options the refresh takes
    */
    static JSONRPC_STATUS RefreshVideo(const CVariant& identifier, const CVariant& parameterObject);

    /*! \brief Fill in the item a refresh acts on from the identifier naming it
     \param identifier the object carrying the item's library id
     \param videodatabase an open video database
     \param item the item to fill in
    */
    static JSONRPC_STATUS ResolveRefreshItem(const CVariant& identifier,
                                             CVideoDatabase& videodatabase,
                                             CFileItem& item);
    static void UpdateVideoTag(const CVariant& parameterObject,
                               CVideoInfoTag& details,
                               KODI::ART::Artwork& artwork,
                               std::set<std::string, std::less<>>& removedArtwork,
                               std::set<std::string, std::less<>>& updatedDetails);
    static void UpdateVideoTagField(const CVariant& parameterObject,
                                    const std::string& fieldName,
                                    std::vector<std::string>& fieldValue,
                                    std::set<std::string, std::less<>>& updatedDetails);
  };
}
