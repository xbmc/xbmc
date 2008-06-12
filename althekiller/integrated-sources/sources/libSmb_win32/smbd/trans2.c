/* 
   Unix SMB/CIFS implementation.
   SMB transaction2 handling
   Copyright (C) Jeremy Allison			1994-2003
   Copyright (C) Stefan (metze) Metzmacher	2003
   Copyright (C) Volker Lendecke		2005
   Copyright (C) Steve French			2005

   Extensively modified by Andrew Tridgell, 1995

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

extern int max_send;
extern enum protocol_types Protocol;
extern int smb_read_error;
extern uint32 global_client_caps;
extern struct current_user current_user;

#define get_file_size(sbuf) ((sbuf).st_size)
#define DIR_ENTRY_SAFETY_MARGIN 4096

/********************************************************************
 Roundup a value to the nearest allocation roundup size boundary.
 Only do this for Windows clients.
********************************************************************/

SMB_BIG_UINT smb_roundup(connection_struct *conn, SMB_BIG_UINT val)
{
	SMB_BIG_UINT rval = lp_allocation_roundup_size(SNUM(conn));

	/* Only roundup for Windows clients. */
	enum remote_arch_types ra_type = get_remote_arch();
	if (rval && (ra_type != RA_SAMBA) && (ra_type != RA_CIFSFS)) {
		val = SMB_ROUNDUP(val,rval);
	}
	return val;
}

/********************************************************************
 Given a stat buffer return the allocated size on disk, taking into
 account sparse files.
********************************************************************/

SMB_BIG_UINT get_allocation_size(connection_struct *conn, files_struct *fsp, SMB_STRUCT_STAT *sbuf)
{
	SMB_BIG_UINT ret;

	if(S_ISDIR(sbuf->st_mode)) {
		return 0;
	}

#if defined(HAVE_STAT_ST_BLOCKS) && defined(STAT_ST_BLOCKSIZE)
	ret = (SMB_BIG_UINT)STAT_ST_BLOCKSIZE * (SMB_BIG_UINT)sbuf->st_blocks;
#else
	ret = (SMB_BIG_UINT)get_file_size(*sbuf);
#endif

	if (fsp && fsp->initial_allocation_size)
		ret = MAX(ret,fsp->initial_allocation_size);

	return smb_roundup(conn, ret);
}

/****************************************************************************
 Utility functions for dealing with extended attributes.
****************************************************************************/

static const char *prohibited_ea_names[] = {
	SAMBA_POSIX_INHERITANCE_EA_NAME,
	SAMBA_XATTR_DOS_ATTRIB,
	NULL
};

/****************************************************************************
 Refuse to allow clients to overwrite our private xattrs.
****************************************************************************/

static BOOL samba_private_attr_name(const char *unix_ea_name)
{
	int i;

	for (i = 0; prohibited_ea_names[i]; i++) {
		if (strequal( prohibited_ea_names[i], unix_ea_name))
			return True;
	}
	return False;
}

/****************************************************************************
 Get one EA value. Fill in a struct ea_struct.
****************************************************************************/

static BOOL get_ea_value(TALLOC_CTX *mem_ctx, connection_struct *conn, files_struct *fsp,
				const char *fname, char *ea_name, struct ea_struct *pea)
{
	/* Get the value of this xattr. Max size is 64k. */
	size_t attr_size = 256;
	char *val = NULL;
	ssize_t sizeret;

 again:

	val = TALLOC_REALLOC_ARRAY(mem_ctx, val, char, attr_size);
	if (!val) {
		return False;
	}

	if (fsp && fsp->fh->fd != -1) {
		sizeret = SMB_VFS_FGETXATTR(fsp, fsp->fh->fd, ea_name, val, attr_size);
	} else {
		sizeret = SMB_VFS_GETXATTR(conn, fname, ea_name, val, attr_size);
	}

	if (sizeret == -1 && errno == ERANGE && attr_size != 65536) {
		attr_size = 65536;
		goto again;
	}

	if (sizeret == -1) {
		return False;
	}

	DEBUG(10,("get_ea_value: EA %s is of length %u: ", ea_name, (unsigned int)sizeret));
	dump_data(10, val, sizeret);

	pea->flags = 0;
	if (strnequal(ea_name, "user.", 5)) {
		pea->name = &ea_name[5];
	} else {
		pea->name = ea_name;
	}
	pea->value.data = (unsigned char *)val;
	pea->value.length = (size_t)sizeret;
	return True;
}

/****************************************************************************
 Return a linked list of the total EA's. Plus the total size
****************************************************************************/

static struct ea_list *get_ea_list_from_file(TALLOC_CTX *mem_ctx, connection_struct *conn, files_struct *fsp,
					const char *fname, size_t *pea_total_len)
{
	/* Get a list of all xattrs. Max namesize is 64k. */
	size_t ea_namelist_size = 1024;
	char *ea_namelist;
	char *p;
	ssize_t sizeret;
	int i;
	struct ea_list *ea_list_head = NULL;

	*pea_total_len = 0;

	if (!lp_ea_support(SNUM(conn))) {
		return NULL;
	}

	for (i = 0, ea_namelist = TALLOC(mem_ctx, ea_namelist_size); i < 6;
			ea_namelist = TALLOC_REALLOC_ARRAY(mem_ctx, ea_namelist, char, ea_namelist_size), i++) {

		if (!ea_namelist) {
			return NULL;
		}

		if (fsp && fsp->fh->fd != -1) {
			sizeret = SMB_VFS_FLISTXATTR(fsp, fsp->fh->fd, ea_namelist, ea_namelist_size);
		} else {
			sizeret = SMB_VFS_LISTXATTR(conn, fname, ea_namelist, ea_namelist_size);
		}

		if (sizeret == -1 && errno == ERANGE) {
			ea_namelist_size *= 2;
		} else {
			break;
		}
	}

	if (sizeret == -1)
		return NULL;

	DEBUG(10,("get_ea_list_from_file: ea_namelist size = %u\n", (unsigned int)sizeret ));

	if (sizeret) {
		for (p = ea_namelist; p - ea_namelist < sizeret; p += strlen(p) + 1) {
			struct ea_list *listp, *tmp;

			if (strnequal(p, "system.", 7) || samba_private_attr_name(p))
				continue;
		
			listp = TALLOC_P(mem_ctx, struct ea_list);
			if (!listp)
				return NULL;

			if (!get_ea_value(mem_ctx, conn, fsp, fname, p, &listp->ea)) {
				return NULL;
			}

			{
				fstring dos_ea_name;
				push_ascii_fstring(dos_ea_name, listp->ea.name);
				*pea_total_len += 4 + strlen(dos_ea_name) + 1 + listp->ea.value.length;
				DEBUG(10,("get_ea_list_from_file: total_len = %u, %s, val len = %u\n",
					(unsigned int)*pea_total_len, dos_ea_name,
					(unsigned int)listp->ea.value.length ));
			}
			DLIST_ADD_END(ea_list_head, listp, tmp);
		}
		/* Add on 4 for total length. */
		if (*pea_total_len) {
			*pea_total_len += 4;
		}
	}

	DEBUG(10,("get_ea_list_from_file: total_len = %u\n", (unsigned int)*pea_total_len));
	return ea_list_head;
}

/****************************************************************************
 Fill a qfilepathinfo buffer with EA's. Returns the length of the buffer
 that was filled.
****************************************************************************/

static unsigned int fill_ea_buffer(TALLOC_CTX *mem_ctx, char *pdata, unsigned int total_data_size,
	connection_struct *conn, struct ea_list *ea_list)
{
	unsigned int ret_data_size = 4;
	char *p = pdata;

	SMB_ASSERT(total_data_size >= 4);

	if (!lp_ea_support(SNUM(conn))) {
		SIVAL(pdata,4,0);
		return 4;
	}

	for (p = pdata + 4; ea_list; ea_list = ea_list->next) {
		size_t dos_namelen;
		fstring dos_ea_name;
		push_ascii_fstring(dos_ea_name, ea_list->ea.name);
		dos_namelen = strlen(dos_ea_name);
		if (dos_namelen > 255 || dos_namelen == 0) {
			break;
		}
		if (ea_list->ea.value.length > 65535) {
			break;
		}
		if (4 + dos_namelen + 1 + ea_list->ea.value.length > total_data_size) {
			break;
		}

		/* We know we have room. */
		SCVAL(p,0,ea_list->ea.flags);
		SCVAL(p,1,dos_namelen);
		SSVAL(p,2,ea_list->ea.value.length);
		fstrcpy(p+4, dos_ea_name);
		memcpy( p + 4 + dos_namelen + 1, ea_list->ea.value.data, ea_list->ea.value.length);

		total_data_size -= 4 + dos_namelen + 1 + ea_list->ea.value.length;
		p += 4 + dos_namelen + 1 + ea_list->ea.value.length;
	}

	ret_data_size = PTR_DIFF(p, pdata);
	DEBUG(10,("fill_ea_buffer: data_size = %u\n", ret_data_size ));
	SIVAL(pdata,0,ret_data_size);
	return ret_data_size;
}

static unsigned int estimate_ea_size(connection_struct *conn, files_struct *fsp, const char *fname)
{
	size_t total_ea_len = 0;
	TALLOC_CTX *mem_ctx = NULL;

	if (!lp_ea_support(SNUM(conn))) {
		return 0;
	}
	mem_ctx = talloc_init("estimate_ea_size");
	(void)get_ea_list_from_file(mem_ctx, conn, fsp, fname, &total_ea_len);
	talloc_destroy(mem_ctx);
	return total_ea_len;
}

/****************************************************************************
 Ensure the EA name is case insensitive by matching any existing EA name.
****************************************************************************/

static void canonicalize_ea_name(connection_struct *conn, files_struct *fsp, const char *fname, fstring unix_ea_name)
{
	size_t total_ea_len;
	TALLOC_CTX *mem_ctx = talloc_init("canonicalize_ea_name");
	struct ea_list *ea_list = get_ea_list_from_file(mem_ctx, conn, fsp, fname, &total_ea_len);

	for (; ea_list; ea_list = ea_list->next) {
		if (strequal(&unix_ea_name[5], ea_list->ea.name)) {
			DEBUG(10,("canonicalize_ea_name: %s -> %s\n",
				&unix_ea_name[5], ea_list->ea.name));
			safe_strcpy(&unix_ea_name[5], ea_list->ea.name, sizeof(fstring)-6);
			break;
		}
	}
	talloc_destroy(mem_ctx);
}

/****************************************************************************
 Set or delete an extended attribute.
****************************************************************************/

NTSTATUS set_ea(connection_struct *conn, files_struct *fsp, const char *fname, struct ea_list *ea_list)
{
	if (!lp_ea_support(SNUM(conn))) {
		return NT_STATUS_EAS_NOT_SUPPORTED;
	}

	for (;ea_list; ea_list = ea_list->next) {
		int ret;
		fstring unix_ea_name;

		fstrcpy(unix_ea_name, "user."); /* All EA's must start with user. */
		fstrcat(unix_ea_name, ea_list->ea.name);

		canonicalize_ea_name(conn, fsp, fname, unix_ea_name);

		DEBUG(10,("set_ea: ea_name %s ealen = %u\n", unix_ea_name, (unsigned int)ea_list->ea.value.length));

		if (samba_private_attr_name(unix_ea_name)) {
			DEBUG(10,("set_ea: ea name %s is a private Samba name.\n", unix_ea_name));
			return NT_STATUS_ACCESS_DENIED;
		}

		if (ea_list->ea.value.length == 0) {
			/* Remove the attribute. */
			if (fsp && (fsp->fh->fd != -1)) {
				DEBUG(10,("set_ea: deleting ea name %s on file %s by file descriptor.\n",
					unix_ea_name, fsp->fsp_name));
				ret = SMB_VFS_FREMOVEXATTR(fsp, fsp->fh->fd, unix_ea_name);
			} else {
				DEBUG(10,("set_ea: deleting ea name %s on file %s.\n",
					unix_ea_name, fname));
				ret = SMB_VFS_REMOVEXATTR(conn, fname, unix_ea_name);
			}
#ifdef ENOATTR
			/* Removing a non existent attribute always succeeds. */
			if (ret == -1 && errno == ENOATTR) {
				DEBUG(10,("set_ea: deleting ea name %s didn't exist - succeeding by default.\n",
						unix_ea_name));
				ret = 0;
			}
#endif
		} else {
			if (fsp && (fsp->fh->fd != -1)) {
				DEBUG(10,("set_ea: setting ea name %s on file %s by file descriptor.\n",
					unix_ea_name, fsp->fsp_name));
				ret = SMB_VFS_FSETXATTR(fsp, fsp->fh->fd, unix_ea_name,
							ea_list->ea.value.data, ea_list->ea.value.length, 0);
			} else {
				DEBUG(10,("set_ea: setting ea name %s on file %s.\n",
					unix_ea_name, fname));
				ret = SMB_VFS_SETXATTR(conn, fname, unix_ea_name,
							ea_list->ea.value.data, ea_list->ea.value.length, 0);
			}
		}

		if (ret == -1) {
#ifdef ENOTSUP
			if (errno == ENOTSUP) {
				return NT_STATUS_EAS_NOT_SUPPORTED;
			}
#endif
			return map_nt_error_from_unix(errno);
		}

	}
	return NT_STATUS_OK;
}
/****************************************************************************
 Read a list of EA names from an incoming data buffer. Create an ea_list with them.
****************************************************************************/

static struct ea_list *read_ea_name_list(TALLOC_CTX *ctx, const char *pdata, size_t data_size)
{
	struct ea_list *ea_list_head = NULL;
	size_t offset = 0;

	while (offset + 2 < data_size) {
		struct ea_list *tmp;
		struct ea_list *eal = TALLOC_ZERO_P(ctx, struct ea_list);
		unsigned int namelen = CVAL(pdata,offset);

		offset++; /* Go past the namelen byte. */

		/* integer wrap paranioa. */
		if ((offset + namelen < offset) || (offset + namelen < namelen) ||
				(offset > data_size) || (namelen > data_size) ||
				(offset + namelen >= data_size)) {
			break;
		}
		/* Ensure the name is null terminated. */
		if (pdata[offset + namelen] != '\0') {
			return NULL;
		}
		pull_ascii_talloc(ctx, &eal->ea.name, &pdata[offset]);
		if (!eal->ea.name) {
			return NULL;
		}

		offset += (namelen + 1); /* Go past the name + terminating zero. */
		DLIST_ADD_END(ea_list_head, eal, tmp);
		DEBUG(10,("read_ea_name_list: read ea name %s\n", eal->ea.name));
	}

	return ea_list_head;
}

/****************************************************************************
 Read one EA list entry from the buffer.
****************************************************************************/

struct ea_list *read_ea_list_entry(TALLOC_CTX *ctx, const char *pdata, size_t data_size, size_t *pbytes_used)
{
	struct ea_list *eal = TALLOC_ZERO_P(ctx, struct ea_list);
	uint16 val_len;
	unsigned int namelen;

	if (!eal) {
		return NULL;
	}

	if (data_size < 6) {
		return NULL;
	}

	eal->ea.flags = CVAL(pdata,0);
	namelen = CVAL(pdata,1);
	val_len = SVAL(pdata,2);

	if (4 + namelen + 1 + val_len > data_size) {
		return NULL;
	}

	/* Ensure the name is null terminated. */
	if (pdata[namelen + 4] != '\0') {
		return NULL;
	}
	pull_ascii_talloc(ctx, &eal->ea.name, pdata + 4);
	if (!eal->ea.name) {
		return NULL;
	}

	eal->ea.value = data_blob(NULL, (size_t)val_len + 1);
	if (!eal->ea.value.data) {
		return NULL;
	}

	memcpy(eal->ea.value.data, pdata + 4 + namelen + 1, val_len);

	/* Ensure we're null terminated just in case we print the value. */
	eal->ea.value.data[val_len] = '\0';
	/* But don't count the null. */
	eal->ea.value.length--;

	if (pbytes_used) {
		*pbytes_used = 4 + namelen + 1 + val_len;
	}

	DEBUG(10,("read_ea_list_entry: read ea name %s\n", eal->ea.name));
	dump_data(10, (const char *)eal->ea.value.data, eal->ea.value.length);

	return eal;
}

/****************************************************************************
 Read a list of EA names and data from an incoming data buffer. Create an ea_list with them.
****************************************************************************/

static struct ea_list *read_ea_list(TALLOC_CTX *ctx, const char *pdata, size_t data_size)
{
	struct ea_list *ea_list_head = NULL;
	size_t offset = 0;
	size_t bytes_used = 0;

	while (offset < data_size) {
		struct ea_list *tmp;
		struct ea_list *eal = read_ea_list_entry(ctx, pdata + offset, data_size - offset, &bytes_used);

		if (!eal) {
			return NULL;
		}

		DLIST_ADD_END(ea_list_head, eal, tmp);
		offset += bytes_used;
	}

	return ea_list_head;
}

/****************************************************************************
 Count the total EA size needed.
****************************************************************************/

static size_t ea_list_size(struct ea_list *ealist)
{
	fstring dos_ea_name;
	struct ea_list *listp;
	size_t ret = 0;

	for (listp = ealist; listp; listp = listp->next) {
		push_ascii_fstring(dos_ea_name, listp->ea.name);
		ret += 4 + strlen(dos_ea_name) + 1 + listp->ea.value.length;
	}
	/* Add on 4 for total length. */
	if (ret) {
		ret += 4;
	}

	return ret;
}

/****************************************************************************
 Return a union of EA's from a file list and a list of names.
 The TALLOC context for the two lists *MUST* be identical as we steal
 memory from one list to add to another. JRA.
****************************************************************************/

static struct ea_list *ea_list_union(struct ea_list *name_list, struct ea_list *file_list, size_t *total_ea_len)
{
	struct ea_list *nlistp, *flistp;

	for (nlistp = name_list; nlistp; nlistp = nlistp->next) {
		for (flistp = file_list; flistp; flistp = flistp->next) {
			if (strequal(nlistp->ea.name, flistp->ea.name)) {
				break;
			}
		}

		if (flistp) {
			/* Copy the data from this entry. */
			nlistp->ea.flags = flistp->ea.flags;
			nlistp->ea.value = flistp->ea.value;
		} else {
			/* Null entry. */
			nlistp->ea.flags = 0;
			ZERO_STRUCT(nlistp->ea.value);
		}
	}

	*total_ea_len = ea_list_size(name_list);
	return name_list;
}

/****************************************************************************
  Send the required number of replies back.
  We assume all fields other than the data fields are
  set correctly for the type of call.
  HACK ! Always assumes smb_setup field is zero.
****************************************************************************/

int send_trans2_replies(char *outbuf,
			int bufsize,
			char *params, 
			int paramsize,
			char *pdata,
			int datasize)
{
	/* As we are using a protocol > LANMAN1 then the max_send
	 variable must have been set in the sessetupX call.
	 This takes precedence over the max_xmit field in the
	 global struct. These different max_xmit variables should
	 be merged as this is now too confusing */

	int data_to_send = datasize;
	int params_to_send = paramsize;
	int useable_space;
	char *pp = params;
	char *pd = pdata;
	int params_sent_thistime, data_sent_thistime, total_sent_thistime;
	int alignment_offset = 1; /* JRA. This used to be 3. Set to 1 to make netmon parse ok. */
	int data_alignment_offset = 0;

	/* Initially set the wcnt area to be 10 - this is true for all trans2 replies */
	
	set_message(outbuf,10,0,True);

	/* If there genuinely are no parameters or data to send just send the empty packet */

	if(params_to_send == 0 && data_to_send == 0) {
		show_msg(outbuf);
		if (!send_smb(smbd_server_fd(),outbuf))
			exit_server("send_trans2_replies: send_smb failed.");
		return 0;
	}

	/* When sending params and data ensure that both are nicely aligned */
	/* Only do this alignment when there is also data to send - else
		can cause NT redirector problems. */

	if (((params_to_send % 4) != 0) && (data_to_send != 0))
		data_alignment_offset = 4 - (params_to_send % 4);

	/* Space is bufsize minus Netbios over TCP header minus SMB header */
	/* The alignment_offset is to align the param bytes on an even byte
		boundary. NT 4.0 Beta needs this to work correctly. */

	useable_space = bufsize - ((smb_buf(outbuf)+ alignment_offset+data_alignment_offset) - outbuf);

	/* useable_space can never be more than max_send minus the alignment offset. */

	useable_space = MIN(useable_space, max_send - (alignment_offset+data_alignment_offset));

	while (params_to_send || data_to_send) {
		/* Calculate whether we will totally or partially fill this packet */

		total_sent_thistime = params_to_send + data_to_send + alignment_offset + data_alignment_offset;

		/* We can never send more than useable_space */
		/*
		 * Note that 'useable_space' does not include the alignment offsets,
		 * but we must include the alignment offsets in the calculation of
		 * the length of the data we send over the wire, as the alignment offsets
		 * are sent here. Fix from Marc_Jacobsen@hp.com.
		 */

		total_sent_thistime = MIN(total_sent_thistime, useable_space+ alignment_offset + data_alignment_offset);

		set_message(outbuf, 10, total_sent_thistime, True);

		/* Set total params and data to be sent */
		SSVAL(outbuf,smb_tprcnt,paramsize);
		SSVAL(outbuf,smb_tdrcnt,datasize);

		/* Calculate how many parameters and data we can fit into
		 * this packet. Parameters get precedence
		 */

		params_sent_thistime = MIN(params_to_send,useable_space);
		data_sent_thistime = useable_space - params_sent_thistime;
		data_sent_thistime = MIN(data_sent_thistime,data_to_send);

		SSVAL(outbuf,smb_prcnt, params_sent_thistime);

		/* smb_proff is the offset from the start of the SMB header to the
			parameter bytes, however the first 4 bytes of outbuf are
			the Netbios over TCP header. Thus use smb_base() to subtract
			them from the calculation */

		SSVAL(outbuf,smb_proff,((smb_buf(outbuf)+alignment_offset) - smb_base(outbuf)));

		if(params_sent_thistime == 0)
			SSVAL(outbuf,smb_prdisp,0);
		else
			/* Absolute displacement of param bytes sent in this packet */
			SSVAL(outbuf,smb_prdisp,pp - params);

		SSVAL(outbuf,smb_drcnt, data_sent_thistime);
		if(data_sent_thistime == 0) {
			SSVAL(outbuf,smb_droff,0);
			SSVAL(outbuf,smb_drdisp, 0);
		} else {
			/* The offset of the data bytes is the offset of the
				parameter bytes plus the number of parameters being sent this time */
			SSVAL(outbuf,smb_droff,((smb_buf(outbuf)+alignment_offset) - 
				smb_base(outbuf)) + params_sent_thistime + data_alignment_offset);
			SSVAL(outbuf,smb_drdisp, pd - pdata);
		}

		/* Copy the param bytes into the packet */

		if(params_sent_thistime)
			memcpy((smb_buf(outbuf)+alignment_offset),pp,params_sent_thistime);

		/* Copy in the data bytes */
		if(data_sent_thistime)
			memcpy(smb_buf(outbuf)+alignment_offset+params_sent_thistime+
				data_alignment_offset,pd,data_sent_thistime);

		DEBUG(9,("t2_rep: params_sent_thistime = %d, data_sent_thistime = %d, useable_space = %d\n",
			params_sent_thistime, data_sent_thistime, useable_space));
		DEBUG(9,("t2_rep: params_to_send = %d, data_to_send = %d, paramsize = %d, datasize = %d\n",
			params_to_send, data_to_send, paramsize, datasize));

		/* Send the packet */
		show_msg(outbuf);
		if (!send_smb(smbd_server_fd(),outbuf))
			exit_server("send_trans2_replies: send_smb failed.");

		pp += params_sent_thistime;
		pd += data_sent_thistime;

		params_to_send -= params_sent_thistime;
		data_to_send -= data_sent_thistime;

		/* Sanity check */
		if(params_to_send < 0 || data_to_send < 0) {
			DEBUG(0,("send_trans2_replies failed sanity check pts = %d, dts = %d\n!!!",
				params_to_send, data_to_send));
			return -1;
		}
	}

	return 0;
}

/****************************************************************************
 Reply to a TRANSACT2_OPEN.
****************************************************************************/

static int call_trans2open(connection_struct *conn, char *inbuf, char *outbuf, int bufsize,  
				char **pparams, int total_params, char **ppdata, int total_data,
				unsigned int max_data_bytes)
{
	char *params = *pparams;
	char *pdata = *ppdata;
	int deny_mode;
	int32 open_attr;
	BOOL oplock_request;
#if 0
	BOOL return_additional_info;
	int16 open_sattr;
	time_t open_time;
#endif
	int open_ofun;
	uint32 open_size;
	char *pname;
	pstring fname;
	SMB_OFF_T size=0;
	int fattr=0,mtime=0;
	SMB_INO_T inode = 0;
	SMB_STRUCT_STAT sbuf;
	int smb_action = 0;
	BOOL bad_path = False;
	files_struct *fsp;
	TALLOC_CTX *ctx = NULL;
	struct ea_list *ea_list = NULL;
	uint16 flags = 0;
	NTSTATUS status;
	uint32 access_mask;
	uint32 share_mode;
	uint32 create_disposition;
	uint32 create_options = 0;

	/*
	 * Ensure we have enough parameters to perform the operation.
	 */

	if (total_params < 29) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	flags = SVAL(params, 0);
	deny_mode = SVAL(params, 2);
	open_attr = SVAL(params,6);
        oplock_request = (flags & REQUEST_OPLOCK) ? EXCLUSIVE_OPLOCK : 0;
        if (oplock_request) {
                oplock_request |= (flags & REQUEST_BATCH_OPLOCK) ? BATCH_OPLOCK : 0;
        }

#if 0
	return_additional_info = BITSETW(params,0);
	open_sattr = SVAL(params, 4);
	open_time = make_unix_date3(params+8);
#endif
	open_ofun = SVAL(params,12);
	open_size = IVAL(params,14);
	pname = &params[28];

	if (IS_IPC(conn)) {
		return(ERROR_DOS(ERRSRV,ERRaccess));
	}

	srvstr_get_path(inbuf, fname, pname, sizeof(fname), -1, STR_TERMINATE, &status);
	if (!NT_STATUS_IS_OK(status)) {
		return ERROR_NT(status);
	}

	DEBUG(3,("call_trans2open %s deny_mode=0x%x attr=%d ofun=0x%x size=%d\n",
		fname, (unsigned int)deny_mode, (unsigned int)open_attr,
		(unsigned int)open_ofun, open_size));

	/* XXXX we need to handle passed times, sattr and flags */

	unix_convert(fname,conn,0,&bad_path,&sbuf);
	if (bad_path) {
		return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
	}
    
	if (!check_name(fname,conn)) {
		return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRnoaccess);
	}

	if (!map_open_params_to_ntcreate(fname, deny_mode, open_ofun,
				&access_mask,
				&share_mode,
				&create_disposition,
				&create_options)) {
		return ERROR_DOS(ERRDOS, ERRbadaccess);
	}

	/* Any data in this call is an EA list. */
	if (total_data && (total_data != 4) && !lp_ea_support(SNUM(conn))) {
		return ERROR_NT(NT_STATUS_EAS_NOT_SUPPORTED);
	}

	if (total_data != 4) {
		if (total_data < 10) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		if (IVAL(pdata,0) > total_data) {
			DEBUG(10,("call_trans2open: bad total data size (%u) > %u\n",
				IVAL(pdata,0), (unsigned int)total_data));
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		ctx = talloc_init("TRANS2_OPEN_SET_EA");
		if (!ctx) {
			return ERROR_NT(NT_STATUS_NO_MEMORY);
		}
		ea_list = read_ea_list(ctx, pdata + 4, total_data - 4);
		if (!ea_list) {
			talloc_destroy(ctx);
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
	} else if (IVAL(pdata,0) != 4) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	fsp = open_file_ntcreate(conn,fname,&sbuf,
		access_mask,
		share_mode,
		create_disposition,
		create_options,
		open_attr,
		oplock_request,
		&smb_action);
      
	if (!fsp) {
		talloc_destroy(ctx);
		if (open_was_deferred(SVAL(inbuf,smb_mid))) {
			/* We have re-scheduled this call. */
			return -1;
		}
		return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRnoaccess);
	}

	size = get_file_size(sbuf);
	fattr = dos_mode(conn,fname,&sbuf);
	mtime = sbuf.st_mtime;
	inode = sbuf.st_ino;
	if (fattr & aDIR) {
		talloc_destroy(ctx);
		close_file(fsp,ERROR_CLOSE);
		return(ERROR_DOS(ERRDOS,ERRnoaccess));
	}

	/* Save the requested allocation size. */
	/* Allocate space for the file if a size hint is supplied */
	if ((smb_action == FILE_WAS_CREATED) || (smb_action == FILE_WAS_OVERWRITTEN)) {
		SMB_BIG_UINT allocation_size = (SMB_BIG_UINT)open_size;
		if (allocation_size && (allocation_size > (SMB_BIG_UINT)size)) {
                        fsp->initial_allocation_size = smb_roundup(fsp->conn, allocation_size);
                        if (fsp->is_directory) {
                                close_file(fsp,ERROR_CLOSE);
                                /* Can't set allocation size on a directory. */
                                return ERROR_NT(NT_STATUS_ACCESS_DENIED);
                        }
                        if (vfs_allocate_file_space(fsp, fsp->initial_allocation_size) == -1) {
                                close_file(fsp,ERROR_CLOSE);
                                return ERROR_NT(NT_STATUS_DISK_FULL);
                        }

			/* Adjust size here to return the right size in the reply.
			   Windows does it this way. */
			size = fsp->initial_allocation_size;
                } else {
                        fsp->initial_allocation_size = smb_roundup(fsp->conn,(SMB_BIG_UINT)size);
                }
	}

	if (total_data && smb_action == FILE_WAS_CREATED) {
		status = set_ea(conn, fsp, fname, ea_list);
		talloc_destroy(ctx);
		if (!NT_STATUS_IS_OK(status)) {
			close_file(fsp,ERROR_CLOSE);
			return ERROR_NT(status);
		}
	}

	/* Realloc the size of parameters and data we will return */
	*pparams = SMB_REALLOC(*pparams, 30);
	if(*pparams == NULL ) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	SSVAL(params,0,fsp->fnum);
	SSVAL(params,2,open_attr);
	srv_put_dos_date2(params,4, mtime);
	SIVAL(params,8, (uint32)size);
	SSVAL(params,12,deny_mode);
	SSVAL(params,14,0); /* open_type - file or directory. */
	SSVAL(params,16,0); /* open_state - only valid for IPC device. */

	if (oplock_request && lp_fake_oplocks(SNUM(conn))) {
		smb_action |= EXTENDED_OPLOCK_GRANTED;
	}

	SSVAL(params,18,smb_action);

	/*
	 * WARNING - this may need to be changed if SMB_INO_T <> 4 bytes.
	 */
	SIVAL(params,20,inode);
	SSVAL(params,24,0); /* Padding. */
	if (flags & 8) {
		uint32 ea_size = estimate_ea_size(conn, fsp, fname);
		SIVAL(params, 26, ea_size);
	} else {
		SIVAL(params, 26, 0);
	}

	/* Send the required number of replies */
	send_trans2_replies(outbuf, bufsize, params, 30, *ppdata, 0);

	return -1;
}

/*********************************************************
 Routine to check if a given string matches exactly.
 as a special case a mask of "." does NOT match. That
 is required for correct wildcard semantics
 Case can be significant or not.
**********************************************************/

static BOOL exact_match(connection_struct *conn, char *str, char *mask)
{
	if (mask[0] == '.' && mask[1] == 0)
		return False;
	if (conn->case_sensitive)
		return strcmp(str,mask)==0;
	if (StrCaseCmp(str,mask) != 0) {
		return False;
	}
	if (dptr_has_wild(conn->dirptr)) {
		return False;
	}
	return True;
}

/****************************************************************************
 Return the filetype for UNIX extensions.
****************************************************************************/

static uint32 unix_filetype(mode_t mode)
{
	if(S_ISREG(mode))
		return UNIX_TYPE_FILE;
	else if(S_ISDIR(mode))
		return UNIX_TYPE_DIR;
#ifdef S_ISLNK
	else if(S_ISLNK(mode))
		return UNIX_TYPE_SYMLINK;
#endif
#ifdef S_ISCHR
	else if(S_ISCHR(mode))
		return UNIX_TYPE_CHARDEV;
#endif
#ifdef S_ISBLK
	else if(S_ISBLK(mode))
		return UNIX_TYPE_BLKDEV;
#endif
#ifdef S_ISFIFO
	else if(S_ISFIFO(mode))
		return UNIX_TYPE_FIFO;
#endif
#ifdef S_ISSOCK
	else if(S_ISSOCK(mode))
		return UNIX_TYPE_SOCKET;
#endif

	DEBUG(0,("unix_filetype: unknown filetype %u", (unsigned)mode));
	return UNIX_TYPE_UNKNOWN;
}

/****************************************************************************
 Map wire perms onto standard UNIX permissions. Obey share restrictions.
****************************************************************************/

static mode_t unix_perms_from_wire( connection_struct *conn, SMB_STRUCT_STAT *pst, uint32 perms)
{
	mode_t ret = 0;

	if (perms == SMB_MODE_NO_CHANGE)
		return pst->st_mode;

	ret |= ((perms & UNIX_X_OTH ) ? S_IXOTH : 0);
	ret |= ((perms & UNIX_W_OTH ) ? S_IWOTH : 0);
	ret |= ((perms & UNIX_R_OTH ) ? S_IROTH : 0);
	ret |= ((perms & UNIX_X_GRP ) ? S_IXGRP : 0);
	ret |= ((perms & UNIX_W_GRP ) ? S_IWGRP : 0);
	ret |= ((perms & UNIX_R_GRP ) ? S_IRGRP : 0);
	ret |= ((perms & UNIX_X_USR ) ? S_IXUSR : 0);
	ret |= ((perms & UNIX_W_USR ) ? S_IWUSR : 0);
	ret |= ((perms & UNIX_R_USR ) ? S_IRUSR : 0);
#ifdef S_ISVTX
	ret |= ((perms & UNIX_STICKY ) ? S_ISVTX : 0);
#endif
#ifdef S_ISGID
	ret |= ((perms & UNIX_SET_GID ) ? S_ISGID : 0);
#endif
#ifdef S_ISUID
	ret |= ((perms & UNIX_SET_UID ) ? S_ISUID : 0);
#endif

	if (VALID_STAT(*pst) && S_ISDIR(pst->st_mode)) {
		ret &= lp_dir_mask(SNUM(conn));
		/* Add in force bits */
		ret |= lp_force_dir_mode(SNUM(conn));
	} else {
		/* Apply mode mask */
		ret &= lp_create_mask(SNUM(conn));
		/* Add in force bits */
		ret |= lp_force_create_mode(SNUM(conn));
	}

	return ret;
}

/****************************************************************************
 Get a level dependent lanman2 dir entry.
****************************************************************************/

static BOOL get_lanman2_dir_entry(connection_struct *conn,
				  void *inbuf, void *outbuf,
				 char *path_mask,uint32 dirtype,int info_level,
				 int requires_resume_key,
				 BOOL dont_descend,char **ppdata, 
				 char *base_data, int space_remaining, 
				 BOOL *out_of_space, BOOL *got_exact_match,
				 int *last_entry_off, struct ea_list *name_list, TALLOC_CTX *ea_ctx)
{
	const char *dname;
	BOOL found = False;
	SMB_STRUCT_STAT sbuf;
	pstring mask;
	pstring pathreal;
	pstring fname;
	char *p, *q, *pdata = *ppdata;
	uint32 reskey=0;
	long prev_dirpos=0;
	uint32 mode=0;
	SMB_OFF_T file_size = 0;
	SMB_BIG_UINT allocation_size = 0;
	uint32 len;
	time_t mdate=0, adate=0, cdate=0;
	char *nameptr;
	char *last_entry_ptr;
	BOOL was_8_3;
	uint32 nt_extmode; /* Used for NT connections instead of mode */
	BOOL needslash = ( conn->dirpath[strlen(conn->dirpath) -1] != '/');
	BOOL check_mangled_names = lp_manglednames(SNUM(conn));

	*fname = 0;
	*out_of_space = False;
	*got_exact_match = False;

	if (!conn->dirptr)
		return(False);

	p = strrchr_m(path_mask,'/');
	if(p != NULL) {
		if(p[1] == '\0')
			pstrcpy(mask,"*.*");
		else
			pstrcpy(mask, p+1);
	} else
		pstrcpy(mask, path_mask);


	while (!found) {
		BOOL got_match;
		BOOL ms_dfs_link = False;

		/* Needed if we run out of space */
		long curr_dirpos = prev_dirpos = dptr_TellDir(conn->dirptr);
		dname = dptr_ReadDirName(conn->dirptr,&curr_dirpos,&sbuf);

		/*
		 * Due to bugs in NT client redirectors we are not using
		 * resume keys any more - set them to zero.
		 * Check out the related comments in findfirst/findnext.
		 * JRA.
		 */

		reskey = 0;

		DEBUG(8,("get_lanman2_dir_entry:readdir on dirptr 0x%lx now at offset %ld\n",
			(long)conn->dirptr,curr_dirpos));
      
		if (!dname) 
			return(False);

		pstrcpy(fname,dname);      

		if(!(got_match = *got_exact_match = exact_match(conn, fname, mask)))
			got_match = mask_match(fname, mask, conn->case_sensitive);

		if(!got_match && check_mangled_names && !mangle_is_8_3(fname, False, SNUM(conn))) {

			/*
			 * It turns out that NT matches wildcards against
			 * both long *and* short names. This may explain some
			 * of the wildcard wierdness from old DOS clients
			 * that some people have been seeing.... JRA.
			 */

			pstring newname;
			pstrcpy( newname, fname);
			mangle_map( newname, True, False, SNUM(conn));
			if(!(got_match = *got_exact_match = exact_match(conn, newname, mask)))
				got_match = mask_match(newname, mask, conn->case_sensitive);
		}

		if(got_match) {
			BOOL isdots = (strequal(fname,"..") || strequal(fname,"."));
			if (dont_descend && !isdots)
				continue;
	  
			pstrcpy(pathreal,conn->dirpath);
			if(needslash)
				pstrcat(pathreal,"/");
			pstrcat(pathreal,dname);

			if (INFO_LEVEL_IS_UNIX(info_level)) {
				if (SMB_VFS_LSTAT(conn,pathreal,&sbuf) != 0) {
					DEBUG(5,("get_lanman2_dir_entry:Couldn't lstat [%s] (%s)\n",
						pathreal,strerror(errno)));
					continue;
				}
			} else if (!VALID_STAT(sbuf) && SMB_VFS_STAT(conn,pathreal,&sbuf) != 0) {

				/* Needed to show the msdfs symlinks as 
				 * directories */

				if(lp_host_msdfs() && 
				   lp_msdfs_root(SNUM(conn)) &&
				   ((ms_dfs_link = is_msdfs_link(NULL,conn, pathreal, NULL, NULL, &sbuf)) == True)) {

					DEBUG(5,("get_lanman2_dir_entry: Masquerading msdfs link %s as a directory\n", pathreal));
					sbuf.st_mode = (sbuf.st_mode & 0xFFF) | S_IFDIR;

				} else {

					DEBUG(5,("get_lanman2_dir_entry:Couldn't stat [%s] (%s)\n",
						pathreal,strerror(errno)));
					continue;
				}
			}

			if (ms_dfs_link) {
				mode = dos_mode_msdfs(conn,pathreal,&sbuf);
			} else {
				mode = dos_mode(conn,pathreal,&sbuf);
			}

			if (!dir_check_ftype(conn,mode,dirtype)) {
				DEBUG(5,("[%s] attribs didn't match %x\n",fname,dirtype));
				continue;
			}

			if (!(mode & aDIR))
				file_size = get_file_size(sbuf);
			allocation_size = get_allocation_size(conn,NULL,&sbuf);
			mdate = sbuf.st_mtime;
			adate = sbuf.st_atime;
			cdate = get_create_time(&sbuf,lp_fake_dir_create_times(SNUM(conn)));

			if (lp_dos_filetime_resolution(SNUM(conn))) {
				cdate &= ~1;
				mdate &= ~1;
				adate &= ~1;
			}


			DEBUG(5,("get_lanman2_dir_entry found %s fname=%s\n",pathreal,fname));
	  
			found = True;

			dptr_DirCacheAdd(conn->dirptr, dname, curr_dirpos);
		}
	}

	mangle_map(fname,False,True,SNUM(conn));

	p = pdata;
	last_entry_ptr = p;

	nt_extmode = mode ? mode : FILE_ATTRIBUTE_NORMAL;

	switch (info_level) {
		case SMB_FIND_INFO_STANDARD:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_INFO_STANDARD\n"));
			if(requires_resume_key) {
				SIVAL(p,0,reskey);
				p += 4;
			}
			srv_put_dos_date2(p,0,cdate);
			srv_put_dos_date2(p,4,adate);
			srv_put_dos_date2(p,8,mdate);
			SIVAL(p,12,(uint32)file_size);
			SIVAL(p,16,(uint32)allocation_size);
			SSVAL(p,20,mode);
			p += 23;
			nameptr = p;
			p += align_string(outbuf, p, 0);
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE);
			if (SVAL(outbuf, smb_flg2) & FLAGS2_UNICODE_STRINGS) {
				if (len > 2) {
					SCVAL(nameptr, -1, len - 2);
				} else {
					SCVAL(nameptr, -1, 0);
				}
			} else {
				if (len > 1) {
					SCVAL(nameptr, -1, len - 1);
				} else {
					SCVAL(nameptr, -1, 0);
				}
			}
			p += len;
			break;

		case SMB_FIND_EA_SIZE:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_EA_SIZE\n"));
			if(requires_resume_key) {
				SIVAL(p,0,reskey);
				p += 4;
			}
			srv_put_dos_date2(p,0,cdate);
			srv_put_dos_date2(p,4,adate);
			srv_put_dos_date2(p,8,mdate);
			SIVAL(p,12,(uint32)file_size);
			SIVAL(p,16,(uint32)allocation_size);
			SSVAL(p,20,mode);
			{
				unsigned int ea_size = estimate_ea_size(conn, NULL, pathreal);
				SIVAL(p,22,ea_size); /* Extended attributes */
			}
			p += 27;
			nameptr = p - 1;
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE | STR_NOALIGN);
			if (SVAL(outbuf, smb_flg2) & FLAGS2_UNICODE_STRINGS) {
				if (len > 2) {
					len -= 2;
				} else {
					len = 0;
				}
			} else {
				if (len > 1) {
					len -= 1;
				} else {
					len = 0;
				}
			}
			SCVAL(nameptr,0,len);
			p += len;
			SCVAL(p,0,0); p += 1; /* Extra zero byte ? - why.. */
			break;

		case SMB_FIND_EA_LIST:
		{
			struct ea_list *file_list = NULL;
			size_t ea_len = 0;

			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_EA_LIST\n"));
			if (!name_list) {
				return False;
			}
			if(requires_resume_key) {
				SIVAL(p,0,reskey);
				p += 4;
			}
			srv_put_dos_date2(p,0,cdate);
			srv_put_dos_date2(p,4,adate);
			srv_put_dos_date2(p,8,mdate);
			SIVAL(p,12,(uint32)file_size);
			SIVAL(p,16,(uint32)allocation_size);
			SSVAL(p,20,mode);
			p += 22; /* p now points to the EA area. */

			file_list = get_ea_list_from_file(ea_ctx, conn, NULL, pathreal, &ea_len);
			name_list = ea_list_union(name_list, file_list, &ea_len);

			/* We need to determine if this entry will fit in the space available. */
			/* Max string size is 255 bytes. */
			if (PTR_DIFF(p + 255 + ea_len,pdata) > space_remaining) {
				/* Move the dirptr back to prev_dirpos */
				dptr_SeekDir(conn->dirptr, prev_dirpos);
				*out_of_space = True;
				DEBUG(9,("get_lanman2_dir_entry: out of space\n"));
				return False; /* Not finished - just out of space */
			}

			/* Push the ea_data followed by the name. */
			p += fill_ea_buffer(ea_ctx, p, space_remaining, conn, name_list);
			nameptr = p;
			len = srvstr_push(outbuf, p + 1, fname, -1, STR_TERMINATE | STR_NOALIGN);
			if (SVAL(outbuf, smb_flg2) & FLAGS2_UNICODE_STRINGS) {
				if (len > 2) {
					len -= 2;
				} else {
					len = 0;
				}
			} else {
				if (len > 1) {
					len -= 1;
				} else {
					len = 0;
				}
			}
			SCVAL(nameptr,0,len);
			p += len + 1;
			SCVAL(p,0,0); p += 1; /* Extra zero byte ? - why.. */
			break;
		}

		case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_FILE_BOTH_DIRECTORY_INFO\n"));
			was_8_3 = mangle_is_8_3(fname, True, SNUM(conn));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			put_long_date(p,cdate); p += 8;
			put_long_date(p,adate); p += 8;
			put_long_date(p,mdate); p += 8;
			put_long_date(p,mdate); p += 8;
			SOFF_T(p,0,file_size); p += 8;
			SOFF_T(p,0,allocation_size); p += 8;
			SIVAL(p,0,nt_extmode); p += 4;
			q = p; p += 4; /* q is placeholder for name length. */
			{
				unsigned int ea_size = estimate_ea_size(conn, NULL, pathreal);
				SIVAL(p,0,ea_size); /* Extended attributes */
				p += 4;
			}
			/* Clear the short name buffer. This is
			 * IMPORTANT as not doing so will trigger
			 * a Win2k client bug. JRA.
			 */
			if (!was_8_3 && check_mangled_names) {
				pstring mangled_name;
				pstrcpy(mangled_name, fname);
				mangle_map(mangled_name,True,True,SNUM(conn));
				mangled_name[12] = 0;
				len = srvstr_push(outbuf, p+2, mangled_name, 24, STR_UPPER|STR_UNICODE);
				if (len < 24) {
					memset(p + 2 + len,'\0',24 - len);
				}
				SSVAL(p, 0, len);
			} else {
				memset(p,'\0',26);
			}
			p += 2 + 24;
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(q,0,len);
			p += len;
			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;

		case SMB_FIND_FILE_DIRECTORY_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_FILE_DIRECTORY_INFO\n"));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			put_long_date(p,cdate); p += 8;
			put_long_date(p,adate); p += 8;
			put_long_date(p,mdate); p += 8;
			put_long_date(p,mdate); p += 8;
			SOFF_T(p,0,file_size); p += 8;
			SOFF_T(p,0,allocation_size); p += 8;
			SIVAL(p,0,nt_extmode); p += 4;
			len = srvstr_push(outbuf, p + 4, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(p,0,len);
			p += 4 + len;
			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;
      
		case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_FILE_FULL_DIRECTORY_INFO\n"));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			put_long_date(p,cdate); p += 8;
			put_long_date(p,adate); p += 8;
			put_long_date(p,mdate); p += 8;
			put_long_date(p,mdate); p += 8;
			SOFF_T(p,0,file_size); p += 8;
			SOFF_T(p,0,allocation_size); p += 8;
			SIVAL(p,0,nt_extmode); p += 4;
			q = p; p += 4; /* q is placeholder for name length. */
			{
				unsigned int ea_size = estimate_ea_size(conn, NULL, pathreal);
				SIVAL(p,0,ea_size); /* Extended attributes */
				p +=4;
			}
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(q, 0, len);
			p += len;

			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;

		case SMB_FIND_FILE_NAMES_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_FILE_NAMES_INFO\n"));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			p += 4;
			/* this must *not* be null terminated or w2k gets in a loop trying to set an
			   acl on a dir (tridge) */
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(p, -4, len);
			p += len;
			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;

		case SMB_FIND_ID_FULL_DIRECTORY_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_ID_FULL_DIRECTORY_INFO\n"));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			put_long_date(p,cdate); p += 8;
			put_long_date(p,adate); p += 8;
			put_long_date(p,mdate); p += 8;
			put_long_date(p,mdate); p += 8;
			SOFF_T(p,0,file_size); p += 8;
			SOFF_T(p,0,allocation_size); p += 8;
			SIVAL(p,0,nt_extmode); p += 4;
			q = p; p += 4; /* q is placeholder for name length. */
			{
				unsigned int ea_size = estimate_ea_size(conn, NULL, pathreal);
				SIVAL(p,0,ea_size); /* Extended attributes */
				p +=4;
			}
			SIVAL(p,0,0); p += 4; /* Unknown - reserved ? */
			SIVAL(p,0,sbuf.st_ino); p += 4; /* FileIndexLow */
			SIVAL(p,0,sbuf.st_dev); p += 4; /* FileIndexHigh */
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(q, 0, len);
			p += len; 
			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;

		case SMB_FIND_ID_BOTH_DIRECTORY_INFO:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_ID_BOTH_DIRECTORY_INFO\n"));
			was_8_3 = mangle_is_8_3(fname, True, SNUM(conn));
			p += 4;
			SIVAL(p,0,reskey); p += 4;
			put_long_date(p,cdate); p += 8;
			put_long_date(p,adate); p += 8;
			put_long_date(p,mdate); p += 8;
			put_long_date(p,mdate); p += 8;
			SOFF_T(p,0,file_size); p += 8;
			SOFF_T(p,0,allocation_size); p += 8;
			SIVAL(p,0,nt_extmode); p += 4;
			q = p; p += 4; /* q is placeholder for name length */
			{
				unsigned int ea_size = estimate_ea_size(conn, NULL, pathreal);
				SIVAL(p,0,ea_size); /* Extended attributes */
				p +=4;
			}
			/* Clear the short name buffer. This is
			 * IMPORTANT as not doing so will trigger
			 * a Win2k client bug. JRA.
			 */
			if (!was_8_3 && check_mangled_names) {
				pstring mangled_name;
				pstrcpy(mangled_name, fname);
				mangle_map(mangled_name,True,True,SNUM(conn));
				mangled_name[12] = 0;
				len = srvstr_push(outbuf, p+2, mangled_name, 24, STR_UPPER|STR_UNICODE);
				SSVAL(p, 0, len);
				if (len < 24) {
					memset(p + 2 + len,'\0',24 - len);
				}
				SSVAL(p, 0, len);
			} else {
				memset(p,'\0',26);
			}
			p += 26;
			SSVAL(p,0,0); p += 2; /* Reserved ? */
			SIVAL(p,0,sbuf.st_ino); p += 4; /* FileIndexLow */
			SIVAL(p,0,sbuf.st_dev); p += 4; /* FileIndexHigh */
			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE_ASCII);
			SIVAL(q,0,len);
			p += len;
			SIVAL(p,0,0); /* Ensure any padding is null. */
			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);
			p = pdata + len;
			break;

		/* CIFS UNIX Extension. */

		case SMB_FIND_FILE_UNIX:
			DEBUG(10,("get_lanman2_dir_entry: SMB_FIND_FILE_UNIX\n"));
			p+= 4;
			SIVAL(p,0,reskey); p+= 4;    /* Used for continuing search. */

			/* Begin of SMB_QUERY_FILE_UNIX_BASIC */
			SOFF_T(p,0,get_file_size(sbuf));             /* File size 64 Bit */
			p+= 8;

			SOFF_T(p,0,get_allocation_size(conn,NULL,&sbuf)); /* Number of bytes used on disk - 64 Bit */
			p+= 8;

			put_long_date(p,sbuf.st_ctime);       /* Inode change Time 64 Bit */
			put_long_date(p+8,sbuf.st_atime);     /* Last access time 64 Bit */
			put_long_date(p+16,sbuf.st_mtime);    /* Last modification time 64 Bit */
			p+= 24;

			SIVAL(p,0,sbuf.st_uid);               /* user id for the owner */
			SIVAL(p,4,0);
			p+= 8;

			SIVAL(p,0,sbuf.st_gid);               /* group id of owner */
			SIVAL(p,4,0);
			p+= 8;

			SIVAL(p,0,unix_filetype(sbuf.st_mode));
			p+= 4;

			SIVAL(p,0,unix_dev_major(sbuf.st_rdev));   /* Major device number if type is device */
			SIVAL(p,4,0);
			p+= 8;

			SIVAL(p,0,unix_dev_minor(sbuf.st_rdev));   /* Minor device number if type is device */
			SIVAL(p,4,0);
			p+= 8;

			SINO_T_VAL(p,0,(SMB_INO_T)sbuf.st_ino);   /* inode number */
			p+= 8;

			SIVAL(p,0, unix_perms_to_wire(sbuf.st_mode));     /* Standard UNIX file permissions */
			SIVAL(p,4,0);
			p+= 8;

			SIVAL(p,0,sbuf.st_nlink);             /* number of hard links */
			SIVAL(p,4,0);
			p+= 8;

			len = srvstr_push(outbuf, p, fname, -1, STR_TERMINATE);
			p += len;
			SIVAL(p,0,0); /* Ensure any padding is null. */

			len = PTR_DIFF(p, pdata);
			len = (len + 3) & ~3;
			SIVAL(pdata,0,len);	/* Offset from this structure to the beginning of the next one */
			p = pdata + len;
			/* End of SMB_QUERY_FILE_UNIX_BASIC */

			break;

		default:      
			return(False);
	}


	if (PTR_DIFF(p,pdata) > space_remaining) {
		/* Move the dirptr back to prev_dirpos */
		dptr_SeekDir(conn->dirptr, prev_dirpos);
		*out_of_space = True;
		DEBUG(9,("get_lanman2_dir_entry: out of space\n"));
		return False; /* Not finished - just out of space */
	}

	/* Setup the last entry pointer, as an offset from base_data */
	*last_entry_off = PTR_DIFF(last_entry_ptr,base_data);
	/* Advance the data pointer to the next slot */
	*ppdata = p;

	return(found);
}

/****************************************************************************
 Reply to a TRANS2_FINDFIRST.
****************************************************************************/

static int call_trans2findfirst(connection_struct *conn, char *inbuf, char *outbuf, int bufsize,  
				char **pparams, int total_params, char **ppdata, int total_data,
				unsigned int max_data_bytes)
{
	/* We must be careful here that we don't return more than the
		allowed number of data bytes. If this means returning fewer than
		maxentries then so be it. We assume that the redirector has
		enough room for the fixed number of parameter bytes it has
		requested. */
	char *params = *pparams;
	char *pdata = *ppdata;
	uint32 dirtype = SVAL(params,0);
	int maxentries = SVAL(params,2);
	uint16 findfirst_flags = SVAL(params,4);
	BOOL close_after_first = (findfirst_flags & FLAG_TRANS2_FIND_CLOSE);
	BOOL close_if_end = (findfirst_flags & FLAG_TRANS2_FIND_CLOSE_IF_END);
	BOOL requires_resume_key = (findfirst_flags & FLAG_TRANS2_FIND_REQUIRE_RESUME);
	int info_level = SVAL(params,6);
	pstring directory;
	pstring mask;
	char *p;
	int last_entry_off=0;
	int dptr_num = -1;
	int numentries = 0;
	int i;
	BOOL finished = False;
	BOOL dont_descend = False;
	BOOL out_of_space = False;
	int space_remaining;
	BOOL bad_path = False;
	BOOL mask_contains_wcard = False;
	SMB_STRUCT_STAT sbuf;
	TALLOC_CTX *ea_ctx = NULL;
	struct ea_list *ea_list = NULL;
	NTSTATUS ntstatus = NT_STATUS_OK;

	if (total_params < 12) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	*directory = *mask = 0;

	DEBUG(3,("call_trans2findfirst: dirtype = %x, maxentries = %d, close_after_first=%d, \
close_if_end = %d requires_resume_key = %d level = 0x%x, max_data_bytes = %d\n",
		(unsigned int)dirtype, maxentries, close_after_first, close_if_end, requires_resume_key,
		info_level, max_data_bytes));

	if (!maxentries) {
		/* W2K3 seems to treat zero as 1. */
		maxentries = 1;
	}
 
	switch (info_level) {
		case SMB_FIND_INFO_STANDARD:
		case SMB_FIND_EA_SIZE:
		case SMB_FIND_EA_LIST:
		case SMB_FIND_FILE_DIRECTORY_INFO:
		case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
		case SMB_FIND_FILE_NAMES_INFO:
		case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
		case SMB_FIND_ID_FULL_DIRECTORY_INFO:
		case SMB_FIND_ID_BOTH_DIRECTORY_INFO:
			break;
		case SMB_FIND_FILE_UNIX:
			if (!lp_unix_extensions()) {
				return ERROR_NT(NT_STATUS_INVALID_LEVEL);
			}
			break;
		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	srvstr_get_path_wcard(inbuf, directory, params+12, sizeof(directory), -1, STR_TERMINATE, &ntstatus, &mask_contains_wcard);
	if (!NT_STATUS_IS_OK(ntstatus)) {
		return ERROR_NT(ntstatus);
	}

	RESOLVE_DFSPATH_WCARD(directory, conn, inbuf, outbuf);

	unix_convert(directory,conn,0,&bad_path,&sbuf);
	if (bad_path) {
		return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
	}
	if(!check_name(directory,conn)) {
		return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
	}

	p = strrchr_m(directory,'/');
	if(p == NULL) {
		/* Windows and OS/2 systems treat search on the root '\' as if it were '\*' */
		if((directory[0] == '.') && (directory[1] == '\0')) {
			pstrcpy(mask,"*");
			mask_contains_wcard = True;
		} else {
			pstrcpy(mask,directory);
		}
		pstrcpy(directory,"./");
	} else {
		pstrcpy(mask,p+1);
		*p = 0;
	}

	DEBUG(5,("dir=%s, mask = %s\n",directory, mask));

	if (info_level == SMB_FIND_EA_LIST) {
		uint32 ea_size;

		if (total_data < 4) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		ea_size = IVAL(pdata,0);
		if (ea_size != total_data) {
			DEBUG(4,("call_trans2findfirst: Rejecting EA request with incorrect \
total_data=%u (should be %u)\n", (unsigned int)total_data, (unsigned int)IVAL(pdata,0) ));
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		if (!lp_ea_support(SNUM(conn))) {
			return ERROR_DOS(ERRDOS,ERReasnotsupported);
		}
                                                                                                                                                        
		if ((ea_ctx = talloc_init("findnext_ea_list")) == NULL) {
			return ERROR_NT(NT_STATUS_NO_MEMORY);
		}
                                                                                                                                                        
		/* Pull out the list of names. */
		ea_list = read_ea_name_list(ea_ctx, pdata + 4, ea_size - 4);
		if (!ea_list) {
			talloc_destroy(ea_ctx);
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
	}

	*ppdata = SMB_REALLOC(*ppdata, max_data_bytes + DIR_ENTRY_SAFETY_MARGIN);
	if(*ppdata == NULL ) {
		talloc_destroy(ea_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	pdata = *ppdata;

	/* Realloc the params space */
	*pparams = SMB_REALLOC(*pparams, 10);
	if (*pparams == NULL) {
		talloc_destroy(ea_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	/* Save the wildcard match and attribs we are using on this directory - 
		needed as lanman2 assumes these are being saved between calls */

	dptr_num = dptr_create(conn,directory, False, True ,SVAL(inbuf,smb_pid), mask, mask_contains_wcard, dirtype);
	if (dptr_num < 0) {
		talloc_destroy(ea_ctx);
		return(UNIXERROR(ERRDOS,ERRbadfile));
	}

	DEBUG(4,("dptr_num is %d, wcard = %s, attr = %d\n",dptr_num, mask, dirtype));

	/* We don't need to check for VOL here as this is returned by 
		a different TRANS2 call. */
  
	DEBUG(8,("dirpath=<%s> dontdescend=<%s>\n", conn->dirpath,lp_dontdescend(SNUM(conn))));
	if (in_list(conn->dirpath,lp_dontdescend(SNUM(conn)),conn->case_sensitive))
		dont_descend = True;
    
	p = pdata;
	space_remaining = max_data_bytes;
	out_of_space = False;

	for (i=0;(i<maxentries) && !finished && !out_of_space;i++) {
		BOOL got_exact_match = False;

		/* this is a heuristic to avoid seeking the dirptr except when 
			absolutely necessary. It allows for a filename of about 40 chars */
		if (space_remaining < DIRLEN_GUESS && numentries > 0) {
			out_of_space = True;
			finished = False;
		} else {
			finished = !get_lanman2_dir_entry(conn,
					inbuf, outbuf,
					mask,dirtype,info_level,
					requires_resume_key,dont_descend,
					&p,pdata,space_remaining, &out_of_space, &got_exact_match,
					&last_entry_off, ea_list, ea_ctx);
		}

		if (finished && out_of_space)
			finished = False;

		if (!finished && !out_of_space)
			numentries++;

		/*
		 * As an optimisation if we know we aren't looking
		 * for a wildcard name (ie. the name matches the wildcard exactly)
		 * then we can finish on any (first) match.
		 * This speeds up large directory searches. JRA.
		 */

		if(got_exact_match)
			finished = True;

		space_remaining = max_data_bytes - PTR_DIFF(p,pdata);
	}
  
	talloc_destroy(ea_ctx);

	/* Check if we can close the dirptr */
	if(close_after_first || (finished && close_if_end)) {
		DEBUG(5,("call_trans2findfirst - (2) closing dptr_num %d\n", dptr_num));
		dptr_close(&dptr_num);
	}

	/* 
	 * If there are no matching entries we must return ERRDOS/ERRbadfile - 
	 * from observation of NT. NB. This changes to ERRDOS,ERRnofiles if
	 * the protocol level is less than NT1. Tested with smbclient. JRA.
	 * This should fix the OS/2 client bug #2335.
	 */

	if(numentries == 0) {
		dptr_close(&dptr_num);
		if (Protocol < PROTOCOL_NT1) {
			return ERROR_DOS(ERRDOS,ERRnofiles);
		} else {
			return ERROR_BOTH(NT_STATUS_NO_SUCH_FILE,ERRDOS,ERRbadfile);
		}
	}

	/* At this point pdata points to numentries directory entries. */

	/* Set up the return parameter block */
	SSVAL(params,0,dptr_num);
	SSVAL(params,2,numentries);
	SSVAL(params,4,finished);
	SSVAL(params,6,0); /* Never an EA error */
	SSVAL(params,8,last_entry_off);

	send_trans2_replies( outbuf, bufsize, params, 10, pdata, PTR_DIFF(p,pdata));

	if ((! *directory) && dptr_path(dptr_num))
		slprintf(directory,sizeof(directory)-1, "(%s)",dptr_path(dptr_num));

	DEBUG( 4, ( "%s mask=%s directory=%s dirtype=%d numentries=%d\n",
		smb_fn_name(CVAL(inbuf,smb_com)), 
		mask, directory, dirtype, numentries ) );

	/* 
	 * Force a name mangle here to ensure that the
	 * mask as an 8.3 name is top of the mangled cache.
	 * The reasons for this are subtle. Don't remove
	 * this code unless you know what you are doing
	 * (see PR#13758). JRA.
	 */

	if(!mangle_is_8_3_wildcards( mask, False, SNUM(conn)))
		mangle_map(mask, True, True, SNUM(conn));

	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_FINDNEXT.
****************************************************************************/

static int call_trans2findnext(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	/* We must be careful here that we don't return more than the
		allowed number of data bytes. If this means returning fewer than
		maxentries then so be it. We assume that the redirector has
		enough room for the fixed number of parameter bytes it has
		requested. */
	char *params = *pparams;
	char *pdata = *ppdata;
	int dptr_num = SVAL(params,0);
	int maxentries = SVAL(params,2);
	uint16 info_level = SVAL(params,4);
	uint32 resume_key = IVAL(params,6);
	uint16 findnext_flags = SVAL(params,10);
	BOOL close_after_request = (findnext_flags & FLAG_TRANS2_FIND_CLOSE);
	BOOL close_if_end = (findnext_flags & FLAG_TRANS2_FIND_CLOSE_IF_END);
	BOOL requires_resume_key = (findnext_flags & FLAG_TRANS2_FIND_REQUIRE_RESUME);
	BOOL continue_bit = (findnext_flags & FLAG_TRANS2_FIND_CONTINUE);
	BOOL mask_contains_wcard = False;
	pstring resume_name;
	pstring mask;
	pstring directory;
	char *p;
	uint16 dirtype;
	int numentries = 0;
	int i, last_entry_off=0;
	BOOL finished = False;
	BOOL dont_descend = False;
	BOOL out_of_space = False;
	int space_remaining;
	TALLOC_CTX *ea_ctx = NULL;
	struct ea_list *ea_list = NULL;
	NTSTATUS ntstatus = NT_STATUS_OK;

	if (total_params < 12) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	*mask = *directory = *resume_name = 0;

	srvstr_get_path_wcard(inbuf, resume_name, params+12, sizeof(resume_name), -1, STR_TERMINATE, &ntstatus, &mask_contains_wcard);
	if (!NT_STATUS_IS_OK(ntstatus)) {
		/* Win9x or OS/2 can send a resume name of ".." or ".". This will cause the parser to
		   complain (it thinks we're asking for the directory above the shared
		   path or an invalid name). Catch this as the resume name is only compared, never used in
		   a file access. JRA. */
		if (NT_STATUS_EQUAL(ntstatus,NT_STATUS_OBJECT_PATH_SYNTAX_BAD)) {
			pstrcpy(resume_name, "..");
		} else if (NT_STATUS_EQUAL(ntstatus,NT_STATUS_OBJECT_NAME_INVALID)) {
			pstrcpy(resume_name, ".");
		} else {
			return ERROR_NT(ntstatus);
		}
	}

	DEBUG(3,("call_trans2findnext: dirhandle = %d, max_data_bytes = %d, maxentries = %d, \
close_after_request=%d, close_if_end = %d requires_resume_key = %d \
resume_key = %d resume name = %s continue=%d level = %d\n",
		dptr_num, max_data_bytes, maxentries, close_after_request, close_if_end, 
		requires_resume_key, resume_key, resume_name, continue_bit, info_level));

	if (!maxentries) {
		/* W2K3 seems to treat zero as 1. */
		maxentries = 1;
	}

	switch (info_level) {
		case SMB_FIND_INFO_STANDARD:
		case SMB_FIND_EA_SIZE:
		case SMB_FIND_EA_LIST:
		case SMB_FIND_FILE_DIRECTORY_INFO:
		case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
		case SMB_FIND_FILE_NAMES_INFO:
		case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
		case SMB_FIND_ID_FULL_DIRECTORY_INFO:
		case SMB_FIND_ID_BOTH_DIRECTORY_INFO:
			break;
		case SMB_FIND_FILE_UNIX:
			if (!lp_unix_extensions()) {
				return ERROR_NT(NT_STATUS_INVALID_LEVEL);
			}
			break;
		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	if (info_level == SMB_FIND_EA_LIST) {
		uint32 ea_size;

		if (total_data < 4) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		ea_size = IVAL(pdata,0);
		if (ea_size != total_data) {
			DEBUG(4,("call_trans2findnext: Rejecting EA request with incorrect \
total_data=%u (should be %u)\n", (unsigned int)total_data, (unsigned int)IVAL(pdata,0) ));
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
                                                                                                                                                     
		if (!lp_ea_support(SNUM(conn))) {
			return ERROR_DOS(ERRDOS,ERReasnotsupported);
		}
                                                                                                                                                     
		if ((ea_ctx = talloc_init("findnext_ea_list")) == NULL) {
			return ERROR_NT(NT_STATUS_NO_MEMORY);
		}

		/* Pull out the list of names. */
		ea_list = read_ea_name_list(ea_ctx, pdata + 4, ea_size - 4);
		if (!ea_list) {
			talloc_destroy(ea_ctx);
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
	}

	*ppdata = SMB_REALLOC( *ppdata, max_data_bytes + DIR_ENTRY_SAFETY_MARGIN);
	if(*ppdata == NULL) {
		talloc_destroy(ea_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}

	pdata = *ppdata;

	/* Realloc the params space */
	*pparams = SMB_REALLOC(*pparams, 6*SIZEOFWORD);
	if(*pparams == NULL ) {
		talloc_destroy(ea_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}

	params = *pparams;

	/* Check that the dptr is valid */
	if(!(conn->dirptr = dptr_fetch_lanman2(dptr_num))) {
		talloc_destroy(ea_ctx);
		return ERROR_DOS(ERRDOS,ERRnofiles);
	}

	string_set(&conn->dirpath,dptr_path(dptr_num));

	/* Get the wildcard mask from the dptr */
	if((p = dptr_wcard(dptr_num))== NULL) {
		DEBUG(2,("dptr_num %d has no wildcard\n", dptr_num));
		talloc_destroy(ea_ctx);
		return ERROR_DOS(ERRDOS,ERRnofiles);
	}

	pstrcpy(mask, p);
	pstrcpy(directory,conn->dirpath);

	/* Get the attr mask from the dptr */
	dirtype = dptr_attr(dptr_num);

	DEBUG(3,("dptr_num is %d, mask = %s, attr = %x, dirptr=(0x%lX,%ld)\n",
		dptr_num, mask, dirtype, 
		(long)conn->dirptr,
		dptr_TellDir(conn->dirptr)));

	/* We don't need to check for VOL here as this is returned by 
		a different TRANS2 call. */

	DEBUG(8,("dirpath=<%s> dontdescend=<%s>\n",conn->dirpath,lp_dontdescend(SNUM(conn))));
	if (in_list(conn->dirpath,lp_dontdescend(SNUM(conn)),conn->case_sensitive))
		dont_descend = True;
    
	p = pdata;
	space_remaining = max_data_bytes;
	out_of_space = False;

	/* 
	 * Seek to the correct position. We no longer use the resume key but
	 * depend on the last file name instead.
	 */

	if(*resume_name && !continue_bit) {
		SMB_STRUCT_STAT st;

		long current_pos = 0;
		/*
		 * Remember, mangle_map is called by
		 * get_lanman2_dir_entry(), so the resume name
		 * could be mangled. Ensure we check the unmangled name.
		 */

		if (mangle_is_mangled(resume_name, SNUM(conn))) {
			mangle_check_cache(resume_name, sizeof(resume_name)-1, SNUM(conn));
		}

		/*
		 * Fix for NT redirector problem triggered by resume key indexes
		 * changing between directory scans. We now return a resume key of 0
		 * and instead look for the filename to continue from (also given
		 * to us by NT/95/smbfs/smbclient). If no other scans have been done between the
		 * findfirst/findnext (as is usual) then the directory pointer
		 * should already be at the correct place.
		 */

		finished = !dptr_SearchDir(conn->dirptr, resume_name, &current_pos, &st);
	} /* end if resume_name && !continue_bit */

	for (i=0;(i<(int)maxentries) && !finished && !out_of_space ;i++) {
		BOOL got_exact_match = False;

		/* this is a heuristic to avoid seeking the dirptr except when 
			absolutely necessary. It allows for a filename of about 40 chars */
		if (space_remaining < DIRLEN_GUESS && numentries > 0) {
			out_of_space = True;
			finished = False;
		} else {
			finished = !get_lanman2_dir_entry(conn,
						inbuf, outbuf,
						mask,dirtype,info_level,
						requires_resume_key,dont_descend,
						&p,pdata,space_remaining, &out_of_space, &got_exact_match,
						&last_entry_off, ea_list, ea_ctx);
		}

		if (finished && out_of_space)
			finished = False;

		if (!finished && !out_of_space)
			numentries++;

		/*
		 * As an optimisation if we know we aren't looking
		 * for a wildcard name (ie. the name matches the wildcard exactly)
		 * then we can finish on any (first) match.
		 * This speeds up large directory searches. JRA.
		 */

		if(got_exact_match)
			finished = True;

		space_remaining = max_data_bytes - PTR_DIFF(p,pdata);
	}
  
	talloc_destroy(ea_ctx);

	/* Check if we can close the dirptr */
	if(close_after_request || (finished && close_if_end)) {
		DEBUG(5,("call_trans2findnext: closing dptr_num = %d\n", dptr_num));
		dptr_close(&dptr_num); /* This frees up the saved mask */
	}

	/* Set up the return parameter block */
	SSVAL(params,0,numentries);
	SSVAL(params,2,finished);
	SSVAL(params,4,0); /* Never an EA error */
	SSVAL(params,6,last_entry_off);

	send_trans2_replies( outbuf, bufsize, params, 8, pdata, PTR_DIFF(p,pdata));

	if ((! *directory) && dptr_path(dptr_num))
		slprintf(directory,sizeof(directory)-1, "(%s)",dptr_path(dptr_num));

	DEBUG( 3, ( "%s mask=%s directory=%s dirtype=%d numentries=%d\n",
		smb_fn_name(CVAL(inbuf,smb_com)), 
		mask, directory, dirtype, numentries ) );

	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_QFSINFO (query filesystem info).
****************************************************************************/

static int call_trans2qfsinfo(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *pdata = *ppdata;
	char *params = *pparams;
	uint16 info_level = SVAL(params,0);
	int data_len, len;
	SMB_STRUCT_STAT st;
	char *vname = volume_label(SNUM(conn));
	int snum = SNUM(conn);
	char *fstype = lp_fstype(SNUM(conn));
	int quota_flag = 0;

	DEBUG(3,("call_trans2qfsinfo: level = %d\n", info_level));

	if(SMB_VFS_STAT(conn,".",&st)!=0) {
		DEBUG(2,("call_trans2qfsinfo: stat of . failed (%s)\n", strerror(errno)));
		return ERROR_DOS(ERRSRV,ERRinvdevice);
	}

	*ppdata = SMB_REALLOC(*ppdata, max_data_bytes + DIR_ENTRY_SAFETY_MARGIN);
	if (*ppdata == NULL ) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}

	pdata = *ppdata;
	memset((char *)pdata,'\0',max_data_bytes + DIR_ENTRY_SAFETY_MARGIN);

	switch (info_level) {
		case SMB_INFO_ALLOCATION:
		{
			SMB_BIG_UINT dfree,dsize,bsize,block_size,sectors_per_unit,bytes_per_sector;
			data_len = 18;
			if (get_dfree_info(conn,".",False,&bsize,&dfree,&dsize) == (SMB_BIG_UINT)-1) {
				return(UNIXERROR(ERRHRD,ERRgeneral));
			}

			block_size = lp_block_size(snum);
			if (bsize < block_size) {
				SMB_BIG_UINT factor = block_size/bsize;
				bsize = block_size;
				dsize /= factor;
				dfree /= factor;
			}
			if (bsize > block_size) {
				SMB_BIG_UINT factor = bsize/block_size;
				bsize = block_size;
				dsize *= factor;
				dfree *= factor;
			}
			bytes_per_sector = 512;
			sectors_per_unit = bsize/bytes_per_sector;

			DEBUG(5,("call_trans2qfsinfo : SMB_INFO_ALLOCATION id=%x, bsize=%u, cSectorUnit=%u, \
cBytesSector=%u, cUnitTotal=%u, cUnitAvail=%d\n", (unsigned int)st.st_dev, (unsigned int)bsize, (unsigned int)sectors_per_unit,
				(unsigned int)bytes_per_sector, (unsigned int)dsize, (unsigned int)dfree));

			SIVAL(pdata,l1_idFileSystem,st.st_dev);
			SIVAL(pdata,l1_cSectorUnit,sectors_per_unit);
			SIVAL(pdata,l1_cUnit,dsize);
			SIVAL(pdata,l1_cUnitAvail,dfree);
			SSVAL(pdata,l1_cbSector,bytes_per_sector);
			break;
		}

		case SMB_INFO_VOLUME:
			/* Return volume name */
			/* 
			 * Add volume serial number - hash of a combination of
			 * the called hostname and the service name.
			 */
			SIVAL(pdata,0,str_checksum(lp_servicename(snum)) ^ (str_checksum(get_local_machine_name())<<16) );
			/*
			 * Win2k3 and previous mess this up by sending a name length
			 * one byte short. I believe only older clients (OS/2 Win9x) use
			 * this call so try fixing this by adding a terminating null to
			 * the pushed string. The change here was adding the STR_TERMINATE. JRA.
			 */
			len = srvstr_push(outbuf, pdata+l2_vol_szVolLabel, vname, -1, STR_NOALIGN|STR_TERMINATE);
			SCVAL(pdata,l2_vol_cch,len);
			data_len = l2_vol_szVolLabel + len;
			DEBUG(5,("call_trans2qfsinfo : time = %x, namelen = %d, name = %s\n",
				(unsigned)st.st_ctime, len, vname));
			break;

		case SMB_QUERY_FS_ATTRIBUTE_INFO:
		case SMB_FS_ATTRIBUTE_INFORMATION:


#if defined(HAVE_SYS_QUOTAS)
			quota_flag = FILE_VOLUME_QUOTAS;
#endif

			SIVAL(pdata,0,FILE_CASE_PRESERVED_NAMES|FILE_CASE_SENSITIVE_SEARCH|
				(lp_nt_acl_support(SNUM(conn)) ? FILE_PERSISTENT_ACLS : 0)|
				quota_flag); /* FS ATTRIBUTES */

			SIVAL(pdata,4,255); /* Max filename component length */
			/* NOTE! the fstype must *not* be null terminated or win98 won't recognise it
				and will think we can't do long filenames */
			len = srvstr_push(outbuf, pdata+12, fstype, -1, STR_UNICODE);
			SIVAL(pdata,8,len);
			data_len = 12 + len;
			break;

		case SMB_QUERY_FS_LABEL_INFO:
		case SMB_FS_LABEL_INFORMATION:
			len = srvstr_push(outbuf, pdata+4, vname, -1, 0);
			data_len = 4 + len;
			SIVAL(pdata,0,len);
			break;

		case SMB_QUERY_FS_VOLUME_INFO:      
		case SMB_FS_VOLUME_INFORMATION:

			/* 
			 * Add volume serial number - hash of a combination of
			 * the called hostname and the service name.
			 */
			SIVAL(pdata,8,str_checksum(lp_servicename(snum)) ^ 
				(str_checksum(get_local_machine_name())<<16));

			len = srvstr_push(outbuf, pdata+18, vname, -1, STR_UNICODE);
			SIVAL(pdata,12,len);
			data_len = 18+len;
			DEBUG(5,("call_trans2qfsinfo : SMB_QUERY_FS_VOLUME_INFO namelen = %d, vol=%s serv=%s\n", 
				(int)strlen(vname),vname, lp_servicename(snum)));
			break;

		case SMB_QUERY_FS_SIZE_INFO:
		case SMB_FS_SIZE_INFORMATION:
		{
			SMB_BIG_UINT dfree,dsize,bsize,block_size,sectors_per_unit,bytes_per_sector;
			data_len = 24;
			if (get_dfree_info(conn,".",False,&bsize,&dfree,&dsize) == (SMB_BIG_UINT)-1) {
				return(UNIXERROR(ERRHRD,ERRgeneral));
			}
			block_size = lp_block_size(snum);
			if (bsize < block_size) {
				SMB_BIG_UINT factor = block_size/bsize;
				bsize = block_size;
				dsize /= factor;
				dfree /= factor;
			}
			if (bsize > block_size) {
				SMB_BIG_UINT factor = bsize/block_size;
				bsize = block_size;
				dsize *= factor;
				dfree *= factor;
			}
			bytes_per_sector = 512;
			sectors_per_unit = bsize/bytes_per_sector;
			DEBUG(5,("call_trans2qfsinfo : SMB_QUERY_FS_SIZE_INFO bsize=%u, cSectorUnit=%u, \
cBytesSector=%u, cUnitTotal=%u, cUnitAvail=%d\n", (unsigned int)bsize, (unsigned int)sectors_per_unit,
				(unsigned int)bytes_per_sector, (unsigned int)dsize, (unsigned int)dfree));
			SBIG_UINT(pdata,0,dsize);
			SBIG_UINT(pdata,8,dfree);
			SIVAL(pdata,16,sectors_per_unit);
			SIVAL(pdata,20,bytes_per_sector);
			break;
		}

		case SMB_FS_FULL_SIZE_INFORMATION:
		{
			SMB_BIG_UINT dfree,dsize,bsize,block_size,sectors_per_unit,bytes_per_sector;
			data_len = 32;
			if (get_dfree_info(conn,".",False,&bsize,&dfree,&dsize) == (SMB_BIG_UINT)-1) {
				return(UNIXERROR(ERRHRD,ERRgeneral));
			}
			block_size = lp_block_size(snum);
			if (bsize < block_size) {
				SMB_BIG_UINT factor = block_size/bsize;
				bsize = block_size;
				dsize /= factor;
				dfree /= factor;
			}
			if (bsize > block_size) {
				SMB_BIG_UINT factor = bsize/block_size;
				bsize = block_size;
				dsize *= factor;
				dfree *= factor;
			}
			bytes_per_sector = 512;
			sectors_per_unit = bsize/bytes_per_sector;
			DEBUG(5,("call_trans2qfsinfo : SMB_QUERY_FS_FULL_SIZE_INFO bsize=%u, cSectorUnit=%u, \
cBytesSector=%u, cUnitTotal=%u, cUnitAvail=%d\n", (unsigned int)bsize, (unsigned int)sectors_per_unit,
				(unsigned int)bytes_per_sector, (unsigned int)dsize, (unsigned int)dfree));
			SBIG_UINT(pdata,0,dsize); /* Total Allocation units. */
			SBIG_UINT(pdata,8,dfree); /* Caller available allocation units. */
			SBIG_UINT(pdata,16,dfree); /* Actual available allocation units. */
			SIVAL(pdata,24,sectors_per_unit); /* Sectors per allocation unit. */
			SIVAL(pdata,28,bytes_per_sector); /* Bytes per sector. */
			break;
		}

		case SMB_QUERY_FS_DEVICE_INFO:
		case SMB_FS_DEVICE_INFORMATION:
			data_len = 8;
			SIVAL(pdata,0,0); /* dev type */
			SIVAL(pdata,4,0); /* characteristics */
			break;

#ifdef HAVE_SYS_QUOTAS
		case SMB_FS_QUOTA_INFORMATION:
		/* 
		 * what we have to send --metze:
		 *
		 * Unknown1: 		24 NULL bytes
		 * Soft Quota Treshold: 8 bytes seems like SMB_BIG_UINT or so
		 * Hard Quota Limit:	8 bytes seems like SMB_BIG_UINT or so
		 * Quota Flags:		2 byte :
		 * Unknown3:		6 NULL bytes
		 *
		 * 48 bytes total
		 * 
		 * details for Quota Flags:
		 * 
		 * 0x0020 Log Limit: log if the user exceeds his Hard Quota
		 * 0x0010 Log Warn:  log if the user exceeds his Soft Quota
		 * 0x0002 Deny Disk: deny disk access when the user exceeds his Hard Quota
		 * 0x0001 Enable Quotas: enable quota for this fs
		 *
		 */
		{
			/* we need to fake up a fsp here,
			 * because its not send in this call
			 */
			files_struct fsp;
			SMB_NTQUOTA_STRUCT quotas;
			
			ZERO_STRUCT(fsp);
			ZERO_STRUCT(quotas);
			
			fsp.conn = conn;
			fsp.fnum = -1;
			
			/* access check */
			if (current_user.ut.uid != 0) {
				DEBUG(0,("set_user_quota: access_denied service [%s] user [%s]\n",
					lp_servicename(SNUM(conn)),conn->user));
				return ERROR_DOS(ERRDOS,ERRnoaccess);
			}
			
			if (vfs_get_ntquota(&fsp, SMB_USER_FS_QUOTA_TYPE, NULL, &quotas)!=0) {
				DEBUG(0,("vfs_get_ntquota() failed for service [%s]\n",lp_servicename(SNUM(conn))));
				return ERROR_DOS(ERRSRV,ERRerror);
			}

			data_len = 48;

			DEBUG(10,("SMB_FS_QUOTA_INFORMATION: for service [%s]\n",lp_servicename(SNUM(conn))));		
		
			/* Unknown1 24 NULL bytes*/
			SBIG_UINT(pdata,0,(SMB_BIG_UINT)0);
			SBIG_UINT(pdata,8,(SMB_BIG_UINT)0);
			SBIG_UINT(pdata,16,(SMB_BIG_UINT)0);
		
			/* Default Soft Quota 8 bytes */
			SBIG_UINT(pdata,24,quotas.softlim);

			/* Default Hard Quota 8 bytes */
			SBIG_UINT(pdata,32,quotas.hardlim);
	
			/* Quota flag 2 bytes */
			SSVAL(pdata,40,quotas.qflags);
		
			/* Unknown3 6 NULL bytes */
			SSVAL(pdata,42,0);
			SIVAL(pdata,44,0);
			
			break;
		}
#endif /* HAVE_SYS_QUOTAS */
		case SMB_FS_OBJECTID_INFORMATION:
			data_len = 64;
			break;

		/*
		 * Query the version and capabilities of the CIFS UNIX extensions
		 * in use.
		 */

		case SMB_QUERY_CIFS_UNIX_INFO:
			if (!lp_unix_extensions()) {
				return ERROR_NT(NT_STATUS_INVALID_LEVEL);
			}
			data_len = 12;
			SSVAL(pdata,0,CIFS_UNIX_MAJOR_VERSION);
			SSVAL(pdata,2,CIFS_UNIX_MINOR_VERSION);
			/* We have POSIX ACLs, pathname and locking capability. */
#if defined(DEVELOPER) /* Not quite finished yet... */
			SBIG_UINT(pdata,4,((SMB_BIG_UINT)(
					CIFS_UNIX_POSIX_ACLS_CAP|
					CIFS_UNIX_POSIX_PATHNAMES_CAP|
					CIFS_UNIX_FCNTL_LOCKS_CAP)));
#else
			SBIG_UINT(pdata,4,((SMB_BIG_UINT)(
					CIFS_UNIX_POSIX_ACLS_CAP|
					CIFS_UNIX_POSIX_PATHNAMES_CAP|
					0)));
#endif
			break;

		case SMB_QUERY_POSIX_FS_INFO:
		{
			int rc;
			vfs_statvfs_struct svfs;

			if (!lp_unix_extensions()) {
				return ERROR_NT(NT_STATUS_INVALID_LEVEL);
			}

			rc = SMB_VFS_STATVFS(conn, ".", &svfs);

			if (!rc) {
				data_len = 56;
				SIVAL(pdata,0,svfs.OptimalTransferSize);
				SIVAL(pdata,4,svfs.BlockSize);
				SBIG_UINT(pdata,8,svfs.TotalBlocks);
				SBIG_UINT(pdata,16,svfs.BlocksAvail);
				SBIG_UINT(pdata,24,svfs.UserBlocksAvail);
				SBIG_UINT(pdata,32,svfs.TotalFileNodes);
				SBIG_UINT(pdata,40,svfs.FreeFileNodes);
				SBIG_UINT(pdata,48,svfs.FsIdentifier);
				DEBUG(5,("call_trans2qfsinfo : SMB_QUERY_POSIX_FS_INFO succsessful\n"));
#ifdef EOPNOTSUPP
			} else if (rc == EOPNOTSUPP) {
				return ERROR_NT(NT_STATUS_INVALID_LEVEL);
#endif /* EOPNOTSUPP */
			} else {
				DEBUG(0,("vfs_statvfs() failed for service [%s]\n",lp_servicename(SNUM(conn))));
				return ERROR_DOS(ERRSRV,ERRerror);
			}
			break;
		}

		case SMB_MAC_QUERY_FS_INFO:
			/*
			 * Thursby MAC extension... ONLY on NTFS filesystems
			 * once we do streams then we don't need this
			 */
			if (strequal(lp_fstype(SNUM(conn)),"NTFS")) {
				data_len = 88;
				SIVAL(pdata,84,0x100); /* Don't support mac... */
				break;
			}
			/* drop through */
		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}


	send_trans2_replies( outbuf, bufsize, params, 0, pdata, data_len);

	DEBUG( 4, ( "%s info_level = %d\n", smb_fn_name(CVAL(inbuf,smb_com)), info_level) );

	return -1;
}

/****************************************************************************
 Reply to a TRANS2_SETFSINFO (set filesystem info).
****************************************************************************/

static int call_trans2setfsinfo(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *pdata = *ppdata;
	char *params = *pparams;
	uint16 info_level;
	int outsize;

	DEBUG(10,("call_trans2setfsinfo: for service [%s]\n",lp_servicename(SNUM(conn))));

	/*  */
	if (total_params < 4) {
		DEBUG(0,("call_trans2setfsinfo: requires total_params(%d) >= 4 bytes!\n",
			total_params));
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	info_level = SVAL(params,2);

	switch(info_level) {
		case SMB_SET_CIFS_UNIX_INFO:
			{
				uint16 client_unix_major;
				uint16 client_unix_minor;
				uint32 client_unix_cap_low;
				uint32 client_unix_cap_high;

				if (!lp_unix_extensions()) {
					return ERROR_NT(NT_STATUS_INVALID_LEVEL);
				}

				/* There should be 12 bytes of capabilities set. */
				if (total_data < 8) {
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}
				client_unix_major = SVAL(pdata,0);
				client_unix_minor = SVAL(pdata,2);
				client_unix_cap_low = IVAL(pdata,4);
				client_unix_cap_high = IVAL(pdata,8);
				/* Just print these values for now. */
				DEBUG(10,("call_trans2setfsinfo: set unix info. major = %u, minor = %u \
cap_low = 0x%x, cap_high = 0x%x\n",
					(unsigned int)client_unix_major,
					(unsigned int)client_unix_minor,
					(unsigned int)client_unix_cap_low,
					(unsigned int)client_unix_cap_high ));

				/* Here is where we must switch to posix pathname processing... */
				if (client_unix_cap_low & CIFS_UNIX_POSIX_PATHNAMES_CAP) {
					lp_set_posix_pathnames();
					mangle_change_to_posix();
				}
#if defined(DEVELOPER)
				if (client_unix_cap_low & CIFS_UNIX_FCNTL_LOCKS_CAP) {
					lp_set_posix_cifsx_locktype(POSIX_LOCK);
				}
#endif
				break;
			}
		case SMB_FS_QUOTA_INFORMATION:
			{
				files_struct *fsp = NULL;
				SMB_NTQUOTA_STRUCT quotas;
	
				ZERO_STRUCT(quotas);

				/* access check */
				if ((current_user.ut.uid != 0)||!CAN_WRITE(conn)) {
					DEBUG(0,("set_user_quota: access_denied service [%s] user [%s]\n",
						lp_servicename(SNUM(conn)),conn->user));
					return ERROR_DOS(ERRSRV,ERRaccess);
				}

				/* note: normaly there're 48 bytes,
				 * but we didn't use the last 6 bytes for now 
				 * --metze 
				 */
				fsp = file_fsp(params,0);
				if (!CHECK_NTQUOTA_HANDLE_OK(fsp,conn)) {
					DEBUG(3,("TRANSACT_GET_USER_QUOTA: no valid QUOTA HANDLE\n"));
					return ERROR_NT(NT_STATUS_INVALID_HANDLE);
				}

				if (total_data < 42) {
					DEBUG(0,("call_trans2setfsinfo: SET_FS_QUOTA: requires total_data(%d) >= 42 bytes!\n",
						total_data));
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}
			
				/* unknown_1 24 NULL bytes in pdata*/
		
				/* the soft quotas 8 bytes (SMB_BIG_UINT)*/
				quotas.softlim = (SMB_BIG_UINT)IVAL(pdata,24);
#ifdef LARGE_SMB_OFF_T
				quotas.softlim |= (((SMB_BIG_UINT)IVAL(pdata,28)) << 32);
#else /* LARGE_SMB_OFF_T */
				if ((IVAL(pdata,28) != 0)&&
					((quotas.softlim != 0xFFFFFFFF)||
					(IVAL(pdata,28)!=0xFFFFFFFF))) {
					/* more than 32 bits? */
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}
#endif /* LARGE_SMB_OFF_T */
		
				/* the hard quotas 8 bytes (SMB_BIG_UINT)*/
				quotas.hardlim = (SMB_BIG_UINT)IVAL(pdata,32);
#ifdef LARGE_SMB_OFF_T
				quotas.hardlim |= (((SMB_BIG_UINT)IVAL(pdata,36)) << 32);
#else /* LARGE_SMB_OFF_T */
				if ((IVAL(pdata,36) != 0)&&
					((quotas.hardlim != 0xFFFFFFFF)||
					(IVAL(pdata,36)!=0xFFFFFFFF))) {
					/* more than 32 bits? */
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}
#endif /* LARGE_SMB_OFF_T */
		
				/* quota_flags 2 bytes **/
				quotas.qflags = SVAL(pdata,40);
		
				/* unknown_2 6 NULL bytes follow*/
		
				/* now set the quotas */
				if (vfs_set_ntquota(fsp, SMB_USER_FS_QUOTA_TYPE, NULL, &quotas)!=0) {
					DEBUG(0,("vfs_set_ntquota() failed for service [%s]\n",lp_servicename(SNUM(conn))));
					return ERROR_DOS(ERRSRV,ERRerror);
				}
			
				break;
			}
		default:
			DEBUG(3,("call_trans2setfsinfo: unknown level (0x%X) not implemented yet.\n",
				info_level));
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
			break;
	}

	/* 
	 * sending this reply works fine, 
	 * but I'm not sure it's the same 
	 * like windows do...
	 * --metze
	 */ 
	outsize = set_message(outbuf,10,0,True);

	return outsize;
}

/****************************************************************************
 Utility function to set bad path error.
****************************************************************************/

int set_bad_path_error(int err, BOOL bad_path, char *outbuf, int def_class, uint32 def_code)
{
	DEBUG(10,("set_bad_path_error: err = %d bad_path = %d\n",
			err, (int)bad_path ));

	if(err == ENOENT) {
		if (bad_path) {
			return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
		} else {
			return ERROR_NT(NT_STATUS_OBJECT_NAME_NOT_FOUND);
		}
	}
	return UNIXERROR(def_class,def_code);
}

#if defined(HAVE_POSIX_ACLS)
/****************************************************************************
 Utility function to count the number of entries in a POSIX acl.
****************************************************************************/

static unsigned int count_acl_entries(connection_struct *conn, SMB_ACL_T posix_acl)
{
	unsigned int ace_count = 0;
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;

	while ( posix_acl && (SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1)) {
		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY) {
			entry_id = SMB_ACL_NEXT_ENTRY;
		}
		ace_count++;
	}
	return ace_count;
}

/****************************************************************************
 Utility function to marshall a POSIX acl into wire format.
****************************************************************************/

static BOOL marshall_posix_acl(connection_struct *conn, char *pdata, SMB_STRUCT_STAT *pst, SMB_ACL_T posix_acl)
{
	int entry_id = SMB_ACL_FIRST_ENTRY;
	SMB_ACL_ENTRY_T entry;

	while ( posix_acl && (SMB_VFS_SYS_ACL_GET_ENTRY(conn, posix_acl, entry_id, &entry) == 1)) {
		SMB_ACL_TAG_T tagtype;
		SMB_ACL_PERMSET_T permset;
		unsigned char perms = 0;
		unsigned int own_grp;

		/* get_next... */
		if (entry_id == SMB_ACL_FIRST_ENTRY) {
			entry_id = SMB_ACL_NEXT_ENTRY;
		}

		if (SMB_VFS_SYS_ACL_GET_TAG_TYPE(conn, entry, &tagtype) == -1) {
			DEBUG(0,("marshall_posix_acl: SMB_VFS_SYS_ACL_GET_TAG_TYPE failed.\n"));
			return False;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, entry, &permset) == -1) {
			DEBUG(0,("marshall_posix_acl: SMB_VFS_SYS_ACL_GET_PERMSET failed.\n"));
			return False;
		}

		perms |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_READ) ? SMB_POSIX_ACL_READ : 0);
		perms |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_WRITE) ? SMB_POSIX_ACL_WRITE : 0);
		perms |= (SMB_VFS_SYS_ACL_GET_PERM(conn, permset, SMB_ACL_EXECUTE) ? SMB_POSIX_ACL_EXECUTE : 0);

		SCVAL(pdata,1,perms);

		switch (tagtype) {
			case SMB_ACL_USER_OBJ:
				SCVAL(pdata,0,SMB_POSIX_ACL_USER_OBJ);
				own_grp = (unsigned int)pst->st_uid;
				SIVAL(pdata,2,own_grp);
				SIVAL(pdata,6,0);
				break;
			case SMB_ACL_USER:
				{
					uid_t *puid = (uid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
					if (!puid) {
						DEBUG(0,("marshall_posix_acl: SMB_VFS_SYS_ACL_GET_QUALIFIER failed.\n"));
					}
					own_grp = (unsigned int)*puid;
					SMB_VFS_SYS_ACL_FREE_QUALIFIER(conn, (void *)puid,tagtype);
					SCVAL(pdata,0,SMB_POSIX_ACL_USER);
					SIVAL(pdata,2,own_grp);
					SIVAL(pdata,6,0);
					break;
				}
			case SMB_ACL_GROUP_OBJ:
				SCVAL(pdata,0,SMB_POSIX_ACL_GROUP_OBJ);
				own_grp = (unsigned int)pst->st_gid;
				SIVAL(pdata,2,own_grp);
				SIVAL(pdata,6,0);
				break;
			case SMB_ACL_GROUP:
				{
					gid_t *pgid= (gid_t *)SMB_VFS_SYS_ACL_GET_QUALIFIER(conn, entry);
					if (!pgid) {
						DEBUG(0,("marshall_posix_acl: SMB_VFS_SYS_ACL_GET_QUALIFIER failed.\n"));
					}
					own_grp = (unsigned int)*pgid;
					SMB_VFS_SYS_ACL_FREE_QUALIFIER(conn, (void *)pgid,tagtype);
					SCVAL(pdata,0,SMB_POSIX_ACL_GROUP);
					SIVAL(pdata,2,own_grp);
					SIVAL(pdata,6,0);
					break;
				}
			case SMB_ACL_MASK:
				SCVAL(pdata,0,SMB_POSIX_ACL_MASK);
				SIVAL(pdata,2,0xFFFFFFFF);
				SIVAL(pdata,6,0xFFFFFFFF);
				break;
			case SMB_ACL_OTHER:
				SCVAL(pdata,0,SMB_POSIX_ACL_OTHER);
				SIVAL(pdata,2,0xFFFFFFFF);
				SIVAL(pdata,6,0xFFFFFFFF);
				break;
			default:
				DEBUG(0,("marshall_posix_acl: unknown tagtype.\n"));
				return False;
		}
		pdata += SMB_POSIX_ACL_ENTRY_SIZE;
	}

	return True;
}
#endif

/****************************************************************************
 Reply to a TRANS2_QFILEPATHINFO or TRANSACT2_QFILEINFO (query file info by
 file name or file id).
****************************************************************************/

static int call_trans2qfilepathinfo(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					unsigned int tran_call,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *params = *pparams;
	char *pdata = *ppdata;
	uint16 info_level;
	int mode=0;
	int nlink;
	SMB_OFF_T file_size=0;
	SMB_BIG_UINT allocation_size=0;
	unsigned int data_size = 0;
	unsigned int param_size = 2;
	SMB_STRUCT_STAT sbuf;
	pstring fname, dos_fname;
	char *fullpathname;
	char *base_name;
	char *p;
	SMB_OFF_T pos = 0;
	BOOL bad_path = False;
	BOOL delete_pending = False;
	int len;
	time_t c_time;
	files_struct *fsp = NULL;
	TALLOC_CTX *data_ctx = NULL;
	struct ea_list *ea_list = NULL;
	uint32 access_mask = 0x12019F; /* Default - GENERIC_EXECUTE mapping from Windows */
#if defined(DEVELOPER)
	char *lock_data = NULL;
#endif

	if (!params)
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);

	ZERO_STRUCT(sbuf);

	if (tran_call == TRANSACT2_QFILEINFO) {
		if (total_params < 4) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		fsp = file_fsp(params,0);
		info_level = SVAL(params,2);

		DEBUG(3,("call_trans2qfilepathinfo: TRANSACT2_QFILEINFO: level = %d\n", info_level));

		if(fsp && (fsp->fake_file_handle)) {
			/*
			 * This is actually for the QUOTA_FAKE_FILE --metze
			 */
						
			pstrcpy(fname, fsp->fsp_name);
			/* We know this name is ok, it's already passed the checks. */
			
		} else if(fsp && (fsp->is_directory || fsp->fh->fd == -1)) {
			/*
			 * This is actually a QFILEINFO on a directory
			 * handle (returned from an NT SMB). NT5.0 seems
			 * to do this call. JRA.
			 */
			/* We know this name is ok, it's already passed the checks. */
			pstrcpy(fname, fsp->fsp_name);
		  
			if (INFO_LEVEL_IS_UNIX(info_level)) {
				/* Always do lstat for UNIX calls. */
				if (SMB_VFS_LSTAT(conn,fname,&sbuf)) {
					DEBUG(3,("call_trans2qfilepathinfo: SMB_VFS_LSTAT of %s failed (%s)\n",fname,strerror(errno)));
					return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
				}
			} else if (SMB_VFS_STAT(conn,fname,&sbuf)) {
				DEBUG(3,("call_trans2qfilepathinfo: SMB_VFS_STAT of %s failed (%s)\n",fname,strerror(errno)));
				return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
			}

			delete_pending = get_delete_on_close_flag(sbuf.st_dev, sbuf.st_ino);
		} else {
			/*
			 * Original code - this is an open file.
			 */
			CHECK_FSP(fsp,conn);

			pstrcpy(fname, fsp->fsp_name);
			if (SMB_VFS_FSTAT(fsp,fsp->fh->fd,&sbuf) != 0) {
				DEBUG(3,("fstat of fnum %d failed (%s)\n", fsp->fnum, strerror(errno)));
				return(UNIXERROR(ERRDOS,ERRbadfid));
			}
			pos = fsp->fh->position_information;
			delete_pending = get_delete_on_close_flag(sbuf.st_dev, sbuf.st_ino);
			access_mask = fsp->access_mask;
		}
	} else {
		NTSTATUS status = NT_STATUS_OK;

		/* qpathinfo */
		if (total_params < 6) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		info_level = SVAL(params,0);

		DEBUG(3,("call_trans2qfilepathinfo: TRANSACT2_QPATHINFO: level = %d\n", info_level));

		srvstr_get_path(inbuf, fname, &params[6], sizeof(fname), -1, STR_TERMINATE, &status);
		if (!NT_STATUS_IS_OK(status)) {
			return ERROR_NT(status);
		}

		RESOLVE_DFSPATH(fname, conn, inbuf, outbuf);

		unix_convert(fname,conn,0,&bad_path,&sbuf);
		if (bad_path) {
			return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
		}
		if (!check_name(fname,conn)) {
			DEBUG(3,("call_trans2qfilepathinfo: fileinfo of %s failed (%s)\n",fname,strerror(errno)));
			return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
		}

		if (INFO_LEVEL_IS_UNIX(info_level)) {
			/* Always do lstat for UNIX calls. */
			if (SMB_VFS_LSTAT(conn,fname,&sbuf)) {
				DEBUG(3,("call_trans2qfilepathinfo: SMB_VFS_LSTAT of %s failed (%s)\n",fname,strerror(errno)));
				return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
			}
		} else if (!VALID_STAT(sbuf) && SMB_VFS_STAT(conn,fname,&sbuf) && (info_level != SMB_INFO_IS_NAME_VALID)) {
			DEBUG(3,("call_trans2qfilepathinfo: SMB_VFS_STAT of %s failed (%s)\n",fname,strerror(errno)));
			return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
		}

		delete_pending = get_delete_on_close_flag(sbuf.st_dev, sbuf.st_ino);
		if (delete_pending) {
			return ERROR_NT(NT_STATUS_DELETE_PENDING);
		}
	}

	nlink = sbuf.st_nlink;

	if ((nlink > 0) && S_ISDIR(sbuf.st_mode)) {
		/* NTFS does not seem to count ".." */
		nlink -= 1;
	}

	if ((nlink > 0) && delete_pending) {
		nlink -= 1;
	}

	if (INFO_LEVEL_IS_UNIX(info_level) && !lp_unix_extensions()) {
		return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	DEBUG(3,("call_trans2qfilepathinfo %s (fnum = %d) level=%d call=%d total_data=%d\n",
		fname,fsp ? fsp->fnum : -1, info_level,tran_call,total_data));

	p = strrchr_m(fname,'/'); 
	if (!p)
		base_name = fname;
	else
		base_name = p+1;

	mode = dos_mode(conn,fname,&sbuf);
	if (!mode)
		mode = FILE_ATTRIBUTE_NORMAL;

	fullpathname = fname;
	if (!(mode & aDIR))
		file_size = get_file_size(sbuf);

	/* Pull out any data sent here before we realloc. */
	switch (info_level) {
		case SMB_INFO_QUERY_EAS_FROM_LIST:
		{
			/* Pull any EA list from the data portion. */
			uint32 ea_size;

			if (total_data < 4) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			ea_size = IVAL(pdata,0);

			if (total_data > 0 && ea_size != total_data) {
				DEBUG(4,("call_trans2qfilepathinfo: Rejecting EA request with incorrect \
total_data=%u (should be %u)\n", (unsigned int)total_data, (unsigned int)IVAL(pdata,0) ));
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if (!lp_ea_support(SNUM(conn))) {
				return ERROR_DOS(ERRDOS,ERReasnotsupported);
			}

			if ((data_ctx = talloc_init("ea_list")) == NULL) {
				return ERROR_NT(NT_STATUS_NO_MEMORY);
			}

			/* Pull out the list of names. */
			ea_list = read_ea_name_list(data_ctx, pdata + 4, ea_size - 4);
			if (!ea_list) {
				talloc_destroy(data_ctx);
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			break;
		}
#if defined(DEVELOPER)
		case SMB_QUERY_POSIX_LOCK:
		{
			if (fsp == NULL || fsp->fh->fd == -1) {
				return ERROR_NT(NT_STATUS_INVALID_HANDLE);
			}

			if (total_data != POSIX_LOCK_DATA_SIZE) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if ((data_ctx = talloc_init("lock_request")) == NULL) {
				return ERROR_NT(NT_STATUS_NO_MEMORY);
			}

			/* Copy the lock range data. */
			lock_data = talloc_memdup(data_ctx, pdata, total_data);
			if (!lock_data) {
				talloc_destroy(data_ctx);
				return ERROR_NT(NT_STATUS_NO_MEMORY);
			}
		}
#endif
		default:
			break;
	}

	*pparams = SMB_REALLOC(*pparams,2);
	if (*pparams == NULL) {
		talloc_destroy(data_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;
	SSVAL(params,0,0);
	data_size = max_data_bytes + DIR_ENTRY_SAFETY_MARGIN;
	*ppdata = SMB_REALLOC(*ppdata, data_size); 
	if (*ppdata == NULL ) {
		talloc_destroy(data_ctx);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	pdata = *ppdata;

	c_time = get_create_time(&sbuf,lp_fake_dir_create_times(SNUM(conn)));

	allocation_size = get_allocation_size(conn,fsp,&sbuf);

	if (fsp) {
		if (fsp->pending_modtime) {
			/* the pending modtime overrides the current modtime */
			sbuf.st_mtime = fsp->pending_modtime;
		}
	} else {
		/* Do we have this path open ? */
		files_struct *fsp1 = file_find_di_first(sbuf.st_dev, sbuf.st_ino);
		if (fsp1 && fsp1->pending_modtime) {
			/* the pending modtime overrides the current modtime */
			sbuf.st_mtime = fsp1->pending_modtime;
		}
		if (fsp1 && fsp1->initial_allocation_size) {
			allocation_size = get_allocation_size(conn, fsp1, &sbuf);
		}
	}

	if (lp_dos_filetime_resolution(SNUM(conn))) {
		c_time &= ~1;
		sbuf.st_atime &= ~1;
		sbuf.st_ctime &= ~1;
		sbuf.st_mtime &= ~1;
	}

	/* NT expects the name to be in an exact form of the *full*
	   filename. See the trans2 torture test */
	if (strequal(base_name,".")) {
		pstrcpy(dos_fname, "\\");
	} else {
		pstr_sprintf(dos_fname, "\\%s", fname);
		string_replace(dos_fname, '/', '\\');
	}

	switch (info_level) {
		case SMB_INFO_STANDARD:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_INFO_STANDARD\n"));
			data_size = 22;
			srv_put_dos_date2(pdata,l1_fdateCreation,c_time);
			srv_put_dos_date2(pdata,l1_fdateLastAccess,sbuf.st_atime);
			srv_put_dos_date2(pdata,l1_fdateLastWrite,sbuf.st_mtime); /* write time */
			SIVAL(pdata,l1_cbFile,(uint32)file_size);
			SIVAL(pdata,l1_cbFileAlloc,(uint32)allocation_size);
			SSVAL(pdata,l1_attrFile,mode);
			break;

		case SMB_INFO_QUERY_EA_SIZE:
		{
			unsigned int ea_size = estimate_ea_size(conn, fsp, fname);
			DEBUG(10,("call_trans2qfilepathinfo: SMB_INFO_QUERY_EA_SIZE\n"));
			data_size = 26;
			srv_put_dos_date2(pdata,0,c_time);
			srv_put_dos_date2(pdata,4,sbuf.st_atime);
			srv_put_dos_date2(pdata,8,sbuf.st_mtime); /* write time */
			SIVAL(pdata,12,(uint32)file_size);
			SIVAL(pdata,16,(uint32)allocation_size);
			SSVAL(pdata,20,mode);
			SIVAL(pdata,22,ea_size);
			break;
		}

		case SMB_INFO_IS_NAME_VALID:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_INFO_IS_NAME_VALID\n"));
			if (tran_call == TRANSACT2_QFILEINFO) {
				/* os/2 needs this ? really ?*/      
				return ERROR_DOS(ERRDOS,ERRbadfunc); 
			}
			data_size = 0;
			param_size = 0;
			break;
			
		case SMB_INFO_QUERY_EAS_FROM_LIST:
		{
			size_t total_ea_len = 0;
			struct ea_list *ea_file_list = NULL;

			DEBUG(10,("call_trans2qfilepathinfo: SMB_INFO_QUERY_EAS_FROM_LIST\n"));

			ea_file_list = get_ea_list_from_file(data_ctx, conn, fsp, fname, &total_ea_len);
			ea_list = ea_list_union(ea_list, ea_file_list, &total_ea_len);

			if (!ea_list || (total_ea_len > data_size)) {
				talloc_destroy(data_ctx);
				data_size = 4;
				SIVAL(pdata,0,4);   /* EA List Length must be set to 4 if no EA's. */
				break;
			}

			data_size = fill_ea_buffer(data_ctx, pdata, data_size, conn, ea_list);
			talloc_destroy(data_ctx);
			break;
		}

		case SMB_INFO_QUERY_ALL_EAS:
		{
			/* We have data_size bytes to put EA's into. */
			size_t total_ea_len = 0;

			DEBUG(10,("call_trans2qfilepathinfo: SMB_INFO_QUERY_ALL_EAS\n"));

			data_ctx = talloc_init("ea_ctx");
			if (!data_ctx) {
				return ERROR_NT(NT_STATUS_NO_MEMORY);
			}

			ea_list = get_ea_list_from_file(data_ctx, conn, fsp, fname, &total_ea_len);
			if (!ea_list || (total_ea_len > data_size)) {
				talloc_destroy(data_ctx);
				data_size = 4;
				SIVAL(pdata,0,4);   /* EA List Length must be set to 4 if no EA's. */
				break;
			}

			data_size = fill_ea_buffer(data_ctx, pdata, data_size, conn, ea_list);
			talloc_destroy(data_ctx);
			break;
		}

		case SMB_FILE_BASIC_INFORMATION:
		case SMB_QUERY_FILE_BASIC_INFO:

			if (info_level == SMB_QUERY_FILE_BASIC_INFO) {
				DEBUG(10,("call_trans2qfilepathinfo: SMB_QUERY_FILE_BASIC_INFO\n"));
				data_size = 36; /* w95 returns 40 bytes not 36 - why ?. */
			} else {
				DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_BASIC_INFORMATION\n"));
				data_size = 40;
				SIVAL(pdata,36,0);
			}
			put_long_date(pdata,c_time);
			put_long_date(pdata+8,sbuf.st_atime);
			put_long_date(pdata+16,sbuf.st_mtime); /* write time */
			put_long_date(pdata+24,sbuf.st_mtime); /* change time */
			SIVAL(pdata,32,mode);

			DEBUG(5,("SMB_QFBI - "));
			{
				time_t create_time = c_time;
				DEBUG(5,("create: %s ", ctime(&create_time)));
			}
			DEBUG(5,("access: %s ", ctime(&sbuf.st_atime)));
			DEBUG(5,("write: %s ", ctime(&sbuf.st_mtime)));
			DEBUG(5,("change: %s ", ctime(&sbuf.st_mtime)));
			DEBUG(5,("mode: %x\n", mode));
			break;

		case SMB_FILE_STANDARD_INFORMATION:
		case SMB_QUERY_FILE_STANDARD_INFO:

			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_STANDARD_INFORMATION\n"));
			data_size = 24;
			SOFF_T(pdata,0,allocation_size);
			SOFF_T(pdata,8,file_size);
			SIVAL(pdata,16,nlink);
			SCVAL(pdata,20,delete_pending?1:0);
			SCVAL(pdata,21,(mode&aDIR)?1:0);
			SSVAL(pdata,22,0); /* Padding. */
			break;

		case SMB_FILE_EA_INFORMATION:
		case SMB_QUERY_FILE_EA_INFO:
		{
			unsigned int ea_size = estimate_ea_size(conn, fsp, fname);
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_EA_INFORMATION\n"));
			data_size = 4;
			SIVAL(pdata,0,ea_size);
			break;
		}

		/* Get the 8.3 name - used if NT SMB was negotiated. */
		case SMB_QUERY_FILE_ALT_NAME_INFO:
		case SMB_FILE_ALTERNATE_NAME_INFORMATION:
		{
			pstring short_name;

			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ALTERNATE_NAME_INFORMATION\n"));
			pstrcpy(short_name,base_name);
			/* Mangle if not already 8.3 */
			if(!mangle_is_8_3(short_name, True, SNUM(conn))) {
				mangle_map(short_name,True,True,SNUM(conn));
			}
			len = srvstr_push(outbuf, pdata+4, short_name, -1, STR_UNICODE);
			data_size = 4 + len;
			SIVAL(pdata,0,len);
			break;
		}

		case SMB_QUERY_FILE_NAME_INFO:
			/*
			  this must be *exactly* right for ACLs on mapped drives to work
			 */
			len = srvstr_push(outbuf, pdata+4, dos_fname, -1, STR_UNICODE);
			DEBUG(10,("call_trans2qfilepathinfo: SMB_QUERY_FILE_NAME_INFO\n"));
			data_size = 4 + len;
			SIVAL(pdata,0,len);
			break;

		case SMB_FILE_ALLOCATION_INFORMATION:
		case SMB_QUERY_FILE_ALLOCATION_INFO:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ALLOCATION_INFORMATION\n"));
			data_size = 8;
			SOFF_T(pdata,0,allocation_size);
			break;

		case SMB_FILE_END_OF_FILE_INFORMATION:
		case SMB_QUERY_FILE_END_OF_FILEINFO:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_END_OF_FILE_INFORMATION\n"));
			data_size = 8;
			SOFF_T(pdata,0,file_size);
			break;

		case SMB_QUERY_FILE_ALL_INFO:
		case SMB_FILE_ALL_INFORMATION:
		{
			unsigned int ea_size = estimate_ea_size(conn, fsp, fname);
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ALL_INFORMATION\n"));
			put_long_date(pdata,c_time);
			put_long_date(pdata+8,sbuf.st_atime);
			put_long_date(pdata+16,sbuf.st_mtime); /* write time */
			put_long_date(pdata+24,sbuf.st_mtime); /* change time */
			SIVAL(pdata,32,mode);
			SIVAL(pdata,36,0); /* padding. */
			pdata += 40;
			SOFF_T(pdata,0,allocation_size);
			SOFF_T(pdata,8,file_size);
			SIVAL(pdata,16,nlink);
			SCVAL(pdata,20,delete_pending);
			SCVAL(pdata,21,(mode&aDIR)?1:0);
			SSVAL(pdata,22,0);
			pdata += 24;
			SIVAL(pdata,0,ea_size);
			pdata += 4; /* EA info */
			len = srvstr_push(outbuf, pdata+4, dos_fname, -1, STR_UNICODE);
			SIVAL(pdata,0,len);
			pdata += 4 + len;
			data_size = PTR_DIFF(pdata,(*ppdata));
			break;
		}
		case SMB_FILE_INTERNAL_INFORMATION:
			/* This should be an index number - looks like
			   dev/ino to me :-) 

			   I think this causes us to fail the IFSKIT
			   BasicFileInformationTest. -tpot */

			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_INTERNAL_INFORMATION\n"));
			SIVAL(pdata,0,sbuf.st_ino); /* FileIndexLow */
			SIVAL(pdata,4,sbuf.st_dev); /* FileIndexHigh */
			data_size = 8;
			break;

		case SMB_FILE_ACCESS_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ACCESS_INFORMATION\n"));
			SIVAL(pdata,0,access_mask);
			data_size = 4;
			break;

		case SMB_FILE_NAME_INFORMATION:
			/* Pathname with leading '\'. */
			{
				size_t byte_len;
				byte_len = dos_PutUniCode(pdata+4,dos_fname,(size_t)max_data_bytes,False);
				DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_NAME_INFORMATION\n"));
				SIVAL(pdata,0,byte_len);
				data_size = 4 + byte_len;
				break;
			}

		case SMB_FILE_DISPOSITION_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_DISPOSITION_INFORMATION\n"));
			data_size = 1;
			SCVAL(pdata,0,delete_pending);
			break;

		case SMB_FILE_POSITION_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_POSITION_INFORMATION\n"));
			data_size = 8;
			SOFF_T(pdata,0,pos);
			break;

		case SMB_FILE_MODE_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_MODE_INFORMATION\n"));
			SIVAL(pdata,0,mode);
			data_size = 4;
			break;

		case SMB_FILE_ALIGNMENT_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ALIGNMENT_INFORMATION\n"));
			SIVAL(pdata,0,0); /* No alignment needed. */
			data_size = 4;
			break;

#if 0
		/*
		 * NT4 server just returns "invalid query" to this - if we try to answer
		 * it then NTws gets a BSOD! (tridge).
		 * W2K seems to want this. JRA.
		 */
		case SMB_QUERY_FILE_STREAM_INFO:
#endif
		case SMB_FILE_STREAM_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_STREAM_INFORMATION\n"));
			if (mode & aDIR) {
				data_size = 0;
			} else {
				size_t byte_len = dos_PutUniCode(pdata+24,"::$DATA", (size_t)0xE, False);
				SIVAL(pdata,0,0); /* ??? */
				SIVAL(pdata,4,byte_len); /* Byte length of unicode string ::$DATA */
				SOFF_T(pdata,8,file_size);
				SIVAL(pdata,16,allocation_size);
				SIVAL(pdata,20,0); /* ??? */
				data_size = 24 + byte_len;
			}
			break;

		case SMB_QUERY_COMPRESSION_INFO:
		case SMB_FILE_COMPRESSION_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_COMPRESSION_INFORMATION\n"));
			SOFF_T(pdata,0,file_size);
			SIVAL(pdata,8,0); /* ??? */
			SIVAL(pdata,12,0); /* ??? */
			data_size = 16;
			break;

		case SMB_FILE_NETWORK_OPEN_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_NETWORK_OPEN_INFORMATION\n"));
			put_long_date(pdata,c_time);
			put_long_date(pdata+8,sbuf.st_atime);
			put_long_date(pdata+16,sbuf.st_mtime); /* write time */
			put_long_date(pdata+24,sbuf.st_mtime); /* change time */
			SIVAL(pdata,32,allocation_size);
			SOFF_T(pdata,40,file_size);
			SIVAL(pdata,48,mode);
			SIVAL(pdata,52,0); /* ??? */
			data_size = 56;
			break;

		case SMB_FILE_ATTRIBUTE_TAG_INFORMATION:
			DEBUG(10,("call_trans2qfilepathinfo: SMB_FILE_ATTRIBUTE_TAG_INFORMATION\n"));
			SIVAL(pdata,0,mode);
			SIVAL(pdata,4,0);
			data_size = 8;
			break;

		/*
		 * CIFS UNIX Extensions.
		 */

		case SMB_QUERY_FILE_UNIX_BASIC:

			DEBUG(10,("call_trans2qfilepathinfo: SMB_QUERY_FILE_UNIX_BASIC\n"));
			DEBUG(4,("call_trans2qfilepathinfo: st_mode=%o\n",(int)sbuf.st_mode));

			SOFF_T(pdata,0,get_file_size(sbuf));             /* File size 64 Bit */
			pdata += 8;

			SOFF_T(pdata,0,get_allocation_size(conn,fsp,&sbuf)); /* Number of bytes used on disk - 64 Bit */
			pdata += 8;

			put_long_date(pdata,sbuf.st_ctime);       /* Creation Time 64 Bit */
			put_long_date(pdata+8,sbuf.st_atime);     /* Last access time 64 Bit */
			put_long_date(pdata+16,sbuf.st_mtime);    /* Last modification time 64 Bit */
			pdata += 24;

			SIVAL(pdata,0,sbuf.st_uid);               /* user id for the owner */
			SIVAL(pdata,4,0);
			pdata += 8;

			SIVAL(pdata,0,sbuf.st_gid);               /* group id of owner */
			SIVAL(pdata,4,0);
			pdata += 8;

			SIVAL(pdata,0,unix_filetype(sbuf.st_mode));
			pdata += 4;

			SIVAL(pdata,0,unix_dev_major(sbuf.st_rdev));   /* Major device number if type is device */
			SIVAL(pdata,4,0);
			pdata += 8;

			SIVAL(pdata,0,unix_dev_minor(sbuf.st_rdev));   /* Minor device number if type is device */
			SIVAL(pdata,4,0);
			pdata += 8;

			SINO_T_VAL(pdata,0,(SMB_INO_T)sbuf.st_ino);   /* inode number */
			pdata += 8;
				
			SIVAL(pdata,0, unix_perms_to_wire(sbuf.st_mode));     /* Standard UNIX file permissions */
			SIVAL(pdata,4,0);
			pdata += 8;

			SIVAL(pdata,0,sbuf.st_nlink);             /* number of hard links */
			SIVAL(pdata,4,0);
			pdata += 8;
			data_size = PTR_DIFF(pdata,(*ppdata));

			{
				int i;
				DEBUG(4,("call_trans2qfilepathinfo: SMB_QUERY_FILE_UNIX_BASIC"));

				for (i=0; i<100; i++)
					DEBUG(4,("%d=%x, ",i, (*ppdata)[i]));
				DEBUG(4,("\n"));
			}

			break;

		case SMB_QUERY_FILE_UNIX_LINK:
			{
				pstring buffer;

				DEBUG(10,("call_trans2qfilepathinfo: SMB_QUERY_FILE_UNIX_LINK\n"));
#ifdef S_ISLNK
				if(!S_ISLNK(sbuf.st_mode))
					return(UNIXERROR(ERRSRV,ERRbadlink));
#else
				return(UNIXERROR(ERRDOS,ERRbadlink));
#endif
				len = SMB_VFS_READLINK(conn,fullpathname, buffer, sizeof(pstring)-1);     /* read link */
				if (len == -1)
					return(UNIXERROR(ERRDOS,ERRnoaccess));
				buffer[len] = 0;
				len = srvstr_push(outbuf, pdata, buffer, -1, STR_TERMINATE);
				pdata += len;
				data_size = PTR_DIFF(pdata,(*ppdata));

				break;
			}

#if defined(HAVE_POSIX_ACLS)
		case SMB_QUERY_POSIX_ACL:
			{
				SMB_ACL_T file_acl = NULL;
				SMB_ACL_T def_acl = NULL;
				uint16 num_file_acls = 0;
				uint16 num_def_acls = 0;

				if (fsp && !fsp->is_directory && (fsp->fh->fd != -1)) {
					file_acl = SMB_VFS_SYS_ACL_GET_FD(fsp, fsp->fh->fd);
				} else {
					file_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fname, SMB_ACL_TYPE_ACCESS);
				}

				if (file_acl == NULL && no_acl_syscall_error(errno)) {
					DEBUG(5,("call_trans2qfilepathinfo: ACLs not implemented on filesystem containing %s\n",
						fname ));
					return ERROR_NT(NT_STATUS_NOT_IMPLEMENTED);
				}

				if (S_ISDIR(sbuf.st_mode)) {
					if (fsp && fsp->is_directory) {
						def_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fsp->fsp_name, SMB_ACL_TYPE_DEFAULT);
					} else {
						def_acl = SMB_VFS_SYS_ACL_GET_FILE(conn, fname, SMB_ACL_TYPE_DEFAULT);
					}
					def_acl = free_empty_sys_acl(conn, def_acl);
				}

				num_file_acls = count_acl_entries(conn, file_acl);
				num_def_acls = count_acl_entries(conn, def_acl);

				if ( data_size < (num_file_acls + num_def_acls)*SMB_POSIX_ACL_ENTRY_SIZE + SMB_POSIX_ACL_HEADER_SIZE) {
					DEBUG(5,("call_trans2qfilepathinfo: data_size too small (%u) need %u\n",
						data_size,
						(unsigned int)((num_file_acls + num_def_acls)*SMB_POSIX_ACL_ENTRY_SIZE +
							SMB_POSIX_ACL_HEADER_SIZE) ));
					if (file_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
					}
					if (def_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
					}
					return ERROR_NT(NT_STATUS_BUFFER_TOO_SMALL);
				}

				SSVAL(pdata,0,SMB_POSIX_ACL_VERSION);
				SSVAL(pdata,2,num_file_acls);
				SSVAL(pdata,4,num_def_acls);
				if (!marshall_posix_acl(conn, pdata + SMB_POSIX_ACL_HEADER_SIZE, &sbuf, file_acl)) {
					if (file_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
					}
					if (def_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
					}
					return ERROR_NT(NT_STATUS_INTERNAL_ERROR);
				}
				if (!marshall_posix_acl(conn, pdata + SMB_POSIX_ACL_HEADER_SIZE + (num_file_acls*SMB_POSIX_ACL_ENTRY_SIZE), &sbuf, def_acl)) {
					if (file_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
					}
					if (def_acl) {
						SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
					}
					return ERROR_NT(NT_STATUS_INTERNAL_ERROR);
				}

				if (file_acl) {
					SMB_VFS_SYS_ACL_FREE_ACL(conn, file_acl);
				}
				if (def_acl) {
					SMB_VFS_SYS_ACL_FREE_ACL(conn, def_acl);
				}
				data_size = (num_file_acls + num_def_acls)*SMB_POSIX_ACL_ENTRY_SIZE + SMB_POSIX_ACL_HEADER_SIZE;
				break;
			}
#endif


#if defined(DEVELOPER)
		case SMB_QUERY_POSIX_LOCK:
		{
			NTSTATUS status = NT_STATUS_INVALID_LEVEL;
			SMB_BIG_UINT count;
			SMB_BIG_UINT offset;
			uint16 lock_pid;
			enum brl_type lock_type;

			if (total_data != POSIX_LOCK_DATA_SIZE) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			switch (SVAL(pdata, POSIX_LOCK_TYPE_OFFSET)) {
				case POSIX_LOCK_TYPE_READ:
					lock_type = READ_LOCK;
					break;
				case POSIX_LOCK_TYPE_WRITE:
					lock_type = WRITE_LOCK;
					break;
				case POSIX_LOCK_TYPE_UNLOCK:
				default:
					/* There's no point in asking for an unlock... */
					talloc_destroy(data_ctx);
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			lock_pid = (uint16)IVAL(pdata, POSIX_LOCK_PID_OFFSET);
#if defined(HAVE_LONGLONG)
			offset = (((SMB_BIG_UINT) IVAL(pdata,(POSIX_LOCK_START_OFFSET+4))) << 32) |
					((SMB_BIG_UINT) IVAL(pdata,POSIX_LOCK_START_OFFSET));
			count = (((SMB_BIG_UINT) IVAL(pdata,(POSIX_LOCK_LEN_OFFSET+4))) << 32) |
					((SMB_BIG_UINT) IVAL(pdata,POSIX_LOCK_LEN_OFFSET));
#else /* HAVE_LONGLONG */
			offset = (SMB_BIG_UINT)IVAL(pdata,POSIX_LOCK_START_OFFSET);
			count = (SMB_BIG_UINT)IVAL(pdata,POSIX_LOCK_LEN_OFFSET);
#endif /* HAVE_LONGLONG */

			status = query_lock(fsp,
					&lock_pid,
					&count,
					&offset,
					&lock_type,
					POSIX_LOCK);

			if (ERROR_WAS_LOCK_DENIED(status)) {
				/* Here we need to report who has it locked... */
				data_size = POSIX_LOCK_DATA_SIZE;

				SSVAL(pdata, POSIX_LOCK_TYPE_OFFSET, lock_type);
				SSVAL(pdata, POSIX_LOCK_FLAGS_OFFSET, 0);
				SIVAL(pdata, POSIX_LOCK_PID_OFFSET, lock_pid);
#if defined(HAVE_LONGLONG)
				SIVAL(pdata, POSIX_LOCK_START_OFFSET, (uint32)(offset & 0xFFFFFFFF));
				SIVAL(pdata, POSIX_LOCK_START_OFFSET + 4, (uint32)((offset >> 32) & 0xFFFFFFFF));
				SIVAL(pdata, POSIX_LOCK_LEN_OFFSET, (uint32)(count & 0xFFFFFFFF));
				SIVAL(pdata, POSIX_LOCK_LEN_OFFSET + 4, (uint32)((count >> 32) & 0xFFFFFFFF));
#else /* HAVE_LONGLONG */
				SIVAL(pdata, POSIX_LOCK_START_OFFSET, offset);
				SIVAL(pdata, POSIX_LOCK_LEN_OFFSET, count);
#endif /* HAVE_LONGLONG */

			} else if (NT_STATUS_IS_OK(status)) {
				/* For success we just return a copy of what we sent
				   with the lock type set to POSIX_LOCK_TYPE_UNLOCK. */
				data_size = POSIX_LOCK_DATA_SIZE;
				memcpy(pdata, lock_data, POSIX_LOCK_DATA_SIZE);
				SSVAL(pdata, POSIX_LOCK_TYPE_OFFSET, POSIX_LOCK_TYPE_UNLOCK);
			} else {
				return ERROR_NT(status);
			}
			break;
		}
#endif

		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	send_trans2_replies(outbuf, bufsize, params, param_size, *ppdata, data_size);

	return(-1);
}

/****************************************************************************
 Set a hard link (called by UNIX extensions and by NT rename with HARD link
 code.
****************************************************************************/

NTSTATUS hardlink_internals(connection_struct *conn, char *oldname, char *newname)
{
	BOOL bad_path_oldname = False;
	BOOL bad_path_newname = False;
	SMB_STRUCT_STAT sbuf1, sbuf2;
	pstring last_component_oldname;
	pstring last_component_newname;
	NTSTATUS status = NT_STATUS_OK;

	ZERO_STRUCT(sbuf1);
	ZERO_STRUCT(sbuf2);

	/* No wildcards. */
	if (ms_has_wild(newname) || ms_has_wild(oldname)) {
		return NT_STATUS_OBJECT_PATH_SYNTAX_BAD;
	}

	unix_convert(oldname,conn,last_component_oldname,&bad_path_oldname,&sbuf1);
	if (bad_path_oldname) {
		return NT_STATUS_OBJECT_PATH_NOT_FOUND;
	}

	/* Quick check for "." and ".." */
	if (last_component_oldname[0] == '.') {
		if (!last_component_oldname[1] || (last_component_oldname[1] == '.' && !last_component_oldname[2])) {
			return NT_STATUS_OBJECT_NAME_INVALID;
		}
	}

	/* source must already exist. */
	if (!VALID_STAT(sbuf1)) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	if (!check_name(oldname,conn)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	unix_convert(newname,conn,last_component_newname,&bad_path_newname,&sbuf2);
	if (bad_path_newname) {
		return NT_STATUS_OBJECT_PATH_NOT_FOUND;
	}

	/* Quick check for "." and ".." */
	if (last_component_newname[0] == '.') {
		if (!last_component_newname[1] || (last_component_newname[1] == '.' && !last_component_newname[2])) {
			return NT_STATUS_OBJECT_NAME_INVALID;
		}
	}

	/* Disallow if newname already exists. */
	if (VALID_STAT(sbuf2)) {
		return NT_STATUS_OBJECT_NAME_COLLISION;
	}

	if (!check_name(newname,conn)) {
		return NT_STATUS_ACCESS_DENIED;
	}

	/* No links from a directory. */
	if (S_ISDIR(sbuf1.st_mode)) {
		return NT_STATUS_FILE_IS_A_DIRECTORY;
	}

	/* Ensure this is within the share. */
	if (!reduce_name(conn, oldname) != 0)
		return NT_STATUS_ACCESS_DENIED;

	DEBUG(10,("hardlink_internals: doing hard link %s -> %s\n", newname, oldname ));

	if (SMB_VFS_LINK(conn,oldname,newname) != 0) {
		status = map_nt_error_from_unix(errno);
		DEBUG(3,("hardlink_internals: Error %s hard link %s -> %s\n",
                                nt_errstr(status), newname, oldname));
	}

	return status;
}

/****************************************************************************
 Reply to a TRANS2_SETFILEINFO (set file info by fileid).
****************************************************************************/

static int call_trans2setfilepathinfo(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					unsigned int tran_call,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *params = *pparams;
	char *pdata = *ppdata;
	uint16 info_level;
	int dosmode=0;
	SMB_OFF_T size=0;
	struct utimbuf tvs;
	SMB_STRUCT_STAT sbuf;
	pstring fname;
	int fd = -1;
	BOOL bad_path = False;
	files_struct *fsp = NULL;
	uid_t set_owner = (uid_t)SMB_UID_NO_CHANGE;
	gid_t set_grp = (uid_t)SMB_GID_NO_CHANGE;
	mode_t unixmode = 0;
	NTSTATUS status = NT_STATUS_OK;

	if (!params)
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);

	ZERO_STRUCT(sbuf);
	ZERO_STRUCT(tvs);

	if (tran_call == TRANSACT2_SETFILEINFO) {
		if (total_params < 4) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		fsp = file_fsp(params,0);
		info_level = SVAL(params,2);    

		if(fsp && (fsp->is_directory || fsp->fh->fd == -1)) {
			/*
			 * This is actually a SETFILEINFO on a directory
			 * handle (returned from an NT SMB). NT5.0 seems
			 * to do this call. JRA.
			 */
			pstrcpy(fname, fsp->fsp_name);
			if (SMB_VFS_STAT(conn,fname,&sbuf) != 0) {
				DEBUG(3,("call_trans2setfilepathinfo: fileinfo of %s failed (%s)\n",fname,strerror(errno)));
				return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
			}
		} else if (fsp && fsp->print_file) {
			/*
			 * Doing a DELETE_ON_CLOSE should cancel a print job.
			 */
			if ((info_level == SMB_SET_FILE_DISPOSITION_INFO) && CVAL(pdata,0)) {
				fsp->fh->private_options |= FILE_DELETE_ON_CLOSE;

				DEBUG(3,("call_trans2setfilepathinfo: Cancelling print job (%s)\n", fsp->fsp_name ));
	
				SSVAL(params,0,0);
				send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
				return(-1);
			} else
				return (UNIXERROR(ERRDOS,ERRbadpath));
	    } else {
			/*
			 * Original code - this is an open file.
			 */
			CHECK_FSP(fsp,conn);

			pstrcpy(fname, fsp->fsp_name);
			fd = fsp->fh->fd;

			if (SMB_VFS_FSTAT(fsp,fd,&sbuf) != 0) {
				DEBUG(3,("call_trans2setfilepathinfo: fstat of fnum %d failed (%s)\n",fsp->fnum, strerror(errno)));
				return(UNIXERROR(ERRDOS,ERRbadfid));
			}
		}
	} else {
		/* set path info */
		if (total_params < 6) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		info_level = SVAL(params,0);    
		srvstr_get_path(inbuf, fname, &params[6], sizeof(fname), -1, STR_TERMINATE, &status);
		if (!NT_STATUS_IS_OK(status)) {
			return ERROR_NT(status);
		}
		unix_convert(fname,conn,0,&bad_path,&sbuf);
		if (bad_path) {
			return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
		}

		/*
		 * For CIFS UNIX extensions the target name may not exist.
		 */

		if(!VALID_STAT(sbuf) && !INFO_LEVEL_IS_UNIX(info_level)) {
			DEBUG(3,("call_trans2setfilepathinfo: stat of %s failed (%s)\n", fname, strerror(errno)));
			return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
		}    

		if(!check_name(fname, conn)) {
			return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRbadpath);
		}

	}

	if (!CAN_WRITE(conn))
		return ERROR_DOS(ERRSRV,ERRaccess);

	if (INFO_LEVEL_IS_UNIX(info_level) && !lp_unix_extensions()) {
		return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	if (VALID_STAT(sbuf))
		unixmode = sbuf.st_mode;

	DEBUG(3,("call_trans2setfilepathinfo(%d) %s (fnum %d) info_level=%d totdata=%d\n",
		tran_call,fname, fsp ? fsp->fnum : -1, info_level,total_data));

	/* Realloc the parameter size */
	*pparams = SMB_REALLOC(*pparams,2);
	if (*pparams == NULL) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	SSVAL(params,0,0);

	if (fsp && fsp->pending_modtime) {
		/* the pending modtime overrides the current modtime */
		sbuf.st_mtime = fsp->pending_modtime;
	}

	size = get_file_size(sbuf);
	tvs.modtime = sbuf.st_mtime;
	tvs.actime = sbuf.st_atime;
	dosmode = dos_mode(conn,fname,&sbuf);
	unixmode = sbuf.st_mode;

	set_owner = VALID_STAT(sbuf) ? sbuf.st_uid : (uid_t)SMB_UID_NO_CHANGE;
	set_grp = VALID_STAT(sbuf) ? sbuf.st_gid : (gid_t)SMB_GID_NO_CHANGE;

	switch (info_level) {
		case SMB_INFO_STANDARD:
		{
			if (total_data < 12) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			/* access time */
			tvs.actime = srv_make_unix_date2(pdata+l1_fdateLastAccess);
			/* write time */
			tvs.modtime = srv_make_unix_date2(pdata+l1_fdateLastWrite);
			break;
		}

		case SMB_INFO_SET_EA:
		{
			struct ea_list *ea_list = NULL;
			TALLOC_CTX *ctx = NULL;

			if (total_data < 10) {

				/* OS/2 workplace shell seems to send SET_EA requests of "null"
				   length. They seem to have no effect. Bug #3212. JRA */

				if ((total_data == 4) && (IVAL(pdata,0) == 4)) {
					/* We're done. We only get EA info in this call. */
					SSVAL(params,0,0);
					send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
					return(-1);
				}

				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if (IVAL(pdata,0) > total_data) {
				DEBUG(10,("call_trans2setfilepathinfo: bad total data size (%u) > %u\n",
					IVAL(pdata,0), (unsigned int)total_data));
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			ctx = talloc_init("SMB_INFO_SET_EA");
			if (!ctx) {
				return ERROR_NT(NT_STATUS_NO_MEMORY);
			}
			ea_list = read_ea_list(ctx, pdata + 4, total_data - 4);
			if (!ea_list) {
				talloc_destroy(ctx);
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			status = set_ea(conn, fsp, fname, ea_list);
			talloc_destroy(ctx);

			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			/* We're done. We only get EA info in this call. */
			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

#if 0
		/* The following 2 info levels are only valid on query, not set. Remove them. JRA. */
		/* XXXX um, i don't think this is right.
			it's also not in the cifs6.txt spec.
		*/
		case SMB_INFO_QUERY_EAS_FROM_LIST:
			if (total_data < 28)
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);

			tvs.actime = make_unix_date2(pdata+8);
			tvs.modtime = make_unix_date2(pdata+12);
			size = IVAL(pdata,16);
			dosmode = IVAL(pdata,24);
			break;

		/* XXXX nor this.  not in cifs6.txt, either. */
		case SMB_INFO_QUERY_ALL_EAS:
			if (total_data < 28)
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);

			tvs.actime = make_unix_date2(pdata+8);
			tvs.modtime = make_unix_date2(pdata+12);
			size = IVAL(pdata,16);
			dosmode = IVAL(pdata,24);
			break;
#endif

		case SMB_SET_FILE_BASIC_INFO:
		case SMB_FILE_BASIC_INFORMATION:
		{
			/* Patch to do this correctly from Paul Eggert <eggert@twinsun.com>. */
			time_t write_time;
			time_t changed_time;

			if (total_data < 36) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			/* Ignore create time at offset pdata. */

			/* access time */
			tvs.actime = interpret_long_date(pdata+8);

			write_time = interpret_long_date(pdata+16);
			changed_time = interpret_long_date(pdata+24);

			tvs.modtime = MIN(write_time, changed_time);

			if (write_time > tvs.modtime && write_time != (time_t)-1) {
				tvs.modtime = write_time;
			}
			/* Prefer a defined time to an undefined one. */
			if (null_mtime(tvs.modtime)) {
				tvs.modtime = null_mtime(write_time) ? changed_time : write_time;
			}

			/* attributes */
			dosmode = IVAL(pdata,32);
			break;
		}

		case SMB_FILE_ALLOCATION_INFORMATION:
		case SMB_SET_FILE_ALLOCATION_INFO:
		{
			int ret = -1;
			SMB_BIG_UINT allocation_size;

			if (total_data < 8) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			allocation_size = (SMB_BIG_UINT)IVAL(pdata,0);
#ifdef LARGE_SMB_OFF_T
			allocation_size |= (((SMB_BIG_UINT)IVAL(pdata,4)) << 32);
#else /* LARGE_SMB_OFF_T */
			if (IVAL(pdata,4) != 0) {
				/* more than 32 bits? */
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
#endif /* LARGE_SMB_OFF_T */
			DEBUG(10,("call_trans2setfilepathinfo: Set file allocation info for file %s to %.0f\n",
					fname, (double)allocation_size ));

			if (allocation_size) {
				allocation_size = smb_roundup(conn, allocation_size);
			}

			if(allocation_size != get_file_size(sbuf)) {
				SMB_STRUCT_STAT new_sbuf;
 
				DEBUG(10,("call_trans2setfilepathinfo: file %s : setting new allocation size to %.0f\n",
					fname, (double)allocation_size ));
 
				if (fd == -1) {
					files_struct *new_fsp = NULL;
 
					new_fsp = open_file_ntcreate(conn, fname, &sbuf,
									FILE_WRITE_DATA,
									FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
									FILE_OPEN,
									0,
									FILE_ATTRIBUTE_NORMAL,
									FORCE_OPLOCK_BREAK_TO_NONE,
									NULL);
 
					if (new_fsp == NULL) {
						if (open_was_deferred(SVAL(inbuf,smb_mid))) {
							/* We have re-scheduled this call. */
							return -1;
						}
						return(UNIXERROR(ERRDOS,ERRnoaccess));
					}
					ret = vfs_allocate_file_space(new_fsp, allocation_size);
					if (SMB_VFS_FSTAT(new_fsp,new_fsp->fh->fd,&new_sbuf) != 0) {
						DEBUG(3,("call_trans2setfilepathinfo: fstat of fnum %d failed (%s)\n",
									new_fsp->fnum, strerror(errno)));
						ret = -1;
					}
					close_file(new_fsp,NORMAL_CLOSE);
				} else {
					ret = vfs_allocate_file_space(fsp, allocation_size);
					if (SMB_VFS_FSTAT(fsp,fd,&new_sbuf) != 0) {
						DEBUG(3,("call_trans2setfilepathinfo: fstat of fnum %d failed (%s)\n",
									fsp->fnum, strerror(errno)));
						ret = -1;
					}
				}
				if (ret == -1)
					return ERROR_NT(NT_STATUS_DISK_FULL);

				/* Allocate can truncate size... */
				size = get_file_size(new_sbuf);
			}

			break;
		}

		case SMB_FILE_END_OF_FILE_INFORMATION:
		case SMB_SET_FILE_END_OF_FILE_INFO:
		{
			if (total_data < 8) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			size = IVAL(pdata,0);
#ifdef LARGE_SMB_OFF_T
			size |= (((SMB_OFF_T)IVAL(pdata,4)) << 32);
#else /* LARGE_SMB_OFF_T */
			if (IVAL(pdata,4) != 0)	{
				/* more than 32 bits? */
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
#endif /* LARGE_SMB_OFF_T */
			DEBUG(10,("call_trans2setfilepathinfo: Set end of file info for file %s to %.0f\n", fname, (double)size ));
			break;
		}

		case SMB_FILE_DISPOSITION_INFORMATION:
		case SMB_SET_FILE_DISPOSITION_INFO: /* Set delete on close for open file. */
		{
			BOOL delete_on_close;

			if (total_data < 1) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			delete_on_close = (CVAL(pdata,0) ? True : False);

			/* Just ignore this set on a path. */
			if (tran_call != TRANSACT2_SETFILEINFO)
				break;

			if (fsp == NULL)
				return(UNIXERROR(ERRDOS,ERRbadfid));

			status = can_set_delete_on_close(fsp, delete_on_close,
							 dosmode);
 
			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			/* The set is across all open files on this dev/inode pair. */
			if (!set_delete_on_close(fsp, delete_on_close, &current_user.ut)) {
				return ERROR_NT(NT_STATUS_ACCESS_DENIED);
			}

			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

		case SMB_FILE_POSITION_INFORMATION:
		{
			SMB_BIG_UINT position_information;

			if (total_data < 8) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			position_information = (SMB_BIG_UINT)IVAL(pdata,0);
#ifdef LARGE_SMB_OFF_T
			position_information |= (((SMB_BIG_UINT)IVAL(pdata,4)) << 32);
#else /* LARGE_SMB_OFF_T */
			if (IVAL(pdata,4) != 0) {
				/* more than 32 bits? */
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
#endif /* LARGE_SMB_OFF_T */
			DEBUG(10,("call_trans2setfilepathinfo: Set file position information for file %s to %.0f\n",
					fname, (double)position_information ));
			if (fsp) {
				fsp->fh->position_information = position_information;
			}

			/* We're done. We only get position info in this call. */
			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

		/* From tridge Samba4 : 
		 * MODE_INFORMATION in setfileinfo (I have no
		 * idea what "mode information" on a file is - it takes a value of 0,
		 * 2, 4 or 6. What could it be?).
		 */

		case SMB_FILE_MODE_INFORMATION:
		{
			uint32 mode;

			if (total_data < 4) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			mode = IVAL(pdata,0);
			if (mode != 0 && mode != 2 && mode != 4 && mode != 6) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			/* We're done. We only get mode info in this call. */
			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

		/*
		 * CIFS UNIX extensions.
		 */

		case SMB_SET_FILE_UNIX_BASIC:
		{
			uint32 raw_unixmode;

			if (total_data < 100) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if(IVAL(pdata, 0) != SMB_SIZE_NO_CHANGE_LO &&
			   IVAL(pdata, 4) != SMB_SIZE_NO_CHANGE_HI) {
				size=IVAL(pdata,0); /* first 8 Bytes are size */
#ifdef LARGE_SMB_OFF_T
				size |= (((SMB_OFF_T)IVAL(pdata,4)) << 32);
#else /* LARGE_SMB_OFF_T */
				if (IVAL(pdata,4) != 0)	{
					/* more than 32 bits? */
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}
#endif /* LARGE_SMB_OFF_T */
			}
			pdata+=24;          /* ctime & st_blocks are not changed */
			tvs.actime = interpret_long_date(pdata); /* access_time */
			tvs.modtime = interpret_long_date(pdata+8); /* modification_time */
			pdata+=16;
			set_owner = (uid_t)IVAL(pdata,0);
			pdata += 8;
			set_grp = (gid_t)IVAL(pdata,0);
			pdata += 8;
			raw_unixmode = IVAL(pdata,28);
			unixmode = unix_perms_from_wire(conn, &sbuf, raw_unixmode);
			dosmode = 0; /* Ensure dos mode change doesn't override this. */

			DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_BASIC: name = %s \
size = %.0f, uid = %u, gid = %u, raw perms = 0%o\n",
				fname, (double)size, (unsigned int)set_owner, (unsigned int)set_grp, (int)raw_unixmode));

			if (!VALID_STAT(sbuf)) {

				/*
				 * The only valid use of this is to create character and block
				 * devices, and named pipes. This is deprecated (IMHO) and 
				 * a new info level should be used for mknod. JRA.
				 */

				uint32 file_type = IVAL(pdata,0);
#if defined(HAVE_MAKEDEV)
				uint32 dev_major = IVAL(pdata,4);
				uint32 dev_minor = IVAL(pdata,12);
#endif

				uid_t myuid = geteuid();
				gid_t mygid = getegid();
				SMB_DEV_T dev = (SMB_DEV_T)0;

				if (tran_call == TRANSACT2_SETFILEINFO)
					return(ERROR_DOS(ERRDOS,ERRnoaccess));

				if (raw_unixmode == SMB_MODE_NO_CHANGE) {
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
				}

#if defined(HAVE_MAKEDEV)
				dev = makedev(dev_major, dev_minor);
#endif

				/* We can only create as the owner/group we are. */

				if ((set_owner != myuid) && (set_owner != (uid_t)SMB_UID_NO_CHANGE))
					return(ERROR_DOS(ERRDOS,ERRnoaccess));
				if ((set_grp != mygid) && (set_grp != (gid_t)SMB_GID_NO_CHANGE))
					return(ERROR_DOS(ERRDOS,ERRnoaccess));

				switch (file_type) {
#if defined(S_IFIFO)
					case UNIX_TYPE_FIFO:
						unixmode |= S_IFIFO;
						break;
#endif
#if defined(S_IFSOCK)
					case UNIX_TYPE_SOCKET:
						unixmode |= S_IFSOCK;
						break;
#endif
#if defined(S_IFCHR)
					case UNIX_TYPE_CHARDEV:
						unixmode |= S_IFCHR;
						break;
#endif
#if defined(S_IFBLK)
					case UNIX_TYPE_BLKDEV:
						unixmode |= S_IFBLK;
						break;
#endif
					default:
						return(ERROR_DOS(ERRDOS,ERRnoaccess));
				}

				DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_BASIC doing mknod dev %.0f mode \
0%o for file %s\n", (double)dev, unixmode, fname ));

				/* Ok - do the mknod. */
				if (SMB_VFS_MKNOD(conn,fname, unixmode, dev) != 0)
					return(UNIXERROR(ERRDOS,ERRnoaccess));

				inherit_access_acl(conn, fname, unixmode);

				SSVAL(params,0,0);
				send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
				return(-1);
			}

			/*
			 * Deal with the UNIX specific mode set.
			 */

			if (raw_unixmode != SMB_MODE_NO_CHANGE) {
				DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_BASIC setting mode 0%o for file %s\n",
					(unsigned int)unixmode, fname ));
				if (SMB_VFS_CHMOD(conn,fname,unixmode) != 0)
					return(UNIXERROR(ERRDOS,ERRnoaccess));
			}

			/*
			 * Deal with the UNIX specific uid set.
			 */

			if ((set_owner != (uid_t)SMB_UID_NO_CHANGE) && (sbuf.st_uid != set_owner)) {
				DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_BASIC changing owner %u for file %s\n",
					(unsigned int)set_owner, fname ));
				if (SMB_VFS_CHOWN(conn,fname,set_owner, (gid_t)-1) != 0)
					return(UNIXERROR(ERRDOS,ERRnoaccess));
			}

			/*
			 * Deal with the UNIX specific gid set.
			 */

			if ((set_grp != (uid_t)SMB_GID_NO_CHANGE) && (sbuf.st_gid != set_grp)) {
				DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_BASIC changing group %u for file %s\n",
					(unsigned int)set_owner, fname ));
				if (SMB_VFS_CHOWN(conn,fname,(uid_t)-1, set_grp) != 0)
					return(UNIXERROR(ERRDOS,ERRnoaccess));
			}
			break;
		}

		case SMB_SET_FILE_UNIX_LINK:
		{
			pstring link_target;
			char *newname = fname;

			/* Set a symbolic link. */
			/* Don't allow this if follow links is false. */

			if (!lp_symlinks(SNUM(conn)))
				return(ERROR_DOS(ERRDOS,ERRnoaccess));

			srvstr_pull(inbuf, link_target, pdata, sizeof(link_target), -1, STR_TERMINATE);

			/* !widelinks forces the target path to be within the share. */
			/* This means we can interpret the target as a pathname. */
			if (!lp_widelinks(SNUM(conn))) {
				pstring rel_name;
				char *last_dirp = NULL;

				unix_format(link_target);
				if (*link_target == '/') {
					/* No absolute paths allowed. */
					return(UNIXERROR(ERRDOS,ERRnoaccess));
				}
				pstrcpy(rel_name, newname);
				last_dirp = strrchr_m(rel_name, '/');
				if (last_dirp) {
					last_dirp[1] = '\0';
				} else {
					pstrcpy(rel_name, "./");
				}
				pstrcat(rel_name, link_target);

				if (!check_name(rel_name, conn)) {
					return(UNIXERROR(ERRDOS,ERRnoaccess));
				}
			}

			DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_LINK doing symlink %s -> %s\n",
				fname, link_target ));

			if (SMB_VFS_SYMLINK(conn,link_target,newname) != 0)
				return(UNIXERROR(ERRDOS,ERRnoaccess));
			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

		case SMB_SET_FILE_UNIX_HLINK:
		{
			pstring oldname;
			char *newname = fname;

			/* Set a hard link. */
			srvstr_get_path(inbuf, oldname, pdata, sizeof(oldname), -1, STR_TERMINATE, &status);
			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			DEBUG(10,("call_trans2setfilepathinfo: SMB_SET_FILE_UNIX_LINK doing hard link %s -> %s\n",
				fname, oldname));

			status = hardlink_internals(conn, oldname, newname);
			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

		case SMB_FILE_RENAME_INFORMATION:
		{
			BOOL overwrite;
			uint32 root_fid;
			uint32 len;
			pstring newname;
			pstring base_name;
			char *p;

			if (total_data < 12) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			overwrite = (CVAL(pdata,0) ? True : False);
			root_fid = IVAL(pdata,4);
			len = IVAL(pdata,8);
			srvstr_get_path(inbuf, newname, &pdata[12], sizeof(newname), len, 0, &status);
			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			/* Check the new name has no '/' characters. */
			if (strchr_m(newname, '/'))
				return ERROR_NT(NT_STATUS_NOT_SUPPORTED);

			RESOLVE_DFSPATH(newname, conn, inbuf, outbuf);

			/* Create the base directory. */
			pstrcpy(base_name, fname);
			p = strrchr_m(base_name, '/');
			if (p)
				*p = '\0';
			/* Append the new name. */
			pstrcat(base_name, "/");
			pstrcat(base_name, newname);

			if (fsp) {
				DEBUG(10,("call_trans2setfilepathinfo: SMB_FILE_RENAME_INFORMATION (fnum %d) %s -> %s\n",
					fsp->fnum, fsp->fsp_name, base_name ));
				status = rename_internals_fsp(conn, fsp, base_name, 0, overwrite);
			} else {
				DEBUG(10,("call_trans2setfilepathinfo: SMB_FILE_RENAME_INFORMATION %s -> %s\n",
					fname, newname ));
				status = rename_internals(conn, fname, base_name, 0, overwrite, False);
			}
			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}
			process_pending_change_notify_queue((time_t)0);
			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}

#if defined(HAVE_POSIX_ACLS)
		case SMB_SET_POSIX_ACL:
		{
			uint16 posix_acl_version;
			uint16 num_file_acls;
			uint16 num_def_acls;
			BOOL valid_file_acls = True;
			BOOL valid_def_acls = True;

			if (total_data < SMB_POSIX_ACL_HEADER_SIZE) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}
			posix_acl_version = SVAL(pdata,0);
			num_file_acls = SVAL(pdata,2);
			num_def_acls = SVAL(pdata,4);

			if (num_file_acls == SMB_POSIX_IGNORE_ACE_ENTRIES) {
				valid_file_acls = False;
				num_file_acls = 0;
			}

			if (num_def_acls == SMB_POSIX_IGNORE_ACE_ENTRIES) {
				valid_def_acls = False;
				num_def_acls = 0;
			}

			if (posix_acl_version != SMB_POSIX_ACL_VERSION) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if (total_data < SMB_POSIX_ACL_HEADER_SIZE +
					(num_file_acls+num_def_acls)*SMB_POSIX_ACL_ENTRY_SIZE) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if (valid_file_acls && !set_unix_posix_acl(conn, fsp, fname, num_file_acls,
					pdata + SMB_POSIX_ACL_HEADER_SIZE)) {
				return(UNIXERROR(ERRDOS,ERRnoaccess));
			}

			if (valid_def_acls && !set_unix_posix_default_acl(conn, fname, &sbuf, num_def_acls,
					pdata + SMB_POSIX_ACL_HEADER_SIZE +
					(num_file_acls*SMB_POSIX_ACL_ENTRY_SIZE))) {
				return(UNIXERROR(ERRDOS,ERRnoaccess));
			}

			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}
#endif

#if defined(DEVELOPER)
		case SMB_SET_POSIX_LOCK:
		{
			SMB_BIG_UINT count;
			SMB_BIG_UINT offset;
			uint16 lock_pid;
			BOOL lock_blocking;
			enum brl_type lock_type;
			BOOL my_lock_ctx;

			if (fsp == NULL || fsp->fh->fd == -1) {
				return ERROR_NT(NT_STATUS_INVALID_HANDLE);
			}

			if (total_data != POSIX_LOCK_DATA_SIZE) {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			switch (SVAL(pdata, POSIX_LOCK_TYPE_OFFSET)) {
				case POSIX_LOCK_TYPE_READ:
					lock_type = READ_LOCK;
					break;
				case POSIX_LOCK_TYPE_WRITE:
					/* Return the right POSIX-mappable error code for files opened read-only. */
					if (!fsp->can_write) {
						return ERROR_NT(NT_STATUS_INVALID_HANDLE);
					}
					lock_type = WRITE_LOCK;
					break;
				case POSIX_LOCK_TYPE_UNLOCK:
					lock_type = UNLOCK_LOCK;
					break;
				default:
					return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			if (SVAL(pdata,POSIX_LOCK_FLAGS_OFFSET) == POSIX_LOCK_FLAG_NOWAIT) {
				lock_blocking = False;
			} else if (SVAL(pdata,POSIX_LOCK_FLAGS_OFFSET) == POSIX_LOCK_FLAG_WAIT) {
				lock_blocking = True;
			} else {
				return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
			}

			lock_pid = (uint16)IVAL(pdata, POSIX_LOCK_PID_OFFSET);
#if defined(HAVE_LONGLONG)
			offset = (((SMB_BIG_UINT) IVAL(pdata,(POSIX_LOCK_START_OFFSET+4))) << 32) |
					((SMB_BIG_UINT) IVAL(pdata,POSIX_LOCK_START_OFFSET));
			count = (((SMB_BIG_UINT) IVAL(pdata,(POSIX_LOCK_LEN_OFFSET+4))) << 32) |
					((SMB_BIG_UINT) IVAL(pdata,POSIX_LOCK_LEN_OFFSET));
#else /* HAVE_LONGLONG */
			offset = (SMB_BIG_UINT)IVAL(pdata,POSIX_LOCK_START_OFFSET);
			count = (SMB_BIG_UINT)IVAL(pdata,POSIX_LOCK_LEN_OFFSET);
#endif /* HAVE_LONGLONG */

			if (lock_type == UNLOCK_LOCK) {
				status = do_unlock(fsp,
						lock_pid,
						count,
						offset,
						POSIX_LOCK);
			} else {
				status = do_lock(fsp,
						lock_pid,
						count,
						offset,
						lock_type,
						POSIX_LOCK,
						&my_lock_ctx);

				if (lp_blocking_locks(SNUM(conn)) && ERROR_WAS_LOCK_DENIED(status)) {
					/*
					 * A blocking lock was requested. Package up
					 * this smb into a queued request and push it
					 * onto the blocking lock queue.
					 */
					if(push_blocking_lock_request(inbuf, length,
								fsp,
								-1, /* infinite timeout. */
								0,
								lock_pid,
								lock_type,
								POSIX_LOCK,
								offset,
								count)) {
						return -1;
					}
				}
			}

			if (!NT_STATUS_IS_OK(status)) {
				return ERROR_NT(status);
			}

			SSVAL(params,0,0);
			send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
			return(-1);
		}
#endif

		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	/* get some defaults (no modifications) if any info is zero or -1. */
	if (null_mtime(tvs.actime)) {
		tvs.actime = sbuf.st_atime;
	}

	if (null_mtime(tvs.modtime)) {
		tvs.modtime = sbuf.st_mtime;
	}

	DEBUG(6,("actime: %s " , ctime(&tvs.actime)));
	DEBUG(6,("modtime: %s ", ctime(&tvs.modtime)));
	DEBUG(6,("size: %.0f ", (double)size));

	if (dosmode) {
		if (S_ISDIR(sbuf.st_mode))
			dosmode |= aDIR;
		else
			dosmode &= ~aDIR;
	}

	DEBUG(6,("dosmode: %x\n"  , dosmode));

	if(!((info_level == SMB_SET_FILE_END_OF_FILE_INFO) ||
		(info_level == SMB_SET_FILE_ALLOCATION_INFO) ||
		(info_level == SMB_FILE_ALLOCATION_INFORMATION) ||
		(info_level == SMB_FILE_END_OF_FILE_INFORMATION))) {

		/*
		 * Only do this test if we are not explicitly
		 * changing the size of a file.
		 */
		if (!size)
			size = get_file_size(sbuf);
	}

	/*
	 * Try and set the times, size and mode of this file -
	 * if they are different from the current values
	 */

	/* check the mode isn't different, before changing it */
	if ((dosmode != 0) && (dosmode != dos_mode(conn, fname, &sbuf))) {

		DEBUG(10,("call_trans2setfilepathinfo: file %s : setting dos mode %x\n", fname, dosmode ));

		if(file_set_dosmode(conn, fname, dosmode, &sbuf, False)) {
			DEBUG(2,("file_set_dosmode of %s failed (%s)\n", fname, strerror(errno)));
			return(UNIXERROR(ERRDOS,ERRnoaccess));
		}
	}

	/* Now the size. */
	if (size != get_file_size(sbuf)) {

		int ret;

		DEBUG(10,("call_trans2setfilepathinfo: file %s : setting new size to %.0f\n",
			fname, (double)size ));

		if (fd == -1) {
			files_struct *new_fsp = NULL;

			new_fsp = open_file_ntcreate(conn, fname, &sbuf,
						FILE_WRITE_DATA,
						FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
						FILE_OPEN,
						0,
						FILE_ATTRIBUTE_NORMAL,
						FORCE_OPLOCK_BREAK_TO_NONE,
						NULL);
	
			if (new_fsp == NULL) {
				if (open_was_deferred(SVAL(inbuf,smb_mid))) {
					/* We have re-scheduled this call. */
					return -1;
				}
				return(UNIXERROR(ERRDOS,ERRnoaccess));
			}
			ret = vfs_set_filelen(new_fsp, size);
			close_file(new_fsp,NORMAL_CLOSE);
		} else {
			ret = vfs_set_filelen(fsp, size);
		}

		if (ret == -1) {
			return (UNIXERROR(ERRHRD,ERRdiskfull));
		}
	}

	/*
	 * Finally the times.
	 */
	if (sbuf.st_mtime != tvs.modtime || sbuf.st_atime != tvs.actime) {
		if(fsp != NULL) {
			/*
			 * This was a setfileinfo on an open file.
			 * NT does this a lot. We also need to 
			 * set the time here, as it can be read by 
			 * FindFirst/FindNext and with the patch for bug #2045
			 * in smbd/fileio.c it ensures that this timestamp is
			 * kept sticky even after a write. We save the request
			 * away and will set it on file close and after a write. JRA.
			 */

			if (tvs.modtime != (time_t)0 && tvs.modtime != (time_t)-1) {
				DEBUG(10,("call_trans2setfilepathinfo: setting pending modtime to %s\n", ctime(&tvs.modtime) ));
				fsp_set_pending_modtime(fsp, tvs.modtime);
			}

		}
		DEBUG(10,("call_trans2setfilepathinfo: setting utimes to modified values.\n"));

		if(file_utime(conn, fname, &tvs)!=0) {
			return(UNIXERROR(ERRDOS,ERRnoaccess));
		}
	}

	SSVAL(params,0,0);
	send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
  
	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_MKDIR (make directory with extended attributes).
****************************************************************************/

static int call_trans2mkdir(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *params = *pparams;
	char *pdata = *ppdata;
	pstring directory;
	int ret = -1;
	SMB_STRUCT_STAT sbuf;
	BOOL bad_path = False;
	NTSTATUS status = NT_STATUS_OK;
	TALLOC_CTX *ctx = NULL;
	struct ea_list *ea_list = NULL;

	if (!CAN_WRITE(conn))
		return ERROR_DOS(ERRSRV,ERRaccess);

	if (total_params < 4) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	srvstr_get_path(inbuf, directory, &params[4], sizeof(directory), -1, STR_TERMINATE, &status);
	if (!NT_STATUS_IS_OK(status)) {
		return ERROR_NT(status);
	}

	DEBUG(3,("call_trans2mkdir : name = %s\n", directory));

	unix_convert(directory,conn,0,&bad_path,&sbuf);
	if (bad_path) {
		return ERROR_NT(NT_STATUS_OBJECT_PATH_NOT_FOUND);
	}

	/* Any data in this call is an EA list. */
	if (total_data && (total_data != 4) && !lp_ea_support(SNUM(conn))) {
		return ERROR_NT(NT_STATUS_EAS_NOT_SUPPORTED);
	}

	/*
	 * OS/2 workplace shell seems to send SET_EA requests of "null"
	 * length (4 bytes containing IVAL 4).
	 * They seem to have no effect. Bug #3212. JRA.
	 */

	if (total_data != 4) {
		if (total_data < 10) {
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		if (IVAL(pdata,0) > total_data) {
			DEBUG(10,("call_trans2mkdir: bad total data size (%u) > %u\n",
				IVAL(pdata,0), (unsigned int)total_data));
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}

		ctx = talloc_init("TRANS2_MKDIR_SET_EA");
		if (!ctx) {
			return ERROR_NT(NT_STATUS_NO_MEMORY);
		}
		ea_list = read_ea_list(ctx, pdata + 4, total_data - 4);
		if (!ea_list) {
			talloc_destroy(ctx);
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
	} else if (IVAL(pdata,0) != 4) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	if (check_name(directory,conn)) {
		ret = vfs_MkDir(conn,directory,unix_mode(conn,aDIR,directory,True));
	}
  
	if(ret < 0) {
		talloc_destroy(ctx);
		DEBUG(5,("call_trans2mkdir error (%s)\n", strerror(errno)));
		return set_bad_path_error(errno, bad_path, outbuf, ERRDOS,ERRnoaccess);
	}

	/* Try and set any given EA. */
	if (total_data) {
		status = set_ea(conn, NULL, directory, ea_list);
		talloc_destroy(ctx);
		if (!NT_STATUS_IS_OK(status)) {
			return ERROR_NT(status);
		}
	}

	/* Realloc the parameter and data sizes */
	*pparams = SMB_REALLOC(*pparams,2);
	if(*pparams == NULL) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	SSVAL(params,0,0);

	send_trans2_replies(outbuf, bufsize, params, 2, *ppdata, 0);
  
	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_FINDNOTIFYFIRST (start monitoring a directory for changes).
 We don't actually do this - we just send a null response.
****************************************************************************/

static int call_trans2findnotifyfirst(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	static uint16 fnf_handle = 257;
	char *params = *pparams;
	uint16 info_level;

	if (total_params < 6) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	info_level = SVAL(params,4);
	DEBUG(3,("call_trans2findnotifyfirst - info_level %d\n", info_level));

	switch (info_level) {
		case 1:
		case 2:
			break;
		default:
			return ERROR_NT(NT_STATUS_INVALID_LEVEL);
	}

	/* Realloc the parameter and data sizes */
	*pparams = SMB_REALLOC(*pparams,6);
	if (*pparams == NULL) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	SSVAL(params,0,fnf_handle);
	SSVAL(params,2,0); /* No changes */
	SSVAL(params,4,0); /* No EA errors */

	fnf_handle++;

	if(fnf_handle == 0)
		fnf_handle = 257;

	send_trans2_replies(outbuf, bufsize, params, 6, *ppdata, 0);
  
	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_FINDNOTIFYNEXT (continue monitoring a directory for 
 changes). Currently this does nothing.
****************************************************************************/

static int call_trans2findnotifynext(connection_struct *conn, char *inbuf, char *outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *params = *pparams;

	DEBUG(3,("call_trans2findnotifynext\n"));

	/* Realloc the parameter and data sizes */
	*pparams = SMB_REALLOC(*pparams,4);
	if (*pparams == NULL) {
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}
	params = *pparams;

	SSVAL(params,0,0); /* No changes */
	SSVAL(params,2,0); /* No EA errors */

	send_trans2_replies(outbuf, bufsize, params, 4, *ppdata, 0);
  
	return(-1);
}

/****************************************************************************
 Reply to a TRANS2_GET_DFS_REFERRAL - Shirish Kalele <kalele@veritas.com>.
****************************************************************************/

static int call_trans2getdfsreferral(connection_struct *conn, char* inbuf, char* outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *params = *pparams;
  	pstring pathname;
	int reply_size = 0;
	int max_referral_level;

	DEBUG(10,("call_trans2getdfsreferral\n"));

	if (total_params < 2) {
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	max_referral_level = SVAL(params,0);

	if(!lp_host_msdfs())
		return ERROR_DOS(ERRDOS,ERRbadfunc);

	srvstr_pull(inbuf, pathname, &params[2], sizeof(pathname), -1, STR_TERMINATE);
	if((reply_size = setup_dfs_referral(conn, pathname,max_referral_level,ppdata)) < 0)
		return UNIXERROR(ERRDOS,ERRbadfile);
    
	SSVAL(outbuf,smb_flg2,SVAL(outbuf,smb_flg2) | FLAGS2_DFS_PATHNAMES);
	send_trans2_replies(outbuf,bufsize,0,0,*ppdata,reply_size);

	return(-1);
}

#define LMCAT_SPL       0x53
#define LMFUNC_GETJOBID 0x60

/****************************************************************************
 Reply to a TRANS2_IOCTL - used for OS/2 printing.
****************************************************************************/

static int call_trans2ioctl(connection_struct *conn, char* inbuf, char* outbuf, int length, int bufsize,
					char **pparams, int total_params, char **ppdata, int total_data,
					unsigned int max_data_bytes)
{
	char *pdata = *ppdata;
	files_struct *fsp = file_fsp(inbuf,smb_vwv15);

	/* check for an invalid fid before proceeding */
	
	if (!fsp)                                
		return(ERROR_DOS(ERRDOS,ERRbadfid));  

	if ((SVAL(inbuf,(smb_setup+4)) == LMCAT_SPL) &&
			(SVAL(inbuf,(smb_setup+6)) == LMFUNC_GETJOBID)) {
		*ppdata = SMB_REALLOC(*ppdata, 32);
		if (*ppdata == NULL) {
			return ERROR_NT(NT_STATUS_NO_MEMORY);
		}
		pdata = *ppdata;

		/* NOTE - THIS IS ASCII ONLY AT THE MOMENT - NOT SURE IF OS/2
			CAN ACCEPT THIS IN UNICODE. JRA. */

		SSVAL(pdata,0,fsp->rap_print_jobid);                     /* Job number */
		srvstr_push( outbuf, pdata + 2, global_myname(), 15, STR_ASCII|STR_TERMINATE); /* Our NetBIOS name */
		srvstr_push( outbuf, pdata+18, lp_servicename(SNUM(conn)), 13, STR_ASCII|STR_TERMINATE); /* Service name */
		send_trans2_replies(outbuf,bufsize,*pparams,0,*ppdata,32);
		return(-1);
	} else {
		DEBUG(2,("Unknown TRANS2_IOCTL\n"));
		return ERROR_DOS(ERRSRV,ERRerror);
	}
}

/****************************************************************************
 Reply to a SMBfindclose (stop trans2 directory search).
****************************************************************************/

int reply_findclose(connection_struct *conn,
		    char *inbuf,char *outbuf,int length,int bufsize)
{
	int outsize = 0;
	int dptr_num=SVALS(inbuf,smb_vwv0);
	START_PROFILE(SMBfindclose);

	DEBUG(3,("reply_findclose, dptr_num = %d\n", dptr_num));

	dptr_close(&dptr_num);

	outsize = set_message(outbuf,0,0,False);

	DEBUG(3,("SMBfindclose dptr_num = %d\n", dptr_num));

	END_PROFILE(SMBfindclose);
	return(outsize);
}

/****************************************************************************
 Reply to a SMBfindnclose (stop FINDNOTIFYFIRST directory search).
****************************************************************************/

int reply_findnclose(connection_struct *conn, 
		     char *inbuf,char *outbuf,int length,int bufsize)
{
	int outsize = 0;
	int dptr_num= -1;
	START_PROFILE(SMBfindnclose);
	
	dptr_num = SVAL(inbuf,smb_vwv0);

	DEBUG(3,("reply_findnclose, dptr_num = %d\n", dptr_num));

	/* We never give out valid handles for a 
	   findnotifyfirst - so any dptr_num is ok here. 
	   Just ignore it. */

	outsize = set_message(outbuf,0,0,False);

	DEBUG(3,("SMB_findnclose dptr_num = %d\n", dptr_num));

	END_PROFILE(SMBfindnclose);
	return(outsize);
}

int handle_trans2(connection_struct *conn,
		  struct trans_state *state,
		  char *inbuf, char *outbuf, int size, int bufsize)
{
	int outsize;

	if (Protocol >= PROTOCOL_NT1) {
		SSVAL(outbuf,smb_flg2,SVAL(outbuf,smb_flg2) | 0x40); /* IS_LONG_NAME */
	}

	/* Now we must call the relevant TRANS2 function */
	switch(state->call)  {
	case TRANSACT2_OPEN:
	{
		START_PROFILE_NESTED(Trans2_open);
		outsize = call_trans2open(
			conn, inbuf, outbuf, bufsize, 
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_open);
		break;
	}

	case TRANSACT2_FINDFIRST:
	{
		START_PROFILE_NESTED(Trans2_findfirst);
		outsize = call_trans2findfirst(
			conn, inbuf, outbuf, bufsize,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_findfirst);
		break;
	}

	case TRANSACT2_FINDNEXT:
	{
		START_PROFILE_NESTED(Trans2_findnext);
		outsize = call_trans2findnext(
			conn, inbuf, outbuf, size, bufsize, 
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_findnext);
		break;
	}

	case TRANSACT2_QFSINFO:
	{
		START_PROFILE_NESTED(Trans2_qfsinfo);
		outsize = call_trans2qfsinfo(
			conn, inbuf, outbuf, size, bufsize,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_qfsinfo);
	    break;
	}

	case TRANSACT2_SETFSINFO:
	{
		START_PROFILE_NESTED(Trans2_setfsinfo);
		outsize = call_trans2setfsinfo(
			conn, inbuf, outbuf, size, bufsize, 
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_setfsinfo);
		break;
	}

	case TRANSACT2_QPATHINFO:
	case TRANSACT2_QFILEINFO:
	{
		START_PROFILE_NESTED(Trans2_qpathinfo);
		outsize = call_trans2qfilepathinfo(
			conn, inbuf, outbuf, size, bufsize, state->call,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_qpathinfo);
		break;
	}

	case TRANSACT2_SETPATHINFO:
	case TRANSACT2_SETFILEINFO:
	{
		START_PROFILE_NESTED(Trans2_setpathinfo);
		outsize = call_trans2setfilepathinfo(
			conn, inbuf, outbuf, size, bufsize, state->call,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_setpathinfo);
		break;
	}

	case TRANSACT2_FINDNOTIFYFIRST:
	{
		START_PROFILE_NESTED(Trans2_findnotifyfirst);
		outsize = call_trans2findnotifyfirst(
			conn, inbuf, outbuf, size, bufsize, 
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_findnotifyfirst);
		break;
	}

	case TRANSACT2_FINDNOTIFYNEXT:
	{
		START_PROFILE_NESTED(Trans2_findnotifynext);
		outsize = call_trans2findnotifynext(
			conn, inbuf, outbuf, size, bufsize, 
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_findnotifynext);
		break;
	}

	case TRANSACT2_MKDIR:
	{
		START_PROFILE_NESTED(Trans2_mkdir);
		outsize = call_trans2mkdir(
			conn, inbuf, outbuf, size, bufsize,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_mkdir);
		break;
	}

	case TRANSACT2_GET_DFS_REFERRAL:
	{
		START_PROFILE_NESTED(Trans2_get_dfs_referral);
		outsize = call_trans2getdfsreferral(
			conn, inbuf, outbuf, size, bufsize,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_get_dfs_referral);
		break;
	}

	case TRANSACT2_IOCTL:
	{
		START_PROFILE_NESTED(Trans2_ioctl);
		outsize = call_trans2ioctl(
			conn, inbuf, outbuf, size, bufsize,
			&state->param, state->total_param,
			&state->data, state->total_data,
			state->max_data_return);
		END_PROFILE_NESTED(Trans2_ioctl);
		break;
	}

	default:
		/* Error in request */
		DEBUG(2,("Unknown request %d in trans2 call\n", state->call));
		outsize = ERROR_DOS(ERRSRV,ERRerror);
	}

	return outsize;
}

/****************************************************************************
 Reply to a SMBtrans2.
 ****************************************************************************/

int reply_trans2(connection_struct *conn, char *inbuf,char *outbuf,
		 int size, int bufsize)
{
	int outsize = 0;
	unsigned int dsoff = SVAL(inbuf, smb_dsoff);
	unsigned int dscnt = SVAL(inbuf, smb_dscnt);
	unsigned int psoff = SVAL(inbuf, smb_psoff);
	unsigned int pscnt = SVAL(inbuf, smb_pscnt);
	unsigned int tran_call = SVAL(inbuf, smb_setup0);
	struct trans_state *state;
	NTSTATUS result;

	START_PROFILE(SMBtrans2);

	result = allow_new_trans(conn->pending_trans, SVAL(inbuf, smb_mid));
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(2, ("Got invalid trans2 request: %s\n",
			  nt_errstr(result)));
		END_PROFILE(SMBtrans2);
		return ERROR_NT(result);
	}

	if (IS_IPC(conn) && (tran_call != TRANSACT2_OPEN)
            && (tran_call != TRANSACT2_GET_DFS_REFERRAL)) {
		END_PROFILE(SMBtrans2);
		return ERROR_DOS(ERRSRV,ERRaccess);
	}

	if ((state = TALLOC_P(NULL, struct trans_state)) == NULL) {
		DEBUG(0, ("talloc failed\n"));
		END_PROFILE(SMBtrans2);
		return ERROR_NT(NT_STATUS_NO_MEMORY);
	}

	state->cmd = SMBtrans2;

	state->mid = SVAL(inbuf, smb_mid);
	state->vuid = SVAL(inbuf, smb_uid);
	state->setup_count = SVAL(inbuf, smb_suwcnt);
	state->total_param = SVAL(inbuf, smb_tpscnt);
	state->param = NULL;
	state->total_data =  SVAL(inbuf, smb_tdscnt);
	state->data = NULL;
	state->max_param_return = SVAL(inbuf, smb_mprcnt);
	state->max_data_return  = SVAL(inbuf, smb_mdrcnt);
	state->max_setup_return = SVAL(inbuf, smb_msrcnt);
	state->close_on_completion = BITSETW(inbuf+smb_vwv5,0);
	state->one_way = BITSETW(inbuf+smb_vwv5,1);

	state->call = tran_call;

	/* All trans2 messages we handle have smb_sucnt == 1 - ensure this
	   is so as a sanity check */
	if (state->setup_count != 1) {
		/*
		 * Need to have rc=0 for ioctl to get job id for OS/2.
		 *  Network printing will fail if function is not successful.
		 *  Similar function in reply.c will be used if protocol
		 *  is LANMAN1.0 instead of LM1.2X002.
		 *  Until DosPrintSetJobInfo with PRJINFO3 is supported,
		 *  outbuf doesn't have to be set(only job id is used).
		 */
		if ( (state->setup_count == 4) && (tran_call == TRANSACT2_IOCTL) &&
				(SVAL(inbuf,(smb_setup+4)) == LMCAT_SPL) &&
				(SVAL(inbuf,(smb_setup+6)) == LMFUNC_GETJOBID)) {
			DEBUG(2,("Got Trans2 DevIOctl jobid\n"));
		} else {
			DEBUG(2,("Invalid smb_sucnt in trans2 call(%u)\n",state->setup_count));
			DEBUG(2,("Transaction is %d\n",tran_call));
			TALLOC_FREE(state);
			END_PROFILE(SMBtrans2);
			return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
		}
	}

	if ((dscnt > state->total_data) || (pscnt > state->total_param))
		goto bad_param;

	if (state->total_data) {
		/* Can't use talloc here, the core routines do realloc on the
		 * params and data. */
		state->data = SMB_MALLOC(state->total_data);
		if (state->data == NULL) {
			DEBUG(0,("reply_trans2: data malloc fail for %u "
				 "bytes !\n", (unsigned int)state->total_data));
			TALLOC_FREE(state);
			END_PROFILE(SMBtrans2);
			return(ERROR_DOS(ERRDOS,ERRnomem));
		}
		if ((dsoff+dscnt < dsoff) || (dsoff+dscnt < dscnt))
			goto bad_param;
		if ((smb_base(inbuf)+dsoff+dscnt > inbuf + size) ||
		    (smb_base(inbuf)+dsoff+dscnt < smb_base(inbuf)))
			goto bad_param;

		memcpy(state->data,smb_base(inbuf)+dsoff,dscnt);
	}

	if (state->total_param) {
		/* Can't use talloc here, the core routines do realloc on the
		 * params and data. */
		state->param = SMB_MALLOC(state->total_param);
		if (state->param == NULL) {
			DEBUG(0,("reply_trans: param malloc fail for %u "
				 "bytes !\n", (unsigned int)state->total_param));
			SAFE_FREE(state->data);
			TALLOC_FREE(state);
			END_PROFILE(SMBtrans2);
			return(ERROR_DOS(ERRDOS,ERRnomem));
		} 
		if ((psoff+pscnt < psoff) || (psoff+pscnt < pscnt))
			goto bad_param;
		if ((smb_base(inbuf)+psoff+pscnt > inbuf + size) ||
		    (smb_base(inbuf)+psoff+pscnt < smb_base(inbuf)))
			goto bad_param;

		memcpy(state->param,smb_base(inbuf)+psoff,pscnt);
	}

	state->received_data  = dscnt;
	state->received_param = pscnt;

	if ((state->received_param == state->total_param) &&
	    (state->received_data == state->total_data)) {

		outsize = handle_trans2(conn, state, inbuf, outbuf,
					size, bufsize);
		SAFE_FREE(state->data);
		SAFE_FREE(state->param);
		TALLOC_FREE(state);
		END_PROFILE(SMBtrans2);
		return outsize;
	}

	DLIST_ADD(conn->pending_trans, state);

	/* We need to send an interim response then receive the rest
	   of the parameter/data bytes */
	outsize = set_message(outbuf,0,0,False);
	show_msg(outbuf);
	END_PROFILE(SMBtrans2);
	return outsize;

  bad_param:

	DEBUG(0,("reply_trans2: invalid trans parameters\n"));
	SAFE_FREE(state->data);
	SAFE_FREE(state->param);
	TALLOC_FREE(state);
	END_PROFILE(SMBtrans2);
	return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
}


/****************************************************************************
 Reply to a SMBtranss2
 ****************************************************************************/

int reply_transs2(connection_struct *conn,
		  char *inbuf,char *outbuf,int size,int bufsize)
{
	int outsize = 0;
	unsigned int pcnt,poff,dcnt,doff,pdisp,ddisp;
	struct trans_state *state;

	START_PROFILE(SMBtranss2);

	show_msg(inbuf);

	for (state = conn->pending_trans; state != NULL;
	     state = state->next) {
		if (state->mid == SVAL(inbuf,smb_mid)) {
			break;
		}
	}

	if ((state == NULL) || (state->cmd != SMBtrans2)) {
		END_PROFILE(SMBtranss2);
		return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
	}

	/* Revise state->total_param and state->total_data in case they have
	   changed downwards */

	if (SVAL(inbuf, smb_tpscnt) < state->total_param)
		state->total_param = SVAL(inbuf, smb_tpscnt);
	if (SVAL(inbuf, smb_tdscnt) < state->total_data)
		state->total_data = SVAL(inbuf, smb_tdscnt);

	pcnt = SVAL(inbuf, smb_spscnt);
	poff = SVAL(inbuf, smb_spsoff);
	pdisp = SVAL(inbuf, smb_spsdisp);

	dcnt = SVAL(inbuf, smb_sdscnt);
	doff = SVAL(inbuf, smb_sdsoff);
	ddisp = SVAL(inbuf, smb_sdsdisp);

	state->received_param += pcnt;
	state->received_data += dcnt;
		
	if ((state->received_data > state->total_data) ||
	    (state->received_param > state->total_param))
		goto bad_param;

	if (pcnt) {
		if (pdisp+pcnt > state->total_param)
			goto bad_param;
		if ((pdisp+pcnt < pdisp) || (pdisp+pcnt < pcnt))
			goto bad_param;
		if (pdisp > state->total_param)
			goto bad_param;
		if ((smb_base(inbuf) + poff + pcnt > inbuf + size) ||
		    (smb_base(inbuf) + poff + pcnt < smb_base(inbuf)))
			goto bad_param;
		if (state->param + pdisp < state->param)
			goto bad_param;

		memcpy(state->param+pdisp,smb_base(inbuf)+poff,
		       pcnt);
	}

	if (dcnt) {
		if (ddisp+dcnt > state->total_data)
			goto bad_param;
		if ((ddisp+dcnt < ddisp) || (ddisp+dcnt < dcnt))
			goto bad_param;
		if (ddisp > state->total_data)
			goto bad_param;
		if ((smb_base(inbuf) + doff + dcnt > inbuf + size) ||
		    (smb_base(inbuf) + doff + dcnt < smb_base(inbuf)))
			goto bad_param;
		if (state->data + ddisp < state->data)
			goto bad_param;

		memcpy(state->data+ddisp, smb_base(inbuf)+doff,
		       dcnt);      
	}

	if ((state->received_param < state->total_param) ||
	    (state->received_data < state->total_data)) {
		END_PROFILE(SMBtranss2);
		return -1;
	}

	/* construct_reply_common has done us the favor to pre-fill the
	 * command field with SMBtranss2 which is wrong :-)
	 */
	SCVAL(outbuf,smb_com,SMBtrans2);

	outsize = handle_trans2(conn, state, inbuf, outbuf, size, bufsize);

	DLIST_REMOVE(conn->pending_trans, state);
	SAFE_FREE(state->data);
	SAFE_FREE(state->param);
	TALLOC_FREE(state);

	if (outsize == 0) {
		END_PROFILE(SMBtranss2);
		return(ERROR_DOS(ERRSRV,ERRnosupport));
	}
	
	END_PROFILE(SMBtranss2);
	return(outsize);

  bad_param:

	DEBUG(0,("reply_transs2: invalid trans parameters\n"));
	DLIST_REMOVE(conn->pending_trans, state);
	SAFE_FREE(state->data);
	SAFE_FREE(state->param);
	TALLOC_FREE(state);
	END_PROFILE(SMBtranss2);
	return ERROR_NT(NT_STATUS_INVALID_PARAMETER);
}
