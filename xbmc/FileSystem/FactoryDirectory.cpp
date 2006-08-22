
#include "../stdafx.h"
#include "factorydirectory.h"
#include "factoryfiledirectory.h"
#include "HDDirectory.h"
#include "ISO9660Directory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "SndtrkDirectory.h"
#include "DAAPDirectory.h"
#include "shoutcastdirectory.h"
#include "lastfmdirectory.h"
#include "zipdirectory.h"
#include "rardirectory.h"
#include "FTPDirectory.h"
#include "VirtualPathDirectory.h"
#include "MultiPathDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "StackDirectory.h"
#include "PlaylistDirectory.h"
#include "UPnPDirectory.h"
#include "MemUnitDirectory.h"
#include "../xbox/network.h"

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
  if (strProtocol == "iso9660") return new CISO9660Directory();
  if (strProtocol == "cdda") return new CCDDADirectory();
  if (strProtocol == "soundtrack") return new CSndtrkDirectory();
  if (strProtocol == "zip") return new CZipDirectory();
  if (strProtocol == "rar") return new CRarDirectory();
  if (strProtocol == "virtualpath") return new CVirtualPathDirectory();
  if (strProtocol == "multipath") return new CMultiPathDirectory();
  if (strProtocol == "stack") return new CStackDirectory();
  if (strProtocol == "musicdb") return new CMusicDatabaseDirectory();
  if (strProtocol == "playlistmusic") return new CPlaylistDirectory();
  if (strProtocol == "playlistvideo") return new CPlaylistDirectory();
  if (strProtocol.Left(3) == "mem") return new CMemUnitDirectory();

  if( g_network.IsAvailable() )
  {
    if (strProtocol == "smb") return new CSMBDirectory();
    if (strProtocol == "daap") return new CDAAPDirectory();
    if (strProtocol == "upnp") return new CUPnPDirectory();
    if (strProtocol == "shout") return new CShoutcastDirectory();
    if (strProtocol == "lastfm") return new CLastFMDirectory();
    if (strProtocol == "xbms") return new CXBMSDirectory();
    if (strProtocol == "ftp" || strProtocol == "ftpx") return new CFTPDirectory();
    if (strProtocol == "rtv") return new CRTVDirectory();
  }

 return NULL;
}
