/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines for Dfs
 *  Copyright (C) Shirish Kalele	2000.
 *  Copyright (C) Jeremy Allison	2001.
 *  Copyright (C) Jelmer Vernooij	2005.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This is the implementation of the dfs pipe. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_MSDFS

/* This function does not return a WERROR or NTSTATUS code but rather 1 if
   dfs exists, or 0 otherwise. */

uint32 _dfs_GetManagerVersion(pipes_struct *p, NETDFS_Q_DFS_GETMANAGERVERSION *q_u, NETDFS_R_DFS_GETMANAGERVERSION *r_u)
{
	if(lp_host_msdfs()) 
		return 1;
	else
		return 0;
}

WERROR _dfs_Add(pipes_struct *p, NETDFS_Q_DFS_ADD* q_u, NETDFS_R_DFS_ADD *r_u)
{
	struct current_user user;
	struct junction_map jn;
	struct referral* old_referral_list = NULL;
	BOOL exists = False;

	pstring dfspath, servername, sharename;
	pstring altpath;

	get_current_user(&user,p);

	if (user.ut.uid != 0) {
		DEBUG(10,("_dfs_add: uid != 0. Access denied.\n"));
		return WERR_ACCESS_DENIED;
	}

	unistr2_to_ascii(dfspath, &q_u->path, sizeof(dfspath)-1);
	unistr2_to_ascii(servername, &q_u->server, sizeof(servername)-1);
	unistr2_to_ascii(sharename, &q_u->share, sizeof(sharename)-1);

	DEBUG(5,("init_reply_dfs_add: Request to add %s -> %s\\%s.\n",
		dfspath, servername, sharename));

	pstrcpy(altpath, servername);
	pstrcat(altpath, "\\");
	pstrcat(altpath, sharename);

	/* The following call can change the cwd. */
	if(get_referred_path(p->mem_ctx, dfspath, &jn, NULL, NULL)) {
		exists = True;
		jn.referral_count += 1;
		old_referral_list = jn.referral_list;
	} else {
		jn.referral_count = 1;
	}

	vfs_ChDir(p->conn,p->conn->connectpath);

	jn.referral_list = TALLOC_ARRAY(p->mem_ctx, struct referral, jn.referral_count);
	if(jn.referral_list == NULL) {
		DEBUG(0,("init_reply_dfs_add: talloc failed for referral list!\n"));
		return WERR_DFS_INTERNAL_ERROR;
	}

	if(old_referral_list) {
		memcpy(jn.referral_list, old_referral_list, sizeof(struct referral)*jn.referral_count-1);
	}
  
	jn.referral_list[jn.referral_count-1].proximity = 0;
	jn.referral_list[jn.referral_count-1].ttl = REFERRAL_TTL;

	pstrcpy(jn.referral_list[jn.referral_count-1].alternate_path, altpath);
  
	if(!create_msdfs_link(&jn, exists)) {
		vfs_ChDir(p->conn,p->conn->connectpath);
		return WERR_DFS_CANT_CREATE_JUNCT;
	}
	vfs_ChDir(p->conn,p->conn->connectpath);

	return WERR_OK;
}

WERROR _dfs_Remove(pipes_struct *p, NETDFS_Q_DFS_REMOVE *q_u, 
                   NETDFS_R_DFS_REMOVE *r_u)
{
	struct current_user user;
	struct junction_map jn;
	BOOL found = False;

	pstring dfspath, servername, sharename;
	pstring altpath;

	get_current_user(&user,p);

	if (user.ut.uid != 0) {
		DEBUG(10,("_dfs_remove: uid != 0. Access denied.\n"));
		return WERR_ACCESS_DENIED;
	}

	unistr2_to_ascii(dfspath, &q_u->path, sizeof(dfspath)-1);
	if(q_u->ptr0_server) {
		unistr2_to_ascii(servername, &q_u->server, sizeof(servername)-1);
	}

	if(q_u->ptr0_share) {
		unistr2_to_ascii(sharename, &q_u->share, sizeof(sharename)-1);
	}

	if(q_u->ptr0_server && q_u->ptr0_share) {
		pstrcpy(altpath, servername);
		pstrcat(altpath, "\\");
		pstrcat(altpath, sharename);
		strlower_m(altpath);
	}

	DEBUG(5,("init_reply_dfs_remove: Request to remove %s -> %s\\%s.\n",
		dfspath, servername, sharename));

	if(!get_referred_path(p->mem_ctx, dfspath, &jn, NULL, NULL)) {
		return WERR_DFS_NO_SUCH_VOL;
	}

	/* if no server-share pair given, remove the msdfs link completely */
	if(!q_u->ptr0_server && !q_u->ptr0_share) {
		if(!remove_msdfs_link(&jn)) {
			vfs_ChDir(p->conn,p->conn->connectpath);
			return WERR_DFS_NO_SUCH_VOL;
		}
		vfs_ChDir(p->conn,p->conn->connectpath);
	} else {
		int i=0;
		/* compare each referral in the list with the one to remove */
		DEBUG(10,("altpath: .%s. refcnt: %d\n", altpath, jn.referral_count));
		for(i=0;i<jn.referral_count;i++) {
			pstring refpath;
			pstrcpy(refpath,jn.referral_list[i].alternate_path);
			trim_char(refpath, '\\', '\\');
			DEBUG(10,("_dfs_remove:  refpath: .%s.\n", refpath));
			if(strequal(refpath, altpath)) {
				*(jn.referral_list[i].alternate_path)='\0';
				DEBUG(10,("_dfs_remove: Removal request matches referral %s\n",
					refpath));
				found = True;
			}
		}

		if(!found) {
			return WERR_DFS_NO_SUCH_SHARE;
		}

		/* Only one referral, remove it */
		if(jn.referral_count == 1) {
			if(!remove_msdfs_link(&jn)) {
				vfs_ChDir(p->conn,p->conn->connectpath);
				return WERR_DFS_NO_SUCH_VOL;
			}
		} else {
			if(!create_msdfs_link(&jn, True)) { 
				vfs_ChDir(p->conn,p->conn->connectpath);
				return WERR_DFS_CANT_CREATE_JUNCT;
			}
		}
		vfs_ChDir(p->conn,p->conn->connectpath);
	}

	return WERR_OK;
}

static BOOL init_reply_dfs_info_1(struct junction_map* j, NETDFS_DFS_INFO1* dfs1)
{
	pstring str;
	dfs1->ptr0_path = 1;
	slprintf(str, sizeof(pstring)-1, "\\\\%s\\%s\\%s", global_myname(), 
		j->service_name, j->volume_name);
	DEBUG(5,("init_reply_dfs_info_1: initing entrypath: %s\n",str));
	init_unistr2(&dfs1->path,str,UNI_STR_TERMINATE);
	return True;
}

static BOOL init_reply_dfs_info_2(struct junction_map* j, NETDFS_DFS_INFO2* dfs2)
{
	pstring str;
	dfs2->ptr0_path = 1;
	slprintf(str, sizeof(pstring)-1, "\\\\%s\\%s\\%s", global_myname(),
		j->service_name, j->volume_name);
	init_unistr2(&dfs2->path, str, UNI_STR_TERMINATE);
	dfs2->ptr0_comment = 0;
	init_unistr2(&dfs2->comment, j->comment, UNI_STR_TERMINATE);
	dfs2->state = 1; /* set up state of dfs junction as OK */
	dfs2->num_stores = j->referral_count;
	return True;
}

static BOOL init_reply_dfs_info_3(TALLOC_CTX *ctx, struct junction_map* j, NETDFS_DFS_INFO3* dfs3)
{
	int ii;
	pstring str;
	dfs3->ptr0_path = 1;
	if (j->volume_name[0] == '\0')
		slprintf(str, sizeof(pstring)-1, "\\\\%s\\%s",
			global_myname(), j->service_name);
	else
		slprintf(str, sizeof(pstring)-1, "\\\\%s\\%s\\%s", global_myname(),
			j->service_name, j->volume_name);

	init_unistr2(&dfs3->path, str, UNI_STR_TERMINATE);
	dfs3->ptr0_comment = 1;
	init_unistr2(&dfs3->comment, j->comment, UNI_STR_TERMINATE);
	dfs3->state = 1;
	dfs3->num_stores = dfs3->size_stores = j->referral_count;
	dfs3->ptr0_stores = 1;
    
	/* also enumerate the stores */
	dfs3->stores = TALLOC_ARRAY(ctx, NETDFS_DFS_STORAGEINFO, j->referral_count);
	if (!dfs3->stores)
		return False;

	memset(dfs3->stores, '\0', j->referral_count * sizeof(NETDFS_DFS_STORAGEINFO));

	for(ii=0;ii<j->referral_count;ii++) {
		char* p; 
		pstring path;
		NETDFS_DFS_STORAGEINFO* stor = &(dfs3->stores[ii]);
		struct referral* ref = &(j->referral_list[ii]);
  
		pstrcpy(path, ref->alternate_path);
		trim_char(path,'\\','\0');
		p = strrchr_m(path,'\\');
		if(p==NULL) {
			DEBUG(4,("init_reply_dfs_info_3: invalid path: no \\ found in %s\n",path));
			continue;
		}
		*p = '\0';
		DEBUG(5,("storage %d: %s.%s\n",ii,path,p+1));
		stor->state = 2; /* set all stores as ONLINE */
		init_unistr2(&stor->server, path, UNI_STR_TERMINATE);
		init_unistr2(&stor->share,  p+1, UNI_STR_TERMINATE);
		stor->ptr0_server = stor->ptr0_share = 1;
	}
	return True;
}

static BOOL init_reply_dfs_info_100(struct junction_map* j, NETDFS_DFS_INFO100* dfs100)
{
	dfs100->ptr0_comment = 1;
	init_unistr2(&dfs100->comment, j->comment, UNI_STR_TERMINATE);
	return True;
}


WERROR _dfs_Enum(pipes_struct *p, NETDFS_Q_DFS_ENUM *q_u, NETDFS_R_DFS_ENUM *r_u)
{
	uint32 level = q_u->level;
	struct junction_map jn[MAX_MSDFS_JUNCTIONS];
	int num_jn = 0;
	int i;

	num_jn = enum_msdfs_links(p->mem_ctx, jn, ARRAY_SIZE(jn));
	vfs_ChDir(p->conn,p->conn->connectpath);
    
	DEBUG(5,("_dfs_Enum: %d junctions found in Dfs, doing level %d\n", num_jn, level));

	r_u->ptr0_info = q_u->ptr0_info;
	r_u->ptr0_total = q_u->ptr0_total;
	r_u->total = num_jn;

	r_u->info = q_u->info;

	/* Create the return array */
	switch (level) {
	case 1:
		if ((r_u->info.e.u.info1.s = TALLOC_ARRAY(p->mem_ctx, NETDFS_DFS_INFO1, num_jn)) == NULL) {
			return WERR_NOMEM;
		}
		r_u->info.e.u.info1.count = num_jn;
		r_u->info.e.u.info1.ptr0_s = 1;
		r_u->info.e.u.info1.size_s = num_jn;
		break;
	case 2:
		if ((r_u->info.e.u.info2.s = TALLOC_ARRAY(p->mem_ctx, NETDFS_DFS_INFO2, num_jn)) == NULL) {
			return WERR_NOMEM;
		}
		r_u->info.e.u.info2.count = num_jn;
		r_u->info.e.u.info2.ptr0_s = 1;
		r_u->info.e.u.info2.size_s = num_jn;
		break;
	case 3:
		if ((r_u->info.e.u.info3.s = TALLOC_ARRAY(p->mem_ctx, NETDFS_DFS_INFO3, num_jn)) == NULL) {
			return WERR_NOMEM;
		}
		r_u->info.e.u.info3.count = num_jn;
		r_u->info.e.u.info3.ptr0_s = 1;
		r_u->info.e.u.info3.size_s = num_jn;
		break;
	default:
		return WERR_INVALID_PARAM;
	}

	for (i = 0; i < num_jn; i++) {
		switch (level) {
		case 1: 
			init_reply_dfs_info_1(&jn[i], &r_u->info.e.u.info1.s[i]);
			break;
		case 2:
			init_reply_dfs_info_2(&jn[i], &r_u->info.e.u.info2.s[i]);
			break;
		case 3:
			init_reply_dfs_info_3(p->mem_ctx, &jn[i], &r_u->info.e.u.info3.s[i]);
			break;
		default:
			return WERR_INVALID_PARAM;
		}
	}
  
	r_u->status = WERR_OK;

	return r_u->status;
}
      
WERROR _dfs_GetInfo(pipes_struct *p, NETDFS_Q_DFS_GETINFO *q_u, 
                     NETDFS_R_DFS_GETINFO *r_u)
{
	UNISTR2* uni_path = &q_u->path;
	uint32 level = q_u->level;
	int consumedcnt = sizeof(pstring);
	pstring path;
	BOOL ret = False;
	struct junction_map jn;

	unistr2_to_ascii(path, uni_path, sizeof(path)-1);
	if(!create_junction(path, &jn))
		return WERR_DFS_NO_SUCH_SERVER;
  
	/* The following call can change the cwd. */
	if(!get_referred_path(p->mem_ctx, path, &jn, &consumedcnt, NULL) || consumedcnt < strlen(path)) {
		vfs_ChDir(p->conn,p->conn->connectpath);
		return WERR_DFS_NO_SUCH_VOL;
	}

	vfs_ChDir(p->conn,p->conn->connectpath);
	r_u->info.switch_value = level;
	r_u->info.ptr0 = 1;
	r_u->status = WERR_OK;

	switch (level) {
		case 1: ret = init_reply_dfs_info_1(&jn, &r_u->info.u.info1); break;
		case 2: ret = init_reply_dfs_info_2(&jn, &r_u->info.u.info2); break;
		case 3: ret = init_reply_dfs_info_3(p->mem_ctx, &jn, &r_u->info.u.info3); break;
		case 100: ret = init_reply_dfs_info_100(&jn, &r_u->info.u.info100); break;
		default:
			r_u->info.ptr0 = 1;
			r_u->info.switch_value = 0;
			r_u->status = WERR_OK;
			ret = True;
			break;
	}

	if (!ret) 
		r_u->status = WERR_INVALID_PARAM;
  
	return r_u->status;
}

WERROR _dfs_SetInfo(pipes_struct *p, NETDFS_Q_DFS_SETINFO *q_u, NETDFS_R_DFS_SETINFO *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_Rename(pipes_struct *p, NETDFS_Q_DFS_RENAME *q_u, NETDFS_R_DFS_RENAME *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_Move(pipes_struct *p, NETDFS_Q_DFS_MOVE *q_u, NETDFS_R_DFS_MOVE *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_ManagerGetConfigInfo(pipes_struct *p, NETDFS_Q_DFS_MANAGERGETCONFIGINFO *q_u, NETDFS_R_DFS_MANAGERGETCONFIGINFO *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_ManagerSendSiteInfo(pipes_struct *p, NETDFS_Q_DFS_MANAGERSENDSITEINFO *q_u, NETDFS_R_DFS_MANAGERSENDSITEINFO *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_AddFtRoot(pipes_struct *p, NETDFS_Q_DFS_ADDFTROOT *q_u, NETDFS_R_DFS_ADDFTROOT *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_RemoveFtRoot(pipes_struct *p, NETDFS_Q_DFS_REMOVEFTROOT *q_u, NETDFS_R_DFS_REMOVEFTROOT *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_AddStdRoot(pipes_struct *p, NETDFS_Q_DFS_ADDSTDROOT *q_u, NETDFS_R_DFS_ADDSTDROOT *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_RemoveStdRoot(pipes_struct *p, NETDFS_Q_DFS_REMOVESTDROOT *q_u, NETDFS_R_DFS_REMOVESTDROOT *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_ManagerInitialize(pipes_struct *p, NETDFS_Q_DFS_MANAGERINITIALIZE *q_u, NETDFS_R_DFS_MANAGERINITIALIZE *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_AddStdRootForced(pipes_struct *p, NETDFS_Q_DFS_ADDSTDROOTFORCED *q_u, NETDFS_R_DFS_ADDSTDROOTFORCED *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_GetDcAddress(pipes_struct *p, NETDFS_Q_DFS_GETDCADDRESS *q_u, NETDFS_R_DFS_GETDCADDRESS *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_SetDcAddress(pipes_struct *p, NETDFS_Q_DFS_SETDCADDRESS *q_u, NETDFS_R_DFS_SETDCADDRESS *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_FlushFtTable(pipes_struct *p, NETDFS_Q_DFS_FLUSHFTTABLE *q_u, NETDFS_R_DFS_FLUSHFTTABLE *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_Add2(pipes_struct *p, NETDFS_Q_DFS_ADD2 *q_u, NETDFS_R_DFS_ADD2 *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_Remove2(pipes_struct *p, NETDFS_Q_DFS_REMOVE2 *q_u, NETDFS_R_DFS_REMOVE2 *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_EnumEx(pipes_struct *p, NETDFS_Q_DFS_ENUMEX *q_u, NETDFS_R_DFS_ENUMEX *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

WERROR _dfs_SetInfo2(pipes_struct *p, NETDFS_Q_DFS_SETINFO2 *q_u, NETDFS_R_DFS_SETINFO2 *r_u)
{
	/* FIXME: Implement your code here */
	return WERR_NOT_SUPPORTED;
}

