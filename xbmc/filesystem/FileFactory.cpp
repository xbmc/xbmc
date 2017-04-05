/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "network/Network.h"
#include "system.h"
#include "FileFactory.h"
#ifdef TARGET_POSIX
#include "posix/PosixFile.h"
#elif defined(TARGET_WINDOWS)
#include "win32/Win32File.h"
#endif // TARGET_WINDOWS
#include "CurlFile.h"
#include "DAVFile.h"
#include "ShoutcastFile.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
#include "win32/Win32SMBFile.h"
#else
#include "SMBFile.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDAFile.h"
#endif
#ifdef HAS_FILESYSTEM
#include "ISOFile.h"
#endif
#if defined(TARGET_ANDROID)
#include "APKFile.h"
#endif
#include "XbtFile.h"
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
#if defined(TARGET_ANDROID)
#include "AndroidAppFile.h"
#endif
#ifdef HAS_UPNP
#include "UPnPFile.h"
#endif
#ifdef HAVE_LIBBLURAY
#include "BlurayFile.h"
#endif
#include "PipeFile.h"
#include "MusicDatabaseFile.h"
#include "VideoDatabaseFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#include "UDFFile.h"
#include "ImageFile.h"
#include "ResourceFile.h"
#include "Application.h"
#include "URL.h"
#include "utils/log.h"
#include "network/WakeOnAccess.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"
#include "addons/VFSEntry.h"
#include "addons/BinaryAddonCache.h"

using namespace ADDON;
using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  if (!CWakeOnAccess::GetInstance().WakeUpHost(url))
    return NULL;

  std::string strProtocol = url.GetProtocol();
  StringUtils::ToLower(strProtocol);

  if (!strProtocol.empty() && CServiceBroker::IsBinaryAddonCacheUp())
  {
    VECADDONS addons;
    ADDON::CBinaryAddonCache &addonCache = CServiceBroker::GetBinaryAddonCache();
    addonCache.GetAddons(addons, ADDON::ADDON_VFS);
    for (size_t i=0;i<addons.size();++i)
    {
      VFSEntryPtr vfs(std::static_pointer_cast<CVFSEntry>(addons[i]));
      if (vfs->HasFiles() && vfs->GetProtocols().find(strProtocol) != std::string::npos)
        return new CVFSEntryIFileWrapper(vfs);
    }
  }

#if defined(TARGET_ANDROID)
  if (url.IsProtocol("apk")) return new CAPKFile();
#endif
  if (url.IsProtocol("zip")) return new CZipFile();
  else if (url.IsProtocol("rar"))
  {
#ifdef HAS_FILESYSTEM_RAR
    return new CRarFile();
#else
    CLog::Log(LOGWARNING, "%s - Compiled without non-free, rar support is disabled", __FUNCTION__);
#endif
  }
  else if (url.IsProtocol("xbt")) return new CXbtFile();
  else if (url.IsProtocol("musicdb")) return new CMusicDatabaseFile();
  else if (url.IsProtocol("videodb")) return new CVideoDatabaseFile();
  else if (url.IsProtocol("library")) return nullptr;
  else if (url.IsProtocol("special")) return new CSpecialProtocolFile();
  else if (url.IsProtocol("multipath")) return new CMultiPathFile();
  else if (url.IsProtocol("image")) return new CImageFile();
#ifdef TARGET_POSIX
  else if (url.IsProtocol("file") || url.GetProtocol().empty()) return new CPosixFile();
#elif defined(TARGET_WINDOWS)
  else if (url.IsProtocol("file") || url.GetProtocol().empty()) return new CWin32File();
#endif // TARGET_WINDOWS 
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  else if (url.IsProtocol("cdda")) return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (url.IsProtocol("iso9660")) return new CISOFile();
#endif
  else if(url.IsProtocol("udf")) return new CUDFFile();
#if defined(TARGET_ANDROID)
  else if (url.IsProtocol("androidapp")) return new CFileAndroidApp();
#endif
  else if (url.IsProtocol("pipe")) return new CPipeFile();
#ifdef HAVE_LIBBLURAY
  else if (url.IsProtocol("bluray")) return new CBlurayFile();
#endif
  else if (url.IsProtocol("resource")) return new CResourceFile();

  bool networkAvailable = g_application.getNetwork().IsAvailable();
  if (networkAvailable)
  {
    if (url.IsProtocol("ftp")
    ||  url.IsProtocol("ftps")
    ||  url.IsProtocol("rss")
    ||  url.IsProtocol("http") 
    ||  url.IsProtocol("https")) return new CCurlFile();
    else if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVFile();
#ifdef HAS_FILESYSTEM_SFTP
    else if (url.IsProtocol("sftp") || url.IsProtocol("ssh")) return new CSFTPFile();
#endif
    else if (url.IsProtocol("shout")) return new CShoutcastFile();
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
    else if (url.IsProtocol("smb")) return new CWin32SMBFile();
#else
    else if (url.IsProtocol("smb")) return new CSMBFile();
#endif
#endif
#ifdef HAS_FILESYSTEM_NFS
    else if (url.IsProtocol("nfs")) return new CNFSFile();
#endif
#ifdef HAS_UPNP
    else if (url.IsProtocol("upnp")) return new CUPnPFile();
#endif
  }

  CLog::Log(LOGWARNING, "%s - %sunsupported protocol(%s) in %s", __FUNCTION__, networkAvailable ? "" : "Network down or ", url.GetProtocol().c_str(), url.GetRedacted().c_str());
  return NULL;
}
