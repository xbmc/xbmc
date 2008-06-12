/* 
   Unix SMB/CIFS implementation.
   NT QUOTA suppport
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_QUOTA

static SMB_BIG_UINT limit_nt2unix(SMB_BIG_UINT in, SMB_BIG_UINT bsize)
{
	SMB_BIG_UINT ret = (SMB_BIG_UINT)0;

	ret = 	(SMB_BIG_UINT)(in/bsize);
	if (in>0 && ret==0) {
		/* we have to make sure that a overflow didn't set NO_LIMIT */
		ret = (SMB_BIG_UINT)1;
	}

	if (in == SMB_NTQUOTAS_NO_LIMIT)
		ret = SMB_QUOTAS_NO_LIMIT;
	else if (in == SMB_NTQUOTAS_NO_SPACE)
		ret = SMB_QUOTAS_NO_SPACE;
	else if (in == SMB_NTQUOTAS_NO_ENTRY)
		ret = SMB_QUOTAS_NO_LIMIT;

	return ret;
}

static SMB_BIG_UINT limit_unix2nt(SMB_BIG_UINT in, SMB_BIG_UINT bsize)
{
	SMB_BIG_UINT ret = (SMB_BIG_UINT)0;

	ret = (SMB_BIG_UINT)(in*bsize);
	
	if (ret < in) {
		/* we overflow */
		ret = SMB_NTQUOTAS_NO_LIMIT;
	}

	if (in == SMB_QUOTAS_NO_LIMIT)
		ret = SMB_NTQUOTAS_NO_LIMIT;

	return ret;
}

static SMB_BIG_UINT limit_blk2inodes(SMB_BIG_UINT in)
{
	SMB_BIG_UINT ret = (SMB_BIG_UINT)0;
	
	ret = (SMB_BIG_UINT)(in/2);
	
	if (ret == 0 && in != 0)
		ret = (SMB_BIG_UINT)1;

	return ret;	
}

int vfs_get_ntquota(files_struct *fsp, enum SMB_QUOTA_TYPE qtype, DOM_SID *psid, SMB_NTQUOTA_STRUCT *qt)
{
	int ret;
	SMB_DISK_QUOTA D;
	unid_t id;

	ZERO_STRUCT(D);

	if (!fsp||!fsp->conn||!qt)
		return (-1);

	ZERO_STRUCT(*qt);

	id.uid = -1;

	if (psid && !sid_to_uid(psid, &id.uid)) {
		DEBUG(0,("sid_to_uid: failed, SID[%s]\n",
			sid_string_static(psid)));	
	}

	ret = SMB_VFS_GET_QUOTA(fsp->conn, qtype, id, &D);

	if (psid)
		qt->sid    = *psid;

	if (ret!=0) {
		return ret;
	}
		
	qt->usedspace = (SMB_BIG_UINT)D.curblocks*D.bsize;
	qt->softlim = limit_unix2nt(D.softlimit, D.bsize);
	qt->hardlim = limit_unix2nt(D.hardlimit, D.bsize);
	qt->qflags = D.qflags;

	
	return 0;
}

int vfs_set_ntquota(files_struct *fsp, enum SMB_QUOTA_TYPE qtype, DOM_SID *psid, SMB_NTQUOTA_STRUCT *qt)
{
	int ret;
	SMB_DISK_QUOTA D;
	unid_t id;
	ZERO_STRUCT(D);

	if (!fsp||!fsp->conn||!qt)
		return (-1);

	id.uid = -1;

	D.bsize     = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	D.softlimit = limit_nt2unix(qt->softlim,D.bsize);
	D.hardlimit = limit_nt2unix(qt->hardlim,D.bsize);
	D.qflags     = qt->qflags;

	D.isoftlimit = limit_blk2inodes(D.softlimit);
	D.ihardlimit = limit_blk2inodes(D.hardlimit);

	if (psid && !sid_to_uid(psid, &id.uid)) {
		DEBUG(0,("sid_to_uid: failed, SID[%s]\n",
			sid_string_static(psid)));	
	}

	ret = SMB_VFS_SET_QUOTA(fsp->conn, qtype, id, &D);
	
	return ret;
}

static BOOL allready_in_quota_list(SMB_NTQUOTA_LIST *qt_list, uid_t uid)
{
	SMB_NTQUOTA_LIST *tmp_list = NULL;
	
	if (!qt_list)
		return False;

	for (tmp_list=qt_list;tmp_list!=NULL;tmp_list=tmp_list->next) {
		if (tmp_list->uid == uid) {
			return True;	
		}
	}

	return False;
}

int vfs_get_user_ntquota_list(files_struct *fsp, SMB_NTQUOTA_LIST **qt_list)
{
	struct passwd *usr;
	TALLOC_CTX *mem_ctx = NULL;

	if (!fsp||!fsp->conn||!qt_list)
		return (-1);

	*qt_list = NULL;

	if ((mem_ctx=talloc_init("SMB_USER_QUOTA_LIST"))==NULL) {
		DEBUG(0,("talloc_init() failed\n"));
		return (-1);
	}

	sys_setpwent();
	while ((usr = sys_getpwent()) != NULL) {
		SMB_NTQUOTA_STRUCT tmp_qt;
		SMB_NTQUOTA_LIST *tmp_list_ent;
		DOM_SID	sid;

		ZERO_STRUCT(tmp_qt);

		if (allready_in_quota_list((*qt_list),usr->pw_uid)) {
			DEBUG(5,("record for uid[%ld] allready in the list\n",(long)usr->pw_uid));
			continue;
		}

		uid_to_sid(&sid, usr->pw_uid);

		if (vfs_get_ntquota(fsp, SMB_USER_QUOTA_TYPE, &sid, &tmp_qt)!=0) {
			DEBUG(5,("no quota entry for sid[%s] path[%s]\n",
				sid_string_static(&sid),fsp->conn->connectpath));
			continue;
		}

		DEBUG(15,("quota entry for id[%s] path[%s]\n",
			sid_string_static(&sid),fsp->conn->connectpath));

		if ((tmp_list_ent=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_LIST))==NULL) {
			DEBUG(0,("talloc_zero() failed\n"));
			*qt_list = NULL;
			talloc_destroy(mem_ctx);
			return (-1);
		}

		if ((tmp_list_ent->quotas=TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_STRUCT))==NULL) {
			DEBUG(0,("talloc_zero() failed\n"));
			*qt_list = NULL;
			talloc_destroy(mem_ctx);
			return (-1);
		}

		tmp_list_ent->uid = usr->pw_uid;
		memcpy(tmp_list_ent->quotas,&tmp_qt,sizeof(tmp_qt));
		tmp_list_ent->mem_ctx = mem_ctx;

		DLIST_ADD((*qt_list),tmp_list_ent);
		
	}
	sys_endpwent();

	return 0;
}

void *init_quota_handle(TALLOC_CTX *mem_ctx)
{
	SMB_NTQUOTA_HANDLE *qt_handle;

	if (!mem_ctx)
		return False;

	qt_handle = TALLOC_ZERO_P(mem_ctx,SMB_NTQUOTA_HANDLE);
	if (qt_handle==NULL) {
		DEBUG(0,("talloc_zero() failed\n"));
		return NULL;
	}

	return (void *)qt_handle;	
}

void destroy_quota_handle(void **pqt_handle)
{
	SMB_NTQUOTA_HANDLE *qt_handle = NULL;
	if (!pqt_handle||!(*pqt_handle))
		return;
	
	qt_handle = (*pqt_handle);
	
	
	if (qt_handle->quota_list)
		free_ntquota_list(&qt_handle->quota_list);

	qt_handle->quota_list = NULL;
	qt_handle->tmp_list = NULL;
	qt_handle = NULL;

	return;
}
