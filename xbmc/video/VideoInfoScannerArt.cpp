/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerArt.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureCacheJob.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "filesystem/Directory.h"
#include "imagefiles/ImageFileURL.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace XFILE;
using namespace ADDON;

namespace
{
void CacheArtwork(const std::string& url,
                  bool retrieveArtDuringScrape,
                  const std::string& knownHash = "")
{
  if (url.empty())
    return;

  const auto& textureCache = CServiceBroker::GetTextureCache();
  if (!retrieveArtDuringScrape)
  {
    textureCache->BackgroundCacheImage(url, knownHash);
    return;
  }

  bool needsRecaching{false};
  if (!textureCache->CheckCachedImage(url, needsRecaching).empty() && !needsRecaching)
    return; // already cached

  // Fetch art or recache as needed
  // This will be slow, but that is the point of the setting - to get the art during scraping
  constexpr int MAX_SYNC_CACHE_ATTEMPTS = 3;
  for (int attempt = 1; attempt <= MAX_SYNC_CACHE_ATTEMPTS; ++attempt)
  {
    if (!textureCache->CacheImage(url, knownHash).empty())
      return; // succeeded
  }

  // Synchronous fetch failed after several attempts (network timeout, etc.)
  // Fall back to the resilient background path.
  textureCache->BackgroundCacheImage(url, knownHash);
  CLog::LogF(LOGDEBUG, "Synchronous art caching for {} failed", url);
}

//! \brief Drop the empty and duplicate urls from a set of artwork. Order is not preserved.
void DedupeArt(std::vector<KODI::VIDEO::ArtToCache>& art)
{
  using KODI::VIDEO::ArtToCache;

  std::erase_if(art, [](const ArtToCache& a) { return a.url.empty(); });

  // Where the same url appears more than once, order the copy kept below first: the one seen
  // earliest, and of those the one with a hash already known, which saves a stat
  std::ranges::sort(art,
                    [](const ArtToCache& a, const ArtToCache& b)
                    {
                      if (a.url != b.url)
                        return a.url < b.url;
                      if (a.priority != b.priority)
                        return a.priority < b.priority;
                      return !a.hash.empty() && b.hash.empty();
                    });
  art.erase(std::ranges::begin(std::ranges::unique(art, {}, &ArtToCache::url)), art.end());
}
std::string ContentToMediaType(ContentType content, bool folder)
{
  switch (content)
  {
    using enum ContentType;
    case MOVIES:
      return MediaTypeMovie;
    case MUSICVIDEOS:
      return MediaTypeMusicVideo;
    case TVSHOWS:
      return folder ? MediaTypeTvShow : MediaTypeEpisode;
    default:
      return "";
  }
}
} // unnamed namespace

namespace KODI::VIDEO
{

CVideoInfoScannerArt::CVideoInfoScannerArt()
{
  m_artRetrievalTiming =
      static_cast<ArtRetrievalTiming>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_VIDEOLIBRARY_ARTRETRIEVALTIMING));
}

CVideoInfoScannerArt::~CVideoInfoScannerArt()
{
  FlushDeferred();
}

void CVideoInfoScannerArt::Cache(const std::string& url, const std::string& knownHash) const
{
  CacheArtwork(url, m_artRetrievalTiming == ArtRetrievalTiming::SYNCHRONOUS, knownHash);
}

void CVideoInfoScannerArt::Cache(const KODI::ART::Artwork& art) const
{
  std::vector<ArtToCache> artToCache;
  artToCache.reserve(art.size());
  for (const auto& [artType, url] : art)
    artToCache.push_back({url, {}, PriorityOfArtType(artType)});

  Cache(std::move(artToCache));
}

void CVideoInfoScannerArt::Cache(std::vector<ArtToCache> art) const
{
  DedupeArt(art);

  if (m_artRetrievalTiming != ArtRetrievalTiming::SYNCHRONOUS)
  {
    for (auto& a : art)
    {
      // Get artwork that is immediately visible to the user first, and defer the rest for later
      if (a.priority == ArtPriority::LIST)
        CacheArtwork(a.url, false, a.hash);
      else
        m_deferredArt.push_back(std::move(a));
    }
    return;
  }

  // Synchronous so fetch all now
  for (const auto& a : art)
    CacheArtwork(a.url, true, a.hash);
}

void CVideoInfoScannerArt::FlushDeferred()
{
  DedupeArt(m_deferredArt);
  if (m_deferredArt.empty())
    return;

  // Fetched in the order the user is likely to view it, rather than the order it was found in
  std::ranges::stable_sort(m_deferredArt, {}, &ArtToCache::priority);

  const auto count = [this](ArtPriority priority)
  { return std::ranges::count(m_deferredArt, priority, &ArtToCache::priority); };

  // Broken down by priority, as one image looks like any other by the time it is fetched
  CLog::LogF(LOGDEBUG,
             "Queueing {} images found during the scan for caching "
             "({} list, {} background, {} detail, {} actor)",
             m_deferredArt.size(), count(ArtPriority::LIST), count(ArtPriority::BACKGROUND),
             count(ArtPriority::DETAIL), count(ArtPriority::ACTOR));

  const auto& textureCache = CServiceBroker::GetTextureCache();
  for (const auto& art : m_deferredArt)
    textureCache->BackgroundCacheImage(art.url, art.hash);

  m_deferredArt.clear();
}

ArtPriority PriorityOfArtType(std::string_view artType)
{
  // A movie's set art is shown wherever the movie's own is
  if (artType.starts_with("set."))
    artType.remove_prefix(4);

  // Numbered, where a scraper offers more than one eg. "fanart2"
  if (artType.starts_with("fanart"))
    return ArtPriority::BACKGROUND;

  if (artType == "poster" || artType == "thumb" || artType == "banner" || artType == "keyart")
    return ArtPriority::LIST;

  return ArtPriority::DETAIL;
}

void CVideoInfoScannerArt::GetArtwork(CFileItem* pItem,
                                      ContentType content,
                                      bool bApplyToDir,
                                      bool useLocal,
                                      const std::string& actorArtPath,
                                      UseRemoteArtWithLocalScraper useRemoteArt /* = yes */) const
{
  int artLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
  if (artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
    return;

  CVideoInfoTag& movieDetails = *pItem->GetVideoInfoTag();
  movieDetails.m_fanart.Unpack();
  movieDetails.m_strPictureURL.Parse();

  KODI::ART::Artwork art = pItem->GetArt();

  // get and cache thumb images
  std::string mediaType = ContentToMediaType(content, pItem->IsFolder());
  std::vector<std::string> artTypes = CVideoThumbLoader::GetArtTypes(mediaType);
  bool moviePartOfSet = content == ContentType::MOVIES && movieDetails.m_set.HasTitle();
  std::vector<std::string> movieSetArtTypes;
  if (moviePartOfSet)
  {
    movieSetArtTypes = CVideoThumbLoader::GetArtTypes(MediaTypeVideoCollection);
    for (const std::string& artType : movieSetArtTypes)
      artTypes.push_back("set." + artType);
  }
  bool addAll = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL;
  bool exactName = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC;
  // find local art
  if (useLocal)
  {
    if (!pItem->SkipLocalArt())
    {
      bool useFolder = false;
      if (bApplyToDir && (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS))
      {
        std::string filename = ART::GetLocalArtBaseFilename(*pItem, useFolder);
        std::string directory = URIUtils::GetDirectory(filename);
        if (filename != directory)
          ART::AddLocalItemArtwork(art, artTypes, filename, addAll, exactName, bApplyToDir);
      }

      // Reset useFolder to false as GetLocalArtBaseFilename may modify it in
      // the previous call.
      useFolder = false;

      std::string path;
      if (content == ContentType::TVSHOWS)
      {
        path = ART::GetLocalArtBaseFilename(*pItem, useFolder,
                                            pItem->GetProperty(MULTIPLE_EPISODES).asBoolean(false)
                                                ? ART::AdditionalIdentifiers::SEASON_AND_EPISODE
                                                : ART::AdditionalIdentifiers::NONE);
      }
      else if (content == ContentType::MOVIE_VERSIONS ||
               (pItem->HasVideoVersions() &&
                pItem->GetProperty("bluray_playlist").asInteger32(-1) > -1))
      {
        // Add playlist identifier only when there are multiple versions of the movie on the same disc
        path =
            ART::GetLocalArtBaseFilename(*pItem, useFolder, ART::AdditionalIdentifiers::PLAYLIST);
      }
      else
        path = ART::GetLocalArtBaseFilename(*pItem, useFolder);
      ART::AddLocalItemArtwork(art, artTypes, path, addAll, exactName, bApplyToDir);
    }

    if (moviePartOfSet)
    {
      std::string movieSetInfoPath =
          CVideoInfoScanner::GetMovieSetInfoFolder(movieDetails.m_set.GetTitle());
      if (!movieSetInfoPath.empty())
      {
        KODI::ART::Artwork movieSetArt;
        ART::AddLocalItemArtwork(movieSetArt, movieSetArtTypes, movieSetInfoPath, addAll, exactName,
                                 true);
        for (const auto& artItem : movieSetArt)
        {
          art["set." + artItem.first] = artItem.second;
        }
      }
    }
  }

  // find embedded art
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_coverArt.empty())
  {
    for (auto& it : pItem->GetVideoInfoTag()->m_coverArt)
    {
      if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(it.m_type, artTypes, exactName)) &&
          !art.contains(it.m_type))
      {
        std::string thumb = IMAGE_FILES::URLFromFile(pItem->GetPath(), "video_" + it.m_type);
        art.insert(std::make_pair(it.m_type, thumb));
      }
    }
  }

  // add online fanart (treated separately due to it being stored in m_fanart)
  if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist("fanart", artTypes, exactName)) &&
      !art.contains("fanart"))
  {
    std::string fanart = pItem->GetVideoInfoTag()->m_fanart.GetImageURL();
    if (!fanart.empty() &&
        !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(fanart)))
      art.insert(std::make_pair("fanart", fanart));
  }

  // add online art
  for (const auto& url : pItem->GetVideoInfoTag()->m_strPictureURL.GetUrls())
  {
    if (url.m_type != CScraperUrl::UrlType::General)
      continue;
    std::string aspect = url.m_aspect;
    if (aspect.empty())
      // Backward compatibility with Kodi 11 Eden NFO files
      aspect = mediaType == MediaTypeEpisode ? "thumb" : "poster";

    if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(aspect, artTypes, exactName)) &&
        !art.contains(aspect))
    {
      std::string image = GetImage(url, pItem->GetPath());
      if (!image.empty() &&
          !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(image)))
        art.insert(std::make_pair(aspect, image));
    }
  }

  if (!art.contains("thumb") &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB) &&
      CDVDFileInfo::CanExtract(*pItem))
  {
    art["thumb"] = CVideoThumbLoader::GetEmbeddedThumbURL(*pItem);
  }

  std::vector<ArtToCache> artToCache;
  for (const auto& artType : artTypes)
    if (art.contains(artType))
      artToCache.push_back({art.at(artType), {}, PriorityOfArtType(artType)});
  Cache(std::move(artToCache));

  pItem->SetArt(art);

  // parent folder to apply the thumb to and to search for local actor thumbs
  std::string parentDir = URIUtils::GetParentPath(pItem->GetPath());
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS))
  {
    // .actors sits alongside the nfo, so for a disc folder it is in BDMV/VIDEO_TS
    const std::string mediaDir{URIUtils::IsOpticalMediaFile(pItem->GetPath())
                                   ? URIUtils::GetDirectory(pItem->GetPath())
                                   : parentDir};
    FetchActorThumbs(movieDetails.m_cast,
                     actorArtPath.empty() ? URIUtils::AddFileToFolder(mediaDir, ".actors")
                                          : actorArtPath,
                     useRemoteArt);
  }
  if (bApplyToDir)
    ApplyThumbToFolder(parentDir, art["thumb"]);
}

std::string CVideoInfoScannerArt::GetImage(const CScraperUrl::SUrlEntry& image,
                                           const std::string& itemPath)
{
  std::string thumb = CScraperUrl::GetThumbUrl(image);
  if (!thumb.empty() && thumb.find('/') == std::string::npos &&
      thumb.find('\\') == std::string::npos)
  {
    std::string strPath = URIUtils::GetDirectory(itemPath);
    thumb = URIUtils::AddFileToFolder(strPath, thumb);
  }
  return thumb;
}

void CVideoInfoScannerArt::ApplyThumbToFolder(const std::string& folder,
                                              const std::string& imdbThumb)
{
  // copy icon to folder also;
  if (!imdbThumb.empty())
  {
    CFileItem folderItem(folder, true);
    CThumbLoader loader;
    loader.SetCachedImage(folderItem, "thumb", imdbThumb);
  }
}

void CVideoInfoScannerArt::GetSeasonThumbs(const CVideoInfoTag& show,
                                           KODI::ART::SeasonsArtwork& seasonArt,
                                           const std::vector<std::string>& artTypes,
                                           bool useLocal /* = true */,
                                           UseRemoteArtWithLocalScraper useRemoteArt /* = yes */,
                                           KODI::REGEXP::RegExpCache* cache /* = nullptr*/)
{
  int artLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
  bool addAll = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL;
  bool exactName = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC;
  if (useLocal)
  {
    // find the maximum number of seasons we have local thumbs for
    int maxSeasons = 0;
    CFileItemList items;
    std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
    if (!show.m_strPath.empty())
    {
      CDirectory::GetDirectory(show.m_strPath, items, extensions,
                               DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    }
    extensions.erase(std::remove(extensions.begin(), extensions.end(), '.'), extensions.end());
    std::shared_ptr<CRegExp> reg;
    const std::string pattern = "season([0-9]+)(-[a-z0-9]+)?\\.(" + extensions + ")";
    if (!items.IsEmpty() && (reg = KODI::REGEXP::GetRegExp(pattern, cache)) != nullptr)
    {
      for (const auto& item : items)
      {
        std::string name = URIUtils::GetFileName(item->GetPath());
        if (reg->RegFind(name) > -1)
        {
          int season = atoi(reg->GetMatch(1).c_str());
          if (season > maxSeasons)
            maxSeasons = season;
        }
      }
    }
    for (int season = -1; season <= maxSeasons; season++)
    {
      // Look for local art irrespective of scraper/existing art as it takes priority
      KODI::ART::Artwork art;
      std::string basePath;
      if (season == -1)
        basePath = "season-all";
      else if (season == 0)
        basePath = "season-specials";
      else
        basePath = StringUtils::Format("season{:02}", season);

      ART::AddLocalItemArtwork(art, artTypes, URIUtils::AddFileToFolder(show.m_strPath, basePath),
                               addAll, exactName, false);

      seasonArt[season] = art;
    }
  }
  // add online art
  for (const auto& url : show.m_strPictureURL.GetUrls())
  {
    if (url.m_type != CScraperUrl::UrlType::Season)
      continue;
    std::string aspect = url.m_aspect;
    if (aspect.empty())
      aspect = "thumb";
    KODI::ART::Artwork& art = seasonArt[url.m_season];
    if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(aspect, artTypes, exactName)) &&
        !art.contains(aspect))
    {
      std::string image = CScraperUrl::GetThumbUrl(url);
      if (!image.empty() &&
          !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(image)))
        art.insert(std::make_pair(aspect, image));
    }
  }
}

void CVideoInfoScannerArt::FetchActorThumbs(
    std::vector<SActorInfo>& actors,
    const std::string& actorsDir,
    UseRemoteArtWithLocalScraper useRemoteArt /* = YES */) const
{
  CFileItemList items;
  // don't try to fetch anything local with plugin source
  if (!URIUtils::IsPlugin(actorsDir) && CDirectory::Exists(actorsDir))
    CDirectory::GetDirectory(actorsDir, items, ".png|.jpg|.tbn", DIR_FLAG_NO_FILE_DIRS);

  // Index the thumbs by filename (without extension), and the hashes taken from the directory
  // listing by url
  std::map<std::string, std::string> thumbs;
  std::map<std::string, std::string> listedHashes;
  for (const auto& item : items)
  {
    if (item->IsFolder())
      continue;

    std::string name{URIUtils::GetFileName(item->GetPath())};
    URIUtils::RemoveExtension(name);
    thumbs.try_emplace(std::move(name), item->GetPath());
    if (std::string hash{CTextureCacheJob::GetImageHash(*item)}; !hash.empty())
      listedHashes.emplace(item->GetPath(), std::move(hash));
  }

  for (auto& actor : actors)
  {
    if (actor.thumb.empty())
    {
      // Must match how the name is turned into a filename when exporting (see
      // CVideoDatabase::GetSafeFile()), or an actor whose name contains a character that is not
      // legal in a filename (ie. a trailing '.') can never be matched to their own exported thumb
      std::string thumbFile = actor.strName;
      StringUtils::Replace(thumbFile, ' ', '_');
      thumbFile = CUtil::MakeLegalFileName(std::move(thumbFile));
      if (const auto thumb{thumbs.find(thumbFile)}; thumb != thumbs.end())
        actor.thumb = thumb->second;
      if (!actor.thumbUrl.GetFirstUrlByType().m_url.empty())
      {
        const std::string thumb{CScraperUrl::GetThumbUrl(actor.thumbUrl.GetFirstUrlByType())};
        const bool notUsingThisRemoteArt{useRemoteArt == UseRemoteArtWithLocalScraper::NO &&
                                         URIUtils::IsRemote(thumb)};
        if (actor.thumb.empty() && !notUsingThisRemoteArt)
          actor.thumb = thumb;
        if (notUsingThisRemoteArt)
          actor.thumbUrl.Clear();
      }
    }
  }

  std::vector<ArtToCache> artToCache;
  artToCache.reserve(actors.size());
  for (const auto& actor : actors)
  {
    const auto hash{listedHashes.find(actor.thumb)};
    artToCache.push_back({actor.thumb, hash != listedHashes.end() ? hash->second : std::string{},
                          ArtPriority::ACTOR});
  }
  Cache(std::move(artToCache));
}

} // namespace KODI::VIDEO
