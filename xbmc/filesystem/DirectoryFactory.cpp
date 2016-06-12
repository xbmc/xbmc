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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include <stdlib.h>
#include "network/Network.h"
#include "system.h"
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
#include "UDFDirectory.h"
#include "Application.h"
#include "utils/log.h"
#include "network/WakeOnAccess.h"

#ifdef TARGET_POSIX
#include "posix/PosixDirectory.h"
#elif defined(TARGET_WINDOWS)
#include "win32/Win32Directory.h"
#endif
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
#include "win32/Win32SMBDirectory.h"
#else
#include "SMBDirectory.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDADirectory.h"
#endif
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "PVRDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKDirectory.h"
#endif
#include "XbtDirectory.h"
#include "ZipDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "RarDirectory.h"
#endif
#include "FileItem.h"
#include "URL.h"
#include "RSSDirectory.h"
#ifdef HAS_ZEROCONF
#include "ZeroconfDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_SFTP
#include "SFTPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_NFS
#include "NFSDirectory.h"
#endif
#ifdef HAVE_LIBBLURAY
#include "BlurayDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "AndroidAppDirectory.h"
#endif
#include "ResourceDirectory.h"

using namespace XFILE;

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
  IFileDirectory* pDir=CFileDirectoryFactory::Create(url, &item);
  if (pDir)
    return pDir;

#ifdef TARGET_POSIX
  if (url.GetProtocol().empty() || url.IsProtocol("file")) return new CPosixDirectory();
#elif defined(TARGET_WINDOWS)
  if (url.GetProtocol().empty() || url.IsProtocol("file")) return new CWin32Directory();
#else
#error Local directory access is not implemented for this platform
#endif
  if (url.IsProtocol("special")) return new CSpecialProtocolDirectory();
  if (url.IsProtocol("sources")) return new CSourcesDirectory();
  if (url.IsProtocol("addons")) return new CAddonsDirectory();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  if (url.IsProtocol("cdda")) return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (url.IsProtocol("iso9660")) return new CISO9660Directory();
#endif
  if (url.IsProtocol("udf")) return new CUDFDirectory();
  if (url.IsProtocol("plugin")) return new CPluginDirectory();
#if defined(TARGET_ANDROID)
  if (url.IsProtocol("apk")) return new CAPKDirectory();
#endif
  if (url.IsProtocol("zip")) return new CZipDirectory();
  if (url.IsProtocol("rar"))
  {
#ifdef HAS_FILESYSTEM_RAR
    return new CRarDirectory();
#else
    CLog::Log(LOGWARNING, "%s - Compiled without non-free, rar support is disabled", __FUNCTION__);
#endif
  }
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

  bool networkAvailable = g_application.getNetwork().IsAvailable();
  if (networkAvailable)
  {
    if (url.IsProtocol("ftp") || url.IsProtocol("ftps")) return new CFTPDirectory();
    if (url.IsProtocol("http") || url.IsProtocol("https")) return new CHTTPDirectory();
    if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVDirectory();
#ifdef HAS_FILESYSTEM_SFTP
    if (url.IsProtocol("sftp") || url.IsProtocol("ssh")) return new CSFTPDirectory();
#endif
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
    if (url.IsProtocol("smb")) return new CWin32SMBDirectory();
#else
    if (url.IsProtocol("smb")) return new CSMBDirectory();
#endif
#endif
#ifdef HAS_FILESYSTEM
#endif
#ifdef HAS_UPNP
    if (url.IsProtocol("upnp")) return new CUPnPDirectory();
#endif
    if (url.IsProtocol("rss")) return new CRSSDirectory();
#ifdef HAS_PVRCLIENTS
    if (url.IsProtocol("pvr")) return new CPVRDirectory();
#endif
#ifdef HAS_ZEROCONF
    if (url.IsProtocol("zeroconf")) return new CZeroconfDirectory();
#endif
#ifdef HAS_FILESYSTEM_NFS
    if (url.IsProtocol("nfs")) return new CNFSDirectory();
#endif
  }

  CLog::Log(LOGWARNING, "%s - %sunsupported protocol(%s) in %s", __FUNCTION__, networkAvailable ? "" : "Network down or ", url.GetProtocol().c_str(), url.GetRedacted().c_str() );
  return NULL;
}

