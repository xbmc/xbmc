
#include "stdafx.h"
#include "FileFactory.h"
#include "FileHD.h"
#include "FileCurl.h"
#include "FileShoutcast.h"
#include "FileLastFM.h"
#include "FileFileReader.h"
#ifdef HAS_FILESYSTEM
#include "FileISO.h"
#include "FileSMB.h"
#include "FileXBMSP.h"
#include "FileRTV.h"
#include "FileSndtrk.h"
#include "FileCDDA.h"
#include "FileMemUnit.h"
#include "FileDAAP.h"
#endif
#include "FileZip.h"
#include "FileRar.h"
#include "FileMusicDatabase.h"
#include "../xbox/network.h"
#include "FileTuxBox.h"
#include "HDHomeRun.h"

using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const CStdString& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  CStdString strProtocol = url.GetProtocol();
  strProtocol.MakeLower();

  if (strProtocol == "zip") return new CFileZip();
  else if (strProtocol == "rar") return new CFileRar();
  else if (strProtocol == "musicdb") return new CFileMusicDatabase();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();
  else if (strProtocol == "filereader") return new CFileFileReader();
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CFileISO();
  else if (strProtocol == "soundtrack") return new CFileSndtrk();
  else if (strProtocol == "cdda") return new CFileCDDA();
  else if (strProtocol.Left(3) == "mem") return new CFileMemUnit();
#endif
  if( g_network.IsAvailable() )
  {
    if (strProtocol == "http" 
    ||  strProtocol == "https") return new CFileCurl();
    else if (strProtocol == "ftp" 
         ||  strProtocol == "ftpx"
         ||  strProtocol == "ftps") return new CFileCurl();
    else if (strProtocol == "upnp") return new CFileCurl();
    else if (strProtocol == "mms") return new CFileCurl();
    else if (strProtocol == "shout") return new CFileShoutcast();
    else if (strProtocol == "lastfm") return new CFileLastFM();
    else if (strProtocol == "tuxbox") return new CFileTuxBox();
    else if (strProtocol == "hdhomerun") return new CFileHomeRun();
#ifdef HAS_FILESYSTEM
    else if (strProtocol == "smb") return new CFileSMB();
    else if (strProtocol == "xbms") return new CFileXBMSP();
    else if (strProtocol == "rtv") return new CFileRTV();
    else if (strProtocol == "daap") return new CFileDAAP();
#endif
  }

  return NULL;
}
