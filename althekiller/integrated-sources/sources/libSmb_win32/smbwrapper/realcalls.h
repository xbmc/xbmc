/* 
   Unix SMB/CIFS implementation.
   defintions of syscall entries
   Copyright (C) Andrew Tridgell 1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#if HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#elif HAVE_SYSCALL_H
#include <syscall.h>
#endif

#ifdef IRIX
/* amazingly, IRIX gets its own syscall numbers wrong! */
#ifdef SYSVoffset
#if (SYSVoffset == 1)
#undef SYSVoffset
#define SYSVoffset 1000
#endif
#endif
#endif

/* this file is partly derived from zlibc by Alain Knaff */

#define real_access(fn, mode)		(syscall(SYS_access, (fn), (mode)))
#define real_chdir(fn)		        (syscall(SYS_chdir, (fn)))
#define real_chmod(fn, mode)		(syscall(SYS_chmod,(fn), (mode)))
#define real_chown(fn, owner, group)	(syscall(SYS_chown,(fn),(owner),(group)))

#ifdef SYS_getdents
#define real_getdents(fd, dirp, count)	(syscall(SYS_getdents, (fd), (dirp), (count)))
#endif

#define real_link(fn1, fn2)		(syscall(SYS_link, (fn1), (fn2)))

#define real_open(fn,flags,mode)	(syscall(SYS_open, (fn), (flags), (mode)))

#ifdef SYS_open64
#define real_open64(fn,flags,mode)	(syscall(SYS_open64, (fn), (flags), (mode)))
#elif HAVE__OPEN64
#define real_open64(fn,flags,mode)    	(_open64(fn,flags,mode))
#define NO_OPEN64_ALIAS
#elif HAVE___OPEN64
#define real_open64(fn,flags,mode)    	(__open64(fn,flags,mode))
#define NO_OPEN64_ALIAS
#endif

#ifdef HAVE__FORK
#define real_fork()            	(_fork())
#elif HAVE___FORK
#define real_fork()            	(__fork())
#elif SYS_fork
#define real_fork()		(syscall(SYS_fork))
#endif

#ifdef HAVE__OPENDIR
#define real_opendir(fn)            	(_opendir(fn))
#elif SYS_opendir
#define real_opendir(fn)		(syscall(SYS_opendir,(fn)))
#elif HAVE___OPENDIR
#define real_opendir(fn)            	(__opendir(fn))
#endif

#ifdef HAVE__READDIR
#define real_readdir(d)            	(_readdir(d))
#elif HAVE___READDIR
#define real_readdir(d)            	(__readdir(d))
#elif SYS_readdir
#define real_readdir(d)		(syscall(SYS_readdir,(d)))
#endif

#ifdef HAVE__CLOSEDIR
#define real_closedir(d)            	(_closedir(d))
#elif SYS_closedir
#define real_closedir(d)		(syscall(SYS_closedir,(d)))
#elif HAVE___CLOSEDIR
#define real_closedir(d)            	(__closedir(d))
#endif

#ifdef HAVE__SEEKDIR
#define real_seekdir(d,l)            	(_seekdir(d,l))
#elif SYS_seekdir
#define real_seekdir(d,l)		(syscall(SYS_seekdir,(d),(l)))
#elif HAVE___SEEKDIR
#define real_seekdir(d,l)            	(__seekdir(d,l))
#else
#define NO_SEEKDIR_WRAPPER
#endif

#ifdef HAVE__TELLDIR
#define real_telldir(d)            	(_telldir(d))
#elif SYS_telldir
#define real_telldir(d)		(syscall(SYS_telldir,(d)))
#elif HAVE___TELLDIR
#define real_telldir(d)            	(__telldir(d))
#endif

#ifdef HAVE__DUP
#define real_dup(d)            	(_dup(d))
#elif SYS_dup
#define real_dup(d)		(syscall(SYS_dup,(d)))
#elif HAVE___DUP
#define real_dup(d)            	(__dup(d))
#endif

#ifdef HAVE__DUP2
#define real_dup2(d1,d2)            	(_dup2(d1,d2))
#elif SYS_dup2
#define real_dup2(d1,d2)		(syscall(SYS_dup2,(d1),(d2)))
#elif HAVE___DUP2
#define real_dup2(d1,d2)            	(__dup2(d1,d2))
#endif

#ifdef HAVE__GETCWD
#define real_getcwd(b,s)            	((char *)_getcwd(b,s))
#elif SYS_getcwd
#define real_getcwd(b,s)		((char *)syscall(SYS_getcwd,(b),(s)))
#elif HAVE___GETCWD
#define real_getcwd(b,s)            	((char *)__getcwd(b,s))
#endif

#ifdef HAVE__STAT
#define real_stat(fn,st)            	(_stat(fn,st))
#elif SYS_stat
#define real_stat(fn,st)		(syscall(SYS_stat,(fn),(st)))
#elif HAVE___STAT
#define real_stat(fn,st)            	(__stat(fn,st))
#endif

#ifdef HAVE__LSTAT
#define real_lstat(fn,st)            	(_lstat(fn,st))
#elif SYS_lstat
#define real_lstat(fn,st)		(syscall(SYS_lstat,(fn),(st)))
#elif HAVE___LSTAT
#define real_lstat(fn,st)            	(__lstat(fn,st))
#endif

#ifdef HAVE__FSTAT
#define real_fstat(fd,st)            	(_fstat(fd,st))
#elif SYS_fstat
#define real_fstat(fd,st)		(syscall(SYS_fstat,(fd),(st)))
#elif HAVE___FSTAT
#define real_fstat(fd,st)            	(__fstat(fd,st))
#endif

#if defined(HAVE_SYS_ACL_H) && defined(HAVE__ACL)
#define real_acl(fn,cmd,n,buf)            	(_acl(fn,cmd,n,buf))
#elif SYS_acl
#define real_acl(fn,cmd,n,buf)		(syscall(SYS_acl,(fn),(cmd),(n),(buf)))
#elif HAVE___ACL
#define real_acl(fn,cmd,n,buf)            	(__acl(fn,cmd,n,buf))
#else
#define NO_ACL_WRAPPER
#endif

#ifdef HAVE__FACL
#define real_facl(fd,cmd,n,buf)            	(_facl(fd,cmd,n,buf))
#elif SYS_facl
#define real_facl(fd,cmd,n,buf)		(syscall(SYS_facl,(fd),(cmd),(n),(buf)))
#elif HAVE___FACL
#define real_facl(fd,cmd,n,buf)            	(__facl(fd,cmd,n,buf))
#else
#define NO_FACL_WRAPPER
#endif


#ifdef HAVE__STAT64
#define real_stat64(fn,st)            	(_stat64(fn,st))
#elif HAVE___STAT64
#define real_stat64(fn,st)            	(__stat64(fn,st))
#endif

#ifdef HAVE__LSTAT64
#define real_lstat64(fn,st)            	(_lstat64(fn,st))
#elif HAVE___LSTAT64
#define real_lstat64(fn,st)            	(__lstat64(fn,st))
#endif

#ifdef HAVE__FSTAT64
#define real_fstat64(fd,st)            	(_fstat64(fd,st))
#elif HAVE___FSTAT64
#define real_fstat64(fd,st)            	(__fstat64(fd,st))
#endif

#ifdef HAVE__READDIR64
#define real_readdir64(d)            	(_readdir64(d))
#elif HAVE___READDIR64
#define real_readdir64(d)            	(__readdir64(d))
#endif

#ifdef HAVE__LLSEEK
#define real_llseek(fd,ofs,whence)            	(_llseek(fd,ofs,whence))
#elif HAVE___LLSEEK
#define real_llseek(fd,ofs,whence)            	(__llseek(fd,ofs,whence))
#elif HAVE___SYS_LLSEEK
#define real_llseek(fd,ofs,whence)            	(__sys_llseek(fd,ofs,whence))
#endif


#ifdef HAVE__PREAD
#define real_pread(fd,buf,size,ofs)            	(_pread(fd,buf,size,ofs))
#elif HAVE___PREAD
#define real_pread(fd,buf,size,ofs)            	(__pread(fd,buf,size,ofs))
#endif

#ifdef HAVE__PREAD64
#define real_pread64(fd,buf,size,ofs)            	(_pread64(fd,buf,size,ofs))
#elif HAVE___PREAD64
#define real_pread64(fd,buf,size,ofs)            	(__pread64(fd,buf,size,ofs))
#endif

#ifdef HAVE__PWRITE
#define real_pwrite(fd,buf,size,ofs)            	(_pwrite(fd,buf,size,ofs))
#elif HAVE___PWRITE
#define real_pwrite(fd,buf,size,ofs)            	(__pwrite(fd,buf,size,ofs))
#endif

#ifdef HAVE__PWRITE64
#define real_pwrite64(fd,buf,size,ofs)            	(_pwrite64(fd,buf,size,ofs))
#elif HAVE___PWRITE64
#define real_pwrite64(fd,buf,size,ofs)            	(__pwrite64(fd,buf,size,ofs))
#endif


#define real_readlink(fn,buf,len)	(syscall(SYS_readlink, (fn), (buf), (len)))
#define real_rename(fn1, fn2)		(syscall(SYS_rename, (fn1), (fn2)))
#define real_symlink(fn1, fn2)		(syscall(SYS_symlink, (fn1), (fn2)))
#define real_read(fd, buf, count )	(syscall(SYS_read, (fd), (buf), (count)))
#define real_lseek(fd, offset, whence)	(syscall(SYS_lseek, (fd), (offset), (whence)))
#define real_write(fd, buf, count )	(syscall(SYS_write, (fd), (buf), (count)))
#define real_close(fd)	                (syscall(SYS_close, (fd)))
#define real_fchdir(fd)	                (syscall(SYS_fchdir, (fd)))
#define real_fcntl(fd,cmd,arg)	        (syscall(SYS_fcntl, (fd), (cmd), (arg)))
#define real_symlink(fn1, fn2)		(syscall(SYS_symlink, (fn1), (fn2)))
#define real_unlink(fn)			(syscall(SYS_unlink, (fn)))
#define real_rmdir(fn)			(syscall(SYS_rmdir, (fn)))
#define real_mkdir(fn, mode)		(syscall(SYS_mkdir, (fn), (mode)))

/*
 * On GNU/Linux distributions which allow to use both 2.4 and 2.6 kernels
 * there is SYS_utimes syscall defined at compile time in glibc-kernheaders but 
 * it is available on 2.6 kernels only. Therefore, we can't rely on syscall at 
 * compile time but have to check that behaviour during program execution. An easy 
 * workaround is to have replacement for utimes() implemented within our wrapper and 
 * do not rely on syscall at all. Thus, if REPLACE_UTIME is defined already (by packager), 
 * skip these syscall shortcuts.
 */
#ifndef REPLACE_UTIME
#ifdef SYS_utime
#define real_utime(fn, buf)		(syscall(SYS_utime, (fn), (buf)))
#else
#define REPLACE_UTIME 1
#endif
#endif

#ifndef REPLACE_UTIMES
#ifdef SYS_utimes
#define real_utimes(fn, buf)		(syscall(SYS_utimes, (fn), (buf)))
#else
#define REPLACE_UTIMES 1
#endif
#endif
