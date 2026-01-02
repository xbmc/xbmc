/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ArtUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/StackDirectory.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <fmt/format.h>

using namespace XFILE;

namespace KODI::ART
{

void FillInDefaultIcon(CFileItem& item)
{
  if (URIUtils::IsPVRGuideItem(item.GetPath()))
  {
    // epg items never have a default icon. no need to execute this expensive method.
    // when filling epg grid window, easily tens of thousands of epg items are processed.
    return;
  }

  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //
  // for folders
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  if (item.GetArt("icon").empty())
  {
    if (!item.IsFolder())
    {
      /* To reduce the average runtime of this code, this list should
       * be ordered with most frequently seen types first.  Also bear
       * in mind the complexity of the code behind the check in the
       * case of IsWhatever() returns false.
       */
      if (item.IsPVRChannel())
      {
        if (URIUtils::IsPVRRadioChannel(item.GetPath()))
          item.SetArt("icon", "DefaultMusicSongs.png");
        else
          item.SetArt("icon", "DefaultTVShows.png");
      }
      else if (item.IsLiveTV())
      {
        // Live TV Channel
        item.SetArt("icon", "DefaultTVShows.png");
      }
      else if (URIUtils::IsArchive(item.GetPath()))
      { // archive
        item.SetArt("icon", "DefaultFile.png");
      }
      else if (item.IsUsablePVRRecording())
      {
        // PVR recording
        item.SetArt("icon", "DefaultVideo.png");
      }
      else if (item.IsDeletedPVRRecording())
      {
        // PVR deleted recording
        item.SetArt("icon", "DefaultVideoDeleted.png");
      }
      else if (item.IsPVRProvider())
      {
        item.SetArt("icon", "DefaultPVRProvider.png");
      }
      else if (PLAYLIST::IsPlayList(item) || PLAYLIST::IsSmartPlayList(item))
      {
        item.SetArt("icon", "DefaultPlaylist.png");
      }
      else if (MUSIC::IsAudio(item))
      {
        // audio
        item.SetArt("icon", "DefaultAudio.png");
      }
      else if (VIDEO::IsVideo(item))
      {
        // video
        item.SetArt("icon", "DefaultVideo.png");
      }
      else if (item.IsPVRTimer())
      {
        item.SetArt("icon", "DefaultVideo.png");
      }
      else if (item.IsPicture())
      {
        // picture
        item.SetArt("icon", "DefaultPicture.png");
      }
      else if (item.IsPythonScript())
      {
        item.SetArt("icon", "DefaultScript.png");
      }
      else if (item.IsFavourite())
      {
        item.SetArt("icon", "DefaultFavourites.png");
      }
      else
      {
        // default icon for unknown file type
        item.SetArt("icon", "DefaultFile.png");
      }
    }
    else
    {
      if (PLAYLIST::IsPlayList(item) || PLAYLIST::IsSmartPlayList(item))
      {
        item.SetArt("icon", "DefaultPlaylist.png");
      }
      else if (item.IsParentFolder())
      {
        item.SetArt("icon", "DefaultFolderBack.png");
      }
      else
      {
        item.SetArt("icon", "DefaultFolder.png");
      }
    }
  }
  // Set the icon overlays (if applicable)
  if (!item.HasOverlay() && !item.HasProperty("icon_never_overlay"))
  {
    if (URIUtils::IsInZIP(item.GetPath()))
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_ZIP);
    else if (URIUtils::IsInArchive(item.GetPath()))
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_RAR);
  }
}

std::string GetFolderThumb(const CFileItem& item, const std::string& folderJPG /* = "folder.jpg" */)
{
  if (item.IsPlugin())
    return "";

  std::string folder{item.GetPath()};
  if (item.IsMultiPath())
    folder = CMultiPathDirectory::GetFirstPath(item.GetPath());

  if (item.IsStack() || URIUtils::IsInArchive(folder) || URIUtils::IsBlurayPath(folder))
    URIUtils::GetParentPath(item.GetPath(), folder);
  else if (URIUtils::IsDiscPath(folder))
    folder = URIUtils::GetDiscBasePath(folder);

  return URIUtils::AddFileToFolder(URIUtils::GetDirectory(folder), folderJPG);
}

std::string GetLocalArt(const CFileItem& item,
                        const std::string& artFile,
                        bool useFolder,
                        AdditionalIdentifiers additionalIdentifiers)
{
  // no retrieving of empty art files from folders
  if (useFolder && artFile.empty())
    return "";

  std::string strFile = GetLocalArtBaseFilename(item, useFolder, additionalIdentifiers);
  if (strFile.empty()) // empty filepath -> nothing to find
    return "";

  if (useFolder)
  {
    if (!artFile.empty())
      return URIUtils::AddFileToFolder(strFile, artFile);
  }
  else
  {
    if (artFile.empty()) // old thumbnail matching
      return URIUtils::ReplaceExtension(strFile, ".tbn");
    else
      return URIUtils::ReplaceExtension(strFile, "-" + artFile);
  }
  return "";
}

std::string GetLocalArtBaseFilename(const CFileItem& item,
                                    bool& useFolder,
                                    AdditionalIdentifiers additionalIdentifiers /* = none */)
{
  std::string strFile;
  if (item.IsStack())
  {
    strFile = CStackDirectory::GetStackTitlePath(item.GetPath());
    URIUtils::RemoveSlashAtEnd(strFile);
    if (!URIUtils::HasExtension(strFile))
      strFile = URIUtils::ReplaceExtension(
          strFile, ".avi"); // If no extension (folder stack) then add dummy one
  }

  std::string file = strFile.empty() ? item.GetPath() : strFile;

  const CURL url{file};
  if (URIUtils::IsInArchive(file) || URIUtils::IsArchive(url))
    strFile = URIUtils::ReplaceExtension(url.GetHostName(), ".avi");

  if (item.IsMultiPath())
    strFile = CMultiPathDirectory::GetFirstPath(item.GetPath());

  if (URIUtils::IsBlurayPath(item.GetDynPath()))
    file = strFile = URIUtils::GetDiscFile(item.GetDynPath());

  if (URIUtils::IsOpticalMediaFile(file) && !URIUtils::IsArchive(url))
  {
    // Optical media files (VIDEO_TS.IFO/INDEX.BDMV) should be treated like folders
    useFolder = true; // ByRef so changes behaviour in GetLocalArt()
    strFile = URIUtils::GetBasePath(file);
  }

  if (!URIUtils::GetFileName(file).empty() && item.HasVideoInfoTag() &&
      additionalIdentifiers != AdditionalIdentifiers::NONE)
  {
    using enum AdditionalIdentifiers;
    switch (additionalIdentifiers)
    {
      case SEASON_AND_EPISODE:
      {
        // Note this is an exception to the optical media rule above - episode art files will be stored alongside the episode .nfo
        const CVideoInfoTag* tag{item.GetVideoInfoTag()};
        if (tag->m_iSeason > -1 && tag->m_iEpisode > -1)
        {
          std::string baseFile{file};
          URIUtils::RemoveExtension(baseFile);
          strFile = fmt::format("{}-S{:02}E{:02}{}", baseFile, tag->m_iSeason, tag->m_iEpisode,
                                URIUtils::GetExtension(file));
          useFolder = false;
        }
        break;
      }
      case PLAYLIST:
      {
        const CVideoInfoTag* tag{item.GetVideoInfoTag()};
        if (tag->m_iTrack > -1)
        {
          std::string baseFile{file};
          URIUtils::RemoveExtension(baseFile);
          strFile =
              fmt::format("{}-{:05}{}", baseFile, tag->m_iTrack, URIUtils::GetExtension(file));
          useFolder = false;
        }
        break;
      }
      case NONE:
      default:
        break;
    }
  }
  else if (useFolder && !(item.IsFolder() && !item.IsFileFolder()))
  {
    file = strFile.empty() ? item.GetPath() : strFile;
    strFile = URIUtils::GetDirectory(file);
  }

  if (strFile.empty())
    strFile = item.GetDynPath();

  return strFile;
}

std::string GetLocalFanart(const CFileItem& item)
{
  if (VIDEO::IsVideoDb(item))
  {
    if (!item.HasVideoInfoTag())
      return ""; // nothing can be done
    CFileItem dbItem(item.IsFolder() ? item.GetVideoInfoTag()->m_strPath
                                     : item.GetVideoInfoTag()->m_strFileNameAndPath,
                     item.IsFolder());
    return GetLocalFanart(dbItem);
  }

  std::string alternateFile;
  std::string file{item.GetPath()};
  if (item.IsStack())
  {
    // Two possible art locations for stacks:
    // First - file stacks   - stack:///path/movie_part_1.avi -> /path/movie-fanart.jpg
    //       - folder stacks - stack:///path/movie/movie_part_1/file.avi -> /path/movie/movie-fanart.jpg
    file = CStackDirectory::GetStackTitlePath(file);
    URIUtils::RemoveSlashAtEnd(file); // Can be folder or file

    // Second - file stacks   - stack:///path/movie_part_1.avi -> /path/movie_part_1-fanart.jpg
    //        - folder stacks - stack:///path/movie/movie_part_1/file.avi -> /path/movie/movie-fanart.jpg
    CFileItem stackItem(CStackDirectory::GetFirstStackedFile(item.GetPath()), false);
    alternateFile = URIUtils::ReplaceExtension(GetTBNFile(stackItem), "-fanart");
  }

  if (URIUtils::IsInArchive(file))
  {
    const CURL url(file);
    file = url.GetHostName();
  }

  // no local fanart available for these
  if (NETWORK::IsInternetStream(item) || URIUtils::IsUPnP(file) || URIUtils::IsBlurayPath(file) ||
      item.IsLiveTV() || item.IsPlugin() || item.IsAddonsPath() || item.IsDVD() ||
      (URIUtils::IsFTP(file) &&
       !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs) ||
      item.GetPath().empty())
    return "";

  const std::string dir{URIUtils::GetDirectory(file)};
  if (dir.empty())
    return "";

  CFileItemList items;
  CDirectory::GetDirectory(dir, items,
                           CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  if (item.IsOpticalMediaFile())
  {
    // Get files from the optical media parent folder as well
    CFileItemList moreItems;
    CDirectory::GetDirectory(item.GetLocalMetadataPath(), moreItems,
                             CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                             DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    items.Append(moreItems);
  }
  if (!alternateFile.empty())
  {
    // Get files from the alternate path as well
    const std::string alternateDir{URIUtils::GetDirectory(alternateFile)};
    if (!alternateDir.empty() && alternateDir != dir)
    {
      CFileItemList moreItems;
      CDirectory::GetDirectory(alternateDir, moreItems,
                               CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                               DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
      items.Append(moreItems);
    }
  }

  std::vector<std::string> fanarts = {"fanart"};

  file = URIUtils::ReplaceExtension(file, "-fanart");
  fanarts.insert(item.IsFolder() ? fanarts.end() : fanarts.begin(), URIUtils::GetFileName(file));

  if (!alternateFile.empty())
    fanarts.insert(item.IsFolder() ? fanarts.end() : fanarts.begin(),
                   URIUtils::GetFileName(alternateFile));

  for (const auto& fanart : fanarts)
  {
    for (const auto& artItem : items)
    {
      std::string strCandidate{URIUtils::GetFileName(artItem->GetPath())};
      URIUtils::RemoveExtension(strCandidate);
      std::string fanart2{fanart};
      URIUtils::RemoveExtension(fanart2);
      if (StringUtils::EqualsNoCase(strCandidate, fanart2))
        return artItem->GetPath();
    }
  }

  return "";
}

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>(-<SxxEyy>).tbn
// <foldername>/ -> <foldername>(-<SxxEyy>).tbn
std::string GetTBNFile(const CFileItem& item, int season /* = - 1 */, int episode /* = -1 */)
{
  std::string file{item.GetPath()};

  if (item.IsStack())
  {
    // First see if there's a .tbn with the first stacked file (rather than in the stack base path)
    CFileItem stackItem(CStackDirectory::GetFirstStackedFile(file), false);
    const std::string TBNFile{GetTBNFile(stackItem)};
    const std::string path{CStackDirectory::GetParentPath(file)};
    const std::string returnPath{URIUtils::AddFileToFolder(path, URIUtils::GetFileName(TBNFile))};
    if (CFile::Exists(returnPath))
      return returnPath; // Will already have .tbn extension

    // Otherwise fall back to the stack base path
    // File stack returns file, folder stack returns folder (so remove slash to get file name)
    file = {CStackDirectory::GetStackTitlePath(file)};
    URIUtils::RemoveSlashAtEnd(file);
  }

  if (URIUtils::IsInArchive(file))
  {
    const CURL url(file);
    file = url.GetHostName();
  }
  else if (URIUtils::IsBlurayPath(file))
    file = URIUtils::GetDiscFile(file);

  CURL url(file);
  file = url.GetFileName();

  if (item.IsFolder() && !item.IsFileFolder())
    URIUtils::RemoveSlashAtEnd(file);

  std::string thumbFile;
  if (!file.empty())
  {
    if (item.IsFolder() && !item.IsFileFolder())
      thumbFile = file + ".tbn"; // Folder, so just add ".tbn"
    else
    {
      if (season > -1 && episode > -1)
      {
        URIUtils::RemoveExtension(file);
        thumbFile = fmt::format("{}-S{:02}E{:02}.tbn", file, season, episode);
      }
      else
        thumbFile = URIUtils::ReplaceExtension(file, ".tbn");
    }

    url.SetFileName(thumbFile);
    thumbFile = url.Get();
  }
  return thumbFile;
}

} // namespace KODI::ART
