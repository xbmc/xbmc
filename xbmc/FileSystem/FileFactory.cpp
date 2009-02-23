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


#include "stdafx.h"
#include "FileFactory.h"
#include "FileHD.h"
#include "FileCurl.h"
#include "FileShoutcast.h"
#include "FileLastFM.h"
#include "FileFileReader.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32PC
#include "WINFileSmb.h"
#else
#include "FileSmb.h"
#endif
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
#include "FileMMS.h"
#include "FileZip.h"
#include "FileRar.h"
#include "FileMusicDatabase.h"
#include "FileSpecialProtocol.h"
#include "MultiPathFile.h"
#include "../utils/Network.h"
#include "FileTuxBox.h"
#include "HDHomeRun.h"
#include "CMythFile.h"
#include "Application.h"
#include "URL.h"

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
  else if (strProtocol == "special") return new CFileSpecialProtocol();
  else if (strProtocol == "multipath") return new CMultiPathFile();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();
  else if (strProtocol == "filereader") return new CFileFileReader();
#ifdef HAS_FILESYSTEM_CDDA
  else if (strProtocol == "cdda") return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CFileISO();
#endif
  if( g_application.getNetwork().IsAvailable() )
  {
    if (strProtocol == "http" 
    ||  strProtocol == "https") return new CFileCurl();
    else if (strProtocol == "ftp" 
         ||  strProtocol == "ftpx"
         ||  strProtocol == "ftps") return new CFileCurl();
    else if (strProtocol == "upnp") return new CFileCurl();
    else if (strProtocol == "mms") return new CFileMMS();
    else if (strProtocol == "shout") return new CFileShoutcast();
    else if (strProtocol == "lastfm") return new CFileLastFM();
    else if (strProtocol == "tuxbox") return new CFileTuxBox();
    else if (strProtocol == "hdhomerun") return new CFileHomeRun();
    else if (strProtocol == "myth") return new CCMythFile();
    else if (strProtocol == "cmyth") return new CCMythFile();
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32PC
    else if (strProtocol == "smb") return new CWINFileSMB();
#else
    else if (strProtocol == "smb") return new CFileSMB();
#endif
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
    else if (strProtocol == "myth") return new CCMythFile();
    else if (strProtocol == "cmyth") return new CCMythFile();
#ifdef HAS_FILESYSTEM_SAP
    else if (strProtocol == "sap") return new CSAPFile();
#endif
#ifdef HAS_FILESYSTEM_VTP
    else if (strProtocol == "vtp") return new CVTPFile();
#endif
  }

  return NULL;
}
