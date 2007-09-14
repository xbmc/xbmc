
#include "stdafx.h"
#include "factorydirectory.h"
#include "HDDirectory.h"
#include "VirtualPathDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "factoryfiledirectory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "shoutcastdirectory.h"
#include "lastfmdirectory.h"
#include "FTPDirectory.h"
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "SndtrkDirectory.h"
#include "DAAPDirectory.h"
#include "MemUnitDirectory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "../xbox/network.h"
#include "zipdirectory.h"
#include "rardirectory.h"
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
#ifdef HAS_FILESYSTEM
  if (strProtocol == "iso9660") return new CISO9660Directory();
  if (strProtocol == "cdda") return new CCDDADirectory();
  if (strProtocol == "soundtrack") return new CSndtrkDirectory();
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
#ifdef HAS_FILESYSTEM
    if (strProtocol == "smb") return new CSMBDirectory();
    if (strProtocol == "daap") return new CDAAPDirectory();
    if (strProtocol == "xbms") return new CXBMSDirectory();
    if (strProtocol == "rtv") return new CRTVDirectory();
#endif
#ifdef HAS_UPNP
    if (strProtocol == "upnp") return new CUPnPDirectory();
#endif
    if (strProtocol == "hdhomerun") return new CDirectoryHomeRun();
  }

 return NULL;
}
