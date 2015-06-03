/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SourcesDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "Util.h"
#include "FileItem.h"
#include "File.h"
#include "profiles/ProfilesManager.h"
#include "settings/MediaSourceSettings.h"
#include "guilib/TextureManager.h"
#include "storage/MediaManager.h"

using namespace XFILE;

CSourcesDirectory::CSourcesDirectory(void)
{
}

CSourcesDirectory::~CSourcesDirectory(void)
{
}

bool CSourcesDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // break up our path
  // format is:  sources://<type>/
  std::string type(url.GetFileName());
  URIUtils::RemoveSlashAtEnd(type);

  VECSOURCES sources;
  VECSOURCES *sourcesFromType = CMediaSourceSettings::Get().GetSources(type);
  if (!sourcesFromType)
    return false;

  sources = *sourcesFromType;
  g_mediaManager.GetRemovableDrives(sources);

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
      if (XFILE::CFile::Exists(strThumb))
        pItem->SetArt("thumb", strThumb);
    }
    else if (URIUtils::IsProtocol(pItem->GetPath(), "addons"))
      strIcon = "DefaultHardDisk.png";
    else if (   pItem->IsVideoDb()
             || pItem->IsMusicDb()
             || pItem->IsPlugin()
             || pItem->IsPath("special://musicplaylists/")
             || pItem->IsPath("special://videoplaylists/")
             || pItem->IsPath("musicsearch://"))
      strIcon = "DefaultFolder.png";
    else if (pItem->IsRemote())
      strIcon = "DefaultNetwork.png";
    else if (pItem->IsISO9660())
      strIcon = "DefaultDVDRom.png";
    else if (pItem->IsDVD())
      strIcon = "DefaultDVDRom.png";
    else if (pItem->IsCDDA())
      strIcon = "DefaultCDDA.png";
    else if (pItem->IsRemovable() && g_TextureManager.HasTexture("DefaultRemovableDisk.png"))
      strIcon = "DefaultRemovableDisk.png";
    else
      strIcon = "DefaultHardDisk.png";
    
    pItem->SetIconImage(strIcon);
    if (share.m_iHasLock == 2 && CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
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
