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

#pragma once

#include <libbluray/filesystem.h>

class CBlurayCallback
{
public:
  static void bluray_logger(const char* msg);
  static void dir_close(BD_DIR_H* dir);
  static BD_DIR_H* dir_open(void*  handle, const char*  rel_path);
  static int dir_read(BD_DIR_H* dir, BD_DIRENT* entry);
  static void file_close(BD_FILE_H* file);
  static int file_eof(BD_FILE_H* file);
  static BD_FILE_H* file_open(void*  handle, const char*  rel_path);
  static int64_t file_read(BD_FILE_H* file, uint8_t* buf, int64_t size);
  static int64_t file_seek(BD_FILE_H* file, int64_t offset, int32_t origin);
  static int64_t file_tell(BD_FILE_H* file);
  static int64_t file_write(BD_FILE_H* file, const uint8_t* buf, int64_t size);

private:
  CBlurayCallback() = default;
  ~CBlurayCallback() = default;
};
