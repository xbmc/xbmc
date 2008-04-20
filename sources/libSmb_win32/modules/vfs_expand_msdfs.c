/* 
 * Expand msdfs targets based on client IP
 *
 * Copyright (C) Volker Lendecke, 2004
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_VFS

/**********************************************************
  Under mapfile we expect a table of the following format:

  IP-Prefix whitespace expansion

  For example:
  192.168.234 local.samba.org
  192.168     remote.samba.org
              default.samba.org

  This is to redirect a DFS client to a host close to it.
***********************************************************/

static BOOL read_target_host(const char *mapfile, pstring targethost)
{
	XFILE *f;
	pstring buf;
	char *space = buf;
	BOOL found = False;
	
	f = x_fopen(mapfile, O_RDONLY, 0);

	if (f == NULL) {
		DEBUG(0,("can't open IP map %s. Error %s\n",
			 mapfile, strerror(errno) ));
		return False;
	}

	DEBUG(10, ("Scanning mapfile [%s]\n", mapfile));

	while (x_fgets(buf, sizeof(buf), f) != NULL) {

		if ((strlen(buf) > 0) && (buf[strlen(buf)-1] == '\n'))
			buf[strlen(buf)-1] = '\0';

		DEBUG(10, ("Scanning line [%s]\n", buf));

		space = strchr_m(buf, ' ');

		if (space == NULL) {
			DEBUG(0, ("Ignoring invalid line %s\n", buf));
			continue;
		}

		*space = '\0';

		if (strncmp(client_addr(), buf, strlen(buf)) == 0) {
			found = True;
			break;
		}
	}

	x_fclose(f);

	if (!found)
		return False;

	space += 1;

	while (isspace(*space))
		space += 1;

	pstrcpy(targethost, space);
	return True;
}

/**********************************************************

  Expand the msdfs target host using read_target_host
  explained above. The syntax used in the msdfs link is

  msdfs:@table-filename@/share

  Everything between and including the two @-signs is
  replaced by the substitution string found in the table
  described above.

***********************************************************/

static BOOL expand_msdfs_target(connection_struct* conn, pstring target)
{
	pstring mapfilename;
	char *filename_start = strchr_m(target, '@');
	char *filename_end;
	int filename_len;
	pstring targethost;
	pstring new_target;

	if (filename_start == NULL) {
		DEBUG(10, ("No filename start in %s\n", target));
		return False;
	}

	filename_end = strchr_m(filename_start+1, '@');

	if (filename_end == NULL) {
		DEBUG(10, ("No filename end in %s\n", target));
		return False;
	}

	filename_len = PTR_DIFF(filename_end, filename_start+1);
	pstrcpy(mapfilename, filename_start+1);
	mapfilename[filename_len] = '\0';

	DEBUG(10, ("Expanding from table [%s]\n", mapfilename));

	if (!read_target_host(mapfilename, targethost)) {
		DEBUG(1, ("Could not expand target host from file %s\n",
			  mapfilename));
		return False;
	}

	standard_sub_conn(conn, mapfilename, sizeof(mapfilename));

	DEBUG(10, ("Expanded targethost to %s\n", targethost));

	*filename_start = '\0';
	pstrcpy(new_target, target);
	pstrcat(new_target, targethost);
	pstrcat(new_target, filename_end+1);

	DEBUG(10, ("New DFS target: %s\n", new_target));
	pstrcpy(target, new_target);
	return True;
}

static int expand_msdfs_readlink(struct vfs_handle_struct *handle,
				 struct connection_struct *conn,
				 const char *path, char *buf, size_t bufsiz)
{
	pstring target;
	int result;

	result = SMB_VFS_NEXT_READLINK(handle, conn, path, target,
				       sizeof(target));

	if (result < 0)
		return result;

	target[result] = '\0';

	if ((strncmp(target, "msdfs:", strlen("msdfs:")) == 0) &&
	    (strchr_m(target, '@') != NULL)) {
		if (!expand_msdfs_target(conn, target)) {
			errno = ENOENT;
			return -1;
		}
	}

	safe_strcpy(buf, target, bufsiz-1);
	return strlen(buf);
}

/* VFS operations structure */

static vfs_op_tuple expand_msdfs_ops[] = {	
	{SMB_VFS_OP(expand_msdfs_readlink), SMB_VFS_OP_READLINK,
	 SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(NULL), SMB_VFS_OP_NOOP, SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_expand_msdfs_init(void)
{
	return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "expand_msdfs",
				expand_msdfs_ops);
}
