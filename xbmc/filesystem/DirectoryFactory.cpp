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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "system.h"
#include "DirectoryFactory.h"
#include "HDDirectory.h"
#include "SpecialProtocolDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FileDirectoryFactory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "LibraryDirectory.h"
#include "AddonsDirectory.h"
#include "SourcesDirectory.h"
#include "LastFMDirectory.h"
#include "FTPDirectory.h"
#include "HTTPDirectory.h"
#include "DAVDirectory.h"
#include "UDFDirectory.h"
#include "Application.h"
#include "addons/Addon.h"
#include "utils/log.h"

#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
#include "windows/WINSMBDirectory.h"
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
#ifdef HAS_FILESYSTEM_RTV
#include "RTVDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "DAAPDirectory.h"
#endif
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_SAP
#include "SAPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_VTP
#include "VTPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "HTSPDirectory.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "PVRDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKDirectory.h"
#endif
#include "ZipDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "RarDirectory.h"
#endif
#include "TuxBoxDirectory.h"
#include "HDHomeRunDirectory.h"
#include "SlingboxDirectory.h"
#include "MythDirectory.h"
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
#ifdef HAS_FILESYSTEM_AFP
#include "AFPDirectory.h"
#endif
#ifdef HAVE_LIBBLURAY
#include "BlurayDirectory.h"
#endif
#if defined(TARGET_ANDROID)
#include "AndroidAppDirectory.h"
#endif

using namespace XFILE;

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CDirectoryFactory::Create(const CStdString& strPath)
{
  CURL url(strPath);

  CFileItem item(strPath, false);
  IFileDirectory* pDir=CFileDirectoryFactory::Create(strPath, &item);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.size() == 0 || strProtocol == "file") return new CHDDirectory();
  if (strProtocol == "special") return new CSpecialProtocolDirectory();
  if (strProtocol == "sources") return new CSourcesDirectory();
  if (strProtocol == "addons") return new CAddonsDirectory();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  if (strProtocol == "cdda") return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (strProtocol == "iso9660") return new CISO9660Directory();
#endif
  if (strProtocol == "udf") return new CUDFDirectory();
  if (strProtocol == "plugin") return new CPluginDirectory();
#if defined(TARGET_ANDROID)
  if (strProtocol == "apk") return new CAPKDirectory();
#endif
  if (strProtocol == "zip") return new CZipDirectory();
  if (strProtocol == "rar") 
  {
#ifdef HAS_FILESYSTEM_RAR
    return new CRarDirectory();
#else
    CLog::Log(LOGWARNING, "%s - Compiled without non-free, rar support is disabled", __FUNCTION__);
#endif
  }
  if (strProtocol == "multipath") return new CMultiPathDirectory();
  if (strProtocol == "stack") return new CStackDirectory();
  if (strProtocol == "playlistmusic") return new CPlaylistDirectory();
  if (strProtocol == "playlistvideo") return new CPlaylistDirectory();
  if (strProtocol == "musicdb") return new CMusicDatabaseDirectory();
  if (strProtocol == "musicsearch") return new CMusicSearchDirectory();
  if (strProtocol == "videodb") return new CVideoDatabaseDirectory();
  if (strProtocol == "library") return new CLibraryDirectory();
  if (strProtocol == "filereader")
    return CDirectoryFactory::Create(url.GetFileName());

  if( g_application.getNetwork().IsAvailable(true) )  // true to wait for the network (if possible)
  {
    if (strProtocol == "lastfm") return new CLastFMDirectory();
    if (strProtocol == "tuxbox") return new CTuxBoxDirectory();
    if (strProtocol == "ftp" || strProtocol == "ftps") return new CFTPDirectory();
    if (strProtocol == "http" || strProtocol == "https") return new CHTTPDirectory();
    if (strProtocol == "dav" || strProtocol == "davs") return new CDAVDirectory();
#ifdef HAS_FILESYSTEM_SFTP
    if (strProtocol == "sftp" || strProtocol == "ssh") return new CSFTPDirectory();
#endif
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
    if (strProtocol == "smb") return new CWINSMBDirectory();
#else
    if (strProtocol == "smb") return new CSMBDirectory();
#endif
#endif
#ifdef HAS_FILESYSTEM
#ifdef HAS_FILESYSTEM_DAAP
    if (strProtocol == "daap") return new CDAAPDirectory();
#endif
#ifdef HAS_FILESYSTEM_RTV
    if (strProtocol == "rtv") return new CRTVDirectory();
#endif
#endif
#ifdef HAS_UPNP
    if (strProtocol == "upnp") return new CUPnPDirectory();
#endif
    if (strProtocol == "hdhomerun") return new CHomeRunDirectory();
    if (strProtocol == "sling") return new CSlingboxDirectory();
    if (strProtocol == "myth") return new CMythDirectory();
    if (strProtocol == "cmyth") return new CMythDirectory();
    if (strProtocol == "rss") return new CRSSDirectory();
#ifdef HAS_FILESYSTEM_SAP
    if (strProtocol == "sap") return new CSAPDirectory();
#endif
#ifdef HAS_FILESYSTEM_VTP
    if (strProtocol == "vtp") return new CVTPDirectory();
#endif
#ifdef HAS_FILESYSTEM_HTSP
    if (strProtocol == "htsp") return new CHTSPDirectory();
#endif
#ifdef HAS_PVRCLIENTS
    if (strProtocol == "pvr") return new CPVRDirectory();
#endif
#ifdef HAS_ZEROCONF
    if (strProtocol == "zeroconf") return new CZeroconfDirectory();
#endif
#ifdef HAS_FILESYSTEM_NFS
    if (strProtocol == "nfs") return new CNFSDirectory();
#endif
#ifdef HAS_FILESYSTEM_AFP
      if (strProtocol == "afp") return new CAFPDirectory();
#endif
#ifdef HAVE_LIBBLURAY
      if (strProtocol == "bluray") return new CBlurayDirectory();
#endif
#if defined(TARGET_ANDROID)
      if (strProtocol == "androidapp") return new CAndroidAppDirectory();
#endif
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, strProtocol.c_str(), url.Get().c_str() );
  return NULL;
}

