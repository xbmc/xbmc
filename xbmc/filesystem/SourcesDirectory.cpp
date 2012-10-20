/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "settings/Settings.h"
#include "guilib/TextureManager.h"
#include "storage/MediaManager.h"

using namespace XFILE;

CSourcesDirectory::CSourcesDirectory(void)
{
}

CSourcesDirectory::~CSourcesDirectory(void)
{
}

bool CSourcesDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // break up our path
  // format is:  sources://<type>/
  CURL url(strPath);
  CStdString type(url.GetFileName());
  URIUtils::RemoveSlashAtEnd(type);

  VECSOURCES sources;
  VECSOURCES *sourcesFromType = g_settings.GetSourcesFromType(type);
  if (sourcesFromType)
    sources = *sourcesFromType;
  g_mediaManager.GetRemovableDrives(sources);

  if (!sourcesFromType)
    return false;

  return GetDirectory(sources, items);
}

bool CSourcesDirectory::GetDirectory(const VECSOURCES &sources, CFileItemList &items)
{
  for (unsigned int i = 0; i < sources.size(); ++i)
  {
    const CMediaSource& share = sources[i];
    CFileItemPtr pItem(new CFileItem(share));
    if (pItem->IsLastFM() || (pItem->GetPath().Left(14).Equals("musicsearch://")))
      pItem->SetCanQueue(false);
    
    CStdString strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && share.m_strThumbnailImage.IsEmpty())
    {
      CUtil::GetDVDDriveIcon( pItem->GetPath(), strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      CStdString strThumb = "special://temp/dvdicon.tbn";
      if (XFILE::CFile::Exists(strThumb))
        pItem->SetArt("thumb", strThumb);
    }
    else if (pItem->GetPath().Left(9) == "addons://")
      strIcon = "DefaultHardDisk.png";
    else if (pItem->IsLastFM()
             || pItem->IsVideoDb()
             || pItem->IsMusicDb()
             || pItem->IsPlugin()
             || pItem->GetPath() == "special://musicplaylists/"
             || pItem->GetPath() == "special://videoplaylists/"
             || pItem->GetPath() == "musicsearch://")
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
    if (share.m_iHasLock == 2 && g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_LOCKED);
    else
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_NONE);
    
    items.Add(pItem);
  }
  return true;
}

bool CSourcesDirectory::Exists(const char* strPath)
{
  return true;
}
