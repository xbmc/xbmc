/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayCallback.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

struct SDirState
{
  CFileItemList list;
  int curr = 0;
};

void CBlurayCallback::bluray_logger(const char* msg)
{
  CLog::Log(LOGDEBUG, "CBlurayCallback::Logger - {}", msg);
}

void CBlurayCallback::dir_close(BD_DIR_H *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CBlurayCallback - Closed dir ({})", fmt::ptr(dir));
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}

BD_DIR_H* CBlurayCallback::dir_open(void *handle, const char* rel_path)
{
  std::string strRelPath(rel_path);
  std::string* strBasePath = reinterpret_cast<std::string*>(handle);
  if (!strBasePath)
  {
    CLog::Log(LOGDEBUG, "CBlurayCallback - Error opening dir, null handle!");
    return nullptr;
  }

  std::string strDirname = URIUtils::AddFileToFolder(*strBasePath, strRelPath);
  if (URIUtils::HasSlashAtEnd(strDirname))
    URIUtils::RemoveSlashAtEnd(strDirname);

  CLog::Log(LOGDEBUG, "CBlurayCallback - Opening dir {}", CURL::GetRedacted(strDirname));

  SDirState *st = new SDirState();
  if (!CDirectory::GetDirectory(strDirname, st->list, "", DIR_FLAG_DEFAULTS))
  {
    if (!CFile::Exists(strDirname))
      CLog::Log(LOGDEBUG, "CBlurayCallback - Error opening dir! ({})",
                CURL::GetRedacted(strDirname));
    delete st;
    return nullptr;
  }

  BD_DIR_H *dir = new BD_DIR_H;
  dir->close = dir_close;
  dir->read = dir_read;
  dir->internal = (void*)st;

  return dir;
}

int CBlurayCallback::dir_read(BD_DIR_H *dir, BD_DIRENT *entry)
{
  SDirState* state = static_cast<SDirState*>(dir->internal);

  if (state->curr >= state->list.Size())
    return 1;

  strncpy(entry->d_name, state->list[state->curr]->GetLabel().c_str(), sizeof(entry->d_name) - 1);
  entry->d_name[sizeof(entry->d_name) - 1] = 0;
  state->curr++;

  return 0;
}

void CBlurayCallback::file_close(BD_FILE_H *file)
{
  if (file)
  {
    delete static_cast<CFile*>(file->internal);
    delete file;
  }
}

int CBlurayCallback::file_eof(BD_FILE_H *file)
{
  if (static_cast<CFile*>(file->internal)->GetPosition() == static_cast<CFile*>(file->internal)->GetLength())
    return 1;
  else
    return 0;
}

BD_FILE_H * CBlurayCallback::file_open(void *handle, const char *rel_path)
{
  std::string strRelPath(rel_path);
  std::string* strBasePath = reinterpret_cast<std::string*>(handle);
  if (!strBasePath)
  {
    CLog::Log(LOGDEBUG, "CBlurayCallback - Error opening dir, null handle!");
    return nullptr;
  }

  std::string strFilename = URIUtils::AddFileToFolder(*strBasePath, strRelPath);

  BD_FILE_H *file = new BD_FILE_H;

  file->close = file_close;
  file->seek = file_seek;
  file->read = file_read;
  file->write = file_write;
  file->tell = file_tell;
  file->eof = file_eof;

  CFile* fp = new CFile();
  if (fp->Open(strFilename))
  {
    file->internal = (void*)fp;
    return file;
  }

  CLog::Log(LOGDEBUG, "CBlurayCallback - Error opening file! ({})", CURL::GetRedacted(strFilename));

  delete fp;
  delete file;

  return nullptr;
}

int64_t CBlurayCallback::file_seek(BD_FILE_H *file, int64_t offset, int32_t origin)
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

int64_t CBlurayCallback::file_tell(BD_FILE_H *file)
{
  return static_cast<CFile*>(file->internal)->GetPosition();
}

int64_t CBlurayCallback::file_read(BD_FILE_H *file, uint8_t *buf, int64_t size)
{
  return static_cast<int64_t>(static_cast<CFile*>(file->internal)->Read(buf, static_cast<size_t>(size)));
}

int64_t CBlurayCallback::file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size)
{
  return static_cast<int64_t>(static_cast<CFile*>(file->internal)->Write(buf, static_cast<size_t>(size)));
}
