/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SourcesDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "guilib/TextureManager.h"
#include "media/MediaLockState.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/ArtTypes.h"
#include "utils/DefaultArt.h"
#include "utils/FileUtils.h"
#include "utils/PlaceholderPaths.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"

using namespace KODI;
using namespace XFILE;

CSourcesDirectory::CSourcesDirectory(void) = default;

CSourcesDirectory::~CSourcesDirectory(void) = default;

std::string CSourcesDirectory::PathOf(KODI::MEDIA::MediaSection section)
{
  return StringUtils::Format("sources://{}/", KODI::MEDIA::NameOf(section));
}

std::optional<KODI::MEDIA::MediaSection> CSourcesDirectory::SectionOf(const std::string& path)
{
  const CURL url{path};
  if (!url.IsProtocol("sources"))
    return {};

  std::string name{url.GetFileName()};
  URIUtils::RemoveSlashAtEnd(name);
  return KODI::MEDIA::MediaSectionFromName(name);
}

bool CSourcesDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const std::optional<KODI::MEDIA::MediaSection> section{SectionOf(url.Get())};
  if (!section)
    return false;

  std::vector<CMediaSource> sources{CMediaSourceSettings::GetInstance().GetSources(*section)};
  CServiceBroker::GetMediaManager().GetRemovableDrives(sources);

  return GetDirectory(sources, items);
}

bool CSourcesDirectory::GetDirectory(const std::vector<CMediaSource>& sources, CFileItemList& items)
{
  for (unsigned int i = 0; i < sources.size(); ++i)
  {
    const CMediaSource& share = sources[i];
    CFileItemPtr pItem(new CFileItem(share));
    if (URIUtils::IsProtocol(pItem->GetPath(), "musicsearch"))
      pItem->SetCanQueue(false);

    std::string strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == SourceType::OPTICAL_DISC && share.m_strThumbnailImage.empty())
    {
      CUtil::GetDVDDriveIcon( pItem->GetPath(), strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      std::string strThumb = "special://temp/dvdicon.tbn";
      if (CFileUtils::Exists(strThumb))
        pItem->SetArt(ART::TYPE::THUMB, strThumb);
    }
    else if (URIUtils::IsProtocol(pItem->GetPath(), "addons"))
      strIcon = ART::DEFAULT::HARD_DISK;
    else if (pItem->IsPath(CUtil::PlaylistsPathOf(KODI::MEDIA::MediaSection::MUSIC)) ||
             pItem->IsPath(CUtil::PlaylistsPathOf(KODI::MEDIA::MediaSection::VIDEO)))
      strIcon = ART::DEFAULT::PLAYLIST;
    else if (VIDEO::IsVideoDb(*pItem) || MUSIC::IsMusicDb(*pItem) || pItem->IsPlugin() ||
             pItem->IsPath(ITEM::PLACEHOLDER::MUSIC_SEARCH))
      strIcon = ART::DEFAULT::FOLDER;
    else if (NETWORK::IsRemote(*pItem))
      strIcon = ART::DEFAULT::NETWORK;
    else if (pItem->IsISO9660())
      strIcon = ART::DEFAULT::DVD_ROM;
    else if (pItem->IsDVD())
      strIcon = ART::DEFAULT::DVD_FULL;
    else if (pItem->IsBluray())
      strIcon = ART::DEFAULT::BLURAY;
    else if (MUSIC::IsCDDA(*pItem))
      strIcon = ART::DEFAULT::CDDA;
    else if (pItem->IsRemovable() &&
             CServiceBroker::GetGUI()->GetTextureManager().HasTexture(ART::DEFAULT::REMOVABLE_DISK))
      strIcon = ART::DEFAULT::REMOVABLE_DISK;
    else
      strIcon = ART::DEFAULT::HARD_DISK;

    pItem->SetArt(ART::TYPE::ICON, strIcon);
    if (share.GetLockInfo().IsLocked() &&
        m_profileManager->GetMasterProfile().getLockMode() != LockMode::EVERYONE)
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_LOCKED);
    else
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_NONE);

    items.Add(pItem);
  }
  return true;
}

bool CSourcesDirectory::Exists(const CURL& url)
{
  return true;
}
