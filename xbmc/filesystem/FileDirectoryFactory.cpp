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


#include "system.h"
#include "utils/URIUtils.h"
#include "FileDirectoryFactory.h"
#ifdef HAS_FILESYSTEM
#include "UDFDirectory.h"
#include "RSSDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKDirectory.h"
#endif
#include "XbtDirectory.h"
#include "ZipDirectory.h"
#include "SmartPlaylistDirectory.h"
#include "playlists/SmartPlayList.h"
#include "PlaylistFileDirectory.h"
#include "playlists/PlayListFactory.h"
#include "Directory.h"
#include "File.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "ServiceBroker.h"
#include "addons/AudioDecoder.h"
#include "addons/VFSEntry.h"
#include "addons/BinaryAddonCache.h"
#include "AudioBookFileDirectory.h"

using namespace ADDON;
using namespace XFILE;
using namespace PLAYLIST;

CFileDirectoryFactory::CFileDirectoryFactory(void)
{}

CFileDirectoryFactory::~CFileDirectoryFactory(void)
{}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFileDirectoryFactory::Create(const CURL& url, CFileItem* pItem, const std::string& strMask)
{
  if (url.IsProtocol("stack")) // disqualify stack as we need to work with each of the parts instead
    return NULL;

  std::string strExtension=URIUtils::GetExtension(url);
  StringUtils::ToLower(strExtension);
  if (!strExtension.empty())
  {
    VECADDONS codecs;
    CBinaryAddonCache &addonCache = CServiceBroker::GetBinaryAddonCache();
    addonCache.GetAddons(codecs, ADDON_AUDIODECODER);
    for (size_t i=0;i<codecs.size();++i)
    {
      std::shared_ptr<CAudioDecoder> dec(std::static_pointer_cast<CAudioDecoder>(codecs[i]));
      if (dec->HasTracks() && dec->GetExtensions().find(strExtension) != std::string::npos)
      {
        CAudioDecoder* result = new CAudioDecoder(*dec);
        result->Create();
        if (result->ContainsFiles(url))
          return result;
        delete result;
        return NULL;
      }
    }
  }

  if (CServiceBroker::IsBinaryAddonCacheUp())
  {
    VECADDONS vfs;
    CBinaryAddonCache &addonCache = CServiceBroker::GetBinaryAddonCache();
    addonCache.GetAddons(vfs, ADDON_VFS);
    for (size_t i=0;i<vfs.size();++i)
    {
      std::shared_ptr<CVFSEntry> dec(std::static_pointer_cast<CVFSEntry>(vfs[i]));
      if (!strExtension.empty() && dec->HasFileDirectories() &&
          dec->GetExtensions().find(strExtension) != std::string::npos)
      {
        CVFSEntryIFileDirectoryWrapper* wrap = new CVFSEntryIFileDirectoryWrapper(dec);
        if (wrap->ContainsFiles(url))
        {
          if (wrap->m_items.Size() == 1)
          {
            // one STORED file - collapse it down
            *pItem = *wrap->m_items[0];
          }
          else
          { // compressed or more than one file -> create a dir
            pItem->SetPath(wrap->m_items.GetPath());
            return wrap;
          }
        }
        else
          pItem->m_bIsFolder = true;

        delete wrap;
        return NULL;
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
    CDirectory::GetDirectory(zipURL, items, strMask);
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
    CDirectory::GetDirectory(zipURL, items, strMask);
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

