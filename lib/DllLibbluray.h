/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#pragma once
#include "system.h"
#ifdef HAVE_LIBBLURAY

#include "DynamicDll.h"


extern "C"
{
#include <libbluray/bluray.h>
#include <libbluray/filesystem.h>
#ifndef HAVE_LIBBLURAY_NOLOGCONTROL
#include <libbluray/log_control.h>
#endif
}

class DllLibblurayInterface
{
public:
  virtual ~DllLibblurayInterface() {};
  virtual uint32_t bd_get_titles(BLURAY *bd, uint8_t flags, uint32_t min_title_length)=0;
  virtual BLURAY_TITLE_INFO* bd_get_title_info(BLURAY *bd, uint32_t title_idx, unsigned angle)=0;
  virtual BLURAY_TITLE_INFO* bd_get_playlist_info(BLURAY *bd, uint32_t playlist, unsigned angle)=0;
  virtual void bd_free_title_info(BLURAY_TITLE_INFO *title_info)=0;
  virtual BLURAY *bd_open(const char* device_path, const char* keyfile_path)=0;
  virtual void bd_close(BLURAY *bd)=0;
  virtual int64_t bd_seek(BLURAY *bd, uint64_t pos)=0;
  virtual int64_t bd_seek_time(BLURAY *bd, uint64_t tick)=0;
  virtual int bd_read(BLURAY *bd, unsigned char *buf, int len)=0;
  virtual int64_t bd_seek_chapter(BLURAY *bd, unsigned chapter)=0;
  virtual int64_t bd_chapter_pos(BLURAY *bd, unsigned chapter)=0;
  virtual uint32_t bd_get_current_chapter(BLURAY *bd)=0;
  virtual int64_t bd_seek_mark(BLURAY *bd, unsigned mark)=0;
  virtual int bd_select_playlist(BLURAY *bd, uint32_t playlist)=0;
  virtual int bd_select_title(BLURAY *bd, uint32_t title)=0;
  virtual int bd_select_angle(BLURAY *bd, unsigned angle)=0;
  virtual void bd_seamless_angle_change(BLURAY *bd, unsigned angle)=0;
  virtual uint64_t bd_get_title_size(BLURAY *bd)=0;
  virtual uint32_t bd_get_current_title(BLURAY *bd)=0;
  virtual unsigned bd_get_current_angle(BLURAY *bd)=0;
  virtual uint64_t bd_tell(BLURAY *bd)=0;
  virtual uint64_t bd_tell_time(BLURAY *bd)=0;
  virtual BD_FILE_OPEN bd_register_file(BD_FILE_OPEN p)=0;
  virtual BD_DIR_OPEN bd_register_dir(BD_DIR_OPEN p)=0;

#ifndef HAVE_LIBBLURAY_NOLOGCONTROL
  virtual void     bd_set_debug_handler(BD_LOG_FUNC)=0;
  virtual void     bd_set_debug_mask(uint32_t mask)=0;
  virtual uint32_t bd_get_debug_mask(void)=0;
#endif
};

class DllLibbluray : public DllDynamic, DllLibblurayInterface
{
  DECLARE_DLL_WRAPPER(DllLibbluray, DLL_PATH_LIBBLURAY)
#ifdef HAVE_LIBBBLURAY_HAVE_LIBBLURAY_NOANGLE
  DEFINE_METHOD3(uint32_t,            bd_get_titles,          (BLURAY *p1, uint8_t p2))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_title_info,      (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_playlist_info,   (BLURAY *p1, uint32_t p2))
#else
  DEFINE_METHOD3(uint32_t,            bd_get_titles,          (BLURAY *p1, uint8_t p2, uint32_t p3))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_title_info,      (BLURAY *p1, uint32_t p2, unsigned p3))
  DEFINE_METHOD3(BLURAY_TITLE_INFO*,  bd_get_playlist_info,   (BLURAY *p1, uint32_t p2, unsigned p3))
#endif
  DEFINE_METHOD1(void,                bd_free_title_info,     (BLURAY_TITLE_INFO *p1))
  DEFINE_METHOD2(BLURAY*,             bd_open,                (const char* p1, const char* p2))
  DEFINE_METHOD1(void,                bd_close,               (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek,                (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD2(int64_t,             bd_seek_time,           (BLURAY *p1, uint64_t p2))
  DEFINE_METHOD3(int,                 bd_read,                (BLURAY *p1, unsigned char *p2, int p3))
  DEFINE_METHOD2(int64_t,             bd_seek_chapter,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int64_t,             bd_chapter_pos,         (BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint32_t,            bd_get_current_chapter, (BLURAY *p1))
  DEFINE_METHOD2(int64_t,             bd_seek_mark,           (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(int,                 bd_select_playlist,     (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_title,        (BLURAY *p1, uint32_t p2))
  DEFINE_METHOD2(int,                 bd_select_angle,        (BLURAY *p1, unsigned p2))
  DEFINE_METHOD2(void,                bd_seamless_angle_change,(BLURAY *p1, unsigned p2))
  DEFINE_METHOD1(uint64_t,            bd_get_title_size,      (BLURAY *p1))
  DEFINE_METHOD1(uint32_t,            bd_get_current_title,   (BLURAY *p1))
  DEFINE_METHOD1(unsigned,            bd_get_current_angle,   (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell,                (BLURAY *p1))
  DEFINE_METHOD1(uint64_t,            bd_tell_time,           (BLURAY *p1))
  DEFINE_METHOD1(BD_FILE_OPEN,        bd_register_file,       (BD_FILE_OPEN p1))
  DEFINE_METHOD1(BD_DIR_OPEN,         bd_register_dir,        (BD_DIR_OPEN p1))

#ifndef HAVE_LIBBLURAY_NOLOGCONTROL
  DEFINE_METHOD1(void,                bd_set_debug_handler,   (BD_LOG_FUNC p1))
  DEFINE_METHOD1(void,                bd_set_debug_mask,      (uint32_t p1))
  DEFINE_METHOD0(uint32_t,            bd_get_debug_mask)
#endif

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(bd_get_titles)
    RESOLVE_METHOD(bd_get_title_info)
    RESOLVE_METHOD(bd_get_playlist_info)
    RESOLVE_METHOD_RENAME(bd_free_title_info,   bd_free_title_info)
    RESOLVE_METHOD_RENAME(bd_open,              bd_open)
    RESOLVE_METHOD_RENAME(bd_close,             bd_close)
    RESOLVE_METHOD_RENAME(bd_seek,              bd_seek)
    RESOLVE_METHOD_RENAME(bd_seek_time,         bd_seek_time)
    RESOLVE_METHOD_RENAME(bd_read,              bd_read)
    RESOLVE_METHOD_RENAME(bd_seek_chapter,      bd_seek_chapter)
    RESOLVE_METHOD_RENAME(bd_chapter_pos,       bd_chapter_pos)
    RESOLVE_METHOD_RENAME(bd_get_current_chapter, bd_get_current_chapter)
    RESOLVE_METHOD_RENAME(bd_seek_mark,         bd_seek_mark)
    RESOLVE_METHOD_RENAME(bd_select_playlist,   bd_select_playlist)
    RESOLVE_METHOD_RENAME(bd_select_title,      bd_select_title)
    RESOLVE_METHOD_RENAME(bd_select_angle,      bd_select_angle)
    RESOLVE_METHOD_RENAME(bd_seamless_angle_change, bd_seamless_angle_change)
    RESOLVE_METHOD_RENAME(bd_get_title_size,    bd_get_title_size)
    RESOLVE_METHOD_RENAME(bd_get_current_title, bd_get_current_title)
    RESOLVE_METHOD_RENAME(bd_get_current_angle, bd_get_current_angle)
    RESOLVE_METHOD_RENAME(bd_tell,              bd_tell)
    RESOLVE_METHOD_RENAME(bd_tell_time,         bd_tell_time)
    RESOLVE_METHOD_RENAME(bd_register_file,     bd_register_file)
    RESOLVE_METHOD_RENAME(bd_register_dir,      bd_register_dir)
#ifndef HAVE_LIBBLURAY_NOLOGCONTROL
    RESOLVE_METHOD(bd_set_debug_handler)
    RESOLVE_METHOD(bd_set_debug_mask)
    RESOLVE_METHOD(bd_get_debug_mask)
#endif
  END_METHOD_RESOLVE()

#ifdef HAVE_LIBBBLURAY_HAVE_LIBBLURAY_NOANGLE
  uint32_t bd_get_titles(BLURAY *bd, uint8_t flags, uint32_t min_title_length)
      {return bd_get_titles_noangle(bd, flags);           }
  BLURAY_TITLE_INFO* bd_get_title_info(BLURAY *bd, uint32_t title_idx, unsigned angle)
      {return bd_get_title_info_noangle(bd, title_idx);   }
  BLURAY_TITLE_INFO* bd_get_playlist_info(BLURAY *bd, uint32_t playlist, unsigned angle)
      {return bd_get_playlist_info_noangle(bd, playlist); }
#endif

public:
  static void       file_close(BD_FILE_H *file);
  static int64_t    file_seek(BD_FILE_H *file, int64_t offset, int32_t origin);
  static int64_t    file_tell(BD_FILE_H *file);
  static int        file_eof(BD_FILE_H *file);
  static int64_t    file_read(BD_FILE_H *file, uint8_t *buf, int64_t size);
  static int64_t    file_write(BD_FILE_H *file, const uint8_t *buf, int64_t size);
  static BD_FILE_H *file_open(const char* filename, const char *mode);
  static void      dir_close(BD_DIR_H *dir);
  static int       dir_read(BD_DIR_H *dir, BD_DIRENT *entry);
  static BD_DIR_H *dir_open(const char* dirname);
  static void      bluray_logger(const char* msg);
};

#endif
