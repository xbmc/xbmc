/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Scraper.h"
#include "utils/Artwork.h"
#include "utils/RegExp.h"

#include <cstdint>
#include <string>
#include <vector>

class CFileItem;
class CVideoInfoTag;
struct SActorInfo;

namespace KODI::VIDEO
{
/*! \brief The artwork side of a video scan

 Finds the art belonging scraped items - local files, what the scraper offered, what
 a folder or a .nfo names - and has the texture cache fetch it.

 One class belongs to a scan, and lives as long as it does.
 */
class CVideoInfoScannerArt
{
public:
  enum class UseRemoteArtWithLocalScraper : bool
  {
    NO,
    YES
  };

  CVideoInfoScannerArt();

  /*! \brief Retrieve any artwork associated with an item
   \param pItem item to find artwork for.
   \param content content type of the item.
   \param bApplyToDir whether we should apply any thumbs to a folder.  Defaults to false.
   \param useLocal whether we should use local thumbs. Defaults to true.
   \param actorArtPath the directory containing actor thumbs. Defaults to empty.
   \param useRemoteArt use remote art if also using local scraper. Defaults to yes.
   */
  void GetArtwork(
      CFileItem* pItem,
      ADDON::ContentType content,
      bool bApplyToDir = false,
      bool useLocal = true,
      const std::string& actorArtPath = "",
      UseRemoteArtWithLocalScraper useRemoteArt = UseRemoteArtWithLocalScraper::YES) const;

  /*! \brief Fetch thumbs for actors
   Updates each actor with their thumb (local or online)
   \param actors - vector of SActorInfo
   \param actorsDir - directory holding the local thumbs (ie. a .actors folder, or the actors
          folder of a library export). Used as given, nothing is appended.
   \param useRemoteArt - use remote art (ie. http://) even if derived from local .nfo file.
          Defaults to yes.
   */
  void FetchActorThumbs(
      std::vector<SActorInfo>& actors,
      const std::string& actorsDir,
      UseRemoteArtWithLocalScraper useRemoteArt = UseRemoteArtWithLocalScraper::YES) const;

  /*! \brief Have the texture cache fetch an image
   \param url the image to cache. Ignored when empty, so callers don't have to check.
   \param knownHash hash of the source file, if the caller already knows it
   */
  void Cache(const std::string& url, const std::string& knownHash = "") const;

  /*! \brief Get season thumbs for a tvshow.
   All seasons (regardless of whether the user has episodes) are added to the art map.
   \param[in] show     tvshow info tag
   \param[in] art      artwork map to which season thumbs are added.
   \param[in] useLocal whether to use local thumbs, defaults to true
   \param[in] useRemoteArt use remote art if also using local scraper. Defaults to yes.
   \param[in] cache regexp cache to avoid repeated compilations
   */
  static void GetSeasonThumbs(
      const CVideoInfoTag& show,
      KODI::ART::SeasonsArtwork& art,
      const std::vector<std::string>& artTypes,
      bool useLocal = true,
      UseRemoteArtWithLocalScraper useRemoteArt = UseRemoteArtWithLocalScraper::YES,
      KODI::REGEXP::RegExpCache* cache = nullptr);

  static std::string GetImage(const CScraperUrl::SUrlEntry& image, const std::string& itemPath);

  static void ApplyThumbToFolder(const std::string& folder, const std::string& imdbThumb);

private:
  enum class ArtRetrievalTiming : uint8_t
  {
    SYNCHRONOUS = 0, //!< retrieve art synchronously during scrape
    BACKGROUND = 1 //!< retrieve art in background after scrape
  };

  ArtRetrievalTiming m_artRetrievalTiming{ArtRetrievalTiming::BACKGROUND};
};

} // namespace KODI::VIDEO
