/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoThumbLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/VideoSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/StereoscopicsManager.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "network/NetworkFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/guilib/VideoVersionHelper.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

using namespace KODI;
using namespace XFILE;

CVideoThumbLoader::CVideoThumbLoader() : CThumbLoader()
{
  m_videoDatabase = new CVideoDatabase();
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
  delete m_videoDatabase;
}

void CVideoThumbLoader::OnLoaderStart()
{
  m_videoDatabase->Open();
  m_artCache.clear();
  CThumbLoader::OnLoaderStart();
}

void CVideoThumbLoader::OnLoaderFinish()
{
  m_videoDatabase->Close();
  m_artCache.clear();
  CThumbLoader::OnLoaderFinish();
}

namespace
{
std::vector<std::string> GetSettingListAsString(const std::string& settingID)
{
  std::vector<CVariant> values =
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(settingID);
  std::vector<std::string> result;
  std::transform(values.begin(), values.end(), std::back_inserter(result),
                 [](const CVariant& s) { return s.asString(); });
  return result;
}

const std::map<std::string, std::vector<std::string>> artTypeDefaults = {
    {MediaTypeEpisode, {"thumb"}},
    {MediaTypeTvShow, {"poster", "fanart", "banner"}},
    {MediaTypeSeason, {"poster", "fanart", "banner"}},
    {MediaTypeMovie, {"poster", "fanart"}},
    {MediaTypeVideoCollection, {"poster", "fanart"}},
    {MediaTypeMusicVideo, {"poster", "fanart"}},
    {MediaTypeVideoVersion, {"poster", "fanart", "banner", "thumb"}},
    {MediaTypeNone, {"poster", "fanart", "banner", "thumb"}},
};

const std::vector<std::string> artTypeDefaultsFallback = {};

const std::vector<std::string>& GetArtTypeDefault(const std::string& mediaType)
{
  auto defaults = artTypeDefaults.find(mediaType);
  if (defaults != artTypeDefaults.end())
    return defaults->second;
  return artTypeDefaultsFallback;
}

const std::map<std::string, std::string> artTypeSettings = {
    {MediaTypeEpisode, CSettings::SETTING_VIDEOLIBRARY_EPISODEART_WHITELIST},
    {MediaTypeTvShow, CSettings::SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST},
    {MediaTypeSeason, CSettings::SETTING_VIDEOLIBRARY_TVSHOWART_WHITELIST},
    {MediaTypeMovie, CSettings::SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST},
    {MediaTypeVideoCollection, CSettings::SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST},
    {MediaTypeMusicVideo, CSettings::SETTING_VIDEOLIBRARY_MUSICVIDEOART_WHITELIST},
    {MediaTypeVideoVersion, CSettings::SETTING_VIDEOLIBRARY_MOVIEART_WHITELIST},
};
} // namespace

std::vector<std::string> CVideoThumbLoader::GetArtTypes(const std::string &type)
{
  int artworkLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
    CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
  if (artworkLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
  {
    return {};
  }

  std::vector<std::string> result = GetArtTypeDefault(type);
  if (artworkLevel != CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_CUSTOM)
  {
    return result;
  }

  auto settings = artTypeSettings.find(type);
  if (settings == artTypeSettings.end())
    return result;

  for (auto& artType : GetSettingListAsString(settings->second))
  {
    if (find(result.begin(), result.end(), artType) == result.end())
      result.push_back(artType);
  }

  return result;
}

bool CVideoThumbLoader::IsValidArtType(const std::string& potentialArtType)
{
  return !potentialArtType.empty() && potentialArtType.length() <= 25 &&
    std::find_if_not(
      potentialArtType.begin(), potentialArtType.end(),
      StringUtils::isasciialphanum
    ) == potentialArtType.end();
}

bool CVideoThumbLoader::IsArtTypeInWhitelist(const std::string& artType, const std::vector<std::string>& whitelist, bool exact)
{
  // whitelist contains art "families", 'fanart' also matches 'fanart1', 'fanart2', and so on
  std::string compareArtType = artType;
  if (!exact)
    StringUtils::TrimRight(compareArtType, "0123456789");

  return std::find(whitelist.begin(), whitelist.end(), compareArtType) != whitelist.end();
}

/**
 * Look for a thumbnail for pItem.  If one does not exist, look for an autogenerated
 * thumbnail.  If that does not exist, attempt to autogenerate one.  Finally, check
 * for the existence of fanart and set properties accordingly.
 * @return: true if pItem has been modified
 */
bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CVideoThumbLoader::LoadItemCached(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive
  ||  pItem->IsParentFolder())
    return false;

  m_videoDatabase->Open();

  if (!pItem->HasVideoInfoTag() || !pItem->GetVideoInfoTag()->HasStreamDetails()) // no stream details
  {
    if ((pItem->HasVideoInfoTag() &&
         pItem->GetVideoInfoTag()->m_iFileId >= 0) // file (or maybe folder) is in the database
        || (!pItem->m_bIsFolder &&
            VIDEO::IsVideo(
                *pItem))) // Some other video file for which we haven't yet got any database details
    {
      if (m_videoDatabase->GetStreamDetails(*pItem))
        pItem->SetInvalid();
    }
  }

  // video db items normally have info in the database
  if (pItem->HasVideoInfoTag() && !pItem->GetProperty("libraryartfilled").asBoolean())
  {
    FillLibraryArt(*pItem);

    if (!pItem->GetVideoInfoTag()->m_type.empty() &&
        pItem->GetVideoInfoTag()->m_type != MediaTypeMovie &&
        pItem->GetVideoInfoTag()->m_type != MediaTypeTvShow &&
        pItem->GetVideoInfoTag()->m_type != MediaTypeEpisode &&
        pItem->GetVideoInfoTag()->m_type != MediaTypeMusicVideo &&
        pItem->GetVideoInfoTag()->m_type != MediaTypeVideoVersion)
    {
      m_videoDatabase->Close();
      return true; // nothing else to be done
    }
  }

  // if we have no art, look for it all
  std::map<std::string, std::string> artwork = pItem->GetArt();
  if (artwork.empty())
  {
    std::vector<std::string> artTypes = GetArtTypes(pItem->HasVideoInfoTag() ? pItem->GetVideoInfoTag()->m_type : "");
    if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
      artTypes.emplace_back("thumb"); // always look for "thumb" art for files
    for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    {
      std::string type = *i;
      std::string art = GetCachedImage(*pItem, type);
      if (!art.empty())
        artwork.insert(std::make_pair(type, art));
    }
    pItem->AppendArt(artwork);
  }

  m_videoDatabase->Close();

  return true;
}

bool CVideoThumbLoader::LoadItemLookup(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive || pItem->IsParentFolder() || pItem->GetPath() == "add")
    return false;

  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_type.empty() &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeMovie &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeTvShow &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeEpisode &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeMusicVideo &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeVideoVersion)
    return false; // Nothing to do here

  m_videoDatabase->Open();

  const bool isLibraryItem = pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iDbId > -1 &&
                             !pItem->GetVideoInfoTag()->m_type.empty();
  const bool libraryArtFilled =
      pItem->HasVideoInfoTag() && pItem->GetProperty("libraryartfilled").asBoolean();
  if (!isLibraryItem || !libraryArtFilled)
  {
    std::map<std::string, std::string> artwork = pItem->GetArt();
    std::vector<std::string> artTypes =
        GetArtTypes(pItem->HasVideoInfoTag() ? pItem->GetVideoInfoTag()->m_type : "");
    if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
      artTypes.emplace_back("thumb"); // always look for "thumb" art for files
    for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    {
      std::string type = *i;
      if (!pItem->HasArt(type))
      {
        std::string art = GetLocalArt(*pItem, type, type == "fanart");
        if (!art.empty()) // cache it
        {
          SetCachedImage(*pItem, type, art);
          CServiceBroker::GetTextureCache()->BackgroundCacheImage(art);
          artwork.insert(std::make_pair(type, art));
        }
        else
        {
          // If nothing was found, try embedded art
          if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_coverArt.empty())
          {
            for (auto& it : pItem->GetVideoInfoTag()->m_coverArt)
            {
              if (it.m_type == type)
              {
                art = CTextureUtils::GetWrappedImageURL(pItem->GetPath(), "video_" + type);
                artwork.insert(std::make_pair(type, art));
              }
            }
          }
        }
      }
    }
    pItem->AppendArt(artwork);
  }

  // We can only extract flags/thumbs for file-like items
  if (!pItem->m_bIsFolder && VIDEO::IsVideo(*pItem))
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (!pItem->HasArt("thumb"))
    {
      std::string thumbURL = GetEmbeddedThumbURL(*pItem);
      if (CDVDFileInfo::CanExtract(*pItem) &&
          settings->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB) &&
          settings->GetInt(CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL) !=
              CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
      {
        pItem->SetArt("thumb", thumbURL);

        if (pItem->HasVideoInfoTag())
        {
          CVideoInfoTag* info = pItem->GetVideoInfoTag();
          if (info->m_iDbId > 0 && !info->m_type.empty())
            m_videoDatabase->SetArtForItem(info->m_iDbId, info->m_type, "thumb", thumbURL);
        }
      }
    }

    // flag extraction mostly for non-library items - should end up somewhere else,
    // like a VideoInfoLoader if it existed
    if (settings->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS) &&
        CDVDFileInfo::CanExtract(*pItem) &&
        (!pItem->HasVideoInfoTag() || !pItem->GetVideoInfoTag()->HasStreamDetails()))
    {
      // No tag or no details set, so extract them
      CLog::LogF(LOGDEBUG, "trying to extract filestream details from video file {}",
                 CURL::GetRedacted(pItem->GetPath()));
      if (CDVDFileInfo::GetFileStreamDetails(pItem))
      {
        CVideoInfoTag* info = pItem->GetVideoInfoTag();
        m_videoDatabase->BeginTransaction();

        if (info->m_iFileId < 0)
          m_videoDatabase->SetStreamDetailsForFile(
              info->m_streamDetails,
              !info->m_strFileNameAndPath.empty() ? info->m_strFileNameAndPath : pItem->GetPath());
        else
          m_videoDatabase->SetStreamDetailsForFileId(info->m_streamDetails, info->m_iFileId);

        // overwrite the runtime value if the one from streamdetails is available
        if (info->m_iDbId > 0 && info->GetStaticDuration() != info->GetDuration())
        {
          info->SetDuration(info->GetDuration());

          // store the updated information in the database
          m_videoDatabase->SetDetailsForItem(info->m_iDbId, info->m_type, *info, pItem->GetArt());
        }

        m_videoDatabase->CommitTransaction();
      }
    }
  }
  DetectAndAddMissingItemData(*pItem);

  m_videoDatabase->Close();
  return true;
}

bool CVideoThumbLoader::FillLibraryArt(CFileItem &item)
{
  CVideoInfoTag &tag = *item.GetVideoInfoTag();
  std::map<std::string, std::string> artwork;
  // Video item can be an album - either a 
  // a) search result with full details including music library album id, or 
  // b) musicvideo album that needs matching to a music album, storing id as well as fetch art.
  if (tag.m_type == MediaTypeAlbum)
  {
    int idAlbum = -1;
    if (item.HasMusicInfoTag()) // Album is a search result
      idAlbum = item.GetMusicInfoTag()->GetAlbumId();
    CMusicDatabase database;
    database.Open();
    if (idAlbum < 0 && !tag.m_strAlbum.empty() &&
        item.GetProperty("musicvideomediatype") == MediaTypeAlbum)
    {
      // Musicvideo album - try to match album in music db on artist(s) and album name.
      // Get review if available and save the matching music library album id.
      std::string strArtist = StringUtils::Join(
          tag.m_artist,
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
      std::string strReview;
      if (database.GetMatchingMusicVideoAlbum(
          tag.m_strAlbum, strArtist, idAlbum, strReview))
      {
        item.SetProperty("album_musicid", idAlbum);
        item.SetProperty("album_description", strReview);
      }
    }
    // Get album art only (not related artist art)
    if (database.GetArtForItem(idAlbum, MediaTypeAlbum, artwork))
      item.SetArt(artwork);
    database.Close();
  }
  else if (tag.m_type == "actor" && !tag.m_artist.empty() &&
           item.GetProperty("musicvideomediatype") == MediaTypeArtist)
  {
    // Try to match artist in music db on name, get bio if available and fetch artist art
    // Save the matching music library artist id.
    CMusicDatabase database;
    database.Open();
    CArtist artist;
    int idArtist = database.GetArtistByName(tag.m_artist[0]);
    if (idArtist > 0)
    {
      database.GetArtist(idArtist, artist);
      tag.m_strPlot = artist.strBiography;
      item.SetProperty("artist_musicid", idArtist);
    }
    if (database.GetArtForItem(idArtist, MediaTypeArtist, artwork))
      item.SetArt(artwork);
    database.Close();
  }

  if (tag.m_iDbId > -1 && !tag.m_type.empty())
  {
    m_videoDatabase->Open();

    // @todo unify asset path for other items path
    if (VIDEO::IsVideoAssetFile(item))
    {
      if (m_videoDatabase->GetArtForAsset(
              tag.m_iFileId,
              (item.GetProperty("noartfallbacktoowner").asBoolean(false) ||
               item.GetVideoInfoTag()->GetAssetInfo().GetType() != VideoAssetType::VERSION)
                  ? ArtFallbackOptions::NONE
                  : ArtFallbackOptions::PARENT,
              artwork))
        item.AppendArt(artwork);
    }
    else if (m_videoDatabase->GetArtForItem(tag.m_iDbId, tag.m_type, artwork))
    {
      item.AppendArt(artwork);
    }
    else if (tag.m_type == "actor" && !tag.m_artist.empty() &&
             item.GetProperty("musicvideomediatype") != MediaTypeArtist)
    {
      // Fallback to music library for actors without art
      //! @todo Is m_artist set other than musicvideo? Remove this fallback if not.
      CMusicDatabase database;
      database.Open();
      int idArtist = database.GetArtistByName(item.GetLabel());
      if (database.GetArtForItem(idArtist, MediaTypeArtist, artwork))
        item.SetArt(artwork);
      database.Close();
    }

    if (tag.m_type == MediaTypeEpisode || tag.m_type == MediaTypeSeason)
    {
      // For episodes and seasons, we want to set fanart for that of the show
      if (!item.HasArt("tvshow.fanart") && tag.m_iIdShow >= 0)
      {
        const ArtMap& artmap = GetArtFromCache(MediaTypeTvShow, tag.m_iIdShow);
        if (!artmap.empty())
        {
          item.AppendArt(artmap, MediaTypeTvShow);
          item.SetArtFallback("fanart", "tvshow.fanart");
          item.SetArtFallback("tvshow.thumb", "tvshow.poster");
        }
      }

      if (tag.m_type == MediaTypeEpisode && !item.HasArt("season.poster") && tag.m_iSeason > -1)
      {
        const ArtMap& artmap = GetArtFromCache(MediaTypeSeason, tag.m_iIdSeason);
        if (!artmap.empty())
          item.AppendArt(artmap, MediaTypeSeason);
      }
    }
    else if (tag.m_type == MediaTypeMovie && tag.m_set.id >= 0 && !item.HasArt("set.fanart"))
    {
      const ArtMap& artmap = GetArtFromCache(MediaTypeVideoCollection, tag.m_set.id);
      if (!artmap.empty())
        item.AppendArt(artmap, MediaTypeVideoCollection);
    }
    m_videoDatabase->Close();
  }
  item.SetProperty("libraryartfilled", true);
  return !item.GetArt().empty();
}

bool CVideoThumbLoader::FillThumb(CFileItem &item)
{
  if (item.HasArt("thumb"))
    return true;
  std::string thumb = GetCachedImage(item, "thumb");
  if (thumb.empty())
  {
    thumb = GetLocalArt(item, "thumb");
    if (!thumb.empty())
      SetCachedImage(item, "thumb", thumb);
  }
  if (!thumb.empty())
    item.SetArt("thumb", thumb);
  else
  {
    // If nothing was found, try embedded art
    if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->m_coverArt.empty())
    {
      for (auto& it : item.GetVideoInfoTag()->m_coverArt)
      {
        if (it.m_type == "thumb")
        {
          thumb = CTextureUtils::GetWrappedImageURL(item.GetPath(), "video_" + it.m_type);
          item.SetArt(it.m_type, thumb);
        }
      }
    }
  }

  return !thumb.empty();
}

std::string CVideoThumbLoader::GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder)
{
  if (item.SkipLocalArt())
    return "";

  /* Cache directory for (sub) folders with Curl("streamed") filesystems. We need to do this
     else entering (new) directories from the app thread becomes much slower. This
     is caused by the fact that Curl Stat/Exist() is really slow and that the
     thumbloader thread accesses the streamed filesystem at the same time as the
     app thread and the latter has to wait for it.
   */

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  const bool cacheAll =
      settings ? settings->GetInt(CSettings::SETTING_FILECACHE_BUFFERMODE) == CACHE_BUFFER_MODE_ALL
               : false;

  if (item.m_bIsFolder && (NETWORK::IsStreamedFilesystem(item) || cacheAll))
  {
    CFileItemList items; // Dummy list
    CDirectory::GetDirectory(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  }

  std::string art;
  if (!type.empty())
  {
    art = item.FindLocalArt(type + ".jpg", checkFolder);
    if (art.empty())
      art = item.FindLocalArt(type + ".png", checkFolder);
  }
  if (art.empty() && (type.empty() || type == "thumb"))
  { // backward compatibility
    art = item.FindLocalArt("", false);
    if (art.empty() && (checkFolder || (item.m_bIsFolder && !item.IsFileFolder()) || item.IsOpticalMediaFile()))
    { // try movie.tbn
      art = item.FindLocalArt("movie.tbn", true);
      if (art.empty()) // try folder.jpg
        art = item.FindLocalArt("folder.jpg", true);
    }
  }

  return art;
}

std::string CVideoThumbLoader::GetEmbeddedThumbURL(const CFileItem &item)
{
  std::string path(item.GetPath());
  if (VIDEO::IsVideoDb(item) && item.HasVideoInfoTag())
    path = item.GetVideoInfoTag()->m_strFileNameAndPath;
  if (URIUtils::IsStack(path))
    path = CStackDirectory::GetFirstStackedFile(path);

  return CTextureUtils::GetWrappedImageURL(path, "video");
}

void CVideoThumbLoader::DetectAndAddMissingItemData(CFileItem &item)
{
  // @todo remove exception for hybrid movie/folder of versions
  if (item.m_bIsFolder && !StringUtils::StartsWith(item.GetPath(), VIDEODB_PATH_VERSION_ID_ALL))
    return;

  if (item.HasVideoInfoTag())
  {
    CStreamDetails& details = item.GetVideoInfoTag()->m_streamDetails;

    // add audio language properties
    for (int i = 1; i <= details.GetAudioStreamCount(); i++)
    {
      std::string index = std::to_string(i);
      item.SetProperty("AudioChannels." + index, details.GetAudioChannels(i));
      item.SetProperty("AudioCodec."    + index, details.GetAudioCodec(i).c_str());
      item.SetProperty("AudioLanguage." + index, details.GetAudioLanguage(i).c_str());
    }

    // add subtitle language properties
    for (int i = 1; i <= details.GetSubtitleStreamCount(); i++)
    {
      std::string index = std::to_string(i);
      item.SetProperty("SubtitleLanguage." + index, details.GetSubtitleLanguage(i).c_str());
    }
  }

  const CStereoscopicsManager &stereoscopicsManager = CServiceBroker::GetGUI()->GetStereoscopicsManager();

  std::string stereoMode;

  // detect stereomode for videos
  if (item.HasVideoInfoTag())
    stereoMode = item.GetVideoInfoTag()->m_streamDetails.GetStereoMode();

  if (stereoMode.empty())
  {
    std::string path = item.GetPath();
    if (VIDEO::IsVideoDb(item) && item.HasVideoInfoTag())
      path = item.GetVideoInfoTag()->GetPath();

    // check for custom stereomode setting in video settings
    CVideoSettings itemVideoSettings;
    m_videoDatabase->Open();
    if (m_videoDatabase->GetVideoSettings(item, itemVideoSettings) && itemVideoSettings.m_StereoMode != RENDER_STEREO_MODE_OFF)
    {
      stereoMode = CStereoscopicsManager::ConvertGuiStereoModeToString(static_cast<RENDER_STEREO_MODE>(itemVideoSettings.m_StereoMode));
    }
    m_videoDatabase->Close();

    // still empty, try grabbing from filename
    //! @todo in case of too many false positives due to using the full path, extract the filename only using string utils
    if (stereoMode.empty())
      stereoMode = stereoscopicsManager.DetectStereoModeByString(path);
  }

  if (!stereoMode.empty())
    item.SetProperty("stereomode", CStereoscopicsManager::NormalizeStereoMode(stereoMode));
}

const ArtMap& CVideoThumbLoader::GetArtFromCache(const std::string &mediaType, const int id)
{
  std::pair<MediaType, int> key = std::make_pair(mediaType, id);
  auto it = m_artCache.find(key);
  if (it == m_artCache.end())
  {
    ArtMap newart;
    m_videoDatabase->GetArtForItem(id, mediaType, newart);
    it = m_artCache.insert(std::make_pair(key, std::move(newart))).first;
  }
  return it->second;
}
