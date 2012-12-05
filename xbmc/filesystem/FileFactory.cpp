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
#include "FileFactory.h"
#include "HDFile.h"
#include "CurlFile.h"
#include "HTTPFile.h"
#include "ShoutcastFile.h"
#include "LastFMFile.h"
#include "FileReaderFile.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
#include "windows/WINFileSmb.h"
#else
#include "SmbFile.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDAFile.h"
#endif
#ifdef HAS_FILESYSTEM
#include "ISOFile.h"
#ifdef HAS_FILESYSTEM_RTV
#include "RTVFile.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "DAAPFile.h"
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
#if defined(TARGET_ANDROID)
#include "APKFile.h"
#endif
#include "ZipFile.h"
#ifdef HAS_FILESYSTEM_RAR
#include "RarFile.h"
#endif
#ifdef HAS_FILESYSTEM_SFTP
#include "SFTPFile.h"
#endif
#ifdef HAS_FILESYSTEM_NFS
#include "NFSFile.h"
#endif
#ifdef HAS_FILESYSTEM_AFP
#include "AFPFile.h"
#endif
#if defined(TARGET_ANDROID)
#include "AndroidAppFile.h"
#endif
#ifdef HAS_UPNP
#include "UPnPFile.h"
#endif
#include "PipesManager.h"
#include "PipeFile.h"
#include "MusicDatabaseFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#include "TuxBoxFile.h"
#include "UDFFile.h"
#include "MythFile.h"
#include "HDHomeRunFile.h"
#include "SlingboxFile.h"
#include "ImageFile.h"
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

#if defined(TARGET_ANDROID)
  if (strProtocol == "apk") return new CAPKFile();
#endif
  if (strProtocol == "zip") return new CZipFile();
  else if (strProtocol == "rar")
  {
#ifdef HAS_FILESYSTEM_RAR
    return new CRarFile();
#else
    CLog::Log(LOGWARNING, "%s - Compiled without non-free, rar support is disabled", __FUNCTION__);
#endif
  }
  else if (strProtocol == "musicdb") return new CMusicDatabaseFile();
  else if (strProtocol == "videodb") return NULL;
  else if (strProtocol == "special") return new CSpecialProtocolFile();
  else if (strProtocol == "multipath") return new CMultiPathFile();
  else if (strProtocol == "image") return new CImageFile();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CHDFile();
  else if (strProtocol == "filereader") return new CFileReaderFile();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  else if (strProtocol == "cdda") return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CISOFile();
#endif
  else if(strProtocol == "udf") return new CUDFFile();

  if( g_application.getNetwork().IsAvailable() )
  {
    if (strProtocol == "dav"
    ||  strProtocol == "davs"
    ||  strProtocol == "ftp"
    ||  strProtocol == "ftps"
    ||  strProtocol == "rss") return new CCurlFile();
    else if (strProtocol == "http" ||  strProtocol == "https") return new CHTTPFile();
#ifdef HAS_FILESYSTEM_SFTP
    else if (strProtocol == "sftp" || strProtocol == "ssh") return new CSFTPFile();
#endif
    else if (strProtocol == "shout") return new CShoutcastFile();
    else if (strProtocol == "lastfm") return new CLastFMFile();
    else if (strProtocol == "tuxbox") return new CTuxBoxFile();
    else if (strProtocol == "hdhomerun") return new CHomeRunFile();
    else if (strProtocol == "sling") return new CSlingboxFile();
    else if (strProtocol == "myth") return new CMythFile();
    else if (strProtocol == "cmyth") return new CMythFile();
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
    else if (strProtocol == "smb") return new CWINFileSMB();
#else
    else if (strProtocol == "smb") return new CSmbFile();
#endif
#endif
#ifdef HAS_FILESYSTEM
#ifdef HAS_FILESYSTEM_RTV
    else if (strProtocol == "rtv") return new CRTVFile();
#endif
#ifdef HAS_FILESYSTEM_DAAP
    else if (strProtocol == "daap") return new CDAAPFile();
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
#ifdef HAS_FILESYSTEM_NFS
    else if (strProtocol == "nfs") return new CNFSFile();
#endif
#ifdef HAS_FILESYSTEM_AFP
    else if (strProtocol == "afp") return new CAFPFile();
#endif
    else if (strProtocol == "pipe") return new CPipeFile();    
#ifdef HAS_UPNP
    else if (strProtocol == "upnp") return new CUPnPFile();
#endif
#if defined(TARGET_ANDROID)
    else if (strProtocol == "androidapp") return new CFileAndroidApp();
#endif
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, strProtocol.c_str(), url.Get().c_str() );
  return NULL;
}
