/*
 *      Copyright (C) 2005-2013 Team XBMC
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


#include "system.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "FileDirectoryFactory.h"
#ifdef HAS_FILESYSTEM
#include "OGGFileDirectory.h"
#include "NSFFileDirectory.h"
#include "SIDFileDirectory.h"
#include "ASAPFileDirectory.h"
#include "RSSDirectory.h"
#include "cores/paplayer/ASAPCodec.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKDirectory.h"
#endif
#include "ArchiveDirectory.h"
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
#include "utils/RegExp.h"

using namespace XFILE;
using namespace PLAYLIST;
using namespace std;

CFileDirectoryFactory::CFileDirectoryFactory(void)
{}

CFileDirectoryFactory::~CFileDirectoryFactory(void)
{}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFileDirectoryFactory::Create(const CStdString& strPath, CFileItem* pItem, const CStdString& strMask)
{
  CStdString strExtension=URIUtils::GetExtension(strPath);
  strExtension.MakeLower();

#ifdef HAS_FILESYSTEM
  if ((strExtension.Equals(".ogg") || strExtension.Equals(".oga")) && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more than one bitstream?
    if (pDir->ContainsFiles(strPath))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".nsf") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CNSFFileDirectory;
    //  Has the nsf file more than one track?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".sid") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CSIDFileDirectory;
    //  Has the sid file more than one track?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
#ifdef HAS_ASAP_CODEC
  if (ASAPCodec::IsSupportedFormat(strExtension) && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CASAPFileDirectory;
    //  Has the asap file more than one track?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
#endif

  if (pItem->IsRSS())
    return new CRSSDirectory();

#endif
#if defined(TARGET_ANDROID)
  if (strExtension.Equals(".apk"))
  {
    CStdString strUrl;
    URIUtils::CreateArchivePath(strUrl, "apk", strPath, "");

    CFileItemList items;
    CDirectory::GetDirectory(strUrl, items, strMask);
    if (items.Size() == 0) // no files
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a apk dir
      pItem->SetPath(strUrl);
      return new CAPKDirectory;
    }
    return NULL;
  }
#endif
  if (strExtension.Equals(".zip"))
  {
    CStdString strUrl;
    URIUtils::CreateArchivePath(strUrl, "zip", strPath, "");

    CFileItemList items;
    CDirectory::GetDirectory(strUrl, items, strMask);
    if (items.Size() == 0) // no files
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a zip dir
      pItem->SetPath(strUrl);
      return new CZipDirectory;
    }
    return NULL;
  }
  if (strExtension.Equals(".rar") || strExtension.Equals(".001"))
  {
#ifdef HAVE_LIBARCHIVE
    CFileItemList itemlist;
    CRegExp regex(true);
    std::vector<CStdString> filePaths;
    CStdString strUrl, pattern, strPathNoExt;
    URIUtils::CreateArchivePath(strUrl, "rar", strPath, "");

    /*
     * Get any multivolume RAR files that may be associated with this archive
     */
    if (strExtension.Equals(".rar"))
    {
      /*
       * In this case, multivolume RAR are those with the
       * glob pattern .part*.rar
       */
      pattern = "^(.*)(\\.part\\d+\\.rar)$";
      if (!regex.RegComp(pattern))
        return NULL;
      if (regex.RegFind(strPath) >= 0)
      {
        if (!CDirectory::GetDirectory(URIUtils::GetDirectory(strPath), itemlist,
          strExtension, DIR_FLAG_NO_FILE_DIRS))
          return NULL;

        strPathNoExt = regex.GetMatch(1);

        /* Remove files that may not be part of the multivolume RAR */
        pattern = "^\\Q" + strPathNoExt + "\\E\\.part\\d+\\.rar$";
        if (!regex.RegComp(pattern))
          return NULL;
        for (int i = 0; i < itemlist.Size(); i++)
        {
          if (regex.RegFind(itemlist[i]->GetPath()) < 0)
          {
            itemlist.Remove(i);
          }
        }
      }
    }
    else
    {
      /* Treat these *.ts.001 files as movie files */
      pattern = ".*\\.ts\\.001$";
      if (!regex.RegComp(pattern) || regex.RegFind(strPath) >= 0)
        return NULL;

      if (!CDirectory::GetDirectory(URIUtils::GetDirectory(strPath), itemlist,
        "", DIR_FLAG_NO_FILE_DIRS))
        return NULL;

      /* Get a sub string of the file path excluding the .<number> part */
      pattern = "^(.*)(\\.\\d{3})$";
      if (!regex.RegComp(pattern) || regex.RegFind(strPath) < 0)
        return NULL;
      strPathNoExt = regex.GetMatch(1);

      /* Remove files that may not be part of the multivolume RAR */
      pattern = "^\\Q" + strPathNoExt + "\\E\\.\\d{3}$";
      if (!regex.RegComp(pattern))
        return NULL;
      for (int i = 0; i < itemlist.Size(); i++)
      {
        if (regex.RegFind(itemlist[i]->GetPath()) < 0)
        {
          itemlist.Remove(i);
        }
      }
    }
    if (itemlist.IsEmpty())
    {
      /* Not a multivolume archive */
      filePaths.push_back(strPath);
    }
    else
    {
      /*
      * Sort the list, add the entries to a vector, and return
      * a CArchiveDirectory object with this vector.
      */
      itemlist.Sort(SORT_METHOD_FULLPATH, SortOrderAscending);
      for (int i = 0; i < itemlist.Size(); i++)
      {
        filePaths.push_back(itemlist[i]->GetPath());
      }
    }
    pItem->SetPath(strUrl);
    return new CArchiveDirectory(ARCHIVE_FORMAT_RAR, ARCHIVE_FILTER_NONE,
      filePaths);
#else
    return NULL;
#endif
  }
  if (strExtension.Equals(".xsp"))
  { // XBMC Smart playlist - just XML renamed to XSP
    // read the name of the playlist in
    CSmartPlaylist playlist;
    if (playlist.OpenAndReadName(strPath))
    {
      pItem->SetLabel(playlist.GetName());
      pItem->SetLabelPreformated(true);
    }
    IFileDirectory* pDir=new CSmartPlaylistDirectory;
    return pDir; // treat as directory
  }
  if (g_advancedSettings.m_playlistAsFolders && CPlayListFactory::IsPlaylist(strPath))
  { // Playlist file
    // currently we only return the directory if it contains
    // more than one file.  Reason is that .pls and .m3u may be used
    // for links to http streams etc.
    IFileDirectory *pDir = new CPlaylistFileDirectory();
    CFileItemList items;
    if (pDir->GetDirectory(strPath, items))
    {
      if (items.Size() > 1)
        return pDir;
    }
    delete pDir;
    return NULL;
  }
  return NULL;
}

