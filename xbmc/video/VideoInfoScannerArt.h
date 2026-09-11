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
#include <string_view>
#include <vector>

class CFileItem;
class CVideoInfoTag;
struct SActorInfo;

namespace KODI::VIDEO
{
//! \brief When an image is likely to be first seen by the user, and so the order to fetch it in
enum class ArtPriority : uint8_t
{
  LIST = 0, //!< poster - shown in library
  BACKGROUND, //!< fanart - shown behind a list
  DETAIL, //!< shown once an item is opened
  ACTOR, //!< shown once a cast list is opened
};

//! \brief An artwork url to cache, with the hash of its source file if that is already known
struct ArtToCache
{
  std::string url;
  std::string hash; //!< empty if unknown, leaving the texture cache to determine it
  ArtPriority priority{ArtPriority::DETAIL};
};

//! \brief When art of the given type (a "poster", a "set.fanart") is first seen
ArtPriority PriorityOfArtType(std::string_view artType);

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

  /*! \brief Queues anything still held back, for a holder that doesn't do so itself
   A library import, say, gathers art without ever running a scan to finish.
   \sa FlushDeferred
   */
  ~CVideoInfoScannerArt();

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

  /*! \brief Have the texture cache fetch a set of an item's artwork
   Each image is placed by the art type it is held under \sa PriorityOfArtType
   \param art the artwork to cache
   */
  void Cache(const KODI::ART::Artwork& art) const;

  /*! \brief Hand the artwork held back during the scan to the texture cache

   Caching an image competes with the scan for the cpu to decode and store what it fetched, so
   with ArtRetrievalTiming::BACKGROUND all but ArtPriority::LIST is collected as it is found and
   queued here instead, once the library is browsable. The texture cache's own queue outlives this
   object, so the images are still cached from that point rather than being left for something to
   ask for them.

   Queued in ArtPriority order, so that what the user comes to first is fetched first.

   Duplicates are dropped, since one image - an actor, a set - often belongs to several items and
   each would otherwise cost a lookup to find it already cached.
   */
  void FlushDeferred();

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

  //! \brief Cache a group of artwork, or collect it for FlushDeferred() to
  void Cache(std::vector<ArtToCache> art) const;

  ArtRetrievalTiming m_artRetrievalTiming{ArtRetrievalTiming::BACKGROUND};

  //! Artwork found during the scan, to be cached once it has finished \sa FlushDeferred
  //! Mutable, as the art is found by the const methods that gather it
  mutable std::vector<ArtToCache> m_deferredArt;
};

} // namespace KODI::VIDEO
