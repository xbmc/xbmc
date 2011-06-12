#pragma once

/*
 *      Copyright (C) 2009 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

#include <archive.h>
#include <archive_entry.h>

class DllLibArchiveInterface
{
public:
  virtual ~DllLibArchiveInterface() {}
  virtual struct archive  *archive_read_new()=0;
  virtual int    archive_read_support_format_all(struct archive *)=0;
  virtual int    archive_read_support_compression_all(struct archive *)=0;
  virtual struct archive  *archive_write_disk_new()=0;
  virtual int    archive_write_disk_set_options(struct archive *, int)=0;
  virtual int  archive_write_disk_set_standard_lookup(struct archive *)=0;
  virtual int    archive_read_open2(struct archive *, void *_client_data,
            archive_open_callback *, archive_read_callback *,
            archive_skip_callback *, archive_close_callback *)=0;
  virtual const char  *archive_error_string(struct archive *)=0;
  virtual int    archive_read_next_header(struct archive *,
            struct archive_entry **)=0;
  virtual const char  *archive_entry_pathname(struct archive_entry *)=0;
  virtual void  archive_entry_set_pathname(struct archive_entry *, const char *)=0;
  virtual int    archive_write_header(struct archive *,
            struct archive_entry *)=0;
  virtual __LA_INT64_T   archive_entry_size(struct archive_entry *)=0;
  virtual int    archive_read_data_block(struct archive *a,
            const void **buff, size_t *size, off_t *offset)=0;
#if ARCHIVE_VERSION_NUMBER < 3000000
  virtual __LA_SSIZE_T   archive_write_data_block(struct archive *,
            const void *, size_t, off_t)=0;
#else
  virtual __LA_SSIZE_T   archive_write_data_block(struct archive *,
            const void *, size_t, __LA_INT64_T)=0;
#endif
  virtual int    archive_write_finish_entry(struct archive *)=0;
  virtual int    archive_read_close(struct archive *)=0;
  virtual int    archive_write_close(struct archive *)=0;
  virtual const struct stat *archive_entry_stat(struct archive_entry *)=0;
};

class DllLibArchive : public DllDynamic, DllLibArchiveInterface
{
  DECLARE_DLL_WRAPPER(DllLibArchive, DLL_PATH_LIBARCHIVE)
  DEFINE_METHOD0(struct archive  *, archive_read_new)
  DEFINE_METHOD1(int, archive_read_support_format_all, (struct archive *p1))
  DEFINE_METHOD1(int, archive_read_support_compression_all, (struct archive *p1))
  DEFINE_METHOD0(struct archive  *, archive_write_disk_new)
  DEFINE_METHOD2(int, archive_write_disk_set_options, (struct archive *p1, int p2))
  DEFINE_METHOD1(int, archive_write_disk_set_standard_lookup, (struct archive *p1))
  DEFINE_METHOD6(int, archive_read_open2, (struct archive *p1, void *p2,
                 archive_open_callback *p3, archive_read_callback *p4,
                 archive_skip_callback *p5, archive_close_callback *p6))
  DEFINE_METHOD1(const char  *, archive_error_string, (struct archive *p1))
  DEFINE_METHOD2(int, archive_read_next_header, (struct archive *p1,
                 struct archive_entry **p2))
  DEFINE_METHOD1(const char  *, archive_entry_pathname, (struct archive_entry *p1))
  DEFINE_METHOD2(void, archive_entry_set_pathname, (struct archive_entry *p1, const char *p2))
  DEFINE_METHOD2(int, archive_write_header, (struct archive *p1,
                 struct archive_entry *p2))
  DEFINE_METHOD1(__LA_INT64_T, archive_entry_size, (struct archive_entry *p1))
  DEFINE_METHOD4(int, archive_read_data_block, (struct archive *p1,
                 const void **p2, size_t *p3, off_t *p4))
#if ARCHIVE_VERSION_NUMBER < 3000000
  DEFINE_METHOD4(__LA_SSIZE_T, archive_write_data_block, (struct archive *p1,
                 const void *p2, size_t p3, off_t p4))
#else
  DEFINE_METHOD4(__LA_SSIZE_T, archive_write_data_block, (struct archive *p1,
                 const void *p2, size_t p3, __LA_INT64_T p4))
#endif
  DEFINE_METHOD1(int, archive_write_finish_entry, (struct archive *p1))
  DEFINE_METHOD1(int, archive_read_close, (struct archive *p1))
  DEFINE_METHOD1(int, archive_write_close, (struct archive *p1))
  DEFINE_METHOD1(const struct stat *, archive_entry_stat, (struct archive_entry *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(archive_read_new)
    RESOLVE_METHOD(archive_read_support_format_all)
    RESOLVE_METHOD(archive_read_support_compression_all)
    RESOLVE_METHOD(archive_write_disk_new)
    RESOLVE_METHOD(archive_write_disk_set_options)
    RESOLVE_METHOD(archive_write_disk_set_standard_lookup)
    RESOLVE_METHOD(archive_read_open2)
    RESOLVE_METHOD(archive_error_string)
    RESOLVE_METHOD(archive_read_next_header)
    RESOLVE_METHOD(archive_entry_pathname)
    RESOLVE_METHOD(archive_entry_set_pathname)
    RESOLVE_METHOD(archive_write_header)
    RESOLVE_METHOD(archive_entry_size)
    RESOLVE_METHOD(archive_read_data_block)
    RESOLVE_METHOD(archive_write_data_block)
    RESOLVE_METHOD(archive_write_finish_entry)
    RESOLVE_METHOD(archive_read_close)
    RESOLVE_METHOD(archive_write_close)
    RESOLVE_METHOD(archive_entry_stat)
  END_METHOD_RESOLVE()
};
