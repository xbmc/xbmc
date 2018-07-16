/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BlurayCallback.h"

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

struct SDirState
{
  CFileItemList list;
  int curr = 0;
};

void CBlurayCallback::bluray_logger(const char* msg)
{
  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray::Logger - %s", msg);
}

void CBlurayCallback::dir_close(BD_DIR_H *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Closed dir (%p)\n", static_cast<void*>(dir));
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
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening dir, null handle!");
    return NULL;
  }

  std::string strDirname = URIUtils::AddFileToFolder(*strBasePath, strRelPath);
  if (URIUtils::HasSlashAtEnd(strDirname))
    URIUtils::RemoveSlashAtEnd(strDirname);

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Opening dir %s\n", CURL::GetRedacted(strDirname).c_str());

  SDirState *st = new SDirState();
  if (!CDirectory::GetDirectory(strDirname, st->list, "", DIR_FLAG_DEFAULTS))
  {
    if (!CFile::Exists(strDirname))
      CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening dir! (%s)\n", CURL::GetRedacted(strDirname).c_str());
    delete st;
    return NULL;
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

  strncpy(entry->d_name, state->list[state->curr]->GetLabel().c_str(), sizeof(entry->d_name));
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
    CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening dir, null handle!");
    return NULL;
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

  CLog::Log(LOGDEBUG, "CDVDInputStreamBluray - Error opening file! (%s)", CURL::GetRedacted(strFilename).c_str());

  delete fp;
  delete file;

  return NULL;
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
