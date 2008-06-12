/* 
 * CAP VFS module for Samba 3.x Version 0.3
 *
 * Copyright (C) Tim Potter, 1999-2000
 * Copyright (C) Alexander Bokovoy, 2002-2003
 * Copyright (C) Stefan (metze) Metzmacher, 2003
 * Copyright (C) TAKAHASHI Motonobu (monyo), 2003
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "includes.h"

/* cap functions */
static char *capencode(char *to, const char *from);
static char *capdecode(char *to, const char *from);

static SMB_BIG_UINT cap_disk_free(vfs_handle_struct *handle, connection_struct *conn, const char *path,
	BOOL small_query, SMB_BIG_UINT *bsize,
	SMB_BIG_UINT *dfree, SMB_BIG_UINT *dsize)
{
        pstring cappath;
        capencode(cappath, path);
	return SMB_VFS_NEXT_DISK_FREE(handle, conn, cappath, small_query, bsize, 
					 dfree, dsize);
}

static SMB_STRUCT_DIR *cap_opendir(vfs_handle_struct *handle, connection_struct *conn, const char *fname, const char *mask, uint32 attr)
{
        pstring capname;
        capencode(capname, fname);
	return SMB_VFS_NEXT_OPENDIR(handle, conn, capname, mask, attr);
}

static SMB_STRUCT_DIRENT *cap_readdir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
        SMB_STRUCT_DIRENT *result;
	DEBUG(3,("cap: cap_readdir\n"));
	result = SMB_VFS_NEXT_READDIR(handle, conn, dirp);
	if (result) {
	  DEBUG(3,("cap: cap_readdir: %s\n", result->d_name));
	  capdecode(result->d_name, result->d_name);
        }
        return result;
}

static int cap_mkdir(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_MKDIR(handle, conn, cappath, mode);
}

static int cap_rmdir(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
        pstring cappath;
        capencode(cappath, path);
	return SMB_VFS_NEXT_RMDIR(handle, conn, cappath);
}

static int cap_open(vfs_handle_struct *handle, connection_struct *conn, const char *fname, int flags, mode_t mode)
{
        pstring capname;
	DEBUG(3,("cap: cap_open for %s\n", fname));
	capencode(capname, fname);
	return SMB_VFS_NEXT_OPEN(handle, conn, capname, flags, mode);
}

static int cap_rename(vfs_handle_struct *handle, connection_struct *conn, const char *oldname, const char *newname)
{
	pstring capold, capnew;
	capencode(capold, oldname);
	capencode(capnew, newname);

	return SMB_VFS_NEXT_RENAME(handle, conn, capold, capnew);
}

static int cap_stat(vfs_handle_struct *handle, connection_struct *conn, const char *fname, SMB_STRUCT_STAT *sbuf)
{
        pstring capname;
	capencode(capname, fname);
	return SMB_VFS_NEXT_STAT(handle, conn, capname, sbuf);
}

static int cap_lstat(vfs_handle_struct *handle, connection_struct *conn, const char *path, SMB_STRUCT_STAT *sbuf)
{
	pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_LSTAT(handle, conn, cappath, sbuf);
}

static int cap_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_UNLINK(handle, conn, cappath);
}

static int cap_chmod(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_CHMOD(handle, conn, cappath, mode);
}

static int cap_chown(vfs_handle_struct *handle, connection_struct *conn, const char *path, uid_t uid, gid_t gid)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_CHOWN(handle, conn, cappath, uid, gid);
}

static int cap_chdir(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
        pstring cappath;
	DEBUG(3,("cap: cap_chdir for %s\n", path));
	capencode(cappath, path);
	return SMB_VFS_NEXT_CHDIR(handle, conn, cappath);
}

static int cap_utime(vfs_handle_struct *handle, connection_struct *conn, const char *path, struct utimbuf *times)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_UTIME(handle, conn, cappath, times);
}


static BOOL cap_symlink(vfs_handle_struct *handle, connection_struct *conn, const char *oldpath, const char *newpath)
{
        pstring capoldpath, capnewpath;
        capencode(capoldpath, oldpath);
        capencode(capnewpath, newpath);
	return SMB_VFS_NEXT_SYMLINK(handle, conn, capoldpath, capnewpath);
}

static BOOL cap_readlink(vfs_handle_struct *handle, connection_struct *conn, const char *path, char *buf, size_t bufsiz)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_READLINK(handle, conn, cappath, buf, bufsiz);
}

static int cap_link(vfs_handle_struct *handle, connection_struct *conn, const char *oldpath, const char *newpath)
{
        pstring capoldpath, capnewpath;
        capencode(capoldpath, oldpath);
        capencode(capnewpath, newpath);
	return SMB_VFS_NEXT_LINK(handle, conn, capoldpath, capnewpath);
}

static int cap_mknod(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode, SMB_DEV_T dev)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_MKNOD(handle, conn, cappath, mode, dev);
}

static char *cap_realpath(vfs_handle_struct *handle, connection_struct *conn, const char *path, char *resolved_path)
{
        /* monyo need capencode'ed and capdecode'ed? */
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_REALPATH(handle, conn, path, resolved_path);
}

static BOOL cap_set_nt_acl(vfs_handle_struct *handle, files_struct *fsp, const char *name, uint32 security_info_sent, struct security_descriptor_info *psd)
{
        pstring capname;
	capencode(capname, name);
	return SMB_VFS_NEXT_SET_NT_ACL(handle, fsp, capname, security_info_sent, psd);
}

static int cap_chmod_acl(vfs_handle_struct *handle, connection_struct *conn, const char *name, mode_t mode)
{
        pstring capname;
	capencode(capname, name);

	/* If the underlying VFS doesn't have ACL support... */
	if (!handle->vfs_next.ops.chmod_acl) {
		errno = ENOSYS;
		return -1;
	}
	return SMB_VFS_NEXT_CHMOD_ACL(handle, conn, capname, mode);
}

static SMB_ACL_T cap_sys_acl_get_file(vfs_handle_struct *handle, connection_struct *conn, const char *path_p, SMB_ACL_TYPE_T type)
{
        pstring cappath_p;
	capencode(cappath_p, path_p);
	return SMB_VFS_NEXT_SYS_ACL_GET_FILE(handle, conn, cappath_p, type);
}

static int cap_sys_acl_set_file(vfs_handle_struct *handle, connection_struct *conn, const char *name, SMB_ACL_TYPE_T acltype, SMB_ACL_T theacl)
{
        pstring capname;
	capencode(capname, name);
	return SMB_VFS_NEXT_SYS_ACL_SET_FILE(handle, conn, capname, acltype, theacl);
}

static int cap_sys_acl_delete_def_file(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
        pstring cappath;
	capencode(cappath, path);
	return SMB_VFS_NEXT_SYS_ACL_DELETE_DEF_FILE(handle, conn, cappath);
}

static ssize_t cap_getxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, void *value, size_t size)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_GETXATTR(handle, conn, cappath, capname, value, size);
}

static ssize_t cap_lgetxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, void *value, size_t
size)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_LGETXATTR(handle, conn, cappath, capname, value, size);
}

static ssize_t cap_fgetxattr(vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name, void *value, size_t size)
{
        pstring capname;
	capencode(capname, name);
        return SMB_VFS_NEXT_FGETXATTR(handle, fsp, fd, capname, value, size);
}

static ssize_t cap_listxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, char *list, size_t size)
{
        pstring cappath;
	capencode(cappath, path);
        return SMB_VFS_NEXT_LISTXATTR(handle, conn, cappath, list, size);
}

static ssize_t cap_llistxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, char *list, size_t size)
{
        pstring cappath;
	capencode(cappath, path);
        return SMB_VFS_NEXT_LLISTXATTR(handle, conn, cappath, list, size);
}

static int cap_removexattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_REMOVEXATTR(handle, conn, cappath, capname);
}

static int cap_lremovexattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_LREMOVEXATTR(handle, conn, cappath, capname);
}

static int cap_fremovexattr(vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name)
{
        pstring capname;
	capencode(capname, name);
        return SMB_VFS_NEXT_FREMOVEXATTR(handle, fsp, fd, capname);
}

static int cap_setxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, const void *value, size_t size, int flags)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_SETXATTR(handle, conn, cappath, capname, value, size, flags);
}

static int cap_lsetxattr(vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, const void *value, size_t size, int flags)
{
        pstring cappath, capname;
	capencode(cappath, path);
	capencode(capname, name);
        return SMB_VFS_NEXT_LSETXATTR(handle, conn, cappath, capname, value, size, flags);
}

static int cap_fsetxattr(vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name, const void *value, size_t size, int flags)
{
        pstring capname;
	capencode(capname, name);
        return SMB_VFS_NEXT_FSETXATTR(handle, fsp, fd, capname, value, size, flags);
}

/* VFS operations structure */

static vfs_op_tuple cap_op_tuples[] = {

	/* Disk operations */

	{SMB_VFS_OP(cap_disk_free),			SMB_VFS_OP_DISK_FREE,		SMB_VFS_LAYER_TRANSPARENT},
	
	/* Directory operations */

	{SMB_VFS_OP(cap_opendir),			SMB_VFS_OP_OPENDIR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_readdir),			SMB_VFS_OP_READDIR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_mkdir),			SMB_VFS_OP_MKDIR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_rmdir),			SMB_VFS_OP_RMDIR,		SMB_VFS_LAYER_TRANSPARENT},

	/* File operations */

	{SMB_VFS_OP(cap_open),				SMB_VFS_OP_OPEN,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_rename),			SMB_VFS_OP_RENAME,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_stat),				SMB_VFS_OP_STAT,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_lstat),			SMB_VFS_OP_LSTAT,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_unlink),			SMB_VFS_OP_UNLINK,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_chmod),			SMB_VFS_OP_CHMOD,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_chown),			SMB_VFS_OP_CHOWN,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_chdir),			SMB_VFS_OP_CHDIR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_utime),			SMB_VFS_OP_UTIME,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_symlink),			SMB_VFS_OP_SYMLINK,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_readlink),			SMB_VFS_OP_READLINK,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_link),				SMB_VFS_OP_LINK,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_mknod),			SMB_VFS_OP_MKNOD,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_realpath),			SMB_VFS_OP_REALPATH,		SMB_VFS_LAYER_TRANSPARENT},

	/* NT File ACL operations */

	{SMB_VFS_OP(cap_set_nt_acl),			SMB_VFS_OP_SET_NT_ACL,		SMB_VFS_LAYER_TRANSPARENT},

	/* POSIX ACL operations */

	{SMB_VFS_OP(cap_chmod_acl),			SMB_VFS_OP_CHMOD_ACL,		SMB_VFS_LAYER_TRANSPARENT},

	{SMB_VFS_OP(cap_sys_acl_get_file),		SMB_VFS_OP_SYS_ACL_GET_FILE,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_sys_acl_set_file),		SMB_VFS_OP_SYS_ACL_SET_FILE,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_sys_acl_delete_def_file),	SMB_VFS_OP_SYS_ACL_DELETE_DEF_FILE,	SMB_VFS_LAYER_TRANSPARENT},
	
	/* EA operations. */
	{SMB_VFS_OP(cap_getxattr),			SMB_VFS_OP_GETXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_lgetxattr),			SMB_VFS_OP_LGETXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_fgetxattr),			SMB_VFS_OP_FGETXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_listxattr),			SMB_VFS_OP_LISTXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_llistxattr),			SMB_VFS_OP_LLISTXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_removexattr),			SMB_VFS_OP_REMOVEXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_lremovexattr),			SMB_VFS_OP_LREMOVEXATTR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_fremovexattr),			SMB_VFS_OP_FREMOVEXATTR,		SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_setxattr),			SMB_VFS_OP_SETXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_lsetxattr),			SMB_VFS_OP_LSETXATTR,			SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(cap_fsetxattr),			SMB_VFS_OP_FSETXATTR,			SMB_VFS_LAYER_TRANSPARENT},

	{NULL,						SMB_VFS_OP_NOOP,			SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_cap_init(void)
{
	return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "cap", cap_op_tuples);
}

/* For CAP functions */
#define hex_tag ':'
#define hex2bin(c)		hex2bin_table[(unsigned char)(c)]
#define bin2hex(c)		bin2hex_table[(unsigned char)(c)]
#define is_hex(s)		((s)[0] == hex_tag)

static unsigned char hex2bin_table[256] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
0000, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0000, /* 0x40 */
0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
0000, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0000, /* 0x60 */
0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x70 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xa0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xb0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xc0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xd0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xe0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* 0xf0 */
};
static unsigned char bin2hex_table[256] = "0123456789abcdef";

/*******************************************************************
  original code -> ":xx"  - CAP format
********************************************************************/
static char *capencode(char *to, const char *from)
{
  pstring cvtbuf;
  char *out;

  if (to == from) {
    from = pstrcpy ((char *) cvtbuf, from);
  }

  for (out = to; *from && (out - to < sizeof(pstring)-7);) {
    /* buffer husoku error */
    if ((unsigned char)*from >= 0x80) {
      *out++ = hex_tag;
      *out++ = bin2hex (((*from)>>4)&0x0f);
      *out++ = bin2hex ((*from)&0x0f);
      from++;
    } 
    else {
      *out++ = *from++;
    }
  }
  *out = '\0';
  return to;
}

/*******************************************************************
  CAP -> original code
********************************************************************/
/* ":xx" -> a byte */
static char *capdecode(char *to, const char *from)
{
  pstring cvtbuf;
  char *out;

  if (to == from) {
    from = pstrcpy ((char *) cvtbuf, from);
  }
  for (out = to; *from && (out - to < sizeof(pstring)-3);) {
    if (is_hex(from)) {
      *out++ = (hex2bin (from[1])<<4) | (hex2bin (from[2]));
      from += 3;
    } else {
      *out++ = *from++;
    }
  }
  *out = '\0';
  return to;
}
