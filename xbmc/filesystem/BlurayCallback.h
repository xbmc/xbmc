/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
