/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/Network.h"
#include "FileFactory.h"
#ifdef TARGET_POSIX
#include "platform/posix/filesystem/PosixFile.h"
#elif defined(TARGET_WINDOWS)
#include "platform/win32/filesystem/Win32File.h"
#ifdef TARGET_WINDOWS_STORE
#include "platform/win10/filesystem/WinLibraryFile.h"
#endif
#endif // TARGET_WINDOWS
#include "CurlFile.h"
#include "DAVFile.h"
#include "ShoutcastFile.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef TARGET_WINDOWS
#include "platform/win32/filesystem/Win32SMBFile.h"
#else
#include "platform/posix/filesystem/SMBFile.h"
#endif
#endif
#include "CDDAFile.h"
#if defined(HAS_ISO9660PP)
#include "ISO9660File.h"
#endif
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/APKFile.h"
#endif
#include "XbtFile.h"
#include "ZipFile.h"
#ifdef HAS_FILESYSTEM_NFS
#include "NFSFile.h"
#endif
#if defined(TARGET_ANDROID)
#include "platform/android/filesystem/AndroidAppFile.h"
#endif
#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/filesystem/TVOSFile.h"
#endif // TARGET_DARWIN_TVOS
#ifdef HAS_UPNP
#include "UPnPFile.h"
#endif
#ifdef HAVE_LIBBLURAY
#include "BlurayFile.h"
#endif
#include "PipeFile.h"
#include "MusicDatabaseFile.h"
#include "VideoDatabaseFile.h"
#include "PluginFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#if defined(HAS_UDFREAD)
#include "UDFFile.h"
#endif
#include "ImageFile.h"
#include "ResourceFile.h"
#include "URL.h"
#include "utils/log.h"
#include "network/WakeOnAccess.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"
#include "addons/VFSEntry.h"

using namespace ADDON;
using namespace XFILE;

CFileFactory::CFileFactory() = default;

CFileFactory::~CFileFactory() = default;

IFile* CFileFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  if (!CWakeOnAccess::GetInstance().WakeUpHost(url))
    return NULL;

  if (!url.GetProtocol().empty() && CServiceBroker::IsAddonInterfaceUp())
  {
    for (const auto& vfsAddon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    {
      auto prots = StringUtils::Split(vfsAddon->GetProtocols(), "|");

      if (vfsAddon->HasFiles() && std::find(prots.begin(), prots.end(), url.GetProtocol()) != prots.end())
        return new CVFSEntryIFileWrapper(vfsAddon);
    }
  }

#if defined(TARGET_ANDROID)
  if (url.IsProtocol("apk")) return new CAPKFile();
#endif
  if (url.IsProtocol("zip")) return new CZipFile();
  else if (url.IsProtocol("xbt")) return new CXbtFile();
  else if (url.IsProtocol("musicdb")) return new CMusicDatabaseFile();
  else if (url.IsProtocol("videodb")) return new CVideoDatabaseFile();
  else if (url.IsProtocol("plugin")) return new CPluginFile();
  else if (url.IsProtocol("library")) return nullptr;
  else if (url.IsProtocol("pvr")) return nullptr;
  else if (url.IsProtocol("special")) return new CSpecialProtocolFile();
  else if (url.IsProtocol("multipath")) return new CMultiPathFile();
  else if (url.IsProtocol("image")) return new CImageFile();
#ifdef TARGET_POSIX
  else if (url.IsProtocol("file") || url.GetProtocol().empty())
  {
#if defined(TARGET_DARWIN_TVOS)
    if (CTVOSFile::WantsFile(url))
      return new CTVOSFile();
#endif
    return new CPosixFile();
  }
#elif defined(TARGET_WINDOWS)
  else if (url.IsProtocol("file") || url.GetProtocol().empty())
  {
#ifdef TARGET_WINDOWS_STORE
    if (CWinLibraryFile::IsInAccessList(url))
      return new CWinLibraryFile();
#endif
    return new CWin32File();
  }
#endif // TARGET_WINDOWS
#if defined(HAS_OPTICAL_DRIVE)
  else if (url.IsProtocol("cdda")) return new CFileCDDA();
#endif
#if defined(HAS_ISO9660PP)
  else if (url.IsProtocol("iso9660"))
    return new CISO9660File();
#endif
#if defined(HAS_UDFREAD)
  else if(url.IsProtocol("udf"))
    return new CUDFFile();
#endif
#if defined(TARGET_ANDROID)
  else if (url.IsProtocol("androidapp")) return new CFileAndroidApp();
#endif
  else if (url.IsProtocol("pipe")) return new CPipeFile();
#ifdef HAVE_LIBBLURAY
  else if (url.IsProtocol("bluray")) return new CBlurayFile();
#endif
  else if (url.IsProtocol("resource")) return new CResourceFile();
#ifdef TARGET_WINDOWS_STORE
  else if (CWinLibraryFile::IsValid(url)) return new CWinLibraryFile();
#endif

  if (url.IsProtocol("ftp")
  ||  url.IsProtocol("ftps")
  ||  url.IsProtocol("rss")
  ||  url.IsProtocol("rsss")
  ||  url.IsProtocol("http")
  ||  url.IsProtocol("https")) return new CCurlFile();
  else if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVFile();
  else if (url.IsProtocol("shout") || url.IsProtocol("shouts")) return new CShoutcastFile();
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

  CLog::Log(LOGWARNING, "{} - unsupported protocol({}) in {}", __FUNCTION__, url.GetProtocol(),
            url.GetRedacted());
  return NULL;
}
