/* 
   Unix SMB/CIFS implementation.
   client file operations
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Jeremy Allison 2001-2002
   
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

/****************************************************************************
 Hard/Symlink a file (UNIX extensions).
 Creates new name (sym)linked to oldname.
****************************************************************************/

static BOOL cli_link_internal(struct cli_state *cli, const char *oldname, const char *newname, BOOL hard_link)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_SETPATHINFO;
	char param[sizeof(pstring)+6];
	pstring data;
	char *rparam=NULL, *rdata=NULL;
	char *p;
	size_t oldlen = 2*(strlen(oldname)+1);
	size_t newlen = 2*(strlen(newname)+1);

	memset(param, 0, sizeof(param));
	SSVAL(param,0,hard_link ? SMB_SET_FILE_UNIX_HLINK : SMB_SET_FILE_UNIX_LINK);
	p = &param[6];

	p += clistr_push(cli, p, newname, MIN(newlen, sizeof(param)-6), STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	p = data;
	p += clistr_push(cli, p, oldname, MIN(oldlen,sizeof(data)), STR_TERMINATE);
	data_len = PTR_DIFF(p, data);

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                          /* fid, flags */
		&setup, 1, 0,                   /* setup, length, max */
		param, param_len, 2,            /* param, length, max */
		(char *)&data,  data_len, cli->max_xmit /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/****************************************************************************
 Map standard UNIX permissions onto wire representations.
****************************************************************************/

uint32 unix_perms_to_wire(mode_t perms)
{
        unsigned int ret = 0;

        ret |= ((perms & S_IXOTH) ?  UNIX_X_OTH : 0);
        ret |= ((perms & S_IWOTH) ?  UNIX_W_OTH : 0);
        ret |= ((perms & S_IROTH) ?  UNIX_R_OTH : 0);
        ret |= ((perms & S_IXGRP) ?  UNIX_X_GRP : 0);
        ret |= ((perms & S_IWGRP) ?  UNIX_W_GRP : 0);
        ret |= ((perms & S_IRGRP) ?  UNIX_R_GRP : 0);
        ret |= ((perms & S_IXUSR) ?  UNIX_X_USR : 0);
        ret |= ((perms & S_IWUSR) ?  UNIX_W_USR : 0);
        ret |= ((perms & S_IRUSR) ?  UNIX_R_USR : 0);
#ifdef S_ISVTX
        ret |= ((perms & S_ISVTX) ?  UNIX_STICKY : 0);
#endif
#ifdef S_ISGID
        ret |= ((perms & S_ISGID) ?  UNIX_SET_GID : 0);
#endif
#ifdef S_ISUID
        ret |= ((perms & S_ISUID) ?  UNIX_SET_UID : 0);
#endif
        return ret;
}

/****************************************************************************
 Map wire permissions to standard UNIX.
****************************************************************************/

mode_t wire_perms_to_unix(uint32 perms)
{
        mode_t ret = (mode_t)0;

        ret |= ((perms & UNIX_X_OTH) ? S_IXOTH : 0);
        ret |= ((perms & UNIX_W_OTH) ? S_IWOTH : 0);
        ret |= ((perms & UNIX_R_OTH) ? S_IROTH : 0);
        ret |= ((perms & UNIX_X_GRP) ? S_IXGRP : 0);
        ret |= ((perms & UNIX_W_GRP) ? S_IWGRP : 0);
        ret |= ((perms & UNIX_R_GRP) ? S_IRGRP : 0);
        ret |= ((perms & UNIX_X_USR) ? S_IXUSR : 0);
        ret |= ((perms & UNIX_W_USR) ? S_IWUSR : 0);
        ret |= ((perms & UNIX_R_USR) ? S_IRUSR : 0);
#ifdef S_ISVTX
        ret |= ((perms & UNIX_STICKY) ? S_ISVTX : 0);
#endif
#ifdef S_ISGID
        ret |= ((perms & UNIX_SET_GID) ? S_ISGID : 0);
#endif
#ifdef S_ISUID
        ret |= ((perms & UNIX_SET_UID) ? S_ISUID : 0);
#endif
        return ret;
}

/****************************************************************************
 Return the file type from the wire filetype for UNIX extensions.
****************************************************************************/
                                                                                                                
static mode_t unix_filetype_from_wire(uint32 wire_type)
{
	switch (wire_type) {
		case UNIX_TYPE_FILE:
			return S_IFREG;
		case UNIX_TYPE_DIR:
			return S_IFDIR;
#ifdef S_IFLNK
		case UNIX_TYPE_SYMLINK:
			return S_IFLNK;
#endif
#ifdef S_IFCHR
		case UNIX_TYPE_CHARDEV:
			return S_IFCHR;
#endif
#ifdef S_IFBLK
		case UNIX_TYPE_BLKDEV:
			return S_IFBLK;
#endif
#ifdef S_IFIFO
		case UNIX_TYPE_FIFO:
			return S_IFIFO;
#endif
#ifdef S_IFSOCK
		case UNIX_TYPE_SOCKET:
			return S_IFSOCK;
#endif
		default:
			return (mode_t)0;
	}
}

/****************************************************************************
 Do a POSIX getfacl (UNIX extensions).
****************************************************************************/

BOOL cli_unix_getfacl(struct cli_state *cli, const char *name, size_t *prb_size, char **retbuf)
{
	unsigned int param_len = 0;
	unsigned int data_len = 0;
	uint16 setup = TRANSACT2_QPATHINFO;
	char param[sizeof(pstring)+6];
	char *rparam=NULL, *rdata=NULL;
	char *p;

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_QUERY_POSIX_ACL);
	p += 6;
	p += clistr_push(cli, p, name, sizeof(pstring)-6, STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                       /* fid, flags */
		&setup, 1, 0,                /* setup, length, max */
		param, param_len, 2,         /* param, length, max */
		NULL,  0, cli->max_xmit      /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	if (data_len < 6) {
		SAFE_FREE(rdata);
		SAFE_FREE(rparam);
		return False;
	}

	SAFE_FREE(rparam);
	*retbuf = rdata;
	*prb_size = (size_t)data_len;

	return True;
}

/****************************************************************************
 Stat a file (UNIX extensions).
****************************************************************************/

BOOL cli_unix_stat(struct cli_state *cli, const char *name, SMB_STRUCT_STAT *sbuf)
{
	unsigned int param_len = 0;
	unsigned int data_len = 0;
	uint16 setup = TRANSACT2_QPATHINFO;
	char param[sizeof(pstring)+6];
	char *rparam=NULL, *rdata=NULL;
	char *p;

	ZERO_STRUCTP(sbuf);

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_QUERY_FILE_UNIX_BASIC);
	p += 6;
	p += clistr_push(cli, p, name, sizeof(pstring)-6, STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                       /* fid, flags */
		&setup, 1, 0,                /* setup, length, max */
		param, param_len, 2,         /* param, length, max */
		NULL,  0, cli->max_xmit      /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	if (data_len < 96) {
		SAFE_FREE(rdata);
		SAFE_FREE(rparam);
		return False;
	}

	sbuf->st_size = IVAL2_TO_SMB_BIG_UINT(rdata,0);     /* total size, in bytes */
#ifdef HAVE_STAT_ST_BLOCKS
	sbuf->st_blocks = IVAL2_TO_SMB_BIG_UINT(rdata,8);   /* number of blocks allocated */
#ifdef(STAT_ST_BLOCKSIZE)
	sbuf->st_blocks /= STAT_ST_BLOCKSIZE;
#else
	/* assume 512 byte blocks */
	sbuf->st_blocks /= 512;
#endif
#endif
	sbuf->st_ctime = interpret_long_date(rdata + 16);    /* time of last change */
	sbuf->st_atime = interpret_long_date(rdata + 24);    /* time of last access */
	sbuf->st_mtime = interpret_long_date(rdata + 32);    /* time of last modification */
	sbuf->st_uid = (uid_t) IVAL(rdata,40);      /* user ID of owner */
	sbuf->st_gid = (gid_t) IVAL(rdata,48);      /* group ID of owner */
	sbuf->st_mode |= unix_filetype_from_wire(IVAL(rdata, 56));
#if defined(HAVE_MAKEDEV)
	{
		uint32 dev_major = IVAL(rdata,60);
		uint32 dev_minor = IVAL(rdata,68);
		sbuf->st_rdev = makedev(dev_major, dev_minor);
	}
#endif
	sbuf->st_ino = (SMB_INO_T)IVAL2_TO_SMB_BIG_UINT(rdata,76);      /* inode */
	sbuf->st_mode |= wire_perms_to_unix(IVAL(rdata,84));     /* protection */
	sbuf->st_nlink = IVAL(rdata,92);    /* number of hard links */

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/****************************************************************************
 Symlink a file (UNIX extensions).
****************************************************************************/

BOOL cli_unix_symlink(struct cli_state *cli, const char *oldname, const char *newname)
{
	return cli_link_internal(cli, oldname, newname, False);
}

/****************************************************************************
 Hard a file (UNIX extensions).
****************************************************************************/

BOOL cli_unix_hardlink(struct cli_state *cli, const char *oldname, const char *newname)
{
	return cli_link_internal(cli, oldname, newname, True);
}

/****************************************************************************
 Chmod or chown a file internal (UNIX extensions).
****************************************************************************/

static BOOL cli_unix_chmod_chown_internal(struct cli_state *cli, const char *fname, uint32 mode, uint32 uid, uint32 gid)
{
	unsigned int data_len = 0;
	unsigned int param_len = 0;
	uint16 setup = TRANSACT2_SETPATHINFO;
	char param[sizeof(pstring)+6];
	char data[100];
	char *rparam=NULL, *rdata=NULL;
	char *p;

	memset(param, 0, sizeof(param));
	memset(data, 0, sizeof(data));
	SSVAL(param,0,SMB_SET_FILE_UNIX_BASIC);
	p = &param[6];

	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	SIVAL(data,40,uid);
	SIVAL(data,48,gid);
	SIVAL(data,84,mode);

	data_len = 100;

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                          /* fid, flags */
		&setup, 1, 0,                   /* setup, length, max */
		param, param_len, 2,            /* param, length, max */
		(char *)&data,  data_len, cli->max_xmit /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/****************************************************************************
 chmod a file (UNIX extensions).
****************************************************************************/

BOOL cli_unix_chmod(struct cli_state *cli, const char *fname, mode_t mode)
{
	return cli_unix_chmod_chown_internal(cli, fname, 
		unix_perms_to_wire(mode), SMB_UID_NO_CHANGE, SMB_GID_NO_CHANGE);
}

/****************************************************************************
 chown a file (UNIX extensions).
****************************************************************************/

BOOL cli_unix_chown(struct cli_state *cli, const char *fname, uid_t uid, gid_t gid)
{
	return cli_unix_chmod_chown_internal(cli, fname, SMB_MODE_NO_CHANGE, (uint32)uid, (uint32)gid);
}

/****************************************************************************
 Rename a file.
****************************************************************************/

BOOL cli_rename(struct cli_state *cli, const char *fname_src, const char *fname_dst)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,1, 0, True);

	SCVAL(cli->outbuf,smb_com,SMBmv);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,aSYSTEM | aHIDDEN | aDIR);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, fname_src, -1, STR_TERMINATE);
	*p++ = 4;
	p += clistr_push(cli, p, fname_dst, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	if (cli_is_error(cli))
		return False;

	return True;
}

/****************************************************************************
 NT Rename a file.
****************************************************************************/

BOOL cli_ntrename(struct cli_state *cli, const char *fname_src, const char *fname_dst)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf, 4, 0, True);

	SCVAL(cli->outbuf,smb_com,SMBntrename);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,aSYSTEM | aHIDDEN | aDIR);
	SSVAL(cli->outbuf,smb_vwv1, RENAME_FLAG_RENAME);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, fname_src, -1, STR_TERMINATE);
	*p++ = 4;
	p += clistr_push(cli, p, fname_dst, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	if (cli_is_error(cli))
		return False;

	return True;
}

/****************************************************************************
 NT hardlink a file.
****************************************************************************/

BOOL cli_nt_hardlink(struct cli_state *cli, const char *fname_src, const char *fname_dst)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf, 4, 0, True);

	SCVAL(cli->outbuf,smb_com,SMBntrename);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,aSYSTEM | aHIDDEN | aDIR);
	SSVAL(cli->outbuf,smb_vwv1, RENAME_FLAG_HARD_LINK);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, fname_src, -1, STR_TERMINATE);
	*p++ = 4;
	p += clistr_push(cli, p, fname_dst, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli))
		return False;

	if (cli_is_error(cli))
		return False;

	return True;
}

/****************************************************************************
 Delete a file.
****************************************************************************/

BOOL cli_unlink(struct cli_state *cli, const char *fname)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,1, 0,True);

	SCVAL(cli->outbuf,smb_com,SMBunlink);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,aSYSTEM | aHIDDEN);
  
	p = smb_buf(cli->outbuf);
	*p++ = 4;      
	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);
	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Create a directory.
****************************************************************************/

BOOL cli_mkdir(struct cli_state *cli, const char *dname)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,0, 0,True);

	SCVAL(cli->outbuf,smb_com,SMBmkdir);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	p = smb_buf(cli->outbuf);
	*p++ = 4;      
	p += clistr_push(cli, p, dname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Remove a directory.
****************************************************************************/

BOOL cli_rmdir(struct cli_state *cli, const char *dname)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,0, 0, True);

	SCVAL(cli->outbuf,smb_com,SMBrmdir);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	p = smb_buf(cli->outbuf);
	*p++ = 4;      
	p += clistr_push(cli, p, dname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Set or clear the delete on close flag.
****************************************************************************/

int cli_nt_delete_on_close(struct cli_state *cli, int fnum, BOOL flag)
{
	unsigned int data_len = 1;
	unsigned int param_len = 6;
	uint16 setup = TRANSACT2_SETFILEINFO;
	pstring param;
	unsigned char data;
	char *rparam=NULL, *rdata=NULL;

	memset(param, 0, param_len);
	SSVAL(param,0,fnum);
	SSVAL(param,2,SMB_SET_FILE_DISPOSITION_INFO);

	data = flag ? 1 : 0;

	if (!cli_send_trans(cli, SMBtrans2,
						NULL,                        /* name */
						-1, 0,                          /* fid, flags */
						&setup, 1, 0,                   /* setup, length, max */
						param, param_len, 2,            /* param, length, max */
						(char *)&data,  data_len, cli->max_xmit /* data, length, max */
						)) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
						&rparam, &param_len,
						&rdata, &data_len)) {
		return False;
	}

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/****************************************************************************
 Open a file - exposing the full horror of the NT API :-).
 Used in smbtorture.
****************************************************************************/

int cli_nt_create_full(struct cli_state *cli, const char *fname, 
		 uint32 CreatFlags, uint32 DesiredAccess,
		 uint32 FileAttributes, uint32 ShareAccess,
		 uint32 CreateDisposition, uint32 CreateOptions,
		 uint8 SecuityFlags)
{
	char *p;
	int len;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,24,0,True);

	SCVAL(cli->outbuf,smb_com,SMBntcreateX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,0xFF);
	if (cli->use_oplocks)
		CreatFlags |= (REQUEST_OPLOCK|REQUEST_BATCH_OPLOCK);
	
	SIVAL(cli->outbuf,smb_ntcreate_Flags, CreatFlags);
	SIVAL(cli->outbuf,smb_ntcreate_RootDirectoryFid, 0x0);
	SIVAL(cli->outbuf,smb_ntcreate_DesiredAccess, DesiredAccess);
	SIVAL(cli->outbuf,smb_ntcreate_FileAttributes, FileAttributes);
	SIVAL(cli->outbuf,smb_ntcreate_ShareAccess, ShareAccess);
	SIVAL(cli->outbuf,smb_ntcreate_CreateDisposition, CreateDisposition);
	SIVAL(cli->outbuf,smb_ntcreate_CreateOptions, CreateOptions);
	SIVAL(cli->outbuf,smb_ntcreate_ImpersonationLevel, 0x02);
	SCVAL(cli->outbuf,smb_ntcreate_SecurityFlags, SecuityFlags);

	p = smb_buf(cli->outbuf);
	/* this alignment and termination is critical for netapp filers. Don't change */
	p += clistr_align_out(cli, p, 0);
	len = clistr_push(cli, p, fname, -1, 0);
	p += len;
	SSVAL(cli->outbuf,smb_ntcreate_NameLength, len);
	/* sigh. this copes with broken netapp filer behaviour */
	p += clistr_push(cli, p, "", -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return -1;
	}

	if (cli_is_error(cli)) {
		return -1;
	}

	return SVAL(cli->inbuf,smb_vwv2 + 1);
}

/****************************************************************************
 Open a file.
****************************************************************************/

int cli_nt_create(struct cli_state *cli, const char *fname, uint32 DesiredAccess)
{
	return cli_nt_create_full(cli, fname, 0, DesiredAccess, 0,
				FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 0x0, 0x0);
}

/****************************************************************************
 Open a file
 WARNING: if you open with O_WRONLY then getattrE won't work!
****************************************************************************/

int cli_open(struct cli_state *cli, const char *fname, int flags, int share_mode)
{
	char *p;
	unsigned openfn=0;
	unsigned accessmode=0;

	if (flags & O_CREAT)
		openfn |= (1<<4);
	if (!(flags & O_EXCL)) {
		if (flags & O_TRUNC)
			openfn |= (1<<1);
		else
			openfn |= (1<<0);
	}

	accessmode = (share_mode<<4);

	if ((flags & O_ACCMODE) == O_RDWR) {
		accessmode |= 2;
	} else if ((flags & O_ACCMODE) == O_WRONLY) {
		accessmode |= 1;
	} 

#if defined(O_SYNC)
	if ((flags & O_SYNC) == O_SYNC) {
		accessmode |= (1<<14);
	}
#endif /* O_SYNC */

	if (share_mode == DENY_FCB) {
		accessmode = 0xFF;
	}

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,15,0,True);

	SCVAL(cli->outbuf,smb_com,SMBopenX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,0);  /* no additional info */
	SSVAL(cli->outbuf,smb_vwv3,accessmode);
	SSVAL(cli->outbuf,smb_vwv4,aSYSTEM | aHIDDEN);
	SSVAL(cli->outbuf,smb_vwv5,0);
	SSVAL(cli->outbuf,smb_vwv8,openfn);

	if (cli->use_oplocks) {
		/* if using oplocks then ask for a batch oplock via
                   core and extended methods */
		SCVAL(cli->outbuf,smb_flg, CVAL(cli->outbuf,smb_flg)|
			FLAG_REQUEST_OPLOCK|FLAG_REQUEST_BATCH_OPLOCK);
		SSVAL(cli->outbuf,smb_vwv2,SVAL(cli->outbuf,smb_vwv2) | 6);
	}
  
	p = smb_buf(cli->outbuf);
	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return -1;
	}

	if (cli_is_error(cli)) {
		return -1;
	}

	return SVAL(cli->inbuf,smb_vwv2);
}

/****************************************************************************
 Close a file.
****************************************************************************/

BOOL cli_close(struct cli_state *cli, int fnum)
{
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,3,0,True);

	SCVAL(cli->outbuf,smb_com,SMBclose);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,fnum);
	SIVALS(cli->outbuf,smb_vwv1,-1);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	return !cli_is_error(cli);
}


/****************************************************************************
 send a lock with a specified locktype 
 this is used for testing LOCKING_ANDX_CANCEL_LOCK
****************************************************************************/

NTSTATUS cli_locktype(struct cli_state *cli, int fnum, 
		      uint32 offset, uint32 len, int timeout, unsigned char locktype)
{
	char *p;
	int saved_timeout = cli->timeout;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0', smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBlockingX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SCVAL(cli->outbuf,smb_vwv3,locktype);
	SIVALS(cli->outbuf, smb_vwv4, timeout);
	SSVAL(cli->outbuf,smb_vwv6,0);
	SSVAL(cli->outbuf,smb_vwv7,1);

	p = smb_buf(cli->outbuf);
	SSVAL(p, 0, cli->pid);
	SIVAL(p, 2, offset);
	SIVAL(p, 6, len);

	p += 10;

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);

	if (timeout != 0) {
		cli->timeout = (timeout == -1) ? 0x7FFFFFFF : (timeout + 2*1000);
	}

	if (!cli_receive_smb(cli)) {
		cli->timeout = saved_timeout;
		return NT_STATUS_UNSUCCESSFUL;
	}

	cli->timeout = saved_timeout;

	return cli_nt_error(cli);
}

/****************************************************************************
 Lock a file.
 note that timeout is in units of 2 milliseconds
****************************************************************************/

BOOL cli_lock(struct cli_state *cli, int fnum, 
	      uint32 offset, uint32 len, int timeout, enum brl_type lock_type)
{
	char *p;
	int saved_timeout = cli->timeout;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0', smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBlockingX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SCVAL(cli->outbuf,smb_vwv3,(lock_type == READ_LOCK? 1 : 0));
	SIVALS(cli->outbuf, smb_vwv4, timeout);
	SSVAL(cli->outbuf,smb_vwv6,0);
	SSVAL(cli->outbuf,smb_vwv7,1);

	p = smb_buf(cli->outbuf);
	SSVAL(p, 0, cli->pid);
	SIVAL(p, 2, offset);
	SIVAL(p, 6, len);

	p += 10;

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);

	if (timeout != 0) {
		cli->timeout = (timeout == -1) ? 0x7FFFFFFF : (timeout*2 + 5*1000);
	}

	if (!cli_receive_smb(cli)) {
		cli->timeout = saved_timeout;
		return False;
	}

	cli->timeout = saved_timeout;

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Unlock a file.
****************************************************************************/

BOOL cli_unlock(struct cli_state *cli, int fnum, uint32 offset, uint32 len)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBlockingX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SCVAL(cli->outbuf,smb_vwv3,0);
	SIVALS(cli->outbuf, smb_vwv4, 0);
	SSVAL(cli->outbuf,smb_vwv6,1);
	SSVAL(cli->outbuf,smb_vwv7,0);

	p = smb_buf(cli->outbuf);
	SSVAL(p, 0, cli->pid);
	SIVAL(p, 2, offset);
	SIVAL(p, 6, len);
	p += 10;
	cli_setup_bcc(cli, p);
	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Lock a file with 64 bit offsets.
****************************************************************************/

BOOL cli_lock64(struct cli_state *cli, int fnum, 
		SMB_BIG_UINT offset, SMB_BIG_UINT len, int timeout, enum brl_type lock_type)
{
	char *p;
        int saved_timeout = cli->timeout;
	int ltype;

	if (! (cli->capabilities & CAP_LARGE_FILES)) {
		return cli_lock(cli, fnum, offset, len, timeout, lock_type);
	}

	ltype = (lock_type == READ_LOCK? 1 : 0);
	ltype |= LOCKING_ANDX_LARGE_FILES;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0', smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBlockingX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SCVAL(cli->outbuf,smb_vwv3,ltype);
	SIVALS(cli->outbuf, smb_vwv4, timeout);
	SSVAL(cli->outbuf,smb_vwv6,0);
	SSVAL(cli->outbuf,smb_vwv7,1);

	p = smb_buf(cli->outbuf);
	SIVAL(p, 0, cli->pid);
	SOFF_T_R(p, 4, offset);
	SOFF_T_R(p, 12, len);
	p += 20;

	cli_setup_bcc(cli, p);
	cli_send_smb(cli);

	if (timeout != 0) {
		cli->timeout = (timeout == -1) ? 0x7FFFFFFF : (timeout + 5*1000);
	}

	if (!cli_receive_smb(cli)) {
                cli->timeout = saved_timeout;
		return False;
	}

	cli->timeout = saved_timeout;

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Unlock a file with 64 bit offsets.
****************************************************************************/

BOOL cli_unlock64(struct cli_state *cli, int fnum, SMB_BIG_UINT offset, SMB_BIG_UINT len)
{
	char *p;

	if (! (cli->capabilities & CAP_LARGE_FILES)) {
		return cli_unlock(cli, fnum, offset, len);
	}

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBlockingX);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SCVAL(cli->outbuf,smb_vwv0,0xFF);
	SSVAL(cli->outbuf,smb_vwv2,fnum);
	SCVAL(cli->outbuf,smb_vwv3,LOCKING_ANDX_LARGE_FILES);
	SIVALS(cli->outbuf, smb_vwv4, 0);
	SSVAL(cli->outbuf,smb_vwv6,1);
	SSVAL(cli->outbuf,smb_vwv7,0);

	p = smb_buf(cli->outbuf);
	SIVAL(p, 0, cli->pid);
	SOFF_T_R(p, 4, offset);
	SOFF_T_R(p, 12, len);
	p += 20;
	cli_setup_bcc(cli, p);
	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Get/unlock a POSIX lock on a file - internal function.
****************************************************************************/

static BOOL cli_posix_lock_internal(struct cli_state *cli, int fnum, 
		SMB_BIG_UINT offset, SMB_BIG_UINT len, BOOL wait_lock, enum brl_type lock_type)
{
	unsigned int param_len = 4;
	unsigned int data_len = POSIX_LOCK_DATA_SIZE;
	uint16 setup = TRANSACT2_SETFILEINFO;
	char param[4];
	unsigned char data[POSIX_LOCK_DATA_SIZE];
	char *rparam=NULL, *rdata=NULL;
	int saved_timeout = cli->timeout;

	SSVAL(param,0,fnum);
	SSVAL(param,2,SMB_SET_POSIX_LOCK);

	switch (lock_type) {
		case READ_LOCK:
			SSVAL(data, POSIX_LOCK_TYPE_OFFSET, POSIX_LOCK_TYPE_READ);
			break;
		case WRITE_LOCK:
			SSVAL(data, POSIX_LOCK_TYPE_OFFSET, POSIX_LOCK_TYPE_WRITE);
			break;
		case UNLOCK_LOCK:
			SSVAL(data, POSIX_LOCK_TYPE_OFFSET, POSIX_LOCK_TYPE_UNLOCK);
			break;
		default:
			return False;
	}

	if (wait_lock) {
		SSVAL(data, POSIX_LOCK_FLAGS_OFFSET, POSIX_LOCK_FLAG_WAIT);
		cli->timeout = 0x7FFFFFFF;
	} else {
		SSVAL(data, POSIX_LOCK_FLAGS_OFFSET, POSIX_LOCK_FLAG_NOWAIT);
	}

	SIVAL(data, POSIX_LOCK_PID_OFFSET, cli->pid);
	SOFF_T(data, POSIX_LOCK_START_OFFSET, offset);
	SOFF_T(data, POSIX_LOCK_LEN_OFFSET, len);

	if (!cli_send_trans(cli, SMBtrans2,
				NULL,                        /* name */
				-1, 0,                          /* fid, flags */
				&setup, 1, 0,                   /* setup, length, max */
				param, param_len, 2,            /* param, length, max */
				(char *)&data,  data_len, cli->max_xmit /* data, length, max */
				)) {
		cli->timeout = saved_timeout;
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
				&rparam, &param_len,
				&rdata, &data_len)) {
		cli->timeout = saved_timeout;
		SAFE_FREE(rdata);
		SAFE_FREE(rparam);
		return False;
	}

	cli->timeout = saved_timeout;

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/****************************************************************************
 POSIX Lock a file.
****************************************************************************/

BOOL cli_posix_lock(struct cli_state *cli, int fnum,
			SMB_BIG_UINT offset, SMB_BIG_UINT len,
			BOOL wait_lock, enum brl_type lock_type)
{
	if (lock_type != READ_LOCK && lock_type != WRITE_LOCK) {
		return False;
	}
	return cli_posix_lock_internal(cli, fnum, offset, len, wait_lock, lock_type);
}

/****************************************************************************
 POSIX Unlock a file.
****************************************************************************/

BOOL cli_posix_unlock(struct cli_state *cli, int fnum, SMB_BIG_UINT offset, SMB_BIG_UINT len)
{
	return cli_posix_lock_internal(cli, fnum, offset, len, False, UNLOCK_LOCK);
}

/****************************************************************************
 POSIX Get any lock covering a file.
****************************************************************************/

BOOL cli_posix_getlock(struct cli_state *cli, int fnum, SMB_BIG_UINT *poffset, SMB_BIG_UINT *plen)
{
	return True;
}

/****************************************************************************
 Do a SMBgetattrE call.
****************************************************************************/

BOOL cli_getattrE(struct cli_state *cli, int fd, 
		  uint16 *attr, SMB_OFF_T *size, 
		  time_t *c_time, time_t *a_time, time_t *m_time)
{
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,1,0,True);

	SCVAL(cli->outbuf,smb_com,SMBgetattrE);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,fd);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}
	
	if (cli_is_error(cli)) {
		return False;
	}

	if (size) {
		*size = IVAL(cli->inbuf, smb_vwv6);
	}

	if (attr) {
		*attr = SVAL(cli->inbuf,smb_vwv10);
	}

	if (c_time) {
		*c_time = cli_make_unix_date2(cli, cli->inbuf+smb_vwv0);
	}

	if (a_time) {
		*a_time = cli_make_unix_date2(cli, cli->inbuf+smb_vwv2);
	}

	if (m_time) {
		*m_time = cli_make_unix_date2(cli, cli->inbuf+smb_vwv4);
	}

	return True;
}

/****************************************************************************
 Do a SMBgetatr call
****************************************************************************/

BOOL cli_getatr(struct cli_state *cli, const char *fname, 
		uint16 *attr, SMB_OFF_T *size, time_t *t)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,0,0,True);

	SCVAL(cli->outbuf,smb_com,SMBgetatr);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}
	
	if (cli_is_error(cli)) {
		return False;
	}

	if (size) {
		*size = IVAL(cli->inbuf, smb_vwv3);
	}

	if (t) {
		*t = cli_make_unix_date3(cli, cli->inbuf+smb_vwv1);
	}

	if (attr) {
		*attr = SVAL(cli->inbuf,smb_vwv0);
	}


	return True;
}

/****************************************************************************
 Do a SMBsetattrE call.
****************************************************************************/

BOOL cli_setattrE(struct cli_state *cli, int fd,
		  time_t c_time, time_t a_time, time_t m_time)

{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,7,0,True);

	SCVAL(cli->outbuf,smb_com,SMBsetattrE);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0, fd);
	cli_put_dos_date2(cli, cli->outbuf,smb_vwv1, c_time);
	cli_put_dos_date2(cli, cli->outbuf,smb_vwv3, a_time);
	cli_put_dos_date2(cli, cli->outbuf,smb_vwv5, m_time);

	p = smb_buf(cli->outbuf);
	*p++ = 4;

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}
	
	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Do a SMBsetatr call.
****************************************************************************/

BOOL cli_setatr(struct cli_state *cli, const char *fname, uint16 attr, time_t t)
{
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,8,0,True);

	SCVAL(cli->outbuf,smb_com,SMBsetatr);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0, attr);
	cli_put_dos_date3(cli, cli->outbuf,smb_vwv1, t);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, fname, -1, STR_TERMINATE);
	*p++ = 4;

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}
	
	if (cli_is_error(cli)) {
		return False;
	}

	return True;
}

/****************************************************************************
 Check for existance of a dir.
****************************************************************************/
BOOL cli_chkpath(struct cli_state *cli, const char *path)
{
	pstring path2;
	char *p;
	
	pstrcpy(path2,path);
	trim_char(path2,'\0','\\');
	if (!*path2)
		*path2 = '\\';
	
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,0,0,True);
	SCVAL(cli->outbuf,smb_com,SMBchkpth);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);
	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, path2, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	if (cli_is_error(cli)) return False;

	return True;
}

/****************************************************************************
 Query disk space.
****************************************************************************/

BOOL cli_dskattr(struct cli_state *cli, int *bsize, int *total, int *avail)
{
	memset(cli->outbuf,'\0',smb_size);
	set_message(cli->outbuf,0,0,True);
	SCVAL(cli->outbuf,smb_com,SMBdskattr);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return False;
	}

	*bsize = SVAL(cli->inbuf,smb_vwv1)*SVAL(cli->inbuf,smb_vwv2);
	*total = SVAL(cli->inbuf,smb_vwv0);
	*avail = SVAL(cli->inbuf,smb_vwv3);
	
	return True;
}

/****************************************************************************
 Create and open a temporary file.
****************************************************************************/

int cli_ctemp(struct cli_state *cli, const char *path, char **tmp_path)
{
	int len;
	char *p;

	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf,3,0,True);

	SCVAL(cli->outbuf,smb_com,SMBctemp);
	SSVAL(cli->outbuf,smb_tid,cli->cnum);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf,smb_vwv0,0);
	SIVALS(cli->outbuf,smb_vwv1,-1);

	p = smb_buf(cli->outbuf);
	*p++ = 4;
	p += clistr_push(cli, p, path, -1, STR_TERMINATE);

	cli_setup_bcc(cli, p);

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return -1;
	}

	if (cli_is_error(cli)) {
		return -1;
	}

	/* despite the spec, the result has a -1, followed by
	   length, followed by name */
	p = smb_buf(cli->inbuf);
	p += 4;
	len = smb_buflen(cli->inbuf) - 4;
	if (len <= 0) return -1;

	if (tmp_path) {
		pstring path2;
		clistr_pull(cli, path2, p, 
			    sizeof(path2), len, STR_ASCII);
		*tmp_path = SMB_STRDUP(path2);
	}

	return SVAL(cli->inbuf,smb_vwv0);
}


/* 
   send a raw ioctl - used by the torture code
*/
NTSTATUS cli_raw_ioctl(struct cli_state *cli, int fnum, uint32 code, DATA_BLOB *blob)
{
	memset(cli->outbuf,'\0',smb_size);
	memset(cli->inbuf,'\0',smb_size);

	set_message(cli->outbuf, 3, 0, True);
	SCVAL(cli->outbuf,smb_com,SMBioctl);
	cli_setup_packet(cli);

	SSVAL(cli->outbuf, smb_vwv0, fnum);
	SSVAL(cli->outbuf, smb_vwv1, code>>16);
	SSVAL(cli->outbuf, smb_vwv2, (code&0xFFFF));

	cli_send_smb(cli);
	if (!cli_receive_smb(cli)) {
		return NT_STATUS_UNEXPECTED_NETWORK_ERROR;
	}

	if (cli_is_error(cli)) {
		return cli_nt_error(cli);
	}

	*blob = data_blob(NULL, 0);

	return NT_STATUS_OK;
}

/*********************************************************
 Set an extended attribute utility fn.
*********************************************************/

static BOOL cli_set_ea(struct cli_state *cli, uint16 setup, char *param, unsigned int param_len,
			const char *ea_name, const char *ea_val, size_t ea_len)
{	
	unsigned int data_len = 0;
	char *data = NULL;
	char *rparam=NULL, *rdata=NULL;
	char *p;
	size_t ea_namelen = strlen(ea_name);

	if (ea_namelen == 0 && ea_len == 0) {
		data_len = 4;
		data = SMB_MALLOC(data_len);
		if (!data) {
			return False;
		}
		p = data;
		SIVAL(p,0,data_len);
	} else {
		data_len = 4 + 4 + ea_namelen + 1 + ea_len;
		data = SMB_MALLOC(data_len);
		if (!data) {
			return False;
		}
		p = data;
		SIVAL(p,0,data_len);
		p += 4;
		SCVAL(p, 0, 0); /* EA flags. */
		SCVAL(p, 1, ea_namelen);
		SSVAL(p, 2, ea_len);
		memcpy(p+4, ea_name, ea_namelen+1); /* Copy in the name. */
		memcpy(p+4+ea_namelen+1, ea_val, ea_len);
	}

	if (!cli_send_trans(cli, SMBtrans2,
		NULL,                        /* name */
		-1, 0,                          /* fid, flags */
		&setup, 1, 0,                   /* setup, length, max */
		param, param_len, 2,            /* param, length, max */
		data,  data_len, cli->max_xmit /* data, length, max */
		)) {
			return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
		&rparam, &param_len,
		&rdata, &data_len)) {
			return False;
	}

	SAFE_FREE(data);
	SAFE_FREE(rdata);
	SAFE_FREE(rparam);

	return True;
}

/*********************************************************
 Set an extended attribute on a pathname.
*********************************************************/

BOOL cli_set_ea_path(struct cli_state *cli, const char *path, const char *ea_name, const char *ea_val, size_t ea_len)
{
	uint16 setup = TRANSACT2_SETPATHINFO;
	unsigned int param_len = 0;
	char param[sizeof(pstring)+6];
	size_t srclen = 2*(strlen(path)+1);
	char *p;

	memset(param, 0, sizeof(param));
	SSVAL(param,0,SMB_INFO_SET_EA);
	p = &param[6];

	p += clistr_push(cli, p, path, MIN(srclen, sizeof(param)-6), STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	return cli_set_ea(cli, setup, param, param_len, ea_name, ea_val, ea_len);
}

/*********************************************************
 Set an extended attribute on an fnum.
*********************************************************/

BOOL cli_set_ea_fnum(struct cli_state *cli, int fnum, const char *ea_name, const char *ea_val, size_t ea_len)
{
	char param[6];
	uint16 setup = TRANSACT2_SETFILEINFO;

	memset(param, 0, 6);
	SSVAL(param,0,fnum);
	SSVAL(param,2,SMB_INFO_SET_EA);

	return cli_set_ea(cli, setup, param, 6, ea_name, ea_val, ea_len);
}

/*********************************************************
 Get an extended attribute list tility fn.
*********************************************************/

static BOOL cli_get_ea_list(struct cli_state *cli,
		uint16 setup, char *param, unsigned int param_len,
		TALLOC_CTX *ctx,
		size_t *pnum_eas,
		struct ea_struct **pea_list)
{
	unsigned int data_len = 0;
	unsigned int rparam_len, rdata_len;
	char *rparam=NULL, *rdata=NULL;
	char *p;
	size_t ea_size;
	size_t num_eas;
	BOOL ret = False;
	struct ea_struct *ea_list;

	*pnum_eas = 0;
	if (pea_list) {
	 	*pea_list = NULL;
	}

	if (!cli_send_trans(cli, SMBtrans2,
			NULL,           /* Name */
			-1, 0,          /* fid, flags */
			&setup, 1, 0,   /* setup, length, max */
			param, param_len, 10, /* param, length, max */
			NULL, data_len, cli->max_xmit /* data, length, max */
				)) {
		return False;
	}

	if (!cli_receive_trans(cli, SMBtrans2,
			&rparam, &rparam_len,
			&rdata, &rdata_len)) {
		return False;
	}

	if (!rdata || rdata_len < 4) {
		goto out;
	}

	ea_size = (size_t)IVAL(rdata,0);
	if (ea_size > rdata_len) {
		goto out;
	}

	if (ea_size == 0) {
		/* No EA's present. */
		ret = True;
		goto out;
	}

	p = rdata + 4;
	ea_size -= 4;

	/* Validate the EA list and count it. */
	for (num_eas = 0; ea_size >= 4; num_eas++) {
		unsigned int ea_namelen = CVAL(p,1);
		unsigned int ea_valuelen = SVAL(p,2);
		if (ea_namelen == 0) {
			goto out;
		}
		if (4 + ea_namelen + 1 + ea_valuelen > ea_size) {
			goto out;
		}
		ea_size -= 4 + ea_namelen + 1 + ea_valuelen;
		p += 4 + ea_namelen + 1 + ea_valuelen;
	}

	if (num_eas == 0) {
		ret = True;
		goto out;
	}

	*pnum_eas = num_eas;
	if (!pea_list) {
		/* Caller only wants number of EA's. */
		ret = True;
		goto out;
	}

	ea_list = TALLOC_ARRAY(ctx, struct ea_struct, num_eas);
	if (!ea_list) {
		goto out;
	}

	ea_size = (size_t)IVAL(rdata,0);
	p = rdata + 4;

	for (num_eas = 0; num_eas < *pnum_eas; num_eas++ ) {
		struct ea_struct *ea = &ea_list[num_eas];
		fstring unix_ea_name;
		unsigned int ea_namelen = CVAL(p,1);
		unsigned int ea_valuelen = SVAL(p,2);

		ea->flags = CVAL(p,0);
		unix_ea_name[0] = '\0';
		pull_ascii_fstring(unix_ea_name, p + 4);
		ea->name = talloc_strdup(ctx, unix_ea_name);
		/* Ensure the value is null terminated (in case it's a string). */
		ea->value = data_blob_talloc(ctx, NULL, ea_valuelen + 1);
		if (!ea->value.data) {
			goto out;
		}
		if (ea_valuelen) {
			memcpy(ea->value.data, p+4+ea_namelen+1, ea_valuelen);
		}
		ea->value.data[ea_valuelen] = 0;
		ea->value.length--;
		p += 4 + ea_namelen + 1 + ea_valuelen;
	}

	*pea_list = ea_list;
	ret = True;

 out :

	SAFE_FREE(rdata);
	SAFE_FREE(rparam);
	return ret;
}

/*********************************************************
 Get an extended attribute list from a pathname.
*********************************************************/

BOOL cli_get_ea_list_path(struct cli_state *cli, const char *path,
		TALLOC_CTX *ctx,
		size_t *pnum_eas,
		struct ea_struct **pea_list)
{
	uint16 setup = TRANSACT2_QPATHINFO;
	unsigned int param_len = 0;
	char param[sizeof(pstring)+6];
	char *p;

	p = param;
	memset(p, 0, 6);
	SSVAL(p, 0, SMB_INFO_QUERY_ALL_EAS);
	p += 6;
	p += clistr_push(cli, p, path, sizeof(pstring)-6, STR_TERMINATE);
	param_len = PTR_DIFF(p, param);

	return cli_get_ea_list(cli, setup, param, param_len, ctx, pnum_eas, pea_list);
}

/*********************************************************
 Get an extended attribute list from an fnum.
*********************************************************/

BOOL cli_get_ea_list_fnum(struct cli_state *cli, int fnum,
		TALLOC_CTX *ctx,
		size_t *pnum_eas,
		struct ea_struct **pea_list)
{
	uint16 setup = TRANSACT2_QFILEINFO;
	char param[6];

	memset(param, 0, 6);
	SSVAL(param,0,fnum);
	SSVAL(param,2,SMB_INFO_SET_EA);

	return cli_get_ea_list(cli, setup, param, 6, ctx, pnum_eas, pea_list);
}
