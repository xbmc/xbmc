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
    if (URIUtils::IsInRAR(item.GetPath()))
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_RAR);
    else if (URIUtils::IsInZIP(item.GetPath()))
      item.SetOverlayImage(CGUIListItem::ICON_OVERLAY_ZIP);
  }
}

std::string GetFolderThumb(const CFileItem& item, const std::string& folderJPG /* = "folder.jpg" */)
{
  std::string strFolder = item.GetPath();

  if (item.IsStack())
  {
    URIUtils::GetParentPath(item.GetPath(), strFolder);
  }

  if (URIUtils::IsInRAR(strFolder) || URIUtils::IsInZIP(strFolder))
  {
    const CURL url(strFolder);
    strFolder = URIUtils::GetDirectory(url.GetHostName());
  }

  if (item.IsMultiPath())
    strFolder = CMultiPathDirectory::GetFirstPath(item.GetPath());

  if (item.IsPlugin())
    return "";

  return URIUtils::AddFileToFolder(strFolder, folderJPG);
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
    std::string strPath;
    URIUtils::GetParentPath(item.GetPath(), strPath);
    strFile = URIUtils::AddFileToFolder(
        strPath, URIUtils::GetFileName(CStackDirectory::GetStackedTitlePath(item.GetPath())));
  }

  std::string file = strFile.empty() ? item.GetPath() : strFile;
  if (URIUtils::IsInRAR(file) || URIUtils::IsInZIP(file))
  {
    std::string strPath = URIUtils::GetDirectory(file);
    std::string strParent;
    URIUtils::GetParentPath(strPath, strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(file));
  }

  if (item.IsMultiPath())
    strFile = CMultiPathDirectory::GetFirstPath(item.GetPath());

  if (URIUtils::IsBlurayPath(file))
    file = strFile = URIUtils::GetDiscFile(file);

  if (URIUtils::IsOpticalMediaFile(file))
  {
    // Optical media files (VIDEO_TS.IFO/INDEX.BDMV) should be treated like folders
    useFolder = true; // ByRef so changes behaviour in GetLocalArt()
    strFile = URIUtils::GetBasePath(file);
  }

  if (URIUtils::IsBlurayPath(file))
    strFile = URIUtils::GetDiscFile(file);

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

  std::string file2;
  std::string file = item.GetPath();
  if (item.IsStack())
  {
    std::string path;
    URIUtils::GetParentPath(item.GetPath(), path);
    CStackDirectory dir;
    std::string path2;
    path2 = dir.GetStackedTitlePath(file);
    file = URIUtils::AddFileToFolder(path, URIUtils::GetFileName(path2));
    CFileItem fan_item(dir.GetFirstStackedFile(item.GetPath()), false);
    std::string TBNFile(URIUtils::ReplaceExtension(GetTBNFile(fan_item), "-fanart"));
    file2 = URIUtils::AddFileToFolder(path, URIUtils::GetFileName(TBNFile));
  }

  if (URIUtils::IsInRAR(file) || URIUtils::IsInZIP(file))
  {
    std::string path = URIUtils::GetDirectory(file);
    std::string parent;
    URIUtils::GetParentPath(path, parent);
    file = URIUtils::AddFileToFolder(parent, URIUtils::GetFileName(item.GetPath()));
  }

  // no local fanart available for these
  if (NETWORK::IsInternetStream(item) || URIUtils::IsUPnP(file) || URIUtils::IsBlurayPath(file) ||
      item.IsLiveTV() || item.IsPlugin() || item.IsAddonsPath() || item.IsDVD() ||
      (URIUtils::IsFTP(file) &&
       !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs) ||
      item.GetPath().empty())
    return "";

  std::string dir = URIUtils::GetDirectory(file);

  if (dir.empty())
    return "";

  CFileItemList items;
  CDirectory::GetDirectory(dir, items,
                           CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  if (item.IsOpticalMediaFile())
  { // grab from the optical media parent folder as well
    CFileItemList moreItems;
    CDirectory::GetDirectory(item.GetLocalMetadataPath(), moreItems,
                             CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                             DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    items.Append(moreItems);
  }

  std::vector<std::string> fanarts = {"fanart"};

  file = URIUtils::ReplaceExtension(file, "-fanart");
  fanarts.insert(item.IsFolder() ? fanarts.end() : fanarts.begin(), URIUtils::GetFileName(file));

  if (!file2.empty())
    fanarts.insert(item.IsFolder() ? fanarts.end() : fanarts.begin(), URIUtils::GetFileName(file2));

  for (const auto& fanart : fanarts)
  {
    for (const auto& item : items)
    {
      std::string strCandidate = URIUtils::GetFileName(item->GetPath());
      URIUtils::RemoveExtension(strCandidate);
      std::string fanart2 = fanart;
      URIUtils::RemoveExtension(fanart2);
      if (StringUtils::EqualsNoCase(strCandidate, fanart2))
        return item->GetPath();
    }
  }

  return "";
}

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>(-<SxxEyy>).tbn
// <foldername>/ -> <foldername>(-<SxxEyy>).tbn
std::string GetTBNFile(const CFileItem& item, int season /* = - 1 */, int episode /* = -1 */)
{
  std::string thumbFile;
  std::string file{item.GetPath()};

  if (item.IsStack())
  {
    std::string path;
    URIUtils::GetParentPath(item.GetPath(), path);
    CFileItem item(CStackDirectory::GetFirstStackedFile(file), false);
    const std::string TBNFile{GetTBNFile(item)};
    const std::string returnPath{URIUtils::AddFileToFolder(path, URIUtils::GetFileName(TBNFile))};
    if (CFile::Exists(returnPath))
      return returnPath;

    const std::string& stackPath{CStackDirectory::GetStackedTitlePath(file)};
    file = URIUtils::AddFileToFolder(path, URIUtils::GetFileName(stackPath));
  }

  if (URIUtils::IsInRAR(file) || URIUtils::IsInZIP(file))
  {
    const std::string path = URIUtils::GetDirectory(file);
    std::string parent;
    URIUtils::GetParentPath(path, parent);
    file = URIUtils::AddFileToFolder(parent, URIUtils::GetFileName(item.GetPath()));
  }

  if (URIUtils::IsBlurayPath(file))
    file = URIUtils::GetDiscFile(file);

  CURL url(file);
  file = url.GetFileName();

  if (item.IsFolder() && !item.IsFileFolder())
    URIUtils::RemoveSlashAtEnd(file);

  if (!file.empty())
  {
    if (item.IsFolder() && !item.IsFileFolder())
      thumbFile = file + ".tbn"; // folder, so just add ".tbn"
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
