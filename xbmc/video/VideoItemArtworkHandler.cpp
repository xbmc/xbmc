/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoItemArtworkHandler.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"
#include "video/tags/VideoTagExtractionHelper.h"

using namespace XFILE;

namespace KODI::VIDEO
{

namespace
{
//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkHandler (Generic handler)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkHandler : public IVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkHandler(const std::shared_ptr<CFileItem>& item,
                                    const std::string& artType)
    : m_item(item), m_artType(artType)
  {
  }

  std::string GetCurrentArt() const override;
  std::string GetEmbeddedArt() const override;
  std::vector<std::string> GetRemoteArt() const override;
  std::string GetLocalArt() const override;

  std::string GetDefaultIcon() const override;

  void AddItemPathToFileBrowserSources(std::vector<CMediaSource>& sources) override;

  void PersistArt(const std::string& art) override;

protected:
  void AddItemPathStringToFileBrowserSources(std::vector<CMediaSource>& sources,
                                             const std::string& itemDir,
                                             const std::string& label);

  const std::shared_ptr<CFileItem> m_item;
  const std::string m_artType;
};

std::string CVideoItemArtworkHandler::GetCurrentArt() const
{
  if (m_artType.empty())
  {
    CLog::LogF(LOGERROR, "Art type not set!");
    return {};
  }

  std::string currentArt;
  if (m_item->HasArt(m_artType))
    currentArt = m_item->GetArt(m_artType);
  else if (m_item->HasArt("thumb") && (m_artType == "poster" || m_artType == "banner"))
    currentArt = m_item->GetArt("thumb");

  return currentArt;
}

std::string CVideoItemArtworkHandler::GetEmbeddedArt() const
{
  if (TAGS::CVideoTagExtractionHelper::IsExtractionSupportedFor(*m_item))
    return TAGS::CVideoTagExtractionHelper::ExtractEmbeddedArtFor(*m_item, m_artType);

  return {};
}

std::vector<std::string> CVideoItemArtworkHandler::GetRemoteArt() const
{
  std::vector<std::string> remoteArt;
  CVideoInfoTag tag(*m_item->GetVideoInfoTag());
  tag.m_strPictureURL.Parse();
  tag.m_strPictureURL.GetThumbUrls(remoteArt, m_artType);
  return remoteArt;
}

std::string CVideoItemArtworkHandler::GetLocalArt() const
{
  return CVideoThumbLoader::GetLocalArt(*m_item, m_artType);
}

std::string CVideoItemArtworkHandler::GetDefaultIcon() const
{
  return m_item->m_bIsFolder ? "DefaultFolder.png" : "DefaultPicture.png";
}

void CVideoItemArtworkHandler::AddItemPathToFileBrowserSources(std::vector<CMediaSource>& sources)
{
  std::string itemDir = m_item->GetVideoInfoTag()->m_basePath;
  //season
  if (itemDir.empty())
    itemDir = m_item->GetVideoInfoTag()->GetPath();

  const CFileItem itemTmp(itemDir, false);
  if (IsVideo(itemTmp))
    itemDir = URIUtils::GetParentPath(itemDir);

  AddItemPathStringToFileBrowserSources(sources, itemDir,
                                        g_localizeStrings.Get(36041) /* * Item folder */);
}

void CVideoItemArtworkHandler::PersistArt(const std::string& art)
{
  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
    return;
  }

  videodb.SetArtForItem(m_item->GetVideoInfoTag()->m_iDbId, m_item->GetVideoInfoTag()->m_type,
                        m_artType, art);
}

void CVideoItemArtworkHandler::AddItemPathStringToFileBrowserSources(
    std::vector<CMediaSource>& sources, const std::string& itemDir, const std::string& label)
{
  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = label;
    itemSource.strPath = itemDir;
    sources.emplace_back(itemSource);
  }
}

//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkArtistHandler (Artist handler)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkArtistHandler : public CVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkArtistHandler(const std::shared_ptr<CFileItem>& item,
                                          const std::string& artType)
    : CVideoItemArtworkHandler(item, artType)
  {
  }

  std::string GetCurrentArt() const override;
  std::vector<std::string> GetRemoteArt() const override;
  std::string GetLocalArt() const override;

  std::string GetDefaultIcon() const override { return "DefaultArtist.png"; }

  void PersistArt(const std::string& art) override;
};

std::string CVideoItemArtworkArtistHandler::GetCurrentArt() const
{
  CMusicDatabase musicdb;
  if (!musicdb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open music database!");
    return {};
  }

  std::string currentArt;
  const int idArtist = musicdb.GetArtistByName(m_item->GetLabel());
  if (idArtist >= 0)
    currentArt = musicdb.GetArtForItem(idArtist, MediaTypeArtist, "thumb");

  if (currentArt.empty())
  {
    CVideoDatabase videodb;
    if (!videodb.Open())
    {
      CLog::LogF(LOGERROR, "Cannot open video database!");
      return {};
    }

    currentArt = videodb.GetArtForItem(m_item->GetVideoInfoTag()->m_iDbId,
                                       m_item->GetVideoInfoTag()->m_type, "thumb");
  }
  return currentArt;
}

std::vector<std::string> CVideoItemArtworkArtistHandler::GetRemoteArt() const
{
  return {};
}

std::string CVideoItemArtworkArtistHandler::GetLocalArt() const
{
  CMusicDatabase musicdb;
  if (!musicdb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open music database!");
    return {};
  }

  std::string localArt;
  const int idArtist = musicdb.GetArtistByName(m_item->GetLabel());
  if (idArtist >= 0)
  {
    // Get artist paths - possible locations for thumb - while music db open
    CArtist artist;
    musicdb.GetArtist(idArtist, artist);
    std::string artistPath;
    musicdb.GetArtistPath(artist, artistPath); // Artist path in artist info folder

    std::string thumb;
    bool existsThumb = false;

    // First look for artist thumb in the primary location
    if (!artistPath.empty())
    {
      thumb = URIUtils::AddFileToFolder(artistPath, "folder.jpg");
      existsThumb = CFileUtils::Exists(thumb);
    }
    // If not there fall back local to music files (historic location for those album artists with a unique folder)
    if (!existsThumb)
    {
      std::string artistOldPath;
      musicdb.GetOldArtistPath(idArtist, artistOldPath); // Old artist path, local to music files
      if (!artistOldPath.empty())
      {
        thumb = URIUtils::AddFileToFolder(artistOldPath, "folder.jpg");
        existsThumb = CFileUtils::Exists(thumb);
      }
    }

    if (existsThumb)
      localArt = thumb;
  }
  return localArt;
}

void CVideoItemArtworkArtistHandler::PersistArt(const std::string& art)
{
  CMusicDatabase musicdb;
  if (!musicdb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open music database!");
    return;
  }

  const int idArtist = musicdb.GetArtistByName(m_item->GetLabel());
  if (idArtist >= 0)
    musicdb.SetArtForItem(idArtist, MediaTypeArtist, m_artType, art);
}

//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkActorHandler (Actor handler)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkActorHandler : public CVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkActorHandler(const std::shared_ptr<CFileItem>& item,
                                         const std::string& artType)
    : CVideoItemArtworkHandler(item, artType)
  {
  }

  std::string GetCurrentArt() const override;
  std::string GetLocalArt() const override;

  std::string GetDefaultIcon() const override { return "DefaultActor.png"; }
};

std::string CVideoItemArtworkActorHandler::GetCurrentArt() const
{
  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
    return {};
  }

  return videodb.GetArtForItem(m_item->GetVideoInfoTag()->m_iDbId,
                               m_item->GetVideoInfoTag()->m_type, "thumb");
}

std::string CVideoItemArtworkActorHandler::GetLocalArt() const
{
  std::string localArt;
  std::string picturePath;
  const std::string thumb = URIUtils::AddFileToFolder(picturePath, "folder.jpg");
  if (CFileUtils::Exists(thumb))
    localArt = thumb;

  return localArt;
}

//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkSeasonHandler (Season handler)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkSeasonHandler : public CVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkSeasonHandler(const std::shared_ptr<CFileItem>& item,
                                          const std::string& artType)
    : CVideoItemArtworkHandler(item, artType)
  {
  }

  std::vector<std::string> GetRemoteArt() const override;
};

std::vector<std::string> CVideoItemArtworkSeasonHandler::GetRemoteArt() const
{
  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
    return {};
  }

  std::vector<std::string> remoteArt;
  CVideoInfoTag tag;
  videodb.GetTvShowInfo("", tag, m_item->GetVideoInfoTag()->m_iIdShow);
  tag.m_strPictureURL.Parse();
  tag.m_strPictureURL.GetThumbUrls(remoteArt, m_artType, m_item->GetVideoInfoTag()->m_iSeason);
  return remoteArt;
}

//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkMovieSetHandler (Movie set handler)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkMovieSetHandler : public CVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkMovieSetHandler(const std::shared_ptr<CFileItem>& item,
                                            const std::string& artType)
    : CVideoItemArtworkHandler(item, artType)
  {
  }

  std::vector<std::string> GetRemoteArt() const override;
  std::string GetLocalArt() const override;

  std::string GetDefaultIcon() const override { return "DefaultVideo.png"; }

  void AddItemPathToFileBrowserSources(std::vector<CMediaSource>& sources) override;
};

std::vector<std::string> CVideoItemArtworkMovieSetHandler::GetRemoteArt() const
{
  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
    return {};
  }

  std::vector<std::string> remoteArt;
  const std::string baseDir =
      StringUtils::Format("videodb://movies/sets/{}", m_item->GetVideoInfoTag()->m_iDbId);
  CFileItemList items;
  if (videodb.GetMoviesNav(baseDir, items))
  {
    for (const auto& item : items)
    {
      CVideoInfoTag* videotag = item->GetVideoInfoTag();
      videotag->m_strPictureURL.Parse();
      videotag->m_strPictureURL.GetThumbUrls(remoteArt, "set." + m_artType, -1, true);
    }
  }
  return remoteArt;
}

std::string CVideoItemArtworkMovieSetHandler::GetLocalArt() const
{
  std::string localArt;
  const std::string infoFolder =
      VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(m_item->GetLabel());
  if (!infoFolder.empty())
  {
    CFileItemList availableArtFiles;
    CDirectory::GetDirectory(infoFolder, availableArtFiles,
                             CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                             DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    for (const auto& artFile : availableArtFiles)
    {
      std::string candidate = URIUtils::GetFileName(artFile->GetDynPath());
      URIUtils::RemoveExtension(candidate);
      if (StringUtils::EqualsNoCase(candidate, m_artType))
      {
        localArt = artFile->GetDynPath();
        break;
      }
    }
  }
  return localArt;
}

void CVideoItemArtworkMovieSetHandler::AddItemPathToFileBrowserSources(
    std::vector<CMediaSource>& sources)
{
  AddItemPathStringToFileBrowserSources(
      sources, VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(m_item->GetLabel()),
      g_localizeStrings.Get(36041) /* * Item folder */);
  AddItemPathStringToFileBrowserSources(
      sources,
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER),
      "* " + g_localizeStrings.Get(20226) /* Movie set information folder */);
}

//-------------------------------------------------------------------------------------------------
// CVideoItemArtworkFanartHandler (Handler for all media types, to manage fanart art type)
//-------------------------------------------------------------------------------------------------

class CVideoItemArtworkFanartHandler : public CVideoItemArtworkHandler
{
public:
  explicit CVideoItemArtworkFanartHandler(const std::shared_ptr<CFileItem>& item,
                                          const std::string& artType)
    : CVideoItemArtworkHandler(item, artType)
  {
    // Ensure the fanart is unpacked
    m_item->GetVideoInfoTag()->m_fanart.Unpack();
  }

  std::string GetCurrentArt() const override;
  std::vector<std::string> GetRemoteArt() const override;
  std::string GetLocalArt() const override;

  std::string GetDefaultIcon() const override { return "DefaultPicture.png"; }
  bool SupportsFlippedArt() const override { return true; }

  std::string UpdateEmbeddedArt(const std::string& art) override;
  std::string UpdateRemoteArt(const std::vector<std::string>& art, int index) override;
};

std::string CVideoItemArtworkFanartHandler::GetCurrentArt() const
{
  return m_item->GetArt("fanart");
}

std::vector<std::string> CVideoItemArtworkFanartHandler::GetRemoteArt() const
{
  std::vector<std::string> remoteArt;
  const CVideoInfoTag* videoTag = m_item->GetVideoInfoTag();
  for (unsigned int i = 0; i < videoTag->m_fanart.GetNumFanarts(); ++i)
  {
    const std::string thumb = videoTag->m_fanart.GetPreviewURL(i);
    if (URIUtils::IsProtocol(thumb, "image"))
      continue;

    remoteArt.emplace_back(CTextureUtils::GetWrappedThumbURL(thumb));
  }
  return remoteArt;
}

std::string CVideoItemArtworkFanartHandler::GetLocalArt() const
{
  return m_item->GetLocalFanart();
}

std::string CVideoItemArtworkFanartHandler::UpdateEmbeddedArt(const std::string& art)
{
  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
    return art;
  }

  CVideoInfoTag* videoTag = m_item->GetVideoInfoTag();
  const int currentTag = videoTag->m_fanart.GetNumFanarts();
  int matchingTag = -1;
  for (int i = 0; i < currentTag; ++i)
  {
    if (URIUtils::IsProtocol(videoTag->m_fanart.GetImageURL(i), "image"))
      matchingTag = i;
  }

  if (matchingTag != -1)
  {
    videoTag->m_fanart.AddFanart(art, "", "");
    matchingTag = currentTag;
  }

  videoTag->m_fanart.SetPrimaryFanart(matchingTag);
  videodb.UpdateFanart(*m_item, m_item->GetVideoContentType());
  return art;
}

std::string CVideoItemArtworkFanartHandler::UpdateRemoteArt(const std::vector<std::string>& art,
                                                            int index)
{
  CVideoInfoTag* videoTag = m_item->GetVideoInfoTag();

  CVideoDatabase videodb;
  if (!videodb.Open())
  {
    CLog::LogF(LOGERROR, "Cannot open video database!");
  }
  else
  {
    videoTag->m_fanart.SetPrimaryFanart(index);
    videodb.UpdateFanart(*m_item, m_item->GetVideoContentType());
  }
  return videoTag->m_fanart.GetImageURL();
}

} // unnamed namespace

//-------------------------------------------------------------------------------------------------
// IVideoItemArtworkHandlerFactory
//-------------------------------------------------------------------------------------------------

std::unique_ptr<IVideoItemArtworkHandler> IVideoItemArtworkHandlerFactory::Create(
    const std::shared_ptr<CFileItem>& item,
    const std::string& mediaType,
    const std::string& artType)
{
  std::unique_ptr<IVideoItemArtworkHandler> artHandler;

  if (artType == "fanart" && mediaType != MediaTypeVideoCollection)
    artHandler = std::make_unique<CVideoItemArtworkFanartHandler>(item, artType);
  else if (mediaType == MediaTypeArtist)
    artHandler = std::make_unique<CVideoItemArtworkArtistHandler>(item, artType);
  else if (mediaType == "actor")
    artHandler = std::make_unique<CVideoItemArtworkActorHandler>(item, artType);
  else if (mediaType == MediaTypeSeason)
    artHandler = std::make_unique<CVideoItemArtworkSeasonHandler>(item, artType);
  else if (mediaType == MediaTypeVideoCollection)
    artHandler = std::make_unique<CVideoItemArtworkMovieSetHandler>(item, artType);
  else
    artHandler = std::make_unique<CVideoItemArtworkHandler>(item, artType);

  return artHandler;
}

} // namespace KODI::VIDEO
