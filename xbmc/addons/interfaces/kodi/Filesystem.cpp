/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Filesystem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/Filesystem.h"

#include "Util.h"
#include "addons/AddonDll.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/Crc32.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace kodi; // addon-dev-kit namespace
using namespace XFILE;

extern "C"
{
namespace ADDON
{

void Interface_Filesystem::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi.kodi->filesystem.can_open_directory = can_open_directory;
  addonInterface->toKodi.kodi->filesystem.create_directory = create_directory;
  addonInterface->toKodi.kodi->filesystem.directory_exists = directory_exists;
  addonInterface->toKodi.kodi->filesystem.remove_directory = remove_directory;
  addonInterface->toKodi.kodi->filesystem.get_directory = get_directory;
  addonInterface->toKodi.kodi->filesystem.free_directory = free_directory;

  addonInterface->toKodi.kodi->filesystem.file_exists = file_exists;
  addonInterface->toKodi.kodi->filesystem.stat_file = stat_file;
  addonInterface->toKodi.kodi->filesystem.delete_file = delete_file;
  addonInterface->toKodi.kodi->filesystem.rename_file = rename_file;
  addonInterface->toKodi.kodi->filesystem.copy_file = copy_file;
  addonInterface->toKodi.kodi->filesystem.file_set_hidden = file_set_hidden;
  addonInterface->toKodi.kodi->filesystem.get_file_md5 = get_file_md5;
  addonInterface->toKodi.kodi->filesystem.get_cache_thumb_name = get_cache_thumb_name;
  addonInterface->toKodi.kodi->filesystem.make_legal_filename = make_legal_filename;
  addonInterface->toKodi.kodi->filesystem.make_legal_path = make_legal_path;
  addonInterface->toKodi.kodi->filesystem.translate_special_protocol = translate_special_protocol;

  addonInterface->toKodi.kodi->filesystem.open_file = open_file;
  addonInterface->toKodi.kodi->filesystem.open_file_for_write = open_file_for_write;
  addonInterface->toKodi.kodi->filesystem.read_file = read_file;
  addonInterface->toKodi.kodi->filesystem.read_file_string = read_file_string;
  addonInterface->toKodi.kodi->filesystem.write_file = write_file;
  addonInterface->toKodi.kodi->filesystem.flush_file = flush_file;
  addonInterface->toKodi.kodi->filesystem.seek_file = seek_file;
  addonInterface->toKodi.kodi->filesystem.truncate_file = truncate_file;
  addonInterface->toKodi.kodi->filesystem.get_file_position = get_file_position;
  addonInterface->toKodi.kodi->filesystem.get_file_length = get_file_length;
  addonInterface->toKodi.kodi->filesystem.get_file_download_speed = get_file_download_speed;
  addonInterface->toKodi.kodi->filesystem.close_file = close_file;
  addonInterface->toKodi.kodi->filesystem.get_file_chunk_size = get_file_chunk_size;

  addonInterface->toKodi.kodi->filesystem.curl_create = curl_create;
  addonInterface->toKodi.kodi->filesystem.curl_add_option = curl_add_option;
  addonInterface->toKodi.kodi->filesystem.curl_open = curl_open;
}

bool Interface_Filesystem::can_open_directory(void* kodiBase, const char* url)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || url == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', url='%p')", __FUNCTION__, addon, url);
    return false;
  }

  CFileItemList items;
  return CDirectory::GetDirectory(url, items);
}

bool Interface_Filesystem::create_directory(void* kodiBase, const char *path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || path == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', path='%p')", __FUNCTION__, addon, path);
    return false;
  }

  return CDirectory::Create(path);
}

bool Interface_Filesystem::directory_exists(void* kodiBase, const char *path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || path == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', path='%p')", __FUNCTION__, addon, path);
    return false;
  }

  return CDirectory::Exists(path);
}

bool Interface_Filesystem::remove_directory(void* kodiBase, const char *path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || path == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', path='%p')", __FUNCTION__, addon, path);
    return false;
  }

  // Empty directory
  CFileItemList fileItems;
  CDirectory::GetDirectory(path, fileItems);
  for (int i = 0; i < fileItems.Size(); ++i)
    CFile::Delete(fileItems.Get(i)->GetPath());

  return CDirectory::Remove(path);
}

static void CFileItemListToVFSDirEntries(VFSDirEntry* entries,
                                         unsigned int num_entries,
                                         const CFileItemList& items)
{
  if (!entries)
    return;

  int toCopy = std::min(num_entries, (unsigned int)items.Size());

  for (int i = 0; i < toCopy; ++i)
  {
    entries[i].label = strdup(items[i]->GetLabel().c_str());
    entries[i].path = strdup(items[i]->GetPath().c_str());
    entries[i].size = items[i]->m_dwSize;
    entries[i].folder = items[i]->m_bIsFolder;
  }
}

bool Interface_Filesystem::get_directory(void* kodiBase, const char *path, const char* mask, VFSDirEntry** items, unsigned int* num_items)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || path == nullptr || mask == nullptr|| items == nullptr || num_items == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', path='%p', mask='%p', items='%p', num_items='%p'", __FUNCTION__, addon, path, mask, items, num_items);
    return false;
  }

  CFileItemList fileItems;
  if (!CDirectory::GetDirectory(path, fileItems, mask, DIR_FLAG_NO_FILE_DIRS))
    return false;

  if (fileItems.Size() > 0)
  {
    *num_items = static_cast<unsigned int>(fileItems.Size());
    *items = new VFSDirEntry[fileItems.Size()];
  }
  else
  {
    *num_items = 0;
    *items = nullptr;
  }

  CFileItemListToVFSDirEntries(*items, *num_items, fileItems);
  return true;
}

void Interface_Filesystem::free_directory(void* kodiBase, VFSDirEntry* items, unsigned int num_items)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || items == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', items='%p')", __FUNCTION__, addon, items);
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

bool Interface_Filesystem::file_exists(void* kodiBase, const char *filename, bool useCache)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p')", __FUNCTION__, addon, filename);
    return false;
  }

  return CFile::Exists(filename, useCache);
}

int Interface_Filesystem::stat_file(void* kodiBase, const char *filename, struct __stat64* buffer)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr || buffer == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p', buffer='%p')", __FUNCTION__, addon, filename, buffer);
    return false;
  }

  return CFile::Stat(filename, buffer);
}

bool Interface_Filesystem::delete_file(void* kodiBase, const char *filename)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p')", __FUNCTION__, addon, filename);
    return false;
  }

  return CFile::Delete(filename);
}

bool Interface_Filesystem::rename_file(void* kodiBase, const char *filename, const char *newFileName)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr || newFileName == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p', newFileName='%p')", __FUNCTION__, addon, filename, newFileName);
    return false;
  }

  return CFile::Rename(filename, newFileName);
}

bool Interface_Filesystem::copy_file(void* kodiBase, const char *filename, const char *dest)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr || dest == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p', dest='%p')", __FUNCTION__, addon, filename, dest);
    return false;
  }

  return CFile::Copy(filename, dest);
}

bool Interface_Filesystem::file_set_hidden(void* kodiBase, const char *filename, bool hidden)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p')", __FUNCTION__, addon, filename);
    return false;
  }

  return CFile::SetHidden(filename, hidden);
}
    
char* Interface_Filesystem::get_file_md5(void* kodiBase, const char* filename)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p)", __FUNCTION__, addon, filename);
    return nullptr;
  }

  std::string string = CUtil::GetFileMD5(filename);
  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_Filesystem::get_cache_thumb_name(void* kodiBase, const char* filename)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p)", __FUNCTION__, addon, filename);
    return nullptr;
  }

  Crc32 crc;
  crc.ComputeFromLowerCase(filename);
  std::string string = StringUtils::Format("%08x.tbn", (unsigned __int32)crc);
  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_Filesystem::make_legal_filename(void* kodiBase, const char* filename)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p)", __FUNCTION__, addon, filename);
    return nullptr;
  }

  std::string string = CUtil::MakeLegalFileName(filename);;
  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_Filesystem::make_legal_path(void* kodiBase, const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || path == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', path='%p)", __FUNCTION__, addon, path);
    return nullptr;
  }

  std::string string = CUtil::MakeLegalPath(path);;
  char* buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_Filesystem::translate_special_protocol(void* kodiBase, const char *strSource)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || strSource == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', strSource='%p)", __FUNCTION__, addon, strSource);
    return nullptr;
  }

  return strdup(CSpecialProtocol::TranslatePath(strSource).c_str());
}

//------------------------------------------------------------------------------

void* Interface_Filesystem::open_file(void* kodiBase, const char* filename, unsigned int flags)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p')", __FUNCTION__, addon, filename);
    return nullptr;
  }

  CFile* file = new CFile;
  if (file->Open(filename, flags))
    return ((void*)file);

  delete file;
  return nullptr;
}

void* Interface_Filesystem::open_file_for_write(void* kodiBase, const char* filename, bool overwrite)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || filename == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', filename='%p')", __FUNCTION__, addon, filename);
    return nullptr;
  }
  
  CFile* file = new CFile;
  if (file->OpenForWrite(filename, overwrite))
    return ((void*)file);

  delete file;
  return nullptr;
}

ssize_t Interface_Filesystem::read_file(void* kodiBase, void* file, void* ptr, size_t size)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr || ptr == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p', ptr='%p')", __FUNCTION__, addon, file, ptr);
    return -1;
  }

  return static_cast<CFile*>(file)->Read(ptr, size);
}

bool Interface_Filesystem::read_file_string(void* kodiBase, void* file, char *szLine, int iLineLength)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return false;
  }

  return static_cast<CFile*>(file)->ReadString(szLine, iLineLength);
}

ssize_t Interface_Filesystem::write_file(void* kodiBase, void* file, const void* ptr, size_t size)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr || ptr == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p', ptr='%p')", __FUNCTION__, addon, file, ptr);
    return -1;
  }

  return static_cast<CFile*>(file)->Write(ptr, size);
}

void Interface_Filesystem::flush_file(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return;
  }

  static_cast<CFile*>(file)->Flush();
}

int64_t Interface_Filesystem::seek_file(void* kodiBase, void* file, int64_t position, int whence)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return 0.0f;
  }

  return static_cast<CFile*>(file)->Seek(position, whence);
}

int Interface_Filesystem::truncate_file(void* kodiBase, void* file, int64_t size)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return 0.0f;
  }

  return static_cast<CFile*>(file)->Truncate(size);
}

int64_t Interface_Filesystem::get_file_position(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return 0;
  }

  return static_cast<CFile*>(file)->GetPosition();
}

int64_t Interface_Filesystem::get_file_length(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return 0;
  }
  
  return static_cast<CFile*>(file)->GetLength();
}

double Interface_Filesystem::get_file_download_speed(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return 0.0f;
  }

  return static_cast<CFile*>(file)->GetDownloadSpeed();
}

void Interface_Filesystem::close_file(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return;
  }

  static_cast<CFile*>(file)->Close();
  delete static_cast<CFile*>(file);
}

int Interface_Filesystem::get_file_chunk_size(void* kodiBase, void* file)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_VFS::%s - invalid data (addon='%p', file='%p)", __FUNCTION__, addon, file);
    return 0;
  }

  return static_cast<CFile*>(file)->GetChunkSize();
}

void* Interface_Filesystem::curl_create(void* kodiBase, const char* url)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || url == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', url='%p')", __FUNCTION__, addon, url);
    return nullptr;
  }

  CFile* file = new CFile;
  if (file->CURLCreate(url))
    return static_cast<void*>(file);

  delete file;
  return nullptr;
}

bool Interface_Filesystem::curl_add_option(void* kodiBase, void* file, int type, const char* name, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr || name == nullptr || value == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p', name='%p', value='%p')", __FUNCTION__, addon, file, name, value);
    return false;
  }

  XFILE::CURLOPTIONTYPE internalType;
  switch (type)
  {
  case ADDON_CURL_OPTION_OPTION:
    internalType = XFILE::CURL_OPTION_OPTION;
    break;
  case ADDON_CURL_OPTION_PROTOCOL:
    internalType = XFILE::CURL_OPTION_PROTOCOL;
    break;
  case ADDON_CURL_OPTION_CREDENTIALS:
    internalType = XFILE::CURL_OPTION_CREDENTIALS;
    break;
  case ADDON_CURL_OPTION_HEADER:
    internalType = XFILE::CURL_OPTION_HEADER;
    break;
  default:
    throw std::logic_error("Interface_Filesystem::curl_add_option - invalid curl option type");
    return false;
  };

  CFile* cfile = static_cast<CFile*>(file);
  if (cfile)
    return cfile->CURLAddOption(internalType, name, value);

  return false;
}

bool Interface_Filesystem::curl_open(void* kodiBase, void* file, unsigned int flags)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || file == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Filesystem::%s - invalid data (addon='%p', file='%p')", __FUNCTION__, addon, file);
    return false;
  }

  CFile* cfile = static_cast<CFile*>(file);
  if (cfile)
    return cfile->CURLOpen(flags);

  return false;
}

} /* namespace ADDON */
} /* extern "C" */
