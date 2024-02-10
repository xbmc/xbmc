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
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"

using namespace KODI;
using namespace XFILE;

CSourcesDirectory::CSourcesDirectory(void) = default;

CSourcesDirectory::~CSourcesDirectory(void) = default;

bool CSourcesDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // break up our path
  // format is:  sources://<type>/
  std::string type(url.GetFileName());
  URIUtils::RemoveSlashAtEnd(type);

  VECSOURCES sources;
  VECSOURCES *sourcesFromType = CMediaSourceSettings::GetInstance().GetSources(type);
  if (!sourcesFromType)
    return false;

  sources = *sourcesFromType;
  CServiceBroker::GetMediaManager().GetRemovableDrives(sources);

  return GetDirectory(sources, items);
}

bool CSourcesDirectory::GetDirectory(const VECSOURCES &sources, CFileItemList &items)
{
  for (unsigned int i = 0; i < sources.size(); ++i)
  {
    const CMediaSource& share = sources[i];
    CFileItemPtr pItem(new CFileItem(share));
    if (URIUtils::IsProtocol(pItem->GetPath(), "musicsearch"))
      pItem->SetCanQueue(false);

    std::string strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && share.m_strThumbnailImage.empty())
    {
      CUtil::GetDVDDriveIcon( pItem->GetPath(), strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      std::string strThumb = "special://temp/dvdicon.tbn";
      if (CFileUtils::Exists(strThumb))
        pItem->SetArt("thumb", strThumb);
    }
    else if (URIUtils::IsProtocol(pItem->GetPath(), "addons"))
      strIcon = "DefaultHardDisk.png";
    else if (   pItem->IsPath("special://musicplaylists/")
             || pItem->IsPath("special://videoplaylists/"))
      strIcon = "DefaultPlaylist.png";
    else if (VIDEO::IsVideoDb(*pItem) || MUSIC::IsMusicDb(*pItem) || pItem->IsPlugin() ||
             pItem->IsPath("musicsearch://"))
      strIcon = "DefaultFolder.png";
    else if (NETWORK::IsRemote(*pItem))
      strIcon = "DefaultNetwork.png";
    else if (pItem->IsISO9660())
      strIcon = "DefaultDVDRom.png";
    else if (pItem->IsDVD())
      strIcon = "DefaultDVDFull.png";
    else if (pItem->IsBluray())
      strIcon = "DefaultBluray.png";
    else if (MUSIC::IsCDDA(*pItem))
      strIcon = "DefaultCDDA.png";
    else if (pItem->IsRemovable() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture("DefaultRemovableDisk.png"))
      strIcon = "DefaultRemovableDisk.png";
    else
      strIcon = "DefaultHardDisk.png";

    pItem->SetArt("icon", strIcon);
    if (share.m_iHasLock == LOCK_STATE_LOCKED &&
        m_profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
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
