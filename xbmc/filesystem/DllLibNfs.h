#pragma once

/*
 *      Copyright (C) 2011 Team XBMC
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

#include "DynamicDll.h"

#ifdef __cplusplus
extern "C" {
#endif  
#include <nfsc/libnfs.h>
#ifdef __cplusplus
}
#endif

class DllLibNfsInterface
{
public:
  virtual ~DllLibNfsInterface() {}

  virtual struct nfs_context *nfs_init_context(void)=0;
  virtual void nfs_destroy_context(struct nfs_context *nfs)=0;  
  virtual size_t nfs_get_readmax(struct nfs_context *nfs)=0;  
  virtual size_t nfs_get_writemax(struct nfs_context *nfs)=0;
  virtual char *nfs_get_error(struct nfs_context *nfs)=0;  
  virtual int nfs_close_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh)=0;
  virtual int nfs_fsync_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh)=0;  
  virtual int nfs_mkdir_sync(struct nfs_context *nfs,     const char *path)=0;
  virtual int nfs_rmdir_sync(struct nfs_context *nfs,     const char *path)=0;
  virtual int nfs_unlink_sync(struct nfs_context *nfs,    const char *path)=0;
  virtual void nfs_closedir(struct nfs_context *nfs,      struct nfsdir *nfsdir)=0;      
  virtual struct nfsdirent *nfs_readdir(struct nfs_context *nfs, struct nfsdir *nfsdir)=0;  
  virtual int nfs_mount_sync(struct nfs_context *nfs,     const char *server,   const char *exportname)=0;
  virtual int nfs_stat_sync(struct nfs_context *nfs,      const char *path,     struct stat *st)=0;
  virtual int nfs_fstat_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh,  struct stat *st)=0;
  virtual int nfs_truncate_sync(struct nfs_context *nfs,  const char *path,     off_t length)=0;
  virtual int nfs_ftruncate_sync(struct nfs_context *nfs, struct nfsfh *nfsfh,  off_t length)=0;
  virtual int nfs_opendir_sync(struct nfs_context *nfs,   const char *path,     struct nfsdir **nfsdir)=0;
  virtual int nfs_statvfs_sync(struct nfs_context *nfs,   const char *path,     struct statvfs *svfs)=0;
  virtual int nfs_chmod_sync(struct nfs_context *nfs,     const char *path,     int mode)=0;
  virtual int nfs_fchmod_sync(struct nfs_context *nfs,    struct nfsfh *nfsfh,  int mode)=0;
  virtual int nfs_access_sync(struct nfs_context *nfs,    const char *path,     int mode)=0;
  virtual int nfs_utimes_sync(struct nfs_context *nfs,    const char *path,     struct timeval *times)=0;
  virtual int nfs_utime_sync(struct nfs_context *nfs,     const char *path,     struct utimbuf *times)=0;
  virtual int nfs_symlink_sync(struct nfs_context *nfs,   const char *oldpath,  const char *newpath)=0;
  virtual int nfs_rename_sync(struct nfs_context *nfs,    const char *oldpath,  const char *newpath)=0;
  virtual int nfs_link_sync(struct nfs_context *nfs,      const char *oldpath,  const char *newpath)=0;  
  virtual int nfs_readlink_sync(struct nfs_context *nfs,  const char *path,     char *buf,    int bufsize)=0;
  virtual int nfs_chown_sync(struct nfs_context *nfs,     const char *path,     int uid,      int gid)=0;
  virtual int nfs_fchown_sync(struct nfs_context *nfs,    struct nfsfh *nfsfh,  int uid,      int gid)=0;
  virtual int nfs_open_sync(struct nfs_context *nfs,      const char *path,     int mode,     struct nfsfh **nfsfh)=0;  
  virtual int nfs_read_sync(struct nfs_context *nfs,      struct nfsfh *nfsfh,  size_t count, char *buf)=0;  
  virtual int nfs_write_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh,  size_t count, char *buf)=0;
  virtual int nfs_creat_sync(struct nfs_context *nfs,     const char *path,     int mode,     struct nfsfh **nfsfh)=0;  
  virtual int nfs_pread_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh,  off_t offset, size_t count, char *buf)=0;
  virtual int nfs_pwrite_sync(struct nfs_context *nfs,    struct nfsfh *nfsfh,  off_t offset, size_t count, char *buf)=0;
  virtual int nfs_lseek_sync(struct nfs_context *nfs,     struct nfsfh *nfsfh,  off_t offset, int whence,   off_t *current_offset)=0;
};

class DllLibNfs : public DllDynamic, DllLibNfsInterface
{
  DECLARE_DLL_WRAPPER(DllLibNfs, DLL_PATH_LIBNFS)
  DEFINE_METHOD0(struct   nfs_context *, nfs_init_context)
  DEFINE_METHOD1(void,    nfs_destroy_context,              (struct nfs_context *p1))
  DEFINE_METHOD1(size_t,  nfs_get_readmax,                  (struct nfs_context *p1))
  DEFINE_METHOD1(size_t,  nfs_get_writemax,                 (struct nfs_context *p1)) 
  DEFINE_METHOD1(char *, nfs_get_error,   (struct nfs_context *p1))    
  DEFINE_METHOD2(struct nfsdirent *, nfs_readdir, (struct nfs_context *p1, struct nfsdir *p2))  
  DEFINE_METHOD2(int, nfs_fsync_sync,     (struct nfs_context *p1, struct nfsfh *p2))
  DEFINE_METHOD2(int, nfs_mkdir_sync,     (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(int, nfs_rmdir_sync,     (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(int, nfs_unlink_sync,    (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(void,nfs_closedir,       (struct nfs_context *p1, struct nfsdir *p2))        
  DEFINE_METHOD2(int, nfs_close_sync,     (struct nfs_context *p1, struct nfsfh *p2)) 
  DEFINE_METHOD3(int, nfs_mount_sync,     (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_stat_sync,      (struct nfs_context *p1, const char *p2,    struct stat *p3))
  DEFINE_METHOD3(int, nfs_fstat_sync,     (struct nfs_context *p1, struct nfsfh *p2,  struct stat *p3))
  DEFINE_METHOD3(int, nfs_truncate_sync,  (struct nfs_context *p1, const char *p2,    off_t p3))
  DEFINE_METHOD3(int, nfs_ftruncate_sync, (struct nfs_context *p1, struct nfsfh *p2,  off_t p3))
  DEFINE_METHOD3(int, nfs_opendir_sync,   (struct nfs_context *p1, const char *p2,    struct nfsdir **p3))
  DEFINE_METHOD3(int, nfs_statvfs_sync,   (struct nfs_context *p1, const char *p2,    struct statvfs *p3))
  DEFINE_METHOD3(int, nfs_chmod_sync,     (struct nfs_context *p1, const char *p2,    int p3))
  DEFINE_METHOD3(int, nfs_fchmod_sync,    (struct nfs_context *p1, struct nfsfh *p2,  int p3))
  DEFINE_METHOD3(int, nfs_utimes_sync,    (struct nfs_context *p1, const char *p2,    struct timeval *p3))
  DEFINE_METHOD3(int, nfs_utime_sync,     (struct nfs_context *p1, const char *p2,    struct utimbuf *p3))
  DEFINE_METHOD3(int, nfs_access_sync,    (struct nfs_context *p1, const char *p2,    int p3))
  DEFINE_METHOD3(int, nfs_symlink_sync,   (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_rename_sync,    (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_link_sync,      (struct nfs_context *p1, const char *p2,    const char *p3))  
  DEFINE_METHOD4(int, nfs_open_sync,      (struct nfs_context *p1, const char *p2,    int p3,     struct nfsfh **p4))
  DEFINE_METHOD4(int, nfs_read_sync,      (struct nfs_context *p1, struct nfsfh *p2,  size_t p3,  char *p4))
  DEFINE_METHOD4(int, nfs_write_sync,     (struct nfs_context *p1, struct nfsfh *p2,  size_t p3,  char *p4))
  DEFINE_METHOD4(int, nfs_creat_sync,     (struct nfs_context *p1, const char *p2,    int p3,     struct nfsfh **p4))
  DEFINE_METHOD4(int, nfs_readlink_sync,  (struct nfs_context *p1, const char *p2,    char *p3,   int p4))
  DEFINE_METHOD4(int, nfs_chown_sync,     (struct nfs_context *p1, const char *p2,    int p3,     int p4))
  DEFINE_METHOD4(int, nfs_fchown_sync,    (struct nfs_context *p1, struct nfsfh *p2,  int p3,     int p4))
  DEFINE_METHOD5(int, nfs_pread_sync,     (struct nfs_context *p1, struct nfsfh *p2,  off_t p3,   size_t p4,  char *p5))
  DEFINE_METHOD5(int, nfs_pwrite_sync,    (struct nfs_context *p1, struct nfsfh *p2,  off_t p3,   size_t p4,  char *p5))
  DEFINE_METHOD5(int, nfs_lseek_sync,     (struct nfs_context *p1, struct nfsfh *p2,  off_t p3,   int p4,     off_t *p5))



  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(nfs_init_context,   nfs_init_context)
    RESOLVE_METHOD_RENAME(nfs_destroy_context,nfs_destroy_context)
    RESOLVE_METHOD_RENAME(nfs_get_readmax,    nfs_get_readmax)
    RESOLVE_METHOD_RENAME(nfs_get_writemax,   nfs_get_writemax)   
    RESOLVE_METHOD_RENAME(nfs_get_error,      nfs_get_error)
    RESOLVE_METHOD_RENAME(nfs_readdir,        nfs_readdir)    
    RESOLVE_METHOD_RENAME(nfs_closedir,       nfs_closedir)  
    RESOLVE_METHOD_RENAME(nfs_mount_sync,     nfs_mount_sync)
    RESOLVE_METHOD_RENAME(nfs_stat_sync,      nfs_stat_sync)
    RESOLVE_METHOD_RENAME(nfs_fstat_sync,     nfs_fstat_sync)
    RESOLVE_METHOD_RENAME(nfs_open_sync,      nfs_open_sync)
    RESOLVE_METHOD_RENAME(nfs_close_sync,     nfs_close_sync)
    RESOLVE_METHOD_RENAME(nfs_pread_sync,     nfs_pread_sync)
    RESOLVE_METHOD_RENAME(nfs_read_sync,      nfs_read_sync)
    RESOLVE_METHOD_RENAME(nfs_pwrite_sync,    nfs_pwrite_sync)
    RESOLVE_METHOD_RENAME(nfs_write_sync,     nfs_write_sync)
    RESOLVE_METHOD_RENAME(nfs_lseek_sync,     nfs_lseek_sync)
    RESOLVE_METHOD_RENAME(nfs_fsync_sync,     nfs_fsync_sync)
    RESOLVE_METHOD_RENAME(nfs_truncate_sync,  nfs_truncate_sync)
    RESOLVE_METHOD_RENAME(nfs_ftruncate_sync, nfs_ftruncate_sync)
    RESOLVE_METHOD_RENAME(nfs_mkdir_sync,     nfs_mkdir_sync)
    RESOLVE_METHOD_RENAME(nfs_rmdir_sync,     nfs_rmdir_sync)
    RESOLVE_METHOD_RENAME(nfs_creat_sync,     nfs_creat_sync)
    RESOLVE_METHOD_RENAME(nfs_unlink_sync,    nfs_unlink_sync)
    RESOLVE_METHOD_RENAME(nfs_opendir_sync,   nfs_opendir_sync)
    RESOLVE_METHOD_RENAME(nfs_statvfs_sync,   nfs_statvfs_sync)
    RESOLVE_METHOD_RENAME(nfs_readlink_sync,  nfs_readlink_sync)
    RESOLVE_METHOD_RENAME(nfs_chmod_sync,     nfs_chmod_sync)
    RESOLVE_METHOD_RENAME(nfs_fchmod_sync,    nfs_fchmod_sync)
    RESOLVE_METHOD_RENAME(nfs_chown_sync,     nfs_chown_sync)
    RESOLVE_METHOD_RENAME(nfs_fchown_sync,    nfs_fchown_sync)
    RESOLVE_METHOD_RENAME(nfs_utimes_sync,    nfs_utimes_sync)
    RESOLVE_METHOD_RENAME(nfs_utime_sync,     nfs_utime_sync)
    RESOLVE_METHOD_RENAME(nfs_access_sync,    nfs_access_sync)
    RESOLVE_METHOD_RENAME(nfs_symlink_sync,   nfs_symlink_sync)
    RESOLVE_METHOD_RENAME(nfs_rename_sync,    nfs_rename_sync)
    RESOLVE_METHOD_RENAME(nfs_link_sync,      nfs_link_sync)      
  END_METHOD_RESOLVE()
};

