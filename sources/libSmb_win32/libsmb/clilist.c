/* 
   Unix SMB/CIFS implementation.
   client directory list routines
   Copyright (C) Andrew Tridgell 1994-1998
   
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

extern file_info def_finfo;

/****************************************************************************
 Interpret a long filename structure - this is mostly guesses at the moment.
 The length of the structure is returned
 The structure of a long filename depends on the info level. 260 is used
 by NT and 2 is used by OS/2
****************************************************************************/

static size_t interpret_long_filename(struct cli_state *cli, int level,char *p,file_info *finfo,
					uint32 *p_resume_key, DATA_BLOB *p_last_name_raw, uint32 *p_last_name_raw_len)
{
	file_info finfo2;
	int len;
	char *base = p;

	if (!finfo) {
		finfo = &finfo2;
	}

	if (p_resume_key) {
		*p_resume_key = 0;
	}
	memcpy(finfo,&def_finfo,sizeof(*finfo));

	switch (level) {
		case 1: /* OS/2 understands this */
			/* these dates are converted to GMT by
                           make_unix_date */
			finfo->ctime = cli_make_unix_date2(cli, p+4);
			finfo->atime = cli_make_unix_date2(cli, p+8);
			finfo->mtime = cli_make_unix_date2(cli, p+12);
			finfo->size = IVAL(p,16);
			finfo->mode = CVAL(p,24);
			len = CVAL(p, 26);
			p += 27;
			p += clistr_align_in(cli, p, 0);
			/* the len+2 below looks strange but it is
			   important to cope with the differences
			   between win2000 and win9x for this call
			   (tridge) */
			p += clistr_pull(cli, finfo->name, p,
					 sizeof(finfo->name),
					 len+2, 
					 STR_TERMINATE);
			return PTR_DIFF(p, base);

		case 2: /* this is what OS/2 uses mostly */
			/* these dates are converted to GMT by
                           make_unix_date */
			finfo->ctime = cli_make_unix_date2(cli, p+4);
			finfo->atime = cli_make_unix_date2(cli, p+8);
			finfo->mtime = cli_make_unix_date2(cli, p+12);
			finfo->size = IVAL(p,16);
			finfo->mode = CVAL(p,24);
			len = CVAL(p, 30);
			p += 31;
			/* check for unisys! */
			p += clistr_pull(cli, finfo->name, p,
					 sizeof(finfo->name),
					 len, 
					 STR_NOALIGN);
			return PTR_DIFF(p, base) + 1;
			
		case 260: /* NT uses this, but also accepts 2 */
		{
			size_t namelen, slen;
			p += 4; /* next entry offset */

			if (p_resume_key) {
				*p_resume_key = IVAL(p,0);
			}
			p += 4; /* fileindex */
				
			/* Offset zero is "create time", not "change time". */
			p += 8;
			finfo->atime = interpret_long_date(p);
			p += 8;
			finfo->mtime = interpret_long_date(p);
			p += 8;
			finfo->ctime = interpret_long_date(p);
			p += 8;
			finfo->size = IVAL2_TO_SMB_BIG_UINT(p,0);
			p += 8;
			p += 8; /* alloc size */
			finfo->mode = CVAL(p,0);
			p += 4;
			namelen = IVAL(p,0);
			p += 4;
			p += 4; /* EA size */
			slen = SVAL(p, 0);
			p += 2; 
			{
				/* stupid NT bugs. grr */
				int flags = 0;
				if (p[1] == 0 && namelen > 1) flags |= STR_UNICODE;
				clistr_pull(cli, finfo->short_name, p,
					    sizeof(finfo->short_name),
					    slen, flags);
			}
			p += 24; /* short name? */	  
			clistr_pull(cli, finfo->name, p,
				    sizeof(finfo->name),
				    namelen, 0);

			/* To be robust in the face of unicode conversion failures
			   we need to copy the raw bytes of the last name seen here.
			   Namelen doesn't include the terminating unicode null, so
			   copy it here. */

			if (p_last_name_raw && p_last_name_raw_len) {
				if (namelen + 2 > p_last_name_raw->length) {
					memset(p_last_name_raw->data, '\0', sizeof(p_last_name_raw->length));
					*p_last_name_raw_len = 0;
				} else {
					memcpy(p_last_name_raw->data, p, namelen);
					SSVAL(p_last_name_raw->data, namelen, 0);
					*p_last_name_raw_len = namelen + 2;
				}
			}
			return (size_t)IVAL(base, 0);
		}
	}
	
	DEBUG(1,("Unknown long filename format %d\n",level));
	return (size_t)IVAL(base,0);
}

/****************************************************************************
 Do a directory listing, calling fn on each file found.
****************************************************************************/

int cli_list_new(struct cli_state *cli,const char *Mask,uint16 attribute, 
		 void (*fn)(const char *, file_info *, const char *, void *), void *state)
{
#if 1
	int max_matches = 1366; /* Match W2k - was 512. */
#else
	int max_matches = 512;
#endif
	int info_level;
	char *p, *p2;
	pstring mask;
	file_info finfo;
	int i;
	char *dirlist = NULL;
	int dirlist_len = 0;
	int total_received = -1;
	BOOL First = True;
	int ff_searchcount=0;
	int ff_eos=0;
	int ff_dir_handle=0;
	int loop_count = 0;
	char *rparam=NULL, *rdata=NULL;
	unsigned int param_len, data_len;	
	uint16 setup;
	pstring param;
	const char *mnt;
	uint32 resume_key = 0;
	uint32 last_name_raw_len = 0;
	DATA_BLOB last_name_raw = data_blob(NULL, 2*sizeof(pstring));

	/* NT uses 260, OS/2 uses 2. Both accept 1. */
	info_level = (cli->capabilities&CAP_NT_SMBS)?260:1;
	
	/* when getting a directory listing from a 2k dfs root share, 
	   we have to include the full path (\server\share\mask) here */
	   
	if ( cli->dfsroot )
		pstr_sprintf( mask, "\\%s\\%s\\%s", cli->desthost, cli->share, Mask );
	else
		pstrcpy(mask,Mask);
	
	while (ff_eos == 0) {
		loop_count++;
		if (loop_count > 200) {
			DEBUG(0,("Error: Looping in FIND_NEXT??\n"));
			break;
		}

		if (First) {
			setup = TRANSACT2_FINDFIRST;
			SSVAL(param,0,attribute); /* attribute */
			SSVAL(param,2,max_matches); /* max count */
			SSVAL(param,4,(FLAG_TRANS2_FIND_REQUIRE_RESUME|FLAG_TRANS2_FIND_CLOSE_IF_END));	/* resume required + close on end */
			SSVAL(param,6,info_level); 
			SIVAL(param,8,0);
			p = param+12;
			p += clistr_push(cli, param+12, mask, sizeof(param)-12, 
					 STR_TERMINATE);
		} else {
			setup = TRANSACT2_FINDNEXT;
			SSVAL(param,0,ff_dir_handle);
			SSVAL(param,2,max_matches); /* max count */
			SSVAL(param,4,info_level); 
			/* For W2K servers serving out FAT filesystems we *must* set the
			   resume key. If it's not FAT then it's returned as zero. */
			SIVAL(param,6,resume_key); /* ff_resume_key */
			/* NB. *DON'T* use continue here. If you do it seems that W2K and bretheren
			   can miss filenames. Use last filename continue instead. JRA */
			SSVAL(param,10,(FLAG_TRANS2_FIND_REQUIRE_RESUME|FLAG_TRANS2_FIND_CLOSE_IF_END));	/* resume required + close on end */
			p = param+12;
			if (last_name_raw_len && (last_name_raw_len < (sizeof(param)-12))) {
				memcpy(p, last_name_raw.data, last_name_raw_len);
				p += last_name_raw_len;
			} else {
				p += clistr_push(cli, param+12, mask, sizeof(param)-12, STR_TERMINATE);
			}
		}

		param_len = PTR_DIFF(p, param);

		if (!cli_send_trans(cli, SMBtrans2, 
				    NULL,                   /* Name */
				    -1, 0,                  /* fid, flags */
				    &setup, 1, 0,           /* setup, length, max */
				    param, param_len, 10,   /* param, length, max */
				    NULL, 0, 
#if 0
				    /* w2k value. */
				    MIN(16384,cli->max_xmit) /* data, length, max. */
#else
				    cli->max_xmit	    /* data, length, max. */
#endif
				    )) {
			break;
		}

		if (!cli_receive_trans(cli, SMBtrans2, 
				       &rparam, &param_len,
				       &rdata, &data_len) &&
                    cli_is_dos_error(cli)) {
			/* we need to work around a Win95 bug - sometimes
			   it gives ERRSRV/ERRerror temprarily */
			uint8 eclass;
			uint32 ecode;

			SAFE_FREE(rdata);
			SAFE_FREE(rparam);

			cli_dos_error(cli, &eclass, &ecode);
			if (eclass != ERRSRV || ecode != ERRerror)
				break;
			smb_msleep(100);
			continue;
		}

                if (cli_is_error(cli) || !rdata || !rparam) {
			SAFE_FREE(rdata);
			SAFE_FREE(rparam);
			break;
		}

		if (total_received == -1)
			total_received = 0;

		/* parse out some important return info */
		p = rparam;
		if (First) {
			ff_dir_handle = SVAL(p,0);
			ff_searchcount = SVAL(p,2);
			ff_eos = SVAL(p,4);
		} else {
			ff_searchcount = SVAL(p,0);
			ff_eos = SVAL(p,2);
		}

		if (ff_searchcount == 0) {
			SAFE_FREE(rdata);
			SAFE_FREE(rparam);
			break;
		}

		/* point to the data bytes */
		p = rdata;

		/* we might need the lastname for continuations */
		for (p2=p,i=0;i<ff_searchcount;i++) {
			if ((info_level == 260) && (i == ff_searchcount-1)) {
				/* Last entry - fixup the last offset length. */
				SIVAL(p2,0,PTR_DIFF((rdata + data_len),p2));
			}
			p2 += interpret_long_filename(cli,info_level,p2,&finfo,
							&resume_key,&last_name_raw,&last_name_raw_len);

			if (!First && *mask && strcsequal(finfo.name, mask)) {
				DEBUG(0,("Error: Looping in FIND_NEXT as name %s has already been seen?\n",
					finfo.name));
				ff_eos = 1;
				break;
			}
		}

		if (ff_searchcount > 0) {
			pstrcpy(mask, finfo.name);
		} else {
			pstrcpy(mask,"");
		}

		/* grab the data for later use */
		/* and add them to the dirlist pool */
		dirlist = SMB_REALLOC(dirlist,dirlist_len + data_len);

		if (!dirlist) {
			DEBUG(0,("cli_list_new: Failed to expand dirlist\n"));
			SAFE_FREE(rdata);
			SAFE_FREE(rparam);
			break;
		}

		memcpy(dirlist+dirlist_len,p,data_len);
		dirlist_len += data_len;

		total_received += ff_searchcount;

		SAFE_FREE(rdata);
		SAFE_FREE(rparam);

		DEBUG(3,("received %d entries (eos=%d)\n",
			 ff_searchcount,ff_eos));

		if (ff_searchcount > 0)
			loop_count = 0;

		First = False;
	}

	mnt = cli_cm_get_mntpoint( cli );

	for (p=dirlist,i=0;i<total_received;i++) {
		p += interpret_long_filename(cli,info_level,p,&finfo,NULL,NULL,NULL);
		fn( mnt,&finfo, Mask, state );
	}

	/* free up the dirlist buffer and last name raw blob */
	SAFE_FREE(dirlist);
	data_blob_free(&last_name_raw);
	return(total_received);
}

/****************************************************************************
 Interpret a short filename structure.
 The length of the structure is returned.
****************************************************************************/

static int interpret_short_filename(struct cli_state *cli, char *p,file_info *finfo)
{

	*finfo = def_finfo;

	finfo->mode = CVAL(p,21);
	
	/* this date is converted to GMT by make_unix_date */
	finfo->ctime = cli_make_unix_date(cli, p+22);
	finfo->mtime = finfo->atime = finfo->ctime;
	finfo->size = IVAL(p,26);
	clistr_pull(cli, finfo->name, p+30, sizeof(finfo->name), 12, STR_ASCII);
	if (strcmp(finfo->name, "..") && strcmp(finfo->name, ".")) {
		strncpy(finfo->short_name,finfo->name, sizeof(finfo->short_name)-1);
		finfo->short_name[sizeof(finfo->short_name)-1] = '\0';
	}

	return(DIR_STRUCT_SIZE);
}


/****************************************************************************
 Do a directory listing, calling fn on each file found.
 this uses the old SMBsearch interface. It is needed for testing Samba,
 but should otherwise not be used.
****************************************************************************/

int cli_list_old(struct cli_state *cli,const char *Mask,uint16 attribute, 
		 void (*fn)(const char *, file_info *, const char *, void *), void *state)
{
	char *p;
	int received = 0;
	BOOL first = True;
	char status[21];
	int num_asked = (cli->max_xmit - 100)/DIR_STRUCT_SIZE;
	int num_received = 0;
	int i;
	char *dirlist = NULL;
	pstring mask;
	
	ZERO_ARRAY(status);

	pstrcpy(mask,Mask);
  
	while (1) {
		memset(cli->outbuf,'\0',smb_size);
		memset(cli->inbuf,'\0',smb_size);

		set_message(cli->outbuf,2,0,True);

		SCVAL(cli->outbuf,smb_com,SMBsearch);

		SSVAL(cli->outbuf,smb_tid,cli->cnum);
		cli_setup_packet(cli);

		SSVAL(cli->outbuf,smb_vwv0,num_asked);
		SSVAL(cli->outbuf,smb_vwv1,attribute);
  
		p = smb_buf(cli->outbuf);
		*p++ = 4;
      
		p += clistr_push(cli, p, first?mask:"", -1, STR_TERMINATE);
		*p++ = 5;
		if (first) {
			SSVAL(p,0,0);
			p += 2;
		} else {
			SSVAL(p,0,21);
			p += 2;
			memcpy(p,status,21);
			p += 21;
		}

		cli_setup_bcc(cli, p);
		cli_send_smb(cli);
		if (!cli_receive_smb(cli)) break;

		received = SVAL(cli->inbuf,smb_vwv0);
		if (received <= 0) break;

		first = False;

		dirlist = SMB_REALLOC(dirlist,(num_received + received)*DIR_STRUCT_SIZE);
		if (!dirlist) {
			DEBUG(0,("cli_list_old: failed to expand dirlist"));
			return 0;
		}

		p = smb_buf(cli->inbuf) + 3;

		memcpy(dirlist+num_received*DIR_STRUCT_SIZE,
		       p,received*DIR_STRUCT_SIZE);
		
		memcpy(status,p + ((received-1)*DIR_STRUCT_SIZE),21);
		
		num_received += received;
		
		if (cli_is_error(cli)) break;
	}

	if (!first) {
		memset(cli->outbuf,'\0',smb_size);
		memset(cli->inbuf,'\0',smb_size);

		set_message(cli->outbuf,2,0,True);
		SCVAL(cli->outbuf,smb_com,SMBfclose);
		SSVAL(cli->outbuf,smb_tid,cli->cnum);
		cli_setup_packet(cli);

		SSVAL(cli->outbuf, smb_vwv0, 0); /* find count? */
		SSVAL(cli->outbuf, smb_vwv1, attribute);

		p = smb_buf(cli->outbuf);
		*p++ = 4;
		fstrcpy(p, "");
		p += strlen(p) + 1;
		*p++ = 5;
		SSVAL(p, 0, 21);
		p += 2;
		memcpy(p,status,21);
		p += 21;
		
		cli_setup_bcc(cli, p);
		cli_send_smb(cli);
		if (!cli_receive_smb(cli)) {
			DEBUG(0,("Error closing search: %s\n",cli_errstr(cli)));
		}
	}

	for (p=dirlist,i=0;i<num_received;i++) {
		file_info finfo;
		p += interpret_short_filename(cli, p,&finfo);
		fn("\\", &finfo, Mask, state);
	}

	SAFE_FREE(dirlist);
	return(num_received);
}

/****************************************************************************
 Do a directory listing, calling fn on each file found.
 This auto-switches between old and new style.
****************************************************************************/

int cli_list(struct cli_state *cli,const char *Mask,uint16 attribute, 
	     void (*fn)(const char *, file_info *, const char *, void *), void *state)
{
	if (cli->protocol <= PROTOCOL_LANMAN1)
		return cli_list_old(cli, Mask, attribute, fn, state);
	return cli_list_new(cli, Mask, attribute, fn, state);
}
