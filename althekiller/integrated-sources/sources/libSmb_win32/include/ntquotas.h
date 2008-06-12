/* 
   Unix SMB/CIFS implementation.
   NT QUOTA code constants
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

#ifndef _NTQUOTAS_H
#define _NTQUOTAS_H

/* 
 * details for Quota Flags:
 * 
 * 0x20 Log Limit: log if the user exceeds his Hard Quota
 * 0x10 Log Warn:  log if the user exceeds his Soft Quota
 * 0x02 Deny Disk: deny disk access when the user exceeds his Hard Quota
 * 0x01 Enable Quotas: enable quota for this fs
 *
 */

#define QUOTAS_ENABLED		0x0001
#define QUOTAS_DENY_DISK	0x0002
#define QUOTAS_LOG_VIOLATIONS	0x0004
#define CONTENT_INDEX_DISABLED	0x0008
#define QUOTAS_LOG_THRESHOLD	0x0010
#define QUOTAS_LOG_LIMIT	0x0020
#define LOG_VOLUME_THRESHOLD	0x0040
#define LOG_VOLUME_LIMIT	0x0080
#define QUOTAS_INCOMPLETE	0x0100
#define QUOTAS_REBUILDING	0x0200
#define QUOTAS_0400		0x0400
#define QUOTAS_0800		0x0800
#define QUOTAS_1000		0x1000
#define QUOTAS_2000		0x2000
#define QUOTAS_4000		0x4000
#define QUOTAS_8000		0x8000

#define SMB_NTQUOTAS_NO_LIMIT	((SMB_BIG_UINT)(-1))
#define SMB_NTQUOTAS_NO_ENTRY	((SMB_BIG_UINT)(-2))
#define SMB_NTQUOTAS_NO_SPACE	((SMB_BIG_UINT)(0))
#define SMB_NTQUOTAS_1_B	(SMB_BIG_UINT)0x0000000000000001
#define SMB_NTQUOTAS_1KB	(SMB_BIG_UINT)0x0000000000000400
#define SMB_NTQUOTAS_1MB	(SMB_BIG_UINT)0x0000000000100000
#define SMB_NTQUOTAS_1GB	(SMB_BIG_UINT)0x0000000040000000
#define SMB_NTQUOTAS_1TB	(SMB_BIG_UINT)0x0000010000000000
#define SMB_NTQUOTAS_1PB	(SMB_BIG_UINT)0x0004000000000000
#define SMB_NTQUOTAS_1EB	(SMB_BIG_UINT)0x1000000000000000

enum SMB_QUOTA_TYPE {
	SMB_INVALID_QUOTA_TYPE = -1,
	SMB_USER_FS_QUOTA_TYPE = 1,
	SMB_USER_QUOTA_TYPE = 2,
	SMB_GROUP_FS_QUOTA_TYPE = 3,/* not used yet */
	SMB_GROUP_QUOTA_TYPE = 4 /* not in use yet, maybe for disk_free queries */
};

typedef struct _SMB_NTQUOTA_STRUCT {
	enum SMB_QUOTA_TYPE qtype;
	SMB_BIG_UINT usedspace;
	SMB_BIG_UINT softlim;
	SMB_BIG_UINT hardlim;
	uint32 qflags;
	DOM_SID sid;
} SMB_NTQUOTA_STRUCT;

typedef struct _SMB_NTQUOTA_LIST {
	struct _SMB_NTQUOTA_LIST *prev,*next;
	TALLOC_CTX *mem_ctx;
	uid_t uid;
	SMB_NTQUOTA_STRUCT *quotas;
} SMB_NTQUOTA_LIST;

typedef struct _SMB_NTQUOTA_HANDLE {
	BOOL valid;
	SMB_NTQUOTA_LIST *quota_list;
	SMB_NTQUOTA_LIST *tmp_list;
} SMB_NTQUOTA_HANDLE;

#define CHECK_NTQUOTA_HANDLE_OK(fsp,conn)	(FNUM_OK(fsp,conn) &&\
	 (fsp)->fake_file_handle &&\
	 ((fsp)->fake_file_handle->type == FAKE_FILE_TYPE_QUOTA) &&\
	 (fsp)->fake_file_handle->pd)

#endif /*_NTQUOTAS_H */
