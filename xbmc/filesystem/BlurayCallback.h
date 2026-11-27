/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <mutex>

#include <libbluray/bluray.h>
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

  /*!
   \brief Held while a disc is opened, so that one disc is opened at a time. libbluray parses a
   disc's metadata with libxml2 as it opens it, and then calls xmlCleanupParser(), which frees
   libxml2's global state under any parse another thread is making.
   */
  static std::mutex& OpenLock();

  /*!
   \brief bd_init(), with libbluray's logging set up. Its handler and mask are process-wide and
   unguarded, so they are set once, before any disc logs, and libbluray sets the rest of its
   logging up on the first message, which bd_init() logs: one bd_init() runs at a time.
   */
  static BLURAY* Init();

private:
  CBlurayCallback() = default;
  ~CBlurayCallback() = default;
};
