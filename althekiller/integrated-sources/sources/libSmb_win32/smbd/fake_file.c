/* 
   Unix SMB/CIFS implementation.
   FAKE FILE suppport, for faking up special files windows want access to
   Copyright (C) Stefan (metze) Metzmacher	2003
   
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

#include "includes.h"

extern struct current_user current_user;

static FAKE_FILE fake_files[] = {
#ifdef WITH_QUOTAS
	{FAKE_FILE_NAME_QUOTA_UNIX,	FAKE_FILE_TYPE_QUOTA,	init_quota_handle,	destroy_quota_handle},
#endif /* WITH_QUOTAS */
	{NULL,				FAKE_FILE_TYPE_NONE,	NULL,			NULL }
};

/****************************************************************************
 Create a fake file handle
****************************************************************************/

static struct _FAKE_FILE_HANDLE *init_fake_file_handle(enum FAKE_FILE_TYPE type)
{
	TALLOC_CTX *mem_ctx = NULL;
	FAKE_FILE_HANDLE *fh = NULL;
	int i;

	for (i=0;fake_files[i].name!=NULL;i++) {
		if (fake_files[i].type==type) {
			DEBUG(5,("init_fake_file_handle: for [%s]\n",fake_files[i].name));

			if ((mem_ctx=talloc_init("fake_file_handle"))==NULL) {
				DEBUG(0,("talloc_init(fake_file_handle) failed.\n"));
				return NULL;	
			}

			if ((fh =TALLOC_ZERO_P(mem_ctx, FAKE_FILE_HANDLE))==NULL) {
				DEBUG(0,("talloc_zero() failed.\n"));
				talloc_destroy(mem_ctx);
				return NULL;
			}

			fh->type = type;
			fh->mem_ctx = mem_ctx;

			if (fake_files[i].init_pd) {
				fh->pd = fake_files[i].init_pd(fh->mem_ctx);
			}

			fh->free_pd = fake_files[i].free_pd;

			return fh;
		}
	}

	return NULL;	
}

/****************************************************************************
 Does this name match a fake filename ?
****************************************************************************/

enum FAKE_FILE_TYPE is_fake_file(const char *fname)
{
#ifdef HAVE_SYS_QUOTAS
	int i;
#endif

	if (!fname) {
		return FAKE_FILE_TYPE_NONE;
	}

#ifdef HAVE_SYS_QUOTAS
	for (i=0;fake_files[i].name!=NULL;i++) {
		if (strncmp(fname,fake_files[i].name,strlen(fake_files[i].name))==0) {
			DEBUG(5,("is_fake_file: [%s] is a fake file\n",fname));
			return fake_files[i].type;
		}
	}
#endif

	return FAKE_FILE_TYPE_NONE;
}


/****************************************************************************
 Open a fake quota file with a share mode.
****************************************************************************/

files_struct *open_fake_file(connection_struct *conn,
				enum FAKE_FILE_TYPE fake_file_type,
				const char *fname,
				uint32 access_mask)
{
	files_struct *fsp = NULL;

	/* access check */
	if (current_user.ut.uid != 0) {
		DEBUG(1,("open_fake_file_shared: access_denied to service[%s] file[%s] user[%s]\n",
			lp_servicename(SNUM(conn)),fname,conn->user));
		errno = EACCES;
		return NULL;
	}

	fsp = file_new(conn);
	if(!fsp) {
		return NULL;
	}

	DEBUG(5,("open_fake_file_shared: fname = %s, FID = %d, access_mask = 0x%x\n",
		fname, fsp->fnum, (unsigned int)access_mask));

	fsp->conn = conn;
	fsp->fh->fd = -1;
	fsp->vuid = current_user.vuid;
	fsp->fh->pos = -1;
	fsp->can_lock = True; /* Should this be true ? */
	fsp->access_mask = access_mask;
	string_set(&fsp->fsp_name,fname);
	
	fsp->fake_file_handle = init_fake_file_handle(fake_file_type);
	
	if (fsp->fake_file_handle==NULL) {
		file_free(fsp);
		return NULL;
	}

	conn->num_files_open++;
	return fsp;
}

void destroy_fake_file_handle(FAKE_FILE_HANDLE **fh)
{
	if (!fh||!(*fh)) {
		return;
	}

	if ((*fh)->free_pd) {
		(*fh)->free_pd(&(*fh)->pd);		
	}

	talloc_destroy((*fh)->mem_ctx);
	(*fh) = NULL;
}

int close_fake_file(files_struct *fsp)
{
	file_free(fsp);
	return 0;
}
