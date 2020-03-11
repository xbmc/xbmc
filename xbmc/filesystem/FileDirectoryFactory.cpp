/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/URIUtils.h"
#include "FileDirectoryFactory.h"
#include "UDFDirectory.h"
#include "RSSDirectory.h"
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/APKDirectory.h"
#endif
#include "XbtDirectory.h"
#include "ZipDirectory.h"
#include "SmartPlaylistDirectory.h"
#include "playlists/SmartPlayList.h"
#include "PlaylistFileDirectory.h"
#include "playlists/PlayListFactory.h"
#include "Directory.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "ServiceBroker.h"
#include "addons/AudioDecoder.h"
#include "addons/VFSEntry.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "AudioBookFileDirectory.h"

using namespace ADDON;
using namespace XFILE;
using namespace PLAYLIST;

CFileDirectoryFactory::CFileDirectoryFactory(void) = default;

CFileDirectoryFactory::~CFileDirectoryFactory(void) = default;

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFileDirectoryFactory::Create(const CURL& url, CFileItem* pItem, const std::string& strMask)
{
  if (url.IsProtocol("stack")) // disqualify stack as we need to work with each of the parts instead
    return NULL;

  std::string strExtension = URIUtils::GetExtension(url);
  StringUtils::ToLower(strExtension);
  if (!strExtension.empty() && CServiceBroker::IsBinaryAddonCacheUp())
  {
    BinaryAddonBaseList addonInfos;
    CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, true, ADDON_AUDIODECODER);
    for (const auto& addonInfo : addonInfos)
    {
      if (CAudioDecoder::HasTracks(addonInfo))
      {
        auto exts = StringUtils::Split(CAudioDecoder::GetExtensions(addonInfo), "|");
        if (std::find(exts.begin(), exts.end(), strExtension) != exts.end())
        {
          CAudioDecoder* result = new CAudioDecoder(addonInfo);
          if (!result->CreateDecoder() || !result->ContainsFiles(url))
          {
            delete result;
            return nullptr;
          }
          return result;
        }
      }
    }
  }

  if (!strExtension.empty() && CServiceBroker::IsBinaryAddonCacheUp())
  {
    for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    {
      if (vfsAddon->HasFileDirectories())
      {
        auto exts = StringUtils::Split(vfsAddon->GetExtensions(), "|");
        if (std::find(exts.begin(), exts.end(), strExtension) != exts.end())
        {
          CVFSEntryIFileDirectoryWrapper* wrap = new CVFSEntryIFileDirectoryWrapper(vfsAddon);
          if (wrap->ContainsFiles(url))
          {
            if (wrap->m_items.Size() == 1)
            {
              // one STORED file - collapse it down
              *pItem = *wrap->m_items[0];
            }
            else
            {
              // compressed or more than one file -> create a dir
              pItem->SetPath(wrap->m_items.GetPath());
            }

            // Check for folder, if yes return also wrap.
            // Needed to fix for e.g. RAR files with only one file inside
            pItem->m_bIsFolder = URIUtils::HasSlashAtEnd(pItem->GetPath());
            if (pItem->m_bIsFolder)
              return wrap;
          }
          else
          {
            pItem->m_bIsFolder = true;
          }

          delete wrap;
          return nullptr;
        }
      }
    }
  }

  if (pItem->IsRSS())
    return new CRSSDirectory();

  if (pItem->IsDiscImage())
    return new CUDFDirectory();

#if defined(TARGET_ANDROID)
  if (url.IsFileType("apk"))
  {
    CURL zipURL = URIUtils::CreateArchivePath("apk", url);

    CFileItemList items;
    CDirectory::GetDirectory(zipURL, items, strMask, DIR_FLAG_DEFAULTS);
    if (items.Size() == 0) // no files
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0 && !items[0]->m_bIsFolder)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a apk dir
      pItem->SetURL(zipURL);
      return new CAPKDirectory;
    }
    return NULL;
  }
#endif
  if (url.IsFileType("zip"))
  {
    CURL zipURL = URIUtils::CreateArchivePath("zip", url);

    CFileItemList items;
    CDirectory::GetDirectory(zipURL, items, strMask, DIR_FLAG_DEFAULTS);
    if (items.Size() == 0) // no files
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0 && !items[0]->m_bIsFolder)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a zip dir
      pItem->SetURL(zipURL);
      return new CZipDirectory;
    }
    return NULL;
  }
  if (url.IsFileType("xbt"))
  {
    CURL xbtUrl = URIUtils::CreateArchivePath("xbt", url);
    pItem->SetURL(xbtUrl);

    return new CXbtDirectory();
  }
  if (url.IsFileType("xsp"))
  { // XBMC Smart playlist - just XML renamed to XSP
    // read the name of the playlist in
    CSmartPlaylist playlist;
    if (playlist.OpenAndReadName(url))
    {
      pItem->SetLabel(playlist.GetName());
      pItem->SetLabelPreformatted(true);
    }
    IFileDirectory* pDir=new CSmartPlaylistDirectory;
    return pDir; // treat as directory
  }
  if (CPlayListFactory::IsPlaylist(url))
  { // Playlist file
    // currently we only return the directory if it contains
    // more than one file.  Reason is that .pls and .m3u may be used
    // for links to http streams etc.
    IFileDirectory *pDir = new CPlaylistFileDirectory();
    CFileItemList items;
    if (pDir->GetDirectory(url, items))
    {
      if (items.Size() > 1)
        return pDir;
    }
    delete pDir;
    return NULL;
  }

  if (pItem->IsAudioBook())
  {
    if (!pItem->HasMusicInfoTag() || pItem->m_lEndOffset <= 0)
    {
      std::unique_ptr<CAudioBookFileDirectory> pDir(new CAudioBookFileDirectory);
      if (pDir->ContainsFiles(url))
        return pDir.release();
    }
    return NULL;
  }
  return NULL;
}

