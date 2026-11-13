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
    static JSONRPC_STATUS GetMovies(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetMovieDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetMovieSets(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetMovieSetDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetTVShows(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetTVShowDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetSeasons(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetSeasonDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetEpisodes(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetEpisodeDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetMusicVideos(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetMusicVideoDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetRecentlyAddedMovies(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetRecentlyAddedEpisodes(const CVariant& parameterObject,
                                                   CVariant& result);
    static JSONRPC_STATUS GetRecentlyAddedMusicVideos(const CVariant& parameterObject,
                                                      CVariant& result);
    static JSONRPC_STATUS GetInProgressTVShows(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS GetGenres(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetTags(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAvailableArtTypes(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetAvailableArt(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS SetMovieDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetMovieSetDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetTVShowDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetSeasonDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetEpisodeDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetMusicVideoDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS RefreshMovie(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RefreshTVShow(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RefreshEpisode(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RefreshMusicVideo(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS RemoveMovie(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RemoveTVShow(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RemoveEpisode(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS RemoveMusicVideo(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS Scan(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetSourceContent(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Export(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS Clean(const CVariant& parameterObject, CVariant& result);

    static bool FillFileItem(
        const std::string& strFilename,
        std::shared_ptr<CFileItem>& item,
        const CVariant& parameterObject = CVariant(CVariant::VariantTypeArray));
    static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);
    static void UpdateResumePoint(const CVariant &parameterObject, CVideoInfoTag &details, CVideoDatabase &videodatabase);

    /*! \brief Provided the JSON-RPC parameter object compute the VideoDbDetails mask
    * \param parameterObject the JSON parameter mask
    * \return the mask value for the requested properties
    */
    static int GetDetailsFromJsonParameters(const CVariant& parameterObject);

  protected:
    struct PlaybackUpdate
    {
      int playCount;
      CDateTime lastPlayed;
    };

    /*! \brief The playback state a show-level update leaves one of its episodes with.
       \return what to store, or nothing when the episode is left as it is
      */
    static std::optional<PlaybackUpdate> EpisodePlaybackUpdate(const CVideoInfoTag& show,
                                                               bool updatePlaycount,
                                                               bool updateLastplayed,
                                                               const CVideoInfoTag& episode);

  private:
    static int RequiresAdditionalDetails(KODI::MEDIA::TYPE mediaType, const CVariant &parameterObject);
    static JSONRPC_STATUS HandleItems(const char *idProperty, const char *resultName, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool limit = true);
    static JSONRPC_STATUS RemoveVideo(const CVariant &parameterObject);
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
