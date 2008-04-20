/* 
 * Catia VFS module
 *
 * Implement a fixed mapping of forbidden NT characters in filenames that are
 * used a lot by the CAD package Catia.
 *
 * Yes, this a BAD BAD UGLY INCOMPLETE hack, but it helps quite some people
 * out there. Catia V4 on AIX uses characters like "<*$ a *lot*, all forbidden
 * under Windows...
 *
 * Copyright (C) Volker Lendecke, 2005
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

static void catia_string_replace(char *s, unsigned char oldc, unsigned 
				 char newc)
{
        static smb_ucs2_t tmpbuf[sizeof(pstring)];
        smb_ucs2_t *ptr = tmpbuf;
        smb_ucs2_t old = oldc;

        push_ucs2(NULL, tmpbuf, s, sizeof(tmpbuf), STR_TERMINATE);

        for (;*ptr;ptr++)
                if (*ptr==old) *ptr=newc;

        pull_ucs2(NULL, s, tmpbuf, -1, sizeof(tmpbuf), STR_TERMINATE);
}

static void from_unix(char *s)
{
        catia_string_replace(s, '\x22', '\xa8');
        catia_string_replace(s, '\x2a', '\xa4');
        catia_string_replace(s, '\x2f', '\xf8');
        catia_string_replace(s, '\x3a', '\xf7');
        catia_string_replace(s, '\x3c', '\xab');
        catia_string_replace(s, '\x3e', '\xbb');
        catia_string_replace(s, '\x3f', '\xbf');
        catia_string_replace(s, '\x5c', '\xff');
        catia_string_replace(s, '\x7c', '\xa6');
        catia_string_replace(s, ' ', '\xb1');
}

static void to_unix(char *s)
{
        catia_string_replace(s, '\xa8', '\x22');
        catia_string_replace(s, '\xa4', '\x2a');
        catia_string_replace(s, '\xf8', '\x2f');
        catia_string_replace(s, '\xf7', '\x3a');
        catia_string_replace(s, '\xab', '\x3c');
        catia_string_replace(s, '\xbb', '\x3e');
        catia_string_replace(s, '\xbf', '\x3f');
        catia_string_replace(s, '\xff', '\x5c');
        catia_string_replace(s, '\xa6', '\x7c');
        catia_string_replace(s, '\xb1', ' ');
}

static SMB_STRUCT_DIR *catia_opendir(vfs_handle_struct *handle, connection_struct 
			  *conn, const char *fname, const char *mask, uint32 attr)
{
        pstring name;
        pstrcpy(name, fname);
        to_unix(name);

        return SMB_VFS_NEXT_OPENDIR(handle, conn, name, mask, attr);
}

static SMB_STRUCT_DIRENT *catia_readdir(vfs_handle_struct *handle, 
					connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
        SMB_STRUCT_DIRENT *result = SMB_VFS_NEXT_READDIR(handle, conn, dirp);

        if (result == NULL)
                return result;

        from_unix(result->d_name);
        return result;
}

static int catia_open(vfs_handle_struct *handle, connection_struct *conn, 
		      const char *fname, int flags, mode_t mode)
{
        pstring name;

        pstrcpy(name, fname);
        to_unix(name);
 
        return SMB_VFS_NEXT_OPEN(handle, conn, name, flags, mode);
}

static int catia_rename(vfs_handle_struct *handle, connection_struct *conn,
			const char *oldname, const char *newname)
{
        pstring oname, nname;

        pstrcpy(oname, oldname);
        to_unix(oname);
        pstrcpy(nname, newname);
        to_unix(nname);

        DEBUG(10, ("converted old name: %s\n", oname));
        DEBUG(10, ("converted new name: %s\n", nname));
 
        return SMB_VFS_NEXT_RENAME(handle, conn, oname, nname);
}

static int catia_stat(vfs_handle_struct *handle, connection_struct *conn, 
		      const char *fname, SMB_STRUCT_STAT *sbuf)
{
        pstring name;
        pstrcpy(name, fname);
        to_unix(name);

        return SMB_VFS_NEXT_STAT(handle, conn, name, sbuf);
}

static int catia_lstat(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path, SMB_STRUCT_STAT *sbuf)
{
        pstring name;
        pstrcpy(name, path);
        to_unix(name);

        return SMB_VFS_NEXT_LSTAT(handle, conn, name, sbuf);
}

static int catia_unlink(vfs_handle_struct *handle, connection_struct *conn,
			const char *path)
{
        pstring name;
        pstrcpy(name, path);
        to_unix(name);

        return SMB_VFS_NEXT_UNLINK(handle, conn, name);
}

static int catia_chmod(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path, mode_t mode)
{
        pstring name;
        pstrcpy(name, path);
        to_unix(name);

        return SMB_VFS_NEXT_CHMOD(handle, conn, name, mode);
}

static int catia_chown(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path, uid_t uid, gid_t gid)
{
        pstring name;
        pstrcpy(name, path);
        to_unix(name);

        return SMB_VFS_NEXT_CHOWN(handle, conn, name, uid, gid);
}

static int catia_chdir(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path)
{
        pstring name;
        pstrcpy(name, path);
        to_unix(name);

        return SMB_VFS_NEXT_CHDIR(handle, conn, name);
}

static char *catia_getwd(vfs_handle_struct *handle, connection_struct *conn,
			 char *buf)
{
        return SMB_VFS_NEXT_GETWD(handle, conn, buf);
}

static int catia_utime(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path, struct utimbuf *times)
{
        return SMB_VFS_NEXT_UTIME(handle, conn, path, times);
}

static BOOL catia_symlink(vfs_handle_struct *handle, connection_struct *conn,
			  const char *oldpath, const char *newpath)
{
        return SMB_VFS_NEXT_SYMLINK(handle, conn, oldpath, newpath);
}

static BOOL catia_readlink(vfs_handle_struct *handle, connection_struct *conn,
			   const char *path, char *buf, size_t bufsiz)
{
        return SMB_VFS_NEXT_READLINK(handle, conn, path, buf, bufsiz);
}

static int catia_link(vfs_handle_struct *handle, connection_struct *conn, 
		      const char *oldpath, const char *newpath)
{
        return SMB_VFS_NEXT_LINK(handle, conn, oldpath, newpath);
}

static int catia_mknod(vfs_handle_struct *handle, connection_struct *conn, 
		       const char *path, mode_t mode, SMB_DEV_T dev)
{
        return SMB_VFS_NEXT_MKNOD(handle, conn, path, mode, dev);
}

static char *catia_realpath(vfs_handle_struct *handle, connection_struct *conn,
			    const char *path, char *resolved_path)
{
        return SMB_VFS_NEXT_REALPATH(handle, conn, path, resolved_path);
}

static size_t catia_get_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			       const char *name, uint32 security_info,
			       struct  security_descriptor_info **ppdesc)
{
        return SMB_VFS_NEXT_GET_NT_ACL(handle, fsp, name, security_info,
				       ppdesc);
}

static BOOL catia_set_nt_acl(vfs_handle_struct *handle, files_struct *fsp, 
			     const char *name, uint32 security_info_sent,
			     struct security_descriptor_info *psd)
{
        return SMB_VFS_NEXT_SET_NT_ACL(handle, fsp, name, security_info_sent,
				       psd);
}

static int catia_chmod_acl(vfs_handle_struct *handle, connection_struct *conn,
			   const char *name, mode_t mode)
{
        /* If the underlying VFS doesn't have ACL support... */
        if (!handle->vfs_next.ops.chmod_acl) {
                errno = ENOSYS;
                return -1;
        }
        return SMB_VFS_NEXT_CHMOD_ACL(handle, conn, name, mode);
}

/* VFS operations structure */

static vfs_op_tuple catia_op_tuples[] = {

        /* Directory operations */

        {SMB_VFS_OP(catia_opendir), SMB_VFS_OP_OPENDIR, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_readdir), SMB_VFS_OP_READDIR, 
SMB_VFS_LAYER_TRANSPARENT},

        /* File operations */

        {SMB_VFS_OP(catia_open), SMB_VFS_OP_OPEN, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_rename),                      SMB_VFS_OP_RENAME, 
        SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_stat), SMB_VFS_OP_STAT, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_lstat),                       SMB_VFS_OP_LSTAT,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_unlink),                      SMB_VFS_OP_UNLINK, 
        SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_chmod),                       SMB_VFS_OP_CHMOD,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_chown),                       SMB_VFS_OP_CHOWN,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_chdir),                       SMB_VFS_OP_CHDIR,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_getwd),                       SMB_VFS_OP_GETWD,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_utime),                       SMB_VFS_OP_UTIME,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_symlink), SMB_VFS_OP_SYMLINK, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_readlink), SMB_VFS_OP_READLINK, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_link), SMB_VFS_OP_LINK, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_mknod),                       SMB_VFS_OP_MKNOD,  
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_realpath), SMB_VFS_OP_REALPATH, 
SMB_VFS_LAYER_TRANSPARENT},

        /* NT File ACL operations */

        {SMB_VFS_OP(catia_get_nt_acl), SMB_VFS_OP_GET_NT_ACL, 
SMB_VFS_LAYER_TRANSPARENT},
        {SMB_VFS_OP(catia_set_nt_acl), SMB_VFS_OP_SET_NT_ACL, 
SMB_VFS_LAYER_TRANSPARENT},

        /* POSIX ACL operations */

        {SMB_VFS_OP(catia_chmod_acl), SMB_VFS_OP_CHMOD_ACL, 
SMB_VFS_LAYER_TRANSPARENT},


        {NULL,                                          SMB_VFS_OP_NOOP,   
SMB_VFS_LAYER_NOOP}
};

NTSTATUS init_module(void)
{
        return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "catia", 
catia_op_tuples);
}
