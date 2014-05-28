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

#ifndef FILESYSTEM_SOURCESDIRECTORY_H_INCLUDED
#define FILESYSTEM_SOURCESDIRECTORY_H_INCLUDED
#include "SourcesDirectory.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_UTIL_H_INCLUDED
#define FILESYSTEM_UTIL_H_INCLUDED
#include "Util.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef FILESYSTEM_FILE_H_INCLUDED
#define FILESYSTEM_FILE_H_INCLUDED
#include "File.h"
#endif

#ifndef FILESYSTEM_PROFILES_PROFILESMANAGER_H_INCLUDED
#define FILESYSTEM_PROFILES_PROFILESMANAGER_H_INCLUDED
#include "profiles/ProfilesManager.h"
#endif

#ifndef FILESYSTEM_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#define FILESYSTEM_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#include "settings/MediaSourceSettings.h"
#endif

#ifndef FILESYSTEM_GUILIB_TEXTUREMANAGER_H_INCLUDED
#define FILESYSTEM_GUILIB_TEXTUREMANAGER_H_INCLUDED
#include "guilib/TextureManager.h"
#endif

#ifndef FILESYSTEM_STORAGE_MEDIAMANAGER_H_INCLUDED
#define FILESYSTEM_STORAGE_MEDIAMANAGER_H_INCLUDED
#include "storage/MediaManager.h"
#endif

#ifndef FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


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
  VECSOURCES *sourcesFromType = CMediaSourceSettings::Get().GetSources(type);
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
    if (StringUtils::StartsWithNoCase(pItem->GetPath(), "musicsearch://"))
      pItem->SetCanQueue(false);
    
    CStdString strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && share.m_strThumbnailImage.empty())
    {
      CUtil::GetDVDDriveIcon( pItem->GetPath(), strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      CStdString strThumb = "special://temp/dvdicon.tbn";
      if (XFILE::CFile::Exists(strThumb))
        pItem->SetArt("thumb", strThumb);
    }
    else if (StringUtils::StartsWith(pItem->GetPath(), "addons://"))
      strIcon = "DefaultHardDisk.png";
    else if (   pItem->IsVideoDb()
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
    if (share.m_iHasLock == 2 && CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
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
