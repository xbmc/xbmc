/* 
   Unix SMB/CIFS implementation.
   SMB wrapper functions - definitions
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

#ifndef _SMBW_H
#define _SMBW_H

#define SMBW_PREFIX "/smb/"
#define SMBW_DUMMY "/dev/null"

#define SMBW_CLI_FD 512
#define SMBW_MAX_OPEN 8192

#define SMBW_FILE_MODE (S_IFREG | 0444)
#define SMBW_DIR_MODE (S_IFDIR | 0555)

struct smbw_server {
	struct smbw_server *next, *prev;
	struct cli_state cli;
	char *server_name;
	char *share_name;
	char *workgroup;
	char *username;
	dev_t dev;
	BOOL no_pathinfo2;
};

struct smbw_filedes {
	int cli_fd;
	int ref_count;
	char *fname;
	off_t offset;
};

struct smbw_file {
	struct smbw_file *next, *prev;
	struct smbw_filedes *f;
	int fd;
	struct smbw_server *srv;
};

struct smbw_dir {
	struct smbw_dir *next, *prev;
	int fd;
	int offset, count, malloced;
	struct smbw_server *srv;
	struct file_info *list;
	char *path;
};

typedef void (*smbw_get_auth_data_fn)(char *server, char *share,
				      char **workgroup, char **username,
				      char **password);

#endif /* _SMBW_H */
