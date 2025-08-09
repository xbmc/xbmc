/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Filesystem.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "Util.h"
#include "addons/binary-addons/AddonDll.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/Filesystem.h"
#include "utils/Crc32.h"
#include "utils/HttpHeader.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <vector>

#if defined(TARGET_WINDOWS)
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) (0)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (0)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) ((m & S_IFLNK) != 0)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m) ((m & _S_IFCHR) != 0)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) ((m & _S_IFDIR) != 0)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) ((m & _S_IFIFO) != 0)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m & _S_IFREG) != 0)
#endif
#endif

using namespace kodi; // addon-dev-kit namespace
using namespace XFILE;

namespace ADDON
{

void Interface_Filesystem::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_filesystem = new AddonToKodiFuncTable_kodi_filesystem();

  addonInterface->toKodi->kodi_filesystem->can_open_directory = can_open_directory;
  addonInterface->toKodi->kodi_filesystem->create_directory = create_directory;
  addonInterface->toKodi->kodi_filesystem->directory_exists = directory_exists;
  addonInterface->toKodi->kodi_filesystem->remove_directory = remove_directory;
  addonInterface->toKodi->kodi_filesystem->remove_directory_recursive = remove_directory_recursive;
  addonInterface->toKodi->kodi_filesystem->get_directory = get_directory;
  addonInterface->toKodi->kodi_filesystem->free_directory = free_directory;

  addonInterface->toKodi->kodi_filesystem->file_exists = file_exists;
  addonInterface->toKodi->kodi_filesystem->stat_file = stat_file;
  addonInterface->toKodi->kodi_filesystem->delete_file = delete_file;
  addonInterface->toKodi->kodi_filesystem->rename_file = rename_file;
  addonInterface->toKodi->kodi_filesystem->copy_file = copy_file;
  addonInterface->toKodi->kodi_filesystem->get_file_md5 = get_file_md5;
  addonInterface->toKodi->kodi_filesystem->get_cache_thumb_name = get_cache_thumb_name;
  addonInterface->toKodi->kodi_filesystem->make_legal_filename = make_legal_filename;
  addonInterface->toKodi->kodi_filesystem->make_legal_path = make_legal_path;
  addonInterface->toKodi->kodi_filesystem->translate_special_protocol = translate_special_protocol;
  addonInterface->toKodi->kodi_filesystem->get_disk_space = get_disk_space;
  addonInterface->toKodi->kodi_filesystem->is_internet_stream = is_internet_stream;
  addonInterface->toKodi->kodi_filesystem->is_on_lan = is_on_lan;
  addonInterface->toKodi->kodi_filesystem->is_remote = is_remote;
  addonInterface->toKodi->kodi_filesystem->is_local = is_local;
  addonInterface->toKodi->kodi_filesystem->is_url = is_url;
  addonInterface->toKodi->kodi_filesystem->get_http_header = get_http_header;
  addonInterface->toKodi->kodi_filesystem->get_mime_type = get_mime_type;
  addonInterface->toKodi->kodi_filesystem->get_content_type = get_content_type;
  addonInterface->toKodi->kodi_filesystem->get_cookies = get_cookies;

  addonInterface->toKodi->kodi_filesystem->http_header_create = http_header_create;
  addonInterface->toKodi->kodi_filesystem->http_header_free = http_header_free;

  addonInterface->toKodi->kodi_filesystem->open_file = open_file;
  addonInterface->toKodi->kodi_filesystem->open_file_for_write = open_file_for_write;
  addonInterface->toKodi->kodi_filesystem->read_file = read_file;
  addonInterface->toKodi->kodi_filesystem->read_file_string = read_file_string;
  addonInterface->toKodi->kodi_filesystem->write_file = write_file;
  addonInterface->toKodi->kodi_filesystem->flush_file = flush_file;
  addonInterface->toKodi->kodi_filesystem->seek_file = seek_file;
  addonInterface->toKodi->kodi_filesystem->truncate_file = truncate_file;
  addonInterface->toKodi->kodi_filesystem->get_file_position = get_file_position;
  addonInterface->toKodi->kodi_filesystem->get_file_length = get_file_length;
  addonInterface->toKodi->kodi_filesystem->get_file_download_speed = get_file_download_speed;
  addonInterface->toKodi->kodi_filesystem->close_file = close_file;
  addonInterface->toKodi->kodi_filesystem->get_file_chunk_size = get_file_chunk_size;
  addonInterface->toKodi->kodi_filesystem->io_control_get_seek_possible =
      io_control_get_seek_possible;
  addonInterface->toKodi->kodi_filesystem->io_control_get_cache_status =
      io_control_get_cache_status;
  addonInterface->toKodi->kodi_filesystem->io_control_set_cache_rate = io_control_set_cache_rate;
  addonInterface->toKodi->kodi_filesystem->io_control_set_retry = io_control_set_retry;
  addonInterface->toKodi->kodi_filesystem->get_property_values = get_property_values;

  addonInterface->toKodi->kodi_filesystem->curl_create = curl_create;
  addonInterface->toKodi->kodi_filesystem->curl_add_option = curl_add_option;
  addonInterface->toKodi->kodi_filesystem->curl_open = curl_open;
}

void Interface_Filesystem::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi) /* <-- Safe check, needed so long old addon way is present */
  {
    delete addonInterface->toKodi->kodi_filesystem;
    addonInterface->toKodi->kodi_filesystem = nullptr;
  }
}

unsigned int Interface_Filesystem::TranslateFileReadBitsToKodi(unsigned int addonFlags)
{
  unsigned int kodiFlags = 0;

  if (addonFlags & ADDON_READ_TRUNCATED)
    kodiFlags |= READ_TRUNCATED;
  if (addonFlags & ADDON_READ_CHUNKED)
    CLog::LogF(LOGWARNING, "Detected use of deprecated 'ADDON_READ_CHUNKED' flag");
  if (addonFlags & ADDON_READ_CACHED)
    kodiFlags |= READ_CACHED;
  if (addonFlags & ADDON_READ_NO_CACHE)
    kodiFlags |= READ_NO_CACHE;
  if (addonFlags & ADDON_READ_BITRATE)
    kodiFlags |= READ_BITRATE;
  if (addonFlags & ADDON_READ_MULTI_STREAM)
    kodiFlags |= READ_MULTI_STREAM;
  if (addonFlags & ADDON_READ_AUDIO_VIDEO)
    kodiFlags |= READ_AUDIO_VIDEO;
  if (addonFlags & ADDON_READ_AFTER_WRITE)
    kodiFlags |= READ_AFTER_WRITE;
  if (addonFlags & ADDON_READ_REOPEN)
    kodiFlags |= READ_REOPEN;
  if (addonFlags & ADDON_READ_NO_BUFFER)
    kodiFlags |= READ_NO_BUFFER;

  return kodiFlags;
}

bool Interface_Filesystem::can_open_directory(void* kodiBase, const char* url)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', url='{}')", kodiBase,
               static_cast<const void*>(url));
    return false;
  }

  CFileItemList items;
  return CDirectory::GetDirectory(url, items, "", DIR_FLAG_DEFAULTS | DIR_FLAG_BYPASS_CACHE);
}

bool Interface_Filesystem::create_directory(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{}')", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return CDirectory::Create(path);
}

bool Interface_Filesystem::directory_exists(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{}')", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return CDirectory::Exists(path, false);
}

bool Interface_Filesystem::remove_directory(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{}')", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  // Empty directory
  CFileItemList fileItems;
  CDirectory::GetDirectory(path, fileItems, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE);
  for (int i = 0; i < fileItems.Size(); ++i)
    CFile::Delete(fileItems.Get(i)->GetPath());

  return CDirectory::Remove(path);
}

bool Interface_Filesystem::remove_directory_recursive(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{}')", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return CDirectory::RemoveRecursive(path);
}

namespace
{
void CFileItemListToVFSDirEntries(VFSDirEntry* entries, const CFileItemList& items)
{
  for (unsigned int i = 0; i < static_cast<unsigned int>(items.Size()); ++i)
  {
    const auto& item{items[i]};
    auto& entry{entries[i]};
    entry.label = strdup(item->GetLabel().c_str());
    entry.path = strdup(item->GetPath().c_str());
    entry.size = item->GetSize();
    entry.folder = item->IsFolder();
    item->GetDateTime().GetAsTime(entry.date_time);
  }
}
} // unnamed namespace

bool Interface_Filesystem::get_directory(void* kodiBase,
                                         const char* path,
                                         const char* mask,
                                         struct VFSDirEntry** items,
                                         unsigned int* num_items)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path || !mask || !items || !num_items)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', path='{}', mask='{}', "
               "items='{}', num_items='{}'",
               kodiBase, static_cast<const void*>(path), static_cast<const void*>(mask),
               static_cast<void*>(items), static_cast<void*>(num_items));
    return false;
  }

  CFileItemList fileItems;
  if (!CDirectory::GetDirectory(path, fileItems, mask,
                                DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE))
    return false;

  if (!fileItems.IsEmpty())
  {
    *num_items = static_cast<unsigned int>(fileItems.Size());
    *items = new VFSDirEntry[fileItems.Size()];
    CFileItemListToVFSDirEntries(*items, fileItems);
  }
  else
  {
    *num_items = 0;
    *items = nullptr;
  }

  return true;
}

void Interface_Filesystem::free_directory(void* kodiBase,
                                          struct VFSDirEntry* items,
                                          unsigned int num_items)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !items)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', items='{}')", kodiBase,
               static_cast<void*>(items));
    return;
  }

  for (unsigned int i = 0; i < num_items; ++i)
  {
    free(items[i].label);
    free(items[i].path);
  }
  delete[] items;
}

//------------------------------------------------------------------------------

bool Interface_Filesystem::file_exists(void* kodiBase, const char* filename, bool useCache)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}')", kodiBase,
               static_cast<const void*>(filename));
    return false;
  }

  return CFile::Exists(filename, useCache);
}

bool Interface_Filesystem::stat_file(void* kodiBase,
                                     const char* filename,
                                     struct STAT_STRUCTURE* buffer)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename || !buffer)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}', buffer='{}')", kodiBase,
               static_cast<const void*>(filename), static_cast<void*>(buffer));
    return false;
  }

  struct __stat64 statBuffer;
  if (CFile::Stat(filename, &statBuffer) != 0)
    return false;

  buffer->deviceId = statBuffer.st_dev;
  buffer->fileSerialNumber = statBuffer.st_ino;
  buffer->size = statBuffer.st_size;
  buffer->accessTime = statBuffer.st_atime;
  buffer->modificationTime = statBuffer.st_mtime;
  buffer->statusTime = statBuffer.st_ctime;
  buffer->isDirectory = S_ISDIR(statBuffer.st_mode);
  buffer->isSymLink = S_ISLNK(statBuffer.st_mode);
  buffer->isBlock = S_ISBLK(statBuffer.st_mode);
  buffer->isCharacter = S_ISCHR(statBuffer.st_mode);
  buffer->isFifo = S_ISFIFO(statBuffer.st_mode);
  buffer->isRegular = S_ISREG(statBuffer.st_mode);
  buffer->isSocket = S_ISSOCK(statBuffer.st_mode);

  return true;
}

bool Interface_Filesystem::delete_file(void* kodiBase, const char* filename)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}')", kodiBase,
               static_cast<const void*>(filename));
    return false;
  }

  return CFile::Delete(filename);
}

bool Interface_Filesystem::rename_file(void* kodiBase,
                                       const char* filename,
                                       const char* newFileName)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename || !newFileName)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}', newFileName='{}')", kodiBase,
               static_cast<const void*>(filename), static_cast<const void*>(newFileName));
    return false;
  }

  return CFile::Rename(filename, newFileName);
}

bool Interface_Filesystem::copy_file(void* kodiBase, const char* filename, const char* dest)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename || !dest)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}', dest='{}')", kodiBase,
               static_cast<const void*>(filename), static_cast<const void*>(dest));
    return false;
  }

  return CFile::Copy(filename, dest);
}

char* Interface_Filesystem::get_file_md5(void* kodiBase, const char* filename)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{})", kodiBase,
               static_cast<const void*>(filename));
    return nullptr;
  }

  const std::string string{CUtil::GetFileDigest(filename, KODI::UTILITY::CDigest::Type::MD5)};
  return strdup(string.c_str());
}

char* Interface_Filesystem::get_cache_thumb_name(void* kodiBase, const char* filename)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{})", kodiBase,
               static_cast<const void*>(filename));
    return nullptr;
  }

  const auto crc = Crc32::ComputeFromLowerCase(filename);
  const std::string hex{StringUtils::Format("{:08x}.tbn", crc)};
  return strdup(hex.c_str());
}

char* Interface_Filesystem::make_legal_filename(void* kodiBase, const char* filename)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{})", kodiBase,
               static_cast<const void*>(filename));
    return nullptr;
  }

  const std::string string{CUtil::MakeLegalFileName(filename)};
  return strdup(string.c_str());
}

char* Interface_Filesystem::make_legal_path(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return nullptr;
  }

  const std::string string{CUtil::MakeLegalPath(path)};
  return strdup(string.c_str());
}

char* Interface_Filesystem::translate_special_protocol(void* kodiBase, const char* strSource)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !strSource)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', strSource='{})", kodiBase,
               static_cast<const void*>(strSource));
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(strSource).c_str());
}

bool Interface_Filesystem::get_disk_space(
    void* kodiBase, const char* path, uint64_t* capacity, uint64_t* free, uint64_t* available)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path || !capacity || !free || !available)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', path='{}, capacity='{}, free='{}, "
               "available='{})",
               kodiBase, static_cast<const void*>(path), static_cast<void*>(capacity),
               static_cast<void*>(free), static_cast<void*>(available));
    return false;
  }

  using namespace KODI::PLATFORM::FILESYSTEM;

  std::error_code ec;
  const space_info freeSpace{space(CSpecialProtocol::TranslatePath(path), ec)};
  if (ec.value() != 0)
    return false;

  *capacity = freeSpace.capacity;
  *free = freeSpace.free;
  *available = freeSpace.available;
  return true;
}

bool Interface_Filesystem::is_internet_stream(void* kodiBase, const char* path, bool strictCheck)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return URIUtils::IsInternetStream(path, strictCheck);
}

bool Interface_Filesystem::is_on_lan(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return URIUtils::IsOnLAN(path);
}

bool Interface_Filesystem::is_remote(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return URIUtils::IsRemote(path);
}

bool Interface_Filesystem::is_local(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return CURL(path).IsLocal();
}

bool Interface_Filesystem::is_url(void* kodiBase, const char* path)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !path)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', path='{})", kodiBase,
               static_cast<const void*>(path));
    return false;
  }

  return URIUtils::IsURL(path);
}

bool Interface_Filesystem::get_mime_type(void* kodiBase,
                                         const char* url,
                                         char** content,
                                         const char* useragent)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url || !content || !useragent)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', url='{}', content='{}', "
               "useragent='{}')",
               kodiBase, static_cast<const void*>(url), static_cast<const void*>(content),
               static_cast<const void*>(useragent));
    return false;
  }

  std::string kodiContent;
  const bool ret{XFILE::CCurlFile::GetMimeType(CURL(url), kodiContent, useragent)};
  if (ret && !kodiContent.empty())
  {
    *content = strdup(kodiContent.c_str());
  }
  return ret;
}

bool Interface_Filesystem::get_content_type(void* kodiBase,
                                            const char* url,
                                            char** content,
                                            const char* useragent)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url || !content || !useragent)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', url='{}', content='{}', "
               "useragent='{}')",
               kodiBase, static_cast<const void*>(url), static_cast<const void*>(content),
               static_cast<const void*>(useragent));
    return false;
  }

  std::string kodiContent;
  const bool ret{XFILE::CCurlFile::GetContentType(CURL(url), kodiContent, useragent)};
  if (ret && !kodiContent.empty())
  {
    *content = strdup(kodiContent.c_str());
  }
  return ret;
}

bool Interface_Filesystem::get_cookies(void* kodiBase, const char* url, char** cookies)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url || !cookies)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', url='{}', cookies='{}')", kodiBase,
               static_cast<const void*>(url), static_cast<const void*>(cookies));
    return false;
  }

  std::string kodiCookies;
  const bool ret{XFILE::CCurlFile::GetCookies(CURL(url), kodiCookies)};
  if (ret && !kodiCookies.empty())
  {
    *cookies = strdup(kodiCookies.c_str());
  }
  return ret;
}

bool Interface_Filesystem::get_http_header(void* kodiBase,
                                           const char* url,
                                           struct KODI_HTTP_HEADER* headers)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url || !headers || !headers->handle)
  {
    CLog::LogF(LOGERROR, "Invalid data pointer given");
    return false;
  }

  CHttpHeader* httpHeader = static_cast<CHttpHeader*>(headers->handle);
  return XFILE::CCurlFile::GetHttpHeader(CURL(url), *httpHeader);
}

//------------------------------------------------------------------------------

bool Interface_Filesystem::http_header_create(void* kodiBase, struct KODI_HTTP_HEADER* headers)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !headers)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', headers='{}')", kodiBase,
               static_cast<const void*>(headers));
    return false;
  }

  headers->handle = new CHttpHeader;
  headers->get_value = http_header_get_value;
  headers->get_values = http_header_get_values;
  headers->get_header = http_header_get_header;
  headers->get_mime_type = http_header_get_mime_type;
  headers->get_charset = http_header_get_charset;
  headers->get_proto_line = http_header_get_proto_line;

  return true;
}

void Interface_Filesystem::http_header_free(void* kodiBase, struct KODI_HTTP_HEADER* headers)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !headers)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', headers='{}')", kodiBase,
               static_cast<const void*>(headers));
    return;
  }

  delete static_cast<CHttpHeader*>(headers->handle);
  headers->handle = nullptr;
}

char* Interface_Filesystem::http_header_get_value(void* kodiBase, void* handle, const char* param)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle || !param)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', handle='{}', param='{}')", kodiBase, handle,
               static_cast<const void*>(param));
    return nullptr;
  }

  const std::string string{static_cast<CHttpHeader*>(handle)->GetValue(param)};

  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

char** Interface_Filesystem::http_header_get_values(void* kodiBase,
                                                    void* handle,
                                                    const char* param,
                                                    int* length)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle || !param || !length)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', handle='{}', param='{}', "
               "length='{}')",
               kodiBase, handle, static_cast<const void*>(param), static_cast<const void*>(length));
    return nullptr;
  }

  const std::vector<std::string> values{static_cast<CHttpHeader*>(handle)->GetValues(param)};
  *length = static_cast<int>(values.size());
  char** ret = static_cast<char**>(malloc(sizeof(char*) * values.size()));
  for (int i = 0; i < *length; ++i)
  {
    ret[i] = strdup(values[i].c_str());
  }
  return ret;
}

char* Interface_Filesystem::http_header_get_header(void* kodiBase, void* handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', handle='{}')", kodiBase, handle);
    return nullptr;
  }

  const std::string string{static_cast<CHttpHeader*>(handle)->GetHeader()};

  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

char* Interface_Filesystem::http_header_get_mime_type(void* kodiBase, void* handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', handle='{}')", kodiBase, handle);
    return nullptr;
  }

  const std::string string{static_cast<CHttpHeader*>(handle)->GetMimeType()};

  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

char* Interface_Filesystem::http_header_get_charset(void* kodiBase, void* handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', handle='{}')", kodiBase, handle);
    return nullptr;
  }

  const std::string string{static_cast<CHttpHeader*>(handle)->GetCharset()};

  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

char* Interface_Filesystem::http_header_get_proto_line(void* kodiBase, void* handle)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !handle)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', handle='{}')", kodiBase, handle);
    return nullptr;
  }

  const std::string string{static_cast<CHttpHeader*>(handle)->GetProtoLine()};

  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

//------------------------------------------------------------------------------

void* Interface_Filesystem::open_file(void* kodiBase, const char* filename, unsigned int flags)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}')", kodiBase,
               static_cast<const void*>(filename));
    return nullptr;
  }

  auto file{std::make_unique<CFile>()};
  if (!file->Open(filename, TranslateFileReadBitsToKodi(flags)))
    return nullptr;

  return static_cast<void*>(file.release());
}

void* Interface_Filesystem::open_file_for_write(void* kodiBase,
                                                const char* filename,
                                                bool overwrite)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !filename)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', filename='{}')", kodiBase,
               static_cast<const void*>(filename));
    return nullptr;
  }

  auto file{std::make_unique<CFile>()};
  if (!file->OpenForWrite(filename, overwrite))
    return nullptr;

  return static_cast<void*>(file.release());
}

ssize_t Interface_Filesystem::read_file(void* kodiBase, void* file, void* ptr, size_t size)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !ptr)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}', ptr='{}')", kodiBase, file, ptr);
    return -1;
  }

  return static_cast<CFile*>(file)->Read(ptr, size);
}

bool Interface_Filesystem::read_file_string(void* kodiBase,
                                            void* file,
                                            char* szLine,
                                            int lineLength)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !szLine)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}', szLine=='{}')", kodiBase, file,
               static_cast<void*>(szLine));
    return false;
  }

  return static_cast<CFile*>(file)->ReadLine(szLine, lineLength).code !=
         CFile::ReadLineResult::FAILURE;
}

ssize_t Interface_Filesystem::write_file(void* kodiBase, void* file, const void* ptr, size_t size)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !ptr)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}', ptr='{}')", kodiBase, file, ptr);
    return -1;
  }

  return static_cast<CFile*>(file)->Write(ptr, size);
}

void Interface_Filesystem::flush_file(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return;
  }

  static_cast<CFile*>(file)->Flush();
}

int64_t Interface_Filesystem::seek_file(void* kodiBase, void* file, int64_t position, int whence)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return -1;
  }

  return static_cast<CFile*>(file)->Seek(position, whence);
}

int Interface_Filesystem::truncate_file(void* kodiBase, void* file, int64_t size)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return -1;
  }

  return static_cast<CFile*>(file)->Truncate(size);
}

int64_t Interface_Filesystem::get_file_position(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return -1;
  }

  return static_cast<CFile*>(file)->GetPosition();
}

int64_t Interface_Filesystem::get_file_length(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return -1;
  }

  return static_cast<CFile*>(file)->GetLength();
}

double Interface_Filesystem::get_file_download_speed(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return 0.0;
  }

  return static_cast<CFile*>(file)->GetDownloadSpeed();
}

void Interface_Filesystem::close_file(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return;
  }

  auto* f{static_cast<CFile*>(file)};
  f->Close();
  delete f;
}

int Interface_Filesystem::get_file_chunk_size(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return -1;
  }

  return static_cast<CFile*>(file)->GetChunkSize();
}

bool Interface_Filesystem::io_control_get_seek_possible(void* kodiBase, void* file)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return false;
  }

  return static_cast<CFile*>(file)->IoControl(IOControl::SEEK_POSSIBLE, nullptr) != 0;
}

bool Interface_Filesystem::io_control_get_cache_status(void* kodiBase,
                                                       void* file,
                                                       struct VFS_CACHE_STATUS_DATA* status)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !status)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}, status='{}')", kodiBase, file,
               static_cast<const void*>(status));
    return false;
  }

  SCacheStatus data = {};
  const int ret{static_cast<CFile*>(file)->IoControl(IOControl::CACHE_STATUS, &data)};
  if (ret >= 0)
  {
    status->forward = data.forward;
    status->maxrate = data.maxrate;
    status->currate = data.currate;
    status->lowrate = data.lowrate;
    return true;
  }
  return false;
}

bool Interface_Filesystem::io_control_set_cache_rate(void* kodiBase, void* file, uint32_t rate)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return false;
  }

  return static_cast<CFile*>(file)->IoControl(IOControl::CACHE_SETRATE, &rate) >= 0;
}

bool Interface_Filesystem::io_control_set_retry(void* kodiBase, void* file, bool retry)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return false;
  }

  return static_cast<CFile*>(file)->IoControl(IOControl::SET_RETRY, &retry) >= 0;
}

char** Interface_Filesystem::get_property_values(
    void* kodiBase, void* file, int type, const char* name, int* numValues)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !name || !numValues)
  {
    CLog::LogF(LOGERROR,
               "Invalid data (addon='{}', file='{}', name='{}', "
               "numValues='{}')",
               kodiBase, file, static_cast<const void*>(name), static_cast<void*>(numValues));
    return nullptr;
  }

  XFILE::FileProperty internalType;
  switch (type)
  {
    case ADDON_FILE_PROPERTY_RESPONSE_PROTOCOL:
      internalType = XFILE::FileProperty::RESPONSE_PROTOCOL;
      break;
    case ADDON_FILE_PROPERTY_RESPONSE_HEADER:
      internalType = XFILE::FileProperty::RESPONSE_HEADER;
      break;
    case ADDON_FILE_PROPERTY_CONTENT_TYPE:
      internalType = XFILE::FileProperty::CONTENT_TYPE;
      break;
    case ADDON_FILE_PROPERTY_CONTENT_CHARSET:
      internalType = XFILE::FileProperty::CONTENT_CHARSET;
      break;
    case ADDON_FILE_PROPERTY_MIME_TYPE:
      internalType = XFILE::FileProperty::MIME_TYPE;
      break;
    case ADDON_FILE_PROPERTY_EFFECTIVE_URL:
      internalType = XFILE::FileProperty::EFFECTIVE_URL;
      break;
    default:
      CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
      return nullptr;
  };

  const std::vector<std::string> values{
      static_cast<CFile*>(file)->GetPropertyValues(internalType, name)};
  *numValues = static_cast<int>(values.size());
  char** ret = static_cast<char**>(malloc(sizeof(char*) * values.size()));
  for (int i = 0; i < *numValues; ++i)
  {
    ret[i] = strdup(values[i].c_str());
  }
  return ret;
}

void* Interface_Filesystem::curl_create(void* kodiBase, const char* url)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', url='{}')", kodiBase,
               static_cast<const void*>(url));
    return nullptr;
  }

  auto file{std::make_unique<CFile>()};
  if (!file->CURLCreate(url))
    return nullptr;

  return static_cast<void*>(file.release());
}

bool Interface_Filesystem::curl_add_option(
    void* kodiBase, void* file, int type, const char* name, const char* value)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file || !name || !value)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}', name='{}', value='{}')", kodiBase,
               file, static_cast<const void*>(name), static_cast<const void*>(value));
    return false;
  }

  XFILE::CURLOptionType internalType;
  switch (type)
  {
    case ADDON_CURL_OPTION_OPTION:
      internalType = XFILE::CURLOptionType::OPTION;
      break;
    case ADDON_CURL_OPTION_PROTOCOL:
      internalType = XFILE::CURLOptionType::PROTOCOL;
      break;
    case ADDON_CURL_OPTION_CREDENTIALS:
      internalType = XFILE::CURLOptionType::CREDENTIALS;
      break;
    case ADDON_CURL_OPTION_HEADER:
      internalType = XFILE::CURLOptionType::HEADER;
      break;
    default:
      throw std::logic_error("Interface_Filesystem::curl_add_option - invalid curl option type");
      return false;
  }

  return static_cast<CFile*>(file)->CURLAddOption(internalType, name, value);
}

bool Interface_Filesystem::curl_open(void* kodiBase, void* file, unsigned int flags)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !file)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', file='{}')", kodiBase, file);
    return false;
  }

  return static_cast<CFile*>(file)->CURLOpen(TranslateFileReadBitsToKodi(flags));
}

} /* namespace ADDON */
