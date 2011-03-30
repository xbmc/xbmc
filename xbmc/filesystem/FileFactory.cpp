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


#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "system.h"
#include "FileFactory.h"
#include "FileHD.h"
#include "FileCurl.h"
#include "FileShoutcast.h"
#include "FileLastFM.h"
#include "FileFileReader.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
#include "WINFileSmb.h"
#else
#include "FileSmb.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_CCX
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
#ifdef HAS_FILESYSTEM_DAAP
#include "FileDAAP.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_SAP
#include "SAPFile.h"
#endif
#ifdef HAS_FILESYSTEM_VTP
#include "VTPFile.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "PVRFile.h"
#endif
#include "FileZip.h"
#ifdef HAS_FILESYSTEM_RAR
#include "FileRar.h"
#endif
#ifdef HAS_FILESYSTEM_SFTP
#include "FileSFTP.h"
#endif
#include "FileMusicDatabase.h"
#include "FileSpecialProtocol.h"
#include "MultiPathFile.h"
#include "FileTuxBox.h"
#include "FileUDF.h"
#include "MythFile.h"
#include "HDHomeRun.h"
#include "Application.h"
#include "URL.h"
#include "utils/log.h"

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
#ifdef HAS_FILESYSTEM_RAR
  else if (strProtocol == "rar") return new CFileRar();
#endif
  else if (strProtocol == "musicdb") return new CFileMusicDatabase();
  else if (strProtocol == "videodb") return NULL;
  else if (strProtocol == "special") return new CFileSpecialProtocol();
  else if (strProtocol == "multipath") return new CMultiPathFile();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();
  else if (strProtocol == "filereader") return new CFileFileReader();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  else if (strProtocol == "cdda") return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CFileISO();
#endif
  else if(strProtocol == "udf") return new CFileUDF();

  if( g_application.getNetwork().IsAvailable() )
  {
    if (strProtocol == "http"
    ||  strProtocol == "https"
    ||  strProtocol == "dav"
    ||  strProtocol == "davs"
    ||  strProtocol == "ftp"
    ||  strProtocol == "ftpx"
    ||  strProtocol == "ftps"
    ||  strProtocol == "rss") return new CFileCurl();
#ifdef HAS_FILESYSTEM_SFTP
    else if (strProtocol == "sftp" || strProtocol == "ssh") return new CFileSFTP();
#endif
    else if (strProtocol == "shout") return new CFileShoutcast();
    else if (strProtocol == "lastfm") return new CFileLastFM();
    else if (strProtocol == "tuxbox") return new CFileTuxBox();
    else if (strProtocol == "hdhomerun") return new CFileHomeRun();
    else if (strProtocol == "myth") return new CMythFile();
    else if (strProtocol == "cmyth") return new CMythFile();
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
    else if (strProtocol == "smb") return new CWINFileSMB();
#else
    else if (strProtocol == "smb") return new CFileSMB();
#endif
#endif
#ifdef HAS_FILESYSTEM_CCX
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
#ifdef HAS_FILESYSTEM_SAP
    else if (strProtocol == "sap") return new CSAPFile();
#endif
#ifdef HAS_FILESYSTEM_VTP
    else if (strProtocol == "vtp") return new CVTPFile();
#endif
#ifdef HAS_PVRCLIENTS
    else if (strProtocol == "pvr") return new CPVRFile();
#endif
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, strProtocol.c_str(), url.Get().c_str() );
  return NULL;
}
