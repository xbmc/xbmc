/* 
   Unix SMB/CIFS implementation.
   VFS module tester

   Copyright (C) Simo Sorce 2002
   Copyright (C) Eric Lorimer 2002

   Most of this code was ripped off of rpcclient.
   Copyright (C) Tim Potter 2000-2001

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

struct func_entry {
	char *name;
	int (*fn)(struct connection_struct *conn, const char *path);
};

struct vfs_state {
	struct connection_struct *conn;
	struct files_struct *files[1024];
	DIR *currentdir;
	void *data;
	size_t data_size;
};

struct cmd_set {
	const char *name;
	NTSTATUS (*fn)(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, 
                       const char **argv);
	const char *description;
	const char *usage;
};
