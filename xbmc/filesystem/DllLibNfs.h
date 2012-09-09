#pragma once

/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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

//  virtual struct rpc_context *rpc_init_context(void)=0;
//  virtual void rpc_destroy_context(struct rpc_context *rpc)=0;
  virtual void mount_free_export_list(struct exportnode *exports)=0;
  virtual struct exportnode *mount_getexports(const char *server)=0;
  virtual struct nfs_server_list *nfs_find_local_servers(void)=0;
  virtual void   free_nfs_srvr_list(struct nfs_server_list *srv)=0;
  virtual struct nfs_context *nfs_init_context(void)=0;
  virtual void nfs_destroy_context(struct nfs_context *nfs)=0;  
  virtual uint64_t nfs_get_readmax(struct nfs_context *nfs)=0;  
  virtual uint64_t nfs_get_writemax(struct nfs_context *nfs)=0;
  virtual char *nfs_get_error(struct nfs_context *nfs)=0;  
  virtual int nfs_close(struct nfs_context *nfs,     struct nfsfh *nfsfh)=0;
  virtual int nfs_fsync(struct nfs_context *nfs,     struct nfsfh *nfsfh)=0;  
  virtual int nfs_mkdir(struct nfs_context *nfs,     const char *path)=0;
  virtual int nfs_rmdir(struct nfs_context *nfs,     const char *path)=0;
  virtual int nfs_unlink(struct nfs_context *nfs,    const char *path)=0;
  virtual void nfs_closedir(struct nfs_context *nfs,      struct nfsdir *nfsdir)=0;      
  virtual struct nfsdirent *nfs_readdir(struct nfs_context *nfs, struct nfsdir *nfsdir)=0;  
  virtual int nfs_mount(struct nfs_context *nfs,     const char *server,   const char *exportname)=0;
  virtual int nfs_stat(struct nfs_context *nfs,      const char *path,     struct stat *st)=0;
  virtual int nfs_fstat(struct nfs_context *nfs,     struct nfsfh *nfsfh,  struct stat *st)=0;
  virtual int nfs_truncate(struct nfs_context *nfs,  const char *path,     uint64_t length)=0;
  virtual int nfs_ftruncate(struct nfs_context *nfs, struct nfsfh *nfsfh,  uint64_t length)=0;
  virtual int nfs_opendir(struct nfs_context *nfs,   const char *path,     struct nfsdir **nfsdir)=0;
  virtual int nfs_statvfs(struct nfs_context *nfs,   const char *path,     struct statvfs *svfs)=0;
  virtual int nfs_chmod(struct nfs_context *nfs,     const char *path,     int mode)=0;
  virtual int nfs_fchmod(struct nfs_context *nfs,    struct nfsfh *nfsfh,  int mode)=0;
  virtual int nfs_access(struct nfs_context *nfs,    const char *path,     int mode)=0;
  virtual int nfs_utimes(struct nfs_context *nfs,    const char *path,     struct timeval *times)=0;
  virtual int nfs_utime(struct nfs_context *nfs,     const char *path,     struct utimbuf *times)=0;
  virtual int nfs_symlink(struct nfs_context *nfs,   const char *oldpath,  const char *newpath)=0;
  virtual int nfs_rename(struct nfs_context *nfs,    const char *oldpath,  const char *newpath)=0;
  virtual int nfs_link(struct nfs_context *nfs,      const char *oldpath,  const char *newpath)=0;  
  virtual int nfs_readlink(struct nfs_context *nfs,  const char *path,     char *buf,    int bufsize)=0;
  virtual int nfs_chown(struct nfs_context *nfs,     const char *path,     int uid,      int gid)=0;
  virtual int nfs_fchown(struct nfs_context *nfs,    struct nfsfh *nfsfh,  int uid,      int gid)=0;
  virtual int nfs_open(struct nfs_context *nfs,      const char *path,     int mode,     struct nfsfh **nfsfh)=0;  
  virtual int nfs_read(struct nfs_context *nfs,      struct nfsfh *nfsfh,  uint64_t count, char *buf)=0;  
  virtual int nfs_write(struct nfs_context *nfs,     struct nfsfh *nfsfh,  uint64_t count, char *buf)=0;
  virtual int nfs_creat(struct nfs_context *nfs,     const char *path,     int mode,     struct nfsfh **nfsfh)=0;  
  virtual int nfs_pread(struct nfs_context *nfs,     struct nfsfh *nfsfh,  uint64_t offset, uint64_t count, char *buf)=0;
  virtual int nfs_pwrite(struct nfs_context *nfs,    struct nfsfh *nfsfh,  uint64_t offset, uint64_t count, char *buf)=0;
  virtual int nfs_lseek(struct nfs_context *nfs,     struct nfsfh *nfsfh,  uint64_t offset, int whence,   uint64_t *current_offset)=0;
};

class DllLibNfs : public DllDynamic, DllLibNfsInterface
{
  DECLARE_DLL_WRAPPER(DllLibNfs, DLL_PATH_LIBNFS)
  DEFINE_METHOD0(struct   nfs_context *, nfs_init_context)
  DEFINE_METHOD0(struct nfs_server_list *, nfs_find_local_servers)
  DEFINE_METHOD1(void, free_nfs_srvr_list, (struct nfs_server_list *p1))
  DEFINE_METHOD1(struct exportnode *, mount_getexports,     (const char *p1))
  DEFINE_METHOD1(void,    mount_free_export_list,           (struct exportnode *p1))
  DEFINE_METHOD1(void,    nfs_destroy_context,              (struct nfs_context *p1))
  DEFINE_METHOD1(uint64_t,  nfs_get_readmax,                  (struct nfs_context *p1))
  DEFINE_METHOD1(uint64_t,  nfs_get_writemax,                 (struct nfs_context *p1)) 
  DEFINE_METHOD1(char *,  nfs_get_error,                    (struct nfs_context *p1))    
  DEFINE_METHOD2(struct nfsdirent *, nfs_readdir,           (struct nfs_context *p1, struct nfsdir *p2))
  DEFINE_METHOD2(int, nfs_fsync,     (struct nfs_context *p1, struct nfsfh *p2))
  DEFINE_METHOD2(int, nfs_mkdir,     (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(int, nfs_rmdir,     (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(int, nfs_unlink,    (struct nfs_context *p1, const char *p2))
  DEFINE_METHOD2(void,nfs_closedir,  (struct nfs_context *p1, struct nfsdir *p2))        
  DEFINE_METHOD2(int, nfs_close,     (struct nfs_context *p1, struct nfsfh *p2)) 
  DEFINE_METHOD3(int, nfs_mount,     (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_stat,      (struct nfs_context *p1, const char *p2,    struct stat *p3))
  DEFINE_METHOD3(int, nfs_fstat,     (struct nfs_context *p1, struct nfsfh *p2,  struct stat *p3))
  DEFINE_METHOD3(int, nfs_truncate,  (struct nfs_context *p1, const char *p2,    uint64_t p3))
  DEFINE_METHOD3(int, nfs_ftruncate, (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3))
  DEFINE_METHOD3(int, nfs_opendir,   (struct nfs_context *p1, const char *p2,    struct nfsdir **p3))
  DEFINE_METHOD3(int, nfs_statvfs,   (struct nfs_context *p1, const char *p2,    struct statvfs *p3))
  DEFINE_METHOD3(int, nfs_chmod,     (struct nfs_context *p1, const char *p2,    int p3))
  DEFINE_METHOD3(int, nfs_fchmod,    (struct nfs_context *p1, struct nfsfh *p2,  int p3))
  DEFINE_METHOD3(int, nfs_utimes,    (struct nfs_context *p1, const char *p2,    struct timeval *p3))
  DEFINE_METHOD3(int, nfs_utime,     (struct nfs_context *p1, const char *p2,    struct utimbuf *p3))
  DEFINE_METHOD3(int, nfs_access,    (struct nfs_context *p1, const char *p2,    int p3))
  DEFINE_METHOD3(int, nfs_symlink,   (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_rename,    (struct nfs_context *p1, const char *p2,    const char *p3))
  DEFINE_METHOD3(int, nfs_link,      (struct nfs_context *p1, const char *p2,    const char *p3))  
  DEFINE_METHOD4(int, nfs_open,      (struct nfs_context *p1, const char *p2,    int p3,     struct nfsfh **p4))
  DEFINE_METHOD4(int, nfs_read,      (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3,  char *p4))
  DEFINE_METHOD4(int, nfs_write,     (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3,  char *p4))
  DEFINE_METHOD4(int, nfs_creat,     (struct nfs_context *p1, const char *p2,    int p3,     struct nfsfh **p4))
  DEFINE_METHOD4(int, nfs_readlink,  (struct nfs_context *p1, const char *p2,    char *p3,   int p4))
  DEFINE_METHOD4(int, nfs_chown,     (struct nfs_context *p1, const char *p2,    int p3,     int p4))
  DEFINE_METHOD4(int, nfs_fchown,    (struct nfs_context *p1, struct nfsfh *p2,  int p3,     int p4))
  DEFINE_METHOD5(int, nfs_pread,     (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3,   uint64_t p4,  char *p5))
  DEFINE_METHOD5(int, nfs_pwrite,    (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3,   uint64_t p4,  char *p5))
  DEFINE_METHOD5(int, nfs_lseek,     (struct nfs_context *p1, struct nfsfh *p2,  uint64_t p3,   int p4,     uint64_t *p5))



  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(mount_free_export_list, mount_free_export_list)
    RESOLVE_METHOD_RENAME(mount_getexports, mount_getexports)
    RESOLVE_METHOD_RENAME(nfs_find_local_servers, nfs_find_local_servers)
    RESOLVE_METHOD_RENAME(free_nfs_srvr_list, free_nfs_srvr_list)        
    RESOLVE_METHOD_RENAME(nfs_init_context,   nfs_init_context)
    RESOLVE_METHOD_RENAME(nfs_destroy_context,nfs_destroy_context)
    RESOLVE_METHOD_RENAME(nfs_get_readmax,    nfs_get_readmax)
    RESOLVE_METHOD_RENAME(nfs_get_writemax,   nfs_get_writemax)   
    RESOLVE_METHOD_RENAME(nfs_get_error,      nfs_get_error)
    RESOLVE_METHOD_RENAME(nfs_readdir,        nfs_readdir)    
    RESOLVE_METHOD_RENAME(nfs_closedir,       nfs_closedir)  
    RESOLVE_METHOD_RENAME(nfs_mount,     nfs_mount)
    RESOLVE_METHOD_RENAME(nfs_stat,      nfs_stat)
    RESOLVE_METHOD_RENAME(nfs_fstat,     nfs_fstat)
    RESOLVE_METHOD_RENAME(nfs_open,      nfs_open)
    RESOLVE_METHOD_RENAME(nfs_close,     nfs_close)
    RESOLVE_METHOD_RENAME(nfs_pread,     nfs_pread)
    RESOLVE_METHOD_RENAME(nfs_read,      nfs_read)
    RESOLVE_METHOD_RENAME(nfs_pwrite,    nfs_pwrite)
    RESOLVE_METHOD_RENAME(nfs_write,     nfs_write)
    RESOLVE_METHOD_RENAME(nfs_lseek,     nfs_lseek)
    RESOLVE_METHOD_RENAME(nfs_fsync,     nfs_fsync)
    RESOLVE_METHOD_RENAME(nfs_truncate,  nfs_truncate)
    RESOLVE_METHOD_RENAME(nfs_ftruncate, nfs_ftruncate)
    RESOLVE_METHOD_RENAME(nfs_mkdir,     nfs_mkdir)
    RESOLVE_METHOD_RENAME(nfs_rmdir,     nfs_rmdir)
    RESOLVE_METHOD_RENAME(nfs_creat,     nfs_creat)
    RESOLVE_METHOD_RENAME(nfs_unlink,    nfs_unlink)
    RESOLVE_METHOD_RENAME(nfs_opendir,   nfs_opendir)
    RESOLVE_METHOD_RENAME(nfs_statvfs,   nfs_statvfs)
    RESOLVE_METHOD_RENAME(nfs_readlink,  nfs_readlink)
    RESOLVE_METHOD_RENAME(nfs_chmod,     nfs_chmod)
    RESOLVE_METHOD_RENAME(nfs_fchmod,    nfs_fchmod)
    RESOLVE_METHOD_RENAME(nfs_chown,     nfs_chown)
    RESOLVE_METHOD_RENAME(nfs_fchown,    nfs_fchown)
    RESOLVE_METHOD_RENAME(nfs_utimes,    nfs_utimes)
    RESOLVE_METHOD_RENAME(nfs_utime,     nfs_utime)
    RESOLVE_METHOD_RENAME(nfs_access,    nfs_access)
    RESOLVE_METHOD_RENAME(nfs_symlink,   nfs_symlink)
    RESOLVE_METHOD_RENAME(nfs_rename,    nfs_rename)
    RESOLVE_METHOD_RENAME(nfs_link,      nfs_link)      
  END_METHOD_RESOLVE()
};

