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
#include "Util.h"
#include "utils/URIUtils.h"
#include "FileDirectoryFactory.h"
#ifdef HAS_FILESYSTEM
#include "OGGFileDirectory.h"
#include "NSFFileDirectory.h"
#include "SIDFileDirectory.h"
#include "ASAPFileDirectory.h"
#include "UDFDirectory.h"
#include "RSSDirectory.h"
#include "cores/paplayer/ASAPCodec.h"
#endif
#ifdef HAS_FILESYSTEM_RAR
#include "RarDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKDirectory.h"
#endif
#include "ZipDirectory.h"
#include "SmartPlaylistDirectory.h"
#include "playlists/SmartPlayList.h"
#include "PlaylistFileDirectory.h"
#include "playlists/PlayListFactory.h"
#include "Directory.h"
#include "File.h"
#include "ZipManager.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace XFILE;
using namespace PLAYLIST;
using namespace std;

CFileDirectoryFactory::CFileDirectoryFactory(void)
{}

CFileDirectoryFactory::~CFileDirectoryFactory(void)
{}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFileDirectoryFactory::Create(const CURL& url, CFileItem* pItem, const std::string& strMask)
{
  if (url.IsProtocol("stack")) // disqualify stack as we need to work with each of the parts instead
    return NULL;

#ifdef HAS_FILESYSTEM
  if ((url.IsFileType("ogg") || url.IsFileType("oga")) && CFile::Exists(url))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more than one bitstream?
    if (pDir->ContainsFiles(url))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  if (url.IsFileType("nsf") && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CNSFFileDirectory;
    //  Has the nsf file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (url.IsFileType("sid") && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CSIDFileDirectory;
    //  Has the sid file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
#ifdef HAS_ASAP_CODEC
  if (ASAPCodec::IsSupportedFormat(url.GetFileType()) && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CASAPFileDirectory;
    //  Has the asap file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
#endif

  if (pItem->IsRSS())
    return new CRSSDirectory();

  if (pItem->IsDiscImage())
    return new CUDFDirectory();

#endif
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
  if (url.IsFileType("rar") || url.IsFileType("001"))
  {
    vector<std::string> tokens;
    const std::string strPath = url.Get();
    StringUtils::Tokenize(strPath,tokens,".");
    if (tokens.size() > 2)
    {
      if (url.IsFileType("001"))
      {
        if (StringUtils::EqualsNoCase(tokens[tokens.size()-2], "ts")) // .ts.001 - treat as a movie file to scratch some users itch
          return NULL;
      }
      std::string token = tokens[tokens.size()-2];
      if (StringUtils::StartsWith(token, "part")) // only list '.part01.rar'
      {
        // need this crap to avoid making mistakes - yeyh for the new rar naming scheme :/
        struct __stat64 stat;
        int digits = token.size()-4;
        std::string strFormat = StringUtils::Format("part%%0%ii", digits);
        std::string strNumber = StringUtils::Format(strFormat.c_str(), 1);
        std::string strPath2 = strPath;
        StringUtils::Replace(strPath2,token,strNumber);
        if (atoi(token.substr(4).c_str()) > 1 && CFile::Stat(strPath2,&stat) == 0)
        {
          pItem->m_bIsFolder = true;
          return NULL;
        }
      }
    }

    CURL rarURL = URIUtils::CreateArchivePath("rar", url);

    CFileItemList items;
    CDirectory::GetDirectory(rarURL, items, strMask);
    if (items.Size() == 0) // no files - hide this
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0x30 && !items[0]->m_bIsFolder)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    {
#ifdef HAS_FILESYSTEM_RAR
      // compressed or more than one file -> create a rar dir
      pItem->SetURL(rarURL);
      return new CRarDirectory;
#else
      return NULL;
#endif
    }
    return NULL;
  }
  if (url.IsFileType("xsp"))
  { // XBMC Smart playlist - just XML renamed to XSP
    // read the name of the playlist in
    CSmartPlaylist playlist;
    if (playlist.OpenAndReadName(url))
    {
      pItem->SetLabel(playlist.GetName());
      pItem->SetLabelPreformated(true);
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
  return NULL;
}

