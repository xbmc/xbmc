
#include "stdafx.h"
#include "FileFactory.h"
#include "FileHD.h"
#include "FileCurl.h"
#include "FileShoutcast.h"
#include "FileLastFM.h"
#include "FileFileReader.h"
#ifdef HAS_FILESYSTEM_SMB
#include "FileSmb.h"
#endif
#ifdef HAS_CCXSTREAM
#include "FileXBMSP.h"
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "FileCDDA.h"
#endif
#ifdef HAS_FILESYSTEM
#include "FileISO.h"
#ifdef HAS_FILESYSTEM_RTV
#include "FileRTV.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "FileSndtrk.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "FileDAAP.h"
#endif
#endif
#ifdef HAS_XBOX_HARDWARE
#include "FileMemUnit.h"
#endif
#ifdef HAS_MMS
#include "FileMMS.h"
#endif
#include "FileZip.h"
#include "FileRar.h"
#include "FileMusicDatabase.h"
#include "../utils/Network.h"
#include "FileTuxBox.h"
#include "HDHomeRun.h"
#include "Application.h"

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
#ifdef HAS_FILESYSTEM_CDDA
  else if (strProtocol == "cdda") return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CFileISO();
#ifndef _LINUX
  else if (strProtocol == "soundtrack") return new CFileSndtrk();
#endif
#endif
#ifdef HAS_XBOX_HARDWARE
  else if (strProtocol.Left(3) == "mem") return new CFileMemUnit();
#endif
  if( g_application.getNetwork().IsAvailable() )
  {
    if (strProtocol == "http" 
    ||  strProtocol == "https") return new CFileCurl();
    else if (strProtocol == "ftp" 
         ||  strProtocol == "ftpx"
         ||  strProtocol == "ftps") return new CFileCurl();
    else if (strProtocol == "upnp") return new CFileCurl();
#ifndef HAS_MMS
    else if (strProtocol == "mms") return new CFileCurl();
#else
    else if (strProtocol == "mms") return new CFileMMS();
#endif
    else if (strProtocol == "shout") return new CFileShoutcast();
    else if (strProtocol == "lastfm") return new CFileLastFM();
    else if (strProtocol == "tuxbox") return new CFileTuxBox();
    else if (strProtocol == "hdhomerun") return new CFileHomeRun();
#ifdef HAS_FILESYSTEM_SMB
    else if (strProtocol == "smb") return new CFileSMB();
#endif
#ifdef HAS_CCXSTREAM
    else if (strProtocol == "xbms") return new CFileXBMSP();
#endif
#ifdef HAS_FILESYSTEM
#ifdef HAS_FILESYSTEM_RTV
    else if (strProtocol == "rtv") return new CFileRTV();
#endif
#ifdef HAS_FILESYSTEM_DAAP
    else if (strProtocol == "daap") return new CFileDAAP();
#endif
#endif
  }

  return NULL;
}
