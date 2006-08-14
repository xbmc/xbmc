
#include "../stdafx.h"
#include "FileFactory.h"
#include "FileShoutcast.h"
#include "FileISO.h"
#include "FileHD.h"
#include "FileSMB.h"
#include "FileXBMSP.h"
#include "FileRTV.h"
#include "FileSndtrk.h"
#include "FileDAAP.h"
#include "FileCDDA.h"
#include "FileZip.h"
#include "FileRar.h"
#include "FileCurl.h"
#include "FileMusicDatabase.h"
#include "FileLastFM.h"
#include "../xbox/network.h"

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

  if (strProtocol == "iso9660") return new CFileISO();
  else if (strProtocol == "soundtrack") return new CFileSndtrk();
  else if (strProtocol == "cdda") return new CFileCDDA();
  else if (strProtocol == "zip") return new CFileZip();
  else if (strProtocol == "rar") return new CFileRar();
  else if (strProtocol == "musicdb") return new CFileMusicDatabase();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();

  if( g_network.IsAvailable() )
  {
    if (strProtocol == "smb") return new CFileSMB();
    else if (strProtocol == "xbms") return new CFileXBMSP();
    else if (strProtocol == "shout") return new CFileShoutcast();
    else if (strProtocol == "lastfm") return new CFileLastFM();
    else if (strProtocol == "daap") return new CFileDAAP();
    else if (strProtocol == "http" || strProtocol == "https") return new CFileCurl();
    else if (strProtocol == "ftp" || strProtocol == "ftpx") return new CFileCurl();
    else if (strProtocol == "upnp") return new CFileCurl();
    else if (strProtocol == "rtv") return new CFileRTV();
  }

  return NULL;
}
