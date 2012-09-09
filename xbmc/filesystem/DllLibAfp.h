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

//libafpclient includes
#ifdef __cplusplus
extern "C" {
#endif   
#include <afpfs-ng/libafpclient.h>
#include <afpfs-ng/map_def.h>
#include <afpfs-ng/midlevel.h>
#ifdef __cplusplus
}
#endif


//#define USE_CVS_AFPFS
#ifdef USE_CVS_AFPFS
#define afp_wrap_open                 afp_ml_open
#define afp_wrap_close                afp_ml_close
#define afp_wrap_read                 afp_ml_read
#define afp_wrap_write                afp_ml_write
#define afp_wrap_getattr              afp_ml_getattr
#define afp_wrap_server_full_connect  afp_server_full_connect
#define afp_wrap_unlink               afp_ml_unlink
#define afp_wrap_rename               afp_ml_rename
#define afp_wrap_creat                afp_ml_creat
#define afp_wrap_readdir              afp_ml_readdir
#define afp_wrap_readlink             afp_ml_readlink
#define afp_wrap_mkdir                afp_ml_mkdir
#define afp_wrap_rmdir                afp_ml_rmdir
#else
#define afp_wrap_open                 ml_open
#define afp_wrap_close                ml_close
#define afp_wrap_read                 ml_read
#define afp_wrap_write                ml_write
#define afp_wrap_getattr              ml_getattr
#define afp_wrap_server_full_connect  afp_server_full_connect
#define afp_wrap_unlink               ml_unlink
#define afp_wrap_rename               ml_rename
#define afp_wrap_creat                ml_creat
#define afp_wrap_readdir              ml_readdir
#define afp_wrap_readlink             ml_readlink
#define afp_wrap_mkdir                ml_mkdir
#define afp_wrap_rmdir                ml_rmdir
#endif


class DllLibAfpInterface
{
public:
  virtual ~DllLibAfpInterface() {}

  virtual void libafpclient_register(struct libafpclient * tmpclient)=0;
  virtual int init_uams(void)=0;
  virtual int afp_main_quick_startup(pthread_t * thread)=0;
  virtual int afp_unmount_all_volumes(struct afp_server * server)=0;
  virtual int afp_unmount_volume(struct afp_volume * volume)=0;
  virtual struct afp_volume * find_volume_by_name(struct afp_server * server, const char * volname)=0;
  virtual int afp_connect_volume(struct afp_volume * volume, struct afp_server * server, char * mesg, unsigned int * l, unsigned int max)=0;
  virtual int afp_parse_url(struct afp_url * url, const char * toparse, int verbose)=0;
  virtual unsigned int find_uam_by_name(const char * name)=0;
  virtual unsigned int default_uams_mask(void)=0;
  virtual char * uam_bitmap_to_string(unsigned int bitmap)=0;
  virtual void afp_default_url(struct afp_url *url)=0;
  virtual char * get_uam_names_list(void)=0;
  virtual void afp_ml_filebase_free(struct afp_file_info **filebase)=0;
  
#ifdef USE_CVS_AFPFS  
  virtual struct afp_server * afp_server_full_connect(void * priv, struct afp_connection_request * req, int * error)=0;
  virtual int afp_ml_open(struct afp_volume * volume, const char *path, int flags, struct afp_file_info **newfp)=0;
  virtual int afp_ml_close(struct afp_volume * volume, const char * path, struct afp_file_info * fp)=0;
  virtual int afp_ml_read(struct afp_volume * volume, const char *path, char *buf, size_t size, off_t offset, struct afp_file_info *fp, int * eof)=0;
  virtual int afp_ml_write(struct afp_volume * volume, const char * path, const char *data, size_t size, off_t offset, struct afp_file_info * fp, uid_t uid, gid_t gid)=0;
  virtual int afp_ml_getattr(struct afp_volume * volume, const char *path, struct stat *stbuf)=0;
  virtual int afp_ml_unlink(struct afp_volume * vol, const char *path)=0;
  virtual int afp_ml_rename(struct afp_volume * vol, const char * path_from, const char * path_to)=0;
  virtual int afp_ml_creat(struct afp_volume * volume, const char *path, mode_t mode)=0;
  virtual int afp_ml_readdir(struct afp_volume * volume, const char *path, struct afp_file_info **base)=0;
  virtual int afp_ml_readlink(struct afp_volume * vol, const char * path, char *buf, size_t size)=0;  
  virtual int afp_ml_mkdir(struct afp_volume * vol, const char * path, mode_t mode)=0;
  virtual int afp_ml_rmdir(struct afp_volume * vol, const char *path)=0;

#else
  virtual struct afp_server * afp_server_full_connect(void * priv, struct afp_connection_request * req)=0;
  virtual int ml_open(struct afp_volume * volume, const char *path, int flags, struct afp_file_info **newfp)=0;
  virtual int ml_close(struct afp_volume * volume, const char * path, struct afp_file_info * fp)=0;
  virtual int ml_read(struct afp_volume * volume, const char *path, char *buf, size_t size, off_t offset, struct afp_file_info *fp, int * eof)=0;
  virtual int ml_write(struct afp_volume * volume, const char * path, const char *data, size_t size, off_t offset, struct afp_file_info * fp, uid_t uid, gid_t gid)=0;
  virtual int ml_getattr(struct afp_volume * volume, const char *path, struct stat *stbuf)=0;
  virtual int ml_unlink(struct afp_volume * vol, const char *path)=0;
  virtual int ml_rename(struct afp_volume * vol, const char * path_from, const char * path_to)=0;
  virtual int ml_creat(struct afp_volume * volume, const char *path, mode_t mode)=0;
  virtual int ml_readdir(struct afp_volume * volume, const char *path, struct afp_file_info **base)=0;
  virtual int ml_readlink(struct afp_volume * vol, const char * path, char *buf, size_t size)=0;
  virtual int ml_mkdir(struct afp_volume * vol, const char * path, mode_t mode)=0;
  virtual int ml_rmdir(struct afp_volume * vol, const char *path)=0;

#endif

};

class DllLibAfp : public DllDynamic, DllLibAfpInterface
{
  DECLARE_DLL_WRAPPER(DllLibAfp, DLL_PATH_LIBAFP)
  DEFINE_METHOD0(int,                   init_uams)  
  DEFINE_METHOD0(unsigned int,          default_uams_mask)    
  DEFINE_METHOD0(char *,                get_uam_names_list)      

  DEFINE_METHOD1(void,                  libafpclient_register,            (struct libafpclient *p1))
  DEFINE_METHOD1(int,                   afp_main_quick_startup,           (pthread_t *p1))  
  DEFINE_METHOD1(int,                   afp_unmount_all_volumes,          (struct afp_server *p1))    
  DEFINE_METHOD1(int,                   afp_unmount_volume,               (struct afp_volume *p1))
  DEFINE_METHOD1(unsigned int,          find_uam_by_name,                 (const char *p1))      
  DEFINE_METHOD1(char *,                uam_bitmap_to_string,             (unsigned int p1))        
  DEFINE_METHOD1(void,                  afp_default_url,                  (struct afp_url *p1))  
  DEFINE_METHOD1(void,                  afp_ml_filebase_free,             (struct afp_file_info **p1))

#ifdef USE_CVS_AFPFS
  DEFINE_METHOD3(struct afp_server *,   afp_wrap_server_full_connect,     (void *p1, struct afp_connection_request *p2, int *p3))        
#else
  DEFINE_METHOD2(struct afp_server *,   afp_wrap_server_full_connect,     (void *p1, struct afp_connection_request *p2))        
#endif

  DEFINE_METHOD2(struct afp_volume *,   find_volume_by_name,              (struct afp_server *p1, const char *p2))      
  DEFINE_METHOD2(int,                   afp_wrap_unlink,                  (struct afp_volume *p1, const char *p2))          
  DEFINE_METHOD2(int,                   afp_wrap_rmdir,                   (struct afp_volume *p1, const char *p2))            
 
  DEFINE_METHOD3(int,                   afp_parse_url,                    (struct afp_url *p1, const char *p2, int p3))      
  DEFINE_METHOD3(int,                   afp_wrap_close,                   (struct afp_volume *p1, const char *p2, struct afp_file_info *p3))        
  DEFINE_METHOD3(int,                   afp_wrap_getattr,                 (struct afp_volume *p1, const char *p2, struct stat *p3))          
  DEFINE_METHOD3(int,                   afp_wrap_rename,                  (struct afp_volume *p1, const char *p2, const char *p3))            
  DEFINE_METHOD3(int,                   afp_wrap_creat,                   (struct afp_volume *p1, const char *p2, mode_t p3))              
  DEFINE_METHOD3(int,                   afp_wrap_readdir,                 (struct afp_volume *p1, const char *p2, struct afp_file_info **p3))                 
  DEFINE_METHOD3(int,                   afp_wrap_mkdir,                   (struct afp_volume *p1, const char *p2, mode_t p3))                  

  DEFINE_METHOD4(int,                   afp_wrap_open,                    (struct afp_volume *p1, const char *p2, int p3, struct afp_file_info **p4))        
  DEFINE_METHOD4(int,                   afp_wrap_readlink,                (struct afp_volume *p1, const char *p2, char *p3, size_t p4))
  
  DEFINE_METHOD5(int,                   afp_connect_volume,               (struct afp_volume *p1, struct afp_server *p2, char *p3, unsigned int *p4, unsigned int p5))        
  
  DEFINE_METHOD7(int,                   afp_wrap_read,                    (struct afp_volume *p1, const char *p2, char *p3, size_t p4, off_t p5, struct afp_file_info *p6, int *p7))
  
  DEFINE_METHOD8(int,                   afp_wrap_write,                   (struct afp_volume *p1, const char *p2, const char *p3, size_t p4, off_t p5, struct afp_file_info *p6, uid_t p7, gid_t p8))



  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(init_uams,init_uams)
    RESOLVE_METHOD_RENAME(libafpclient_register, libafpclient_register)
    RESOLVE_METHOD_RENAME(afp_main_quick_startup, afp_main_quick_startup)
    RESOLVE_METHOD_RENAME(afp_unmount_all_volumes, afp_unmount_all_volumes)        
    RESOLVE_METHOD_RENAME(afp_unmount_all_volumes, afp_unmount_all_volumes)
    RESOLVE_METHOD_RENAME(find_volume_by_name, find_volume_by_name)    
    RESOLVE_METHOD_RENAME(afp_connect_volume, afp_connect_volume)        
    RESOLVE_METHOD_RENAME(afp_parse_url, afp_parse_url)            
    RESOLVE_METHOD_RENAME(find_uam_by_name, find_uam_by_name)                
    RESOLVE_METHOD_RENAME(default_uams_mask, default_uams_mask) 
    RESOLVE_METHOD_RENAME(uam_bitmap_to_string, uam_bitmap_to_string)     
    RESOLVE_METHOD_RENAME(afp_default_url, afp_default_url)         
    RESOLVE_METHOD_RENAME(get_uam_names_list, get_uam_names_list)
    RESOLVE_METHOD_RENAME(afp_ml_filebase_free, afp_ml_filebase_free)

#ifdef USE_CVS_AFPFS                       
    RESOLVE_METHOD_RENAME(afp_server_full_connect, afp_server_full_connect)                        
    RESOLVE_METHOD_RENAME(afp_ml_open, afp_ml_open)                            
    RESOLVE_METHOD_RENAME(afp_ml_close, afp_ml_close)                                
    RESOLVE_METHOD_RENAME(afp_ml_read, afp_ml_read)                                    
    RESOLVE_METHOD_RENAME(afp_ml_write, afp_ml_write)                                        
    RESOLVE_METHOD_RENAME(afp_ml_getattr, afp_ml_getattr)                                            
    RESOLVE_METHOD_RENAME(afp_ml_unlink, afp_ml_unlink)                                                
    RESOLVE_METHOD_RENAME(afp_ml_rename, afp_ml_rename)                                                    
    RESOLVE_METHOD_RENAME(afp_ml_creat, afp_ml_creat)                                                        
    RESOLVE_METHOD_RENAME(afp_ml_readdir, afp_ml_readdir)
    RESOLVE_METHOD_RENAME(afp_ml_readlink, afp_ml_readlink)    
    RESOLVE_METHOD_RENAME(afp_ml_mkdir, afp_ml_mkdir)                                                                
    RESOLVE_METHOD_RENAME(afp_ml_rmdir, afp_ml_rmdir)  
#else
    RESOLVE_METHOD_RENAME(afp_server_full_connect, afp_server_full_connect)                        
    RESOLVE_METHOD_RENAME(ml_open, ml_open)                            
    RESOLVE_METHOD_RENAME(ml_close, ml_close)                                
    RESOLVE_METHOD_RENAME(ml_read, ml_read)                                    
    RESOLVE_METHOD_RENAME(ml_write, ml_write)                                        
    RESOLVE_METHOD_RENAME(ml_getattr, ml_getattr)                                            
    RESOLVE_METHOD_RENAME(ml_unlink, ml_unlink)                                                
    RESOLVE_METHOD_RENAME(ml_rename, ml_rename)                                                    
    RESOLVE_METHOD_RENAME(ml_creat, ml_creat)                                                        
    RESOLVE_METHOD_RENAME(ml_readdir, ml_readdir)
    RESOLVE_METHOD_RENAME(ml_readlink, ml_readlink)
    RESOLVE_METHOD_RENAME(ml_mkdir, ml_mkdir)                                                                
    RESOLVE_METHOD_RENAME(ml_rmdir, ml_rmdir) 
#endif
  END_METHOD_RESOLVE()
};
