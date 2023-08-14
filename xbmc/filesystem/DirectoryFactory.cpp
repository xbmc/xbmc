/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>
#include "network/Network.h"
#include "DirectoryFactory.h"
#include "SpecialProtocolDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FileDirectoryFactory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "FavouritesDirectory.h"
#include "LibraryDirectory.h"
#include "EventsDirectory.h"
#include "AddonsDirectory.h"
#include "SourcesDirectory.h"
#include "FTPDirectory.h"
#include "HTTPDirectory.h"
#include "DAVDirectory.h"
#if defined(HAS_UDFREAD)
#include "UDFDirectory.h"
#endif
#include "utils/log.h"
#include "network/WakeOnAccess.h"

#ifdef TARGET_POSIX
#include "platform/posix/filesystem/PosixDirectory.h"
#elif defined(TARGET_WINDOWS)
#include "platform/win32/filesystem/Win32Directory.h"
#ifdef TARGET_WINDOWS_STORE
#include "platform/win10/filesystem/WinLibraryDirectory.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
#include "platform/win32/filesystem/Win32SMBDirectory.h"
#else
#include "platform/posix/filesystem/SMBDirectory.h"
#endif
#endif
#include "CDDADirectory.h"
#include "PluginDirectory.h"
#if defined(HAS_ISO9660PP)
#include "ISO9660Directory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "PVRDirectory.h"
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/APKDirectory.h"
#elif defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/filesystem/TVOSDirectory.h"
#endif
#include "XbtDirectory.h"
#include "ZipDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "RSSDirectory.h"
#ifdef HAS_ZEROCONF
#include "ZeroconfDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_NFS
#include "NFSDirectory.h"
#endif
#ifdef HAVE_LIBBLURAY
#include "BlurayDirectory.h"
#endif
#ifdef HAS_OPTICAL_DRIVE
#include "DVDDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/AndroidAppDirectory.h"
#endif
#include "ResourceDirectory.h"
#include "ServiceBroker.h"
#include "addons/VFSEntry.h"
#include "utils/StringUtils.h"

using namespace ADDON;

using namespace XFILE;

/*!
 \brief Create a IDirectory object of the share type specified in a given item path.
 \param item Specifies the item to which the factory will create the directory instance
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CDirectoryFactory::Create(const CFileItem& item)
{
  return Create(CURL{item.GetDynPath()});
}

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CDirectoryFactory::Create(const CURL& url)
{
  if (!CWakeOnAccess::GetInstance().WakeUpHost(url))
    return NULL;

  CFileItem item(url.Get(), true);
  IFileDirectory* pDir = CFileDirectoryFactory::Create(url, &item);
  if (pDir)
    return pDir;

  if (!url.GetProtocol().empty() && CServiceBroker::IsAddonInterfaceUp())
  {
    for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    {
      auto prots = StringUtils::Split(vfsAddon->GetProtocols(), "|");

      if (vfsAddon->HasDirectories() && std::find(prots.begin(), prots.end(), url.GetProtocol()) != prots.end())
        return new CVFSEntryIDirectoryWrapper(vfsAddon);
    }
  }

#ifdef TARGET_POSIX
  if (url.GetProtocol().empty() || url.IsProtocol("file"))
  {
#if defined(TARGET_DARWIN_TVOS)
    if (CTVOSDirectory::WantsDirectory(url))
      return new CTVOSDirectory();
#endif
    return new CPosixDirectory();
  }
#elif defined(TARGET_WINDOWS)
  if (url.GetProtocol().empty() || url.IsProtocol("file")) return new CWin32Directory();
#else
#error Local directory access is not implemented for this platform
#endif
  if (url.IsProtocol("special")) return new CSpecialProtocolDirectory();
  if (url.IsProtocol("sources")) return new CSourcesDirectory();
  if (url.IsProtocol("addons")) return new CAddonsDirectory();
#if defined(HAS_OPTICAL_DRIVE)
  if (url.IsProtocol("cdda")) return new CCDDADirectory();
#endif
#if defined(HAS_ISO9660PP)
  if (url.IsProtocol("iso9660")) return new CISO9660Directory();
#endif
#if defined(HAS_UDFREAD)
  if (url.IsProtocol("udf")) return new CUDFDirectory();
#endif
  if (url.IsProtocol("plugin")) return new CPluginDirectory();
#if defined(TARGET_ANDROID)
  if (url.IsProtocol("apk")) return new CAPKDirectory();
#endif
  if (url.IsProtocol("zip")) return new CZipDirectory();
  if (url.IsProtocol("xbt")) return new CXbtDirectory();
  if (url.IsProtocol("multipath")) return new CMultiPathDirectory();
  if (url.IsProtocol("stack")) return new CStackDirectory();
  if (url.IsProtocol("playlistmusic")) return new CPlaylistDirectory();
  if (url.IsProtocol("playlistvideo")) return new CPlaylistDirectory();
  if (url.IsProtocol("musicdb")) return new CMusicDatabaseDirectory();
  if (url.IsProtocol("musicsearch")) return new CMusicSearchDirectory();
  if (url.IsProtocol("videodb")) return new CVideoDatabaseDirectory();
  if (url.IsProtocol("library")) return new CLibraryDirectory();
  if (url.IsProtocol("favourites")) return new CFavouritesDirectory();
#if defined(TARGET_ANDROID)
  if (url.IsProtocol("androidapp")) return new CAndroidAppDirectory();
#endif
#ifdef HAVE_LIBBLURAY
  if (url.IsProtocol("bluray")) return new CBlurayDirectory();
#endif
  if (url.IsProtocol("resource")) return new CResourceDirectory();
  if (url.IsProtocol("events")) return new CEventsDirectory();
#ifdef TARGET_WINDOWS_STORE
  if (CWinLibraryDirectory::IsValid(url)) return new CWinLibraryDirectory();
#endif

  if (url.IsProtocol("ftp") || url.IsProtocol("ftps")) return new CFTPDirectory();
  if (url.IsProtocol("http") || url.IsProtocol("https")) return new CHTTPDirectory();
  if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVDirectory();
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
  if (url.IsProtocol("smb")) return new CWin32SMBDirectory();
#else
  if (url.IsProtocol("smb")) return new CSMBDirectory();
#endif
#endif
#ifdef HAS_UPNP
  if (url.IsProtocol("upnp")) return new CUPnPDirectory();
#endif
  if (url.IsProtocol("rss") || url.IsProtocol("rsss")) return new CRSSDirectory();
#ifdef HAS_ZEROCONF
  if (url.IsProtocol("zeroconf")) return new CZeroconfDirectory();
#endif
#ifdef HAS_FILESYSTEM_NFS
  if (url.IsProtocol("nfs")) return new CNFSDirectory();
#endif
#ifdef HAS_OPTICAL_DRIVE
  if (url.IsProtocol("dvd"))
    return new CDVDDirectory();
#endif

  if (url.IsProtocol("pvr"))
    return new CPVRDirectory();

  CLog::Log(LOGWARNING, "{} - unsupported protocol({}) in {}", __FUNCTION__, url.GetProtocol(),
            url.GetRedacted());
  return NULL;
}

