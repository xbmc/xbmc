/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "stdafx.h"
#include "Util.h"
#include "FactoryFileDirectory.h"
#ifdef HAS_FILESYSTEM
#include "OGGFileDirectory.h"
#include "NSFFileDirectory.h"
#include "SIDFileDirectory.h"
#include "ASAPFileDirectory.h"
#include "cores/paplayer/ASAPCodec.h"
#endif
#include "RarDirectory.h"
#include "ZipDirectory.h"
#include "SmartPlaylistDirectory.h"
#include "SmartPlaylist.h"
#include "PlaylistFileDirectory.h"
#include "PlayListFactory.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/ZipManager.h"
#include "Settings.h"
#include "FileItem.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace std;

CFactoryFileDirectory::CFactoryFileDirectory(void)
{}

CFactoryFileDirectory::~CFactoryFileDirectory(void)
{}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath, CFileItem* pItem, const CStdString& strMask)
{
  CStdString strExtension=CUtil::GetExtension(strPath);
  if (strExtension.size() == 0) return NULL;
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
  if (strExtension.Equals(".zip"))
  {
    CStdString strUrl;
    CUtil::CreateArchivePath(strUrl, "zip", strPath, "");

    if (!g_guiSettings.GetBool("filelists.unrollarchives") || g_ZipManager.HasMultipleEntries(strPath))
    {
      pItem->m_strPath = strUrl;
      return new CZipDirectory;
    }

    CFileItemList item;
    CDirectory dir; dir.GetDirectory(strUrl,item,strMask);
    if (item.Size())
      *pItem = *item[0];
    else
      pItem->m_bIsFolder = true;

    return NULL;
  }
  if (strExtension.Equals(".rar") || strExtension.Equals(".001"))
  {
    CStdString strUrl;
    CUtil::CreateArchivePath(strUrl, "rar", strPath, "");

    vector<CStdString> tokens;
    CUtil::Tokenize(strPath,tokens,".");
    if (tokens.size() > 2)
    {
      if (strExtension.Equals(".001"))
      {
        if (tokens[tokens.size()-2].Equals("ts")) // .ts.001 - treat as a movie file to scratch some users itch
          return NULL;
      }
      CStdString token = tokens[tokens.size()-2];
      if (token.Left(4).CompareNoCase("part") == 0) // only list '.part01.rar'
      {
        // need this crap to avoid making mistakes - yeyh for the new rar naming scheme :/
        struct __stat64 stat;
        int digits = token.size()-4;
        CStdString strNumber, strFormat;
        strFormat.Format("part%%0%ii",digits);
        strNumber.Format(strFormat.c_str(),1);
        CStdString strPath2=strPath;
        strPath2.Replace(token,strNumber);
        if (atoi(token.substr(4).c_str()) > 1 && CFile::Stat(strPath2,&stat) == 0)
        {
          pItem->m_bIsFolder = true;
          return NULL;
        }
      }
    }

    if (!g_guiSettings.GetBool("filelists.unrollarchives") || g_RarManager.HasMultipleEntries(strPath))
    {
      pItem->m_strPath = strUrl;
      return new CRarDirectory;
    }

    CFileItemList item;
    CDirectory dir; dir.GetDirectory(strUrl,item,strMask);
    if (item.Size())
      *pItem = *item[0];
    else
      pItem->m_bIsFolder = true;

    return NULL;
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

