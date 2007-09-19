
#include "stdafx.h"
#include "FactoryDirectory.h"
#include "HDDirectory.h"
#include "VirtualPathDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FactoryFileDirectory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "ShoutcastDirectory.h"
#include "LastFMDirectory.h"
#include "FTPDirectory.h"
#ifdef HAS_FILESYSTEM_SMB
#include "SMBDirectory.h"
#endif
#ifdef HAS_CCXSTREAM
#include "XBMSDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDADirectory.h"
#endif
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#include "XBMSDirectory.h"
#ifdef HAS_FILESYSTEM_RTV
#include "RTVDirectory.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "SndtrkDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "DAAPDirectory.h"
#endif
#endif
#ifdef HAS_XBOX_HARDWARE
#include "MemUnitDirectory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "../utils/Network.h"
#include "ZipDirectory.h"
#include "RarDirectory.h"
#include "DirectoryTuxBox.h"
#include "HDHomeRun.h"

using namespace DIRECTORY;

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
  CURL url(strPath);

  CFileItem item;
  IFileDirectory* pDir=CFactoryFileDirectory::Create(strPath, &item);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.size() == 0 || strProtocol == "file") return new CHDDirectory();
#ifdef HAS_FILESYSTEM_CDDA
  if (strProtocol == "cdda") return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (strProtocol == "iso9660") return new CISO9660Directory();
#ifdef HAS_XBOX_HARDWARE
  if (strProtocol == "soundtrack") return new CSndtrkDirectory();
#endif
#endif
  if (strProtocol == "plugin") return new CPluginDirectory();
  if (strProtocol == "zip") return new CZipDirectory();
  if (strProtocol == "rar") return new CRarDirectory();
  if (strProtocol == "virtualpath") return new CVirtualPathDirectory();
  if (strProtocol == "multipath") return new CMultiPathDirectory();
  if (strProtocol == "stack") return new CStackDirectory();
  if (strProtocol == "playlistmusic") return new CPlaylistDirectory();
  if (strProtocol == "playlistvideo") return new CPlaylistDirectory();
  if (strProtocol == "musicdb") return new CMusicDatabaseDirectory();
  if (strProtocol == "musicsearch") return new CMusicSearchDirectory();
  if (strProtocol == "videodb") return new CVideoDatabaseDirectory();
  if (strProtocol == "filereader") 
    return CFactoryDirectory::Create(url.GetFileName());
#ifdef HAS_XBOX_HARDWARE
  if (strProtocol.Left(3) == "mem") return new CMemUnitDirectory();
#endif

  if( g_network.IsAvailable() )
  {
    if (strProtocol == "shout") return new CShoutcastDirectory();
    if (strProtocol == "lastfm") return new CLastFMDirectory();
    if (strProtocol == "tuxbox") return new CDirectoryTuxBox();
    if (strProtocol == "ftp" 
    ||  strProtocol == "ftpx"
    ||  strProtocol == "ftps") return new CFTPDirectory();
#ifdef HAS_FILESYSTEM_SMB
    if (strProtocol == "smb") return new CSMBDirectory();
#endif
#ifdef HAS_CCXSTREAM
    if (strProtocol == "xbms") return new CXBMSDirectory();
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
    if (strProtocol == "hdhomerun") return new CDirectoryHomeRun();
  }

 return NULL;
}

