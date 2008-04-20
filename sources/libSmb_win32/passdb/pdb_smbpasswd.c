/*
 * Unix SMB/CIFS implementation. 
 * SMB parameters and setup
 * Copyright (C) Andrew Tridgell       1992-1998 
 * Modified by Jeremy Allison          1995.
 * Modified by Gerald (Jerry) Carter   2000-2001,2003
 * Modified by Andrew Bartlett         2002.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

/* 
   smb_passwd is analogous to sam_passwd used everywhere
   else.  However, smb_passwd is limited to the information
   stored by an smbpasswd entry 
 */
 
struct smb_passwd
{
        uint32 smb_userid;        /* this is actually the unix uid_t */
        const char *smb_name;     /* username string */

        const unsigned char *smb_passwd;    /* Null if no password */
        const unsigned char *smb_nt_passwd; /* Null if no password */

        uint16 acct_ctrl;             /* account info (ACB_xxxx bit-mask) */
        time_t pass_last_set_time;    /* password last set time */
};

struct smbpasswd_privates
{
	/* used for maintain locks on the smbpasswd file */
	int 	pw_file_lock_depth;
	
	/* Global File pointer */
	FILE 	*pw_file;
	
	/* formerly static variables */
	struct smb_passwd pw_buf;
	pstring  user_name;
	unsigned char smbpwd[16];
	unsigned char smbntpwd[16];

	/* retrive-once info */
	const char *smbpasswd_file;
};

enum pwf_access_type { PWF_READ, PWF_UPDATE, PWF_CREATE };

/***************************************************************
 Lock an fd. Abandon after waitsecs seconds.
****************************************************************/

static BOOL pw_file_lock(int fd, int type, int secs, int *plock_depth)
{
	if (fd < 0) {
		return False;
	}

	if(*plock_depth == 0) {
		if (!do_file_lock(fd, secs, type)) {
			DEBUG(10,("pw_file_lock: locking file failed, error = %s.\n",
				strerror(errno)));
			return False;
		}
	}

	(*plock_depth)++;

	return True;
}

/***************************************************************
 Unlock an fd. Abandon after waitsecs seconds.
****************************************************************/

static BOOL pw_file_unlock(int fd, int *plock_depth)
{
	BOOL ret=True;

	if (fd == 0 || *plock_depth == 0) {
		return True;
	}

	if(*plock_depth == 1) {
		ret = do_file_lock(fd, 5, F_UNLCK);
	}

	if (*plock_depth > 0) {
		(*plock_depth)--;
	}

	if(!ret) {
		DEBUG(10,("pw_file_unlock: unlocking file failed, error = %s.\n",
			strerror(errno)));
	}
	return ret;
}

/**************************************************************
 Intialize a smb_passwd struct
 *************************************************************/

static void pdb_init_smb(struct smb_passwd *user)
{
	if (user == NULL) 
		return;
	ZERO_STRUCTP (user);
	
	user->pass_last_set_time = (time_t)0;
}

/***************************************************************
 Internal fn to enumerate the smbpasswd list. Returns a void pointer
 to ensure no modification outside this module. Checks for atomic
 rename of smbpasswd file on update or create once the lock has
 been granted to prevent race conditions. JRA.
****************************************************************/

static FILE *startsmbfilepwent(const char *pfile, enum pwf_access_type type, int *lock_depth)
{
	FILE *fp = NULL;
	const char *open_mode = NULL;
	int race_loop = 0;
	int lock_type = F_RDLCK;

	if (!*pfile) {
		DEBUG(0, ("startsmbfilepwent: No SMB password file set\n"));
		return (NULL);
	}

	switch(type) {
		case PWF_READ:
			open_mode = "rb";
			lock_type = F_RDLCK;
			break;
		case PWF_UPDATE:
			open_mode = "r+b";
			lock_type = F_WRLCK;
			break;
		case PWF_CREATE:
			/*
			 * Ensure atomic file creation.
			 */
			{
				int i, fd = -1;

				for(i = 0; i < 5; i++) {
					if((fd = sys_open(pfile, O_CREAT|O_TRUNC|O_EXCL|O_RDWR, 0600))!=-1) {
						break;
					}
					sys_usleep(200); /* Spin, spin... */
				}
				if(fd == -1) {
					DEBUG(0,("startsmbfilepwent_internal: too many race conditions \
creating file %s\n", pfile));
					return NULL;
				}
				close(fd);
				open_mode = "r+b";
				lock_type = F_WRLCK;
				break;
			}
	}
		       
	for(race_loop = 0; race_loop < 5; race_loop++) {
		DEBUG(10, ("startsmbfilepwent_internal: opening file %s\n", pfile));

		if((fp = sys_fopen(pfile, open_mode)) == NULL) {

			/*
			 * If smbpasswd file doesn't exist, then create new one. This helps to avoid
			 * confusing error msg when adding user account first time.
			 */
			if (errno == ENOENT) {
				if ((fp = sys_fopen(pfile, "a+")) != NULL) {
					DEBUG(0, ("startsmbfilepwent_internal: file %s did not \
exist. File successfully created.\n", pfile));
				} else {
					DEBUG(0, ("startsmbfilepwent_internal: file %s did not \
exist. Couldn't create new one. Error was: %s",
					pfile, strerror(errno)));
					return NULL;
				}
			} else {
				DEBUG(0, ("startsmbfilepwent_internal: unable to open file %s. \
Error was: %s\n", pfile, strerror(errno)));
				return NULL;
			}
		}

		if (!pw_file_lock(fileno(fp), lock_type, 5, lock_depth)) {
			DEBUG(0, ("startsmbfilepwent_internal: unable to lock file %s. \
Error was %s\n", pfile, strerror(errno) ));
			fclose(fp);
			return NULL;
		}

		/*
		 * Only check for replacement races on update or create.
		 * For read we don't mind if the data is one record out of date.
		 */

		if(type == PWF_READ) {
			break;
		} else {
			SMB_STRUCT_STAT sbuf1, sbuf2;

			/*
			 * Avoid the potential race condition between the open and the lock
			 * by doing a stat on the filename and an fstat on the fd. If the
			 * two inodes differ then someone did a rename between the open and
			 * the lock. Back off and try the open again. Only do this 5 times to
			 * prevent infinate loops. JRA.
			 */

			if (sys_stat(pfile,&sbuf1) != 0) {
				DEBUG(0, ("startsmbfilepwent_internal: unable to stat file %s. \
Error was %s\n", pfile, strerror(errno)));
				pw_file_unlock(fileno(fp), lock_depth);
				fclose(fp);
				return NULL;
			}

			if (sys_fstat(fileno(fp),&sbuf2) != 0) {
				DEBUG(0, ("startsmbfilepwent_internal: unable to fstat file %s. \
Error was %s\n", pfile, strerror(errno)));
				pw_file_unlock(fileno(fp), lock_depth);
				fclose(fp);
				return NULL;
			}

			if( sbuf1.st_ino == sbuf2.st_ino) {
				/* No race. */
				break;
			}

			/*
			 * Race occurred - back off and try again...
			 */

			pw_file_unlock(fileno(fp), lock_depth);
			fclose(fp);
		}
	}

	if(race_loop == 5) {
		DEBUG(0, ("startsmbfilepwent_internal: too many race conditions opening file %s\n", pfile));
		return NULL;
	}

	/* Set a buffer to do more efficient reads */
	setvbuf(fp, (char *)NULL, _IOFBF, 1024);

	/* Make sure it is only rw by the owner */
#ifdef HAVE_FCHMOD
	if(fchmod(fileno(fp), S_IRUSR|S_IWUSR) == -1) {
#else
	if(chmod(pfile, S_IRUSR|S_IWUSR) == -1) {
#endif
		DEBUG(0, ("startsmbfilepwent_internal: failed to set 0600 permissions on password file %s. \
Error was %s\n.", pfile, strerror(errno) ));
		pw_file_unlock(fileno(fp), lock_depth);
		fclose(fp);
		return NULL;
	}

	/* We have a lock on the file. */
	return fp;
}

/***************************************************************
 End enumeration of the smbpasswd list.
****************************************************************/

static void endsmbfilepwent(FILE *fp, int *lock_depth)
{
	if (!fp) {
		return;
	}

	pw_file_unlock(fileno(fp), lock_depth);
	fclose(fp);
	DEBUG(7, ("endsmbfilepwent_internal: closed password file.\n"));
}

/*************************************************************************
 Routine to return the next entry in the smbpasswd list.
 *************************************************************************/

static struct smb_passwd *getsmbfilepwent(struct smbpasswd_privates *smbpasswd_state, FILE *fp)
{
	/* Static buffers we will return. */
	struct smb_passwd *pw_buf = &smbpasswd_state->pw_buf;
	char  *user_name = smbpasswd_state->user_name;
	unsigned char *smbpwd = smbpasswd_state->smbpwd;
	unsigned char *smbntpwd = smbpasswd_state->smbntpwd;
	char linebuf[256];
	int c;
	unsigned char *p;
	long uidval;
	size_t linebuf_len;
	char *status;

	if(fp == NULL) {
		DEBUG(0,("getsmbfilepwent: Bad password file pointer.\n"));
		return NULL;
	}

	pdb_init_smb(pw_buf);
	pw_buf->acct_ctrl = ACB_NORMAL;  

	/*
	 * Scan the file, a line at a time and check if the name matches.
	 */
	status = linebuf;
	while (status && !feof(fp)) {
		linebuf[0] = '\0';

		status = fgets(linebuf, 256, fp);
		if (status == NULL && ferror(fp)) {
			return NULL;
		}

		/*
		 * Check if the string is terminated with a newline - if not
		 * then we must keep reading and discard until we get one.
		 */
		if ((linebuf_len = strlen(linebuf)) == 0) {
			continue;
		}

		if (linebuf[linebuf_len - 1] != '\n') {
			c = '\0';
			while (!ferror(fp) && !feof(fp)) {
				c = fgetc(fp);
				if (c == '\n') {
					break;
				}
			}
		} else {
			linebuf[linebuf_len - 1] = '\0';
		}

#ifdef DEBUG_PASSWORD
		DEBUG(100, ("getsmbfilepwent: got line |%s|\n", linebuf));
#endif
		if ((linebuf[0] == 0) && feof(fp)) {
			DEBUG(4, ("getsmbfilepwent: end of file reached\n"));
			break;
		}

		/*
		 * The line we have should be of the form :-
		 * 
		 * username:uid:32hex bytes:[Account type]:LCT-12345678....other flags presently
		 * ignored....
		 * 
		 * or,
		 *
		 * username:uid:32hex bytes:32hex bytes:[Account type]:LCT-12345678....ignored....
		 *
		 * if Windows NT compatible passwords are also present.
		 * [Account type] is an ascii encoding of the type of account.
		 * LCT-(8 hex digits) is the time_t value of the last change time.
		 */

		if (linebuf[0] == '#' || linebuf[0] == '\0') {
			DEBUG(6, ("getsmbfilepwent: skipping comment or blank line\n"));
			continue;
		}
		p = (unsigned char *) strchr_m(linebuf, ':');
		if (p == NULL) {
			DEBUG(0, ("getsmbfilepwent: malformed password entry (no :)\n"));
			continue;
		}

		/*
		 * As 256 is shorter than a pstring we don't need to check
		 * length here - if this ever changes....
		 */
		SMB_ASSERT(sizeof(pstring) > sizeof(linebuf));

		strncpy(user_name, linebuf, PTR_DIFF(p, linebuf));
		user_name[PTR_DIFF(p, linebuf)] = '\0';

		/* Get smb uid. */

		p++; /* Go past ':' */

		if(*p == '-') {
			DEBUG(0, ("getsmbfilepwent: user name %s has a negative uid.\n", user_name));
			continue;
		}

		if (!isdigit(*p)) {
			DEBUG(0, ("getsmbfilepwent: malformed password entry for user %s (uid not number)\n",
				user_name));
			continue;
		}

		uidval = atoi((char *) p);

		while (*p && isdigit(*p)) {
			p++;
		}

		if (*p != ':') {
			DEBUG(0, ("getsmbfilepwent: malformed password entry for user %s (no : after uid)\n",
				user_name));
			continue;
		}

		pw_buf->smb_name = user_name;
		pw_buf->smb_userid = uidval;

		/*
		 * Now get the password value - this should be 32 hex digits
		 * which are the ascii representations of a 16 byte string.
		 * Get two at a time and put them into the password.
		 */

		/* Skip the ':' */
		p++;

		if (linebuf_len < (PTR_DIFF(p, linebuf) + 33)) {
			DEBUG(0, ("getsmbfilepwent: malformed password entry for user %s (passwd too short)\n",
				user_name ));
			continue;
		}

		if (p[32] != ':') {
			DEBUG(0, ("getsmbfilepwent: malformed password entry for user %s (no terminating :)\n",
				user_name));
			continue;
		}

		if (strnequal((char *) p, "NO PASSWORD", 11)) {
			pw_buf->smb_passwd = NULL;
			pw_buf->acct_ctrl |= ACB_PWNOTREQ;
		} else {
			if (*p == '*' || *p == 'X') {
				/* NULL LM password */
				pw_buf->smb_passwd = NULL;
				DEBUG(10, ("getsmbfilepwent: LM password for user %s invalidated\n", user_name));
			} else if (pdb_gethexpwd((char *)p, smbpwd)) {
				pw_buf->smb_passwd = smbpwd;
			} else {
				pw_buf->smb_passwd = NULL;
				DEBUG(0, ("getsmbfilepwent: Malformed Lanman password entry for user %s \
(non hex chars)\n", user_name));
			}
		}

		/* 
		 * Now check if the NT compatible password is
		 * available.
		 */
		pw_buf->smb_nt_passwd = NULL;
		p += 33; /* Move to the first character of the line after the lanman password. */
		if ((linebuf_len >= (PTR_DIFF(p, linebuf) + 33)) && (p[32] == ':')) {
			if (*p != '*' && *p != 'X') {
				if(pdb_gethexpwd((char *)p,smbntpwd)) {
					pw_buf->smb_nt_passwd = smbntpwd;
				}
			}
			p += 33; /* Move to the first character of the line after the NT password. */
		}

		DEBUG(5,("getsmbfilepwent: returning passwd entry for user %s, uid %ld\n",
			user_name, uidval));

		if (*p == '[') {
			unsigned char *end_p = (unsigned char *)strchr_m((char *)p, ']');
			pw_buf->acct_ctrl = pdb_decode_acct_ctrl((char*)p);

			/* Must have some account type set. */
			if(pw_buf->acct_ctrl == 0) {
				pw_buf->acct_ctrl = ACB_NORMAL;
			}

			/* Now try and get the last change time. */
			if(end_p) {
				p = end_p + 1;
			}
			if(*p == ':') {
				p++;
				if(*p && (StrnCaseCmp((char *)p, "LCT-", 4)==0)) {
					int i;
					p += 4;
					for(i = 0; i < 8; i++) {
						if(p[i] == '\0' || !isxdigit(p[i])) {
							break;
						}
					}
					if(i == 8) {
						/*
						 * p points at 8 characters of hex digits - 
						 * read into a time_t as the seconds since
						 * 1970 that the password was last changed.
						 */
						pw_buf->pass_last_set_time = (time_t)strtol((char *)p, NULL, 16);
					}
				}
			}
		} else {
			/* 'Old' style file. Fake up based on user name. */
			/*
			 * Currently trust accounts are kept in the same
			 * password file as 'normal accounts'. If this changes
			 * we will have to fix this code. JRA.
			 */
			if(pw_buf->smb_name[strlen(pw_buf->smb_name) - 1] == '$') {
				pw_buf->acct_ctrl &= ~ACB_NORMAL;
				pw_buf->acct_ctrl |= ACB_WSTRUST;
			}
		}

		return pw_buf;
	}

	DEBUG(5,("getsmbfilepwent: end of file reached.\n"));
	return NULL;
}

/************************************************************************
 Create a new smbpasswd entry - malloced space returned.
*************************************************************************/

static char *format_new_smbpasswd_entry(const struct smb_passwd *newpwd)
{
	int new_entry_length;
	char *new_entry;
	char *p;

	new_entry_length = strlen(newpwd->smb_name) + 1 + 15 + 1 + 32 + 1 + 32 + 1 + 
				NEW_PW_FORMAT_SPACE_PADDED_LEN + 1 + 13 + 2;

	if((new_entry = (char *)SMB_MALLOC( new_entry_length )) == NULL) {
		DEBUG(0, ("format_new_smbpasswd_entry: Malloc failed adding entry for user %s.\n",
			newpwd->smb_name ));
		return NULL;
	}

	slprintf(new_entry, new_entry_length - 1, "%s:%u:", newpwd->smb_name, (unsigned)newpwd->smb_userid);

	p = new_entry+strlen(new_entry);
	pdb_sethexpwd(p, newpwd->smb_passwd, newpwd->acct_ctrl);
	p+=strlen(p);
	*p = ':';
	p++;

	pdb_sethexpwd(p, newpwd->smb_nt_passwd, newpwd->acct_ctrl);
	p+=strlen(p);
	*p = ':';
	p++;

	/* Add the account encoding and the last change time. */
	slprintf((char *)p, new_entry_length - 1 - (p - new_entry),  "%s:LCT-%08X:\n",
		pdb_encode_acct_ctrl(newpwd->acct_ctrl, NEW_PW_FORMAT_SPACE_PADDED_LEN),
		(uint32)newpwd->pass_last_set_time);

	return new_entry;
}

/************************************************************************
 Routine to add an entry to the smbpasswd file.
*************************************************************************/

static BOOL add_smbfilepwd_entry(struct smbpasswd_privates *smbpasswd_state, struct smb_passwd *newpwd)
{
	const char *pfile = smbpasswd_state->smbpasswd_file;
	struct smb_passwd *pwd = NULL;
	FILE *fp = NULL;
	int wr_len;
	int fd;
	size_t new_entry_length;
	char *new_entry;
	SMB_OFF_T offpos;
 
	/* Open the smbpassword file - for update. */
	fp = startsmbfilepwent(pfile, PWF_UPDATE, &smbpasswd_state->pw_file_lock_depth);

	if (fp == NULL && errno == ENOENT) {
		/* Try again - create. */
		fp = startsmbfilepwent(pfile, PWF_CREATE, &smbpasswd_state->pw_file_lock_depth);
	}

	if (fp == NULL) {
		DEBUG(0, ("add_smbfilepwd_entry: unable to open file.\n"));
		return False;
	}

	/*
	 * Scan the file, a line at a time and check if the name matches.
	 */

	while ((pwd = getsmbfilepwent(smbpasswd_state, fp)) != NULL) {
		if (strequal(newpwd->smb_name, pwd->smb_name)) {
			DEBUG(0, ("add_smbfilepwd_entry: entry with name %s already exists\n", pwd->smb_name));
			endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
			return False;
		}
	}

	/* Ok - entry doesn't exist. We can add it */

	/* Create a new smb passwd entry and set it to the given password. */
	/* 
	 * The add user write needs to be atomic - so get the fd from 
	 * the fp and do a raw write() call.
	 */
	fd = fileno(fp);

	if((offpos = sys_lseek(fd, 0, SEEK_END)) == -1) {
		DEBUG(0, ("add_smbfilepwd_entry(sys_lseek): Failed to add entry for user %s to file %s. \
Error was %s\n", newpwd->smb_name, pfile, strerror(errno)));
		endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
		return False;
	}

	if((new_entry = format_new_smbpasswd_entry(newpwd)) == NULL) {
		DEBUG(0, ("add_smbfilepwd_entry(malloc): Failed to add entry for user %s to file %s. \
Error was %s\n", newpwd->smb_name, pfile, strerror(errno)));
		endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
		return False;
	}

	new_entry_length = strlen(new_entry);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("add_smbfilepwd_entry(%d): new_entry_len %d made line |%s|", 
			fd, (int)new_entry_length, new_entry));
#endif

	if ((wr_len = write(fd, new_entry, new_entry_length)) != new_entry_length) {
		DEBUG(0, ("add_smbfilepwd_entry(write): %d Failed to add entry for user %s to file %s. \
Error was %s\n", wr_len, newpwd->smb_name, pfile, strerror(errno)));

		/* Remove the entry we just wrote. */
		if(sys_ftruncate(fd, offpos) == -1) {
			DEBUG(0, ("add_smbfilepwd_entry: ERROR failed to ftruncate file %s. \
Error was %s. Password file may be corrupt ! Please examine by hand !\n", 
				newpwd->smb_name, strerror(errno)));
		}

		endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
		free(new_entry);
		return False;
	}

	free(new_entry);
	endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
	return True;
}

/************************************************************************
 Routine to search the smbpasswd file for an entry matching the username.
 and then modify its password entry. We can't use the startsmbpwent()/
 getsmbpwent()/endsmbpwent() interfaces here as we depend on looking
 in the actual file to decide how much room we have to write data.
 override = False, normal
 override = True, override XXXXXXXX'd out password or NO PASS
************************************************************************/

static BOOL mod_smbfilepwd_entry(struct smbpasswd_privates *smbpasswd_state, const struct smb_passwd* pwd)
{
	/* Static buffers we will return. */
	pstring user_name;

	char *status;
	char linebuf[256];
	char readbuf[1024];
	int c;
	fstring ascii_p16;
	fstring encode_bits;
	unsigned char *p = NULL;
	size_t linebuf_len = 0;
	FILE *fp;
	int lockfd;
	const char *pfile = smbpasswd_state->smbpasswd_file;
	BOOL found_entry = False;
	BOOL got_pass_last_set_time = False;

	SMB_OFF_T pwd_seekpos = 0;

	int i;
	int wr_len;
	int fd;

	if (!*pfile) {
		DEBUG(0, ("No SMB password file set\n"));
		return False;
	}
	DEBUG(10, ("mod_smbfilepwd_entry: opening file %s\n", pfile));

	fp = sys_fopen(pfile, "r+");

	if (fp == NULL) {
		DEBUG(0, ("mod_smbfilepwd_entry: unable to open file %s\n", pfile));
		return False;
	}
	/* Set a buffer to do more efficient reads */
	setvbuf(fp, readbuf, _IOFBF, sizeof(readbuf));

	lockfd = fileno(fp);

	if (!pw_file_lock(lockfd, F_WRLCK, 5, &smbpasswd_state->pw_file_lock_depth)) {
		DEBUG(0, ("mod_smbfilepwd_entry: unable to lock file %s\n", pfile));
		fclose(fp);
		return False;
	}

	/* Make sure it is only rw by the owner */
	chmod(pfile, 0600);

	/* We have a write lock on the file. */
	/*
	 * Scan the file, a line at a time and check if the name matches.
	 */
	status = linebuf;
	while (status && !feof(fp)) {
		pwd_seekpos = sys_ftell(fp);

		linebuf[0] = '\0';

		status = fgets(linebuf, sizeof(linebuf), fp);
		if (status == NULL && ferror(fp)) {
			pw_file_unlock(lockfd, &smbpasswd_state->pw_file_lock_depth);
			fclose(fp);
			return False;
		}

		/*
		 * Check if the string is terminated with a newline - if not
		 * then we must keep reading and discard until we get one.
		 */
		linebuf_len = strlen(linebuf);
		if (linebuf[linebuf_len - 1] != '\n') {
			c = '\0';
			while (!ferror(fp) && !feof(fp)) {
				c = fgetc(fp);
				if (c == '\n') {
					break;
				}
			}
		} else {
			linebuf[linebuf_len - 1] = '\0';
		}

#ifdef DEBUG_PASSWORD
		DEBUG(100, ("mod_smbfilepwd_entry: got line |%s|\n", linebuf));
#endif

		if ((linebuf[0] == 0) && feof(fp)) {
			DEBUG(4, ("mod_smbfilepwd_entry: end of file reached\n"));
			break;
		}

		/*
		 * The line we have should be of the form :-
		 * 
		 * username:uid:[32hex bytes]:....other flags presently
		 * ignored....
		 * 
		 * or,
		 *
		 * username:uid:[32hex bytes]:[32hex bytes]:[attributes]:LCT-XXXXXXXX:...ignored.
		 *
		 * if Windows NT compatible passwords are also present.
		 */

		if (linebuf[0] == '#' || linebuf[0] == '\0') {
			DEBUG(6, ("mod_smbfilepwd_entry: skipping comment or blank line\n"));
			continue;
		}

		p = (unsigned char *) strchr_m(linebuf, ':');

		if (p == NULL) {
			DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry (no :)\n"));
			continue;
		}

		/*
		 * As 256 is shorter than a pstring we don't need to check
		 * length here - if this ever changes....
		 */

		SMB_ASSERT(sizeof(user_name) > sizeof(linebuf));

		strncpy(user_name, linebuf, PTR_DIFF(p, linebuf));
		user_name[PTR_DIFF(p, linebuf)] = '\0';
		if (strequal(user_name, pwd->smb_name)) {
			found_entry = True;
			break;
		}
	}

	if (!found_entry) {
		pw_file_unlock(lockfd, &smbpasswd_state->pw_file_lock_depth);
		fclose(fp);

		DEBUG(2, ("Cannot update entry for user %s, as they don't exist in the smbpasswd file!\n",
			pwd->smb_name));
		return False;
	}

	DEBUG(6, ("mod_smbfilepwd_entry: entry exists for user %s\n", pwd->smb_name));

	/* User name matches - get uid and password */
	p++; /* Go past ':' */

	if (!isdigit(*p)) {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (uid not number)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd, &smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	while (*p && isdigit(*p)) {
		p++;
	}
	if (*p != ':') {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (no : after uid)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd, &smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	/*
	 * Now get the password value - this should be 32 hex digits
	 * which are the ascii representations of a 16 byte string.
	 * Get two at a time and put them into the password.
	 */
	p++;

	/* Record exact password position */
	pwd_seekpos += PTR_DIFF(p, linebuf);

	if (linebuf_len < (PTR_DIFF(p, linebuf) + 33)) {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (passwd too short)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return (False);
	}

	if (p[32] != ':') {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (no terminating :)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	/* Now check if the NT compatible password is available. */
	p += 33; /* Move to the first character of the line after the lanman password. */
	if (linebuf_len < (PTR_DIFF(p, linebuf) + 33)) {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (passwd too short)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return (False);
	}

	if (p[32] != ':') {
		DEBUG(0, ("mod_smbfilepwd_entry: malformed password entry for user %s (no terminating :)\n",
			pwd->smb_name));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	/* 
	 * Now check if the account info and the password last
	 * change time is available.
	 */
	p += 33; /* Move to the first character of the line after the NT password. */

	if (*p == '[') {
		i = 0;
		encode_bits[i++] = *p++;
		while((linebuf_len > PTR_DIFF(p, linebuf)) && (*p != ']')) {
			encode_bits[i++] = *p++;
		}

		encode_bits[i++] = ']';
		encode_bits[i++] = '\0';

		if(i == NEW_PW_FORMAT_SPACE_PADDED_LEN) {
			/*
			 * We are using a new format, space padded
			 * acct ctrl field. Encode the given acct ctrl
			 * bits into it.
			 */
			fstrcpy(encode_bits, pdb_encode_acct_ctrl(pwd->acct_ctrl, NEW_PW_FORMAT_SPACE_PADDED_LEN));
		} else {
			DEBUG(0,("mod_smbfilepwd_entry:  Using old smbpasswd format for user %s. \
This is no longer supported.!\n", pwd->smb_name));
			DEBUG(0,("mod_smbfilepwd_entry:  No changes made, failing.!\n"));
			pw_file_unlock(lockfd, &smbpasswd_state->pw_file_lock_depth);
			fclose(fp);
			return False;
		}

		/* Go past the ']' */
		if(linebuf_len > PTR_DIFF(p, linebuf)) {
			p++;
		}

		if((linebuf_len > PTR_DIFF(p, linebuf)) && (*p == ':')) {
			p++;

			/* We should be pointing at the LCT entry. */
			if((linebuf_len > (PTR_DIFF(p, linebuf) + 13)) && (StrnCaseCmp((char *)p, "LCT-", 4) == 0)) {
				p += 4;
				for(i = 0; i < 8; i++) {
					if(p[i] == '\0' || !isxdigit(p[i])) {
						break;
					}
				}
				if(i == 8) {
					/*
					 * p points at 8 characters of hex digits -
					 * read into a time_t as the seconds since
					 * 1970 that the password was last changed.
					 */
					got_pass_last_set_time = True;
				} /* i == 8 */
			} /* *p && StrnCaseCmp() */
		} /* p == ':' */
	} /* p == '[' */

	/* Entry is correctly formed. */

	/* Create the 32 byte representation of the new p16 */
	pdb_sethexpwd(ascii_p16, pwd->smb_passwd, pwd->acct_ctrl);

	/* Add on the NT md4 hash */
	ascii_p16[32] = ':';
	wr_len = 66;
	pdb_sethexpwd(ascii_p16+33, pwd->smb_nt_passwd, pwd->acct_ctrl);
	ascii_p16[65] = ':';
	ascii_p16[66] = '\0'; /* null-terminate the string so that strlen works */

	/* Add on the account info bits and the time of last password change. */
	if(got_pass_last_set_time) {
		slprintf(&ascii_p16[strlen(ascii_p16)], 
			sizeof(ascii_p16)-(strlen(ascii_p16)+1),
			"%s:LCT-%08X:", 
			encode_bits, (uint32)pwd->pass_last_set_time );
		wr_len = strlen(ascii_p16);
	}

#ifdef DEBUG_PASSWORD
	DEBUG(100,("mod_smbfilepwd_entry: "));
	dump_data(100, ascii_p16, wr_len);
#endif

	if(wr_len > sizeof(linebuf)) {
		DEBUG(0, ("mod_smbfilepwd_entry: line to write (%d) is too long.\n", wr_len+1));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return (False);
	}

	/*
	 * Do an atomic write into the file at the position defined by
	 * seekpos.
	 */

	/* The mod user write needs to be atomic - so get the fd from 
		the fp and do a raw write() call.
	 */

	fd = fileno(fp);

	if (sys_lseek(fd, pwd_seekpos - 1, SEEK_SET) != pwd_seekpos - 1) {
		DEBUG(0, ("mod_smbfilepwd_entry: seek fail on file %s.\n", pfile));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	/* Sanity check - ensure the areas we are writing are framed by ':' */
	if (read(fd, linebuf, wr_len+1) != wr_len+1) {
		DEBUG(0, ("mod_smbfilepwd_entry: read fail on file %s.\n", pfile));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	if ((linebuf[0] != ':') || (linebuf[wr_len] != ':'))	{
		DEBUG(0, ("mod_smbfilepwd_entry: check on passwd file %s failed.\n", pfile));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}
 
	if (sys_lseek(fd, pwd_seekpos, SEEK_SET) != pwd_seekpos) {
		DEBUG(0, ("mod_smbfilepwd_entry: seek fail on file %s.\n", pfile));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	if (write(fd, ascii_p16, wr_len) != wr_len) {
		DEBUG(0, ("mod_smbfilepwd_entry: write failed in passwd file %s\n", pfile));
		pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
		fclose(fp);
		return False;
	}

	pw_file_unlock(lockfd,&smbpasswd_state->pw_file_lock_depth);
	fclose(fp);
	return True;
}

/************************************************************************
 Routine to delete an entry in the smbpasswd file by name.
*************************************************************************/

static BOOL del_smbfilepwd_entry(struct smbpasswd_privates *smbpasswd_state, const char *name)
{
	const char *pfile = smbpasswd_state->smbpasswd_file;
	pstring pfile2;
	struct smb_passwd *pwd = NULL;
	FILE *fp = NULL;
	FILE *fp_write = NULL;
	int pfile2_lockdepth = 0;

	slprintf(pfile2, sizeof(pfile2)-1, "%s.%u", pfile, (unsigned)sys_getpid() );

	/*
	 * Open the smbpassword file - for update. It needs to be update
	 * as we need any other processes to wait until we have replaced
	 * it.
	 */

	if((fp = startsmbfilepwent(pfile, PWF_UPDATE, &smbpasswd_state->pw_file_lock_depth)) == NULL) {
		DEBUG(0, ("del_smbfilepwd_entry: unable to open file %s.\n", pfile));
		return False;
	}

	/*
	 * Create the replacement password file.
	 */
	if((fp_write = startsmbfilepwent(pfile2, PWF_CREATE, &pfile2_lockdepth)) == NULL) {
		DEBUG(0, ("del_smbfilepwd_entry: unable to open file %s.\n", pfile));
		endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
		return False;
	}

	/*
	 * Scan the file, a line at a time and check if the name matches.
	 */

	while ((pwd = getsmbfilepwent(smbpasswd_state, fp)) != NULL) {
		char *new_entry;
		size_t new_entry_length;

		if (strequal(name, pwd->smb_name)) {
			DEBUG(10, ("add_smbfilepwd_entry: found entry with name %s - deleting it.\n", name));
			continue;
		}

		/*
		 * We need to copy the entry out into the second file.
		 */

		if((new_entry = format_new_smbpasswd_entry(pwd)) == NULL) {
			DEBUG(0, ("del_smbfilepwd_entry(malloc): Failed to copy entry for user %s to file %s. \
Error was %s\n", pwd->smb_name, pfile2, strerror(errno)));
			unlink(pfile2);
			endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
			endsmbfilepwent(fp_write, &pfile2_lockdepth);
			return False;
		}

		new_entry_length = strlen(new_entry);

		if(fwrite(new_entry, 1, new_entry_length, fp_write) != new_entry_length) {
			DEBUG(0, ("del_smbfilepwd_entry(write): Failed to copy entry for user %s to file %s. \
Error was %s\n", pwd->smb_name, pfile2, strerror(errno)));
			unlink(pfile2);
			endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
			endsmbfilepwent(fp_write, &pfile2_lockdepth);
			free(new_entry);
			return False;
		}

		free(new_entry);
	}

	/*
	 * Ensure pfile2 is flushed before rename.
	 */

	if(fflush(fp_write) != 0) {
		DEBUG(0, ("del_smbfilepwd_entry: Failed to flush file %s. Error was %s\n", pfile2, strerror(errno)));
		endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
		endsmbfilepwent(fp_write,&pfile2_lockdepth);
		return False;
	}

	/*
	 * Do an atomic rename - then release the locks.
	 */

	if(rename(pfile2,pfile) != 0) {
		unlink(pfile2);
	}
  
	endsmbfilepwent(fp, &smbpasswd_state->pw_file_lock_depth);
	endsmbfilepwent(fp_write,&pfile2_lockdepth);
	return True;
}

/*********************************************************************
 Create a smb_passwd struct from a struct samu.
 We will not allocate any new memory.  The smb_passwd struct
 should only stay around as long as the struct samu does.
 ********************************************************************/

static BOOL build_smb_pass (struct smb_passwd *smb_pw, const struct samu *sampass)
{
	uint32 rid;

	if (sampass == NULL) 
		return False;
	ZERO_STRUCTP(smb_pw);

	if (!IS_SAM_DEFAULT(sampass, PDB_USERSID)) {
		rid = pdb_get_user_rid(sampass);
		
		/* If the user specified a RID, make sure its able to be both stored and retreived */
		if (rid == DOMAIN_USER_RID_GUEST) {
			struct passwd *passwd = getpwnam_alloc(NULL, lp_guestaccount());
			if (!passwd) {
				DEBUG(0, ("Could not find guest account via getpwnam()! (%s)\n", lp_guestaccount()));
				return False;
			}
			smb_pw->smb_userid=passwd->pw_uid;
			TALLOC_FREE(passwd);
		} else if (algorithmic_pdb_rid_is_user(rid)) {
			smb_pw->smb_userid=algorithmic_pdb_user_rid_to_uid(rid);
		} else {
			DEBUG(0,("build_sam_pass: Failing attempt to store user with non-uid based user RID. \n"));
			return False;
		}
	}

	smb_pw->smb_name=(const char*)pdb_get_username(sampass);

	smb_pw->smb_passwd=pdb_get_lanman_passwd(sampass);
	smb_pw->smb_nt_passwd=pdb_get_nt_passwd(sampass);

	smb_pw->acct_ctrl=pdb_get_acct_ctrl(sampass);
	smb_pw->pass_last_set_time=pdb_get_pass_last_set_time(sampass);

	return True;
}	

/*********************************************************************
 Create a struct samu from a smb_passwd struct
 ********************************************************************/

static BOOL build_sam_account(struct smbpasswd_privates *smbpasswd_state, 
			      struct samu *sam_pass, const struct smb_passwd *pw_buf)
{
	struct passwd *pwfile;
	
	if ( !sam_pass ) {
		DEBUG(5,("build_sam_account: struct samu is NULL\n"));
		return False;
	}

	/* verify the user account exists */

	if ( !(pwfile = Get_Pwnam_alloc(NULL, pw_buf->smb_name )) ) {
		DEBUG(0,("build_sam_account: smbpasswd database is corrupt!  username %s with uid "
		"%u is not in unix passwd database!\n", pw_buf->smb_name, pw_buf->smb_userid));
			return False;
	}
	
	if ( !NT_STATUS_IS_OK( samu_set_unix(sam_pass, pwfile )) )
		return False;
		
	TALLOC_FREE(pwfile);

	/* set remaining fields */
		
	pdb_set_nt_passwd (sam_pass, pw_buf->smb_nt_passwd, PDB_SET);
	pdb_set_lanman_passwd (sam_pass, pw_buf->smb_passwd, PDB_SET);			
	pdb_set_acct_ctrl (sam_pass, pw_buf->acct_ctrl, PDB_SET);
	pdb_set_pass_last_set_time (sam_pass, pw_buf->pass_last_set_time, PDB_SET);
	pdb_set_pass_can_change_time (sam_pass, pw_buf->pass_last_set_time, PDB_SET);
	
	return True;
}

/*****************************************************************
 Functions to be implemented by the new passdb API 
 ****************************************************************/

static NTSTATUS smbpasswd_setsampwent (struct pdb_methods *my_methods, BOOL update, uint32 acb_mask)
{
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	
	smbpasswd_state->pw_file = startsmbfilepwent(smbpasswd_state->smbpasswd_file, 
						       update ? PWF_UPDATE : PWF_READ, 
						       &(smbpasswd_state->pw_file_lock_depth));
				   
	/* did we fail?  Should we try to create it? */
	if (!smbpasswd_state->pw_file && update && errno == ENOENT) {
		FILE *fp;
		/* slprintf(msg_str,msg_str_len-1,
		   "smbpasswd file did not exist - attempting to create it.\n"); */
		DEBUG(0,("smbpasswd file did not exist - attempting to create it.\n"));
		fp = sys_fopen(smbpasswd_state->smbpasswd_file, "w");
		if (fp) {
			fprintf(fp, "# Samba SMB password file\n");
			fclose(fp);
		}
		
		smbpasswd_state->pw_file = startsmbfilepwent(smbpasswd_state->smbpasswd_file, 
							     update ? PWF_UPDATE : PWF_READ, 
							     &(smbpasswd_state->pw_file_lock_depth));
	}
	
	if (smbpasswd_state->pw_file != NULL)
		return NT_STATUS_OK;
	else
		return NT_STATUS_UNSUCCESSFUL;  
}

static void smbpasswd_endsampwent (struct pdb_methods *my_methods)
{
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	endsmbfilepwent(smbpasswd_state->pw_file, &(smbpasswd_state->pw_file_lock_depth));
}
 
/*****************************************************************
 ****************************************************************/

static NTSTATUS smbpasswd_getsampwent(struct pdb_methods *my_methods, struct samu *user)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	struct smb_passwd *pw_buf=NULL;
	BOOL done = False;

	DEBUG(5,("pdb_getsampwent\n"));

	if ( !user ) {
		DEBUG(5,("pdb_getsampwent (smbpasswd): user is NULL\n"));
		return nt_status;
	}

	while (!done) {
		/* do we have an entry? */
		pw_buf = getsmbfilepwent(smbpasswd_state, smbpasswd_state->pw_file);
		if (pw_buf == NULL) 
			return nt_status;

		/* build the struct samu entry from the smb_passwd struct. 
		   We loop in case the user in the pdb does not exist in 
		   the local system password file */
		if (build_sam_account(smbpasswd_state, user, pw_buf))
			done = True;
	}

	DEBUG(5,("getsampwent (smbpasswd): done\n"));

	/* success */
	return NT_STATUS_OK;
}

/****************************************************************
 Search smbpasswd file by iterating over the entries.  Do not
 call getpwnam() for unix account information until we have found
 the correct entry
 ***************************************************************/

static NTSTATUS smbpasswd_getsampwnam(struct pdb_methods *my_methods, 
				  struct samu *sam_acct, const char *username)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	struct smb_passwd *smb_pw;
	void *fp = NULL;

	DEBUG(10, ("getsampwnam (smbpasswd): search by name: %s\n", username));

	/* startsmbfilepwent() is used here as we don't want to lookup
	   the UNIX account in the local system password file until
	   we have a match.  */
	fp = startsmbfilepwent(smbpasswd_state->smbpasswd_file, PWF_READ, &(smbpasswd_state->pw_file_lock_depth));

	if (fp == NULL) {
		DEBUG(0, ("Unable to open passdb database.\n"));
		return nt_status;
	}

	while ( ((smb_pw=getsmbfilepwent(smbpasswd_state, fp)) != NULL)&& (!strequal(smb_pw->smb_name, username)) )
		/* do nothing....another loop */ ;
	
	endsmbfilepwent(fp, &(smbpasswd_state->pw_file_lock_depth));


	/* did we locate the username in smbpasswd  */
	if (smb_pw == NULL)
		return nt_status;
	
	DEBUG(10, ("getsampwnam (smbpasswd): found by name: %s\n", smb_pw->smb_name));

	if (!sam_acct) {
		DEBUG(10,("getsampwnam (smbpasswd): struct samu is NULL\n"));
		return nt_status;
	}
		
	/* now build the struct samu */
	if (!build_sam_account(smbpasswd_state, sam_acct, smb_pw))
		return nt_status;

	/* success */
	return NT_STATUS_OK;
}

static NTSTATUS smbpasswd_getsampwsid(struct pdb_methods *my_methods, struct samu *sam_acct, const DOM_SID *sid)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	struct smb_passwd *smb_pw;
	void *fp = NULL;
	fstring sid_str;
	uint32 rid;
	
	DEBUG(10, ("smbpasswd_getsampwrid: search by sid: %s\n", sid_to_string(sid_str, sid)));

	if (!sid_peek_check_rid(get_global_sam_sid(), sid, &rid))
		return NT_STATUS_UNSUCCESSFUL;

	/* More special case 'guest account' hacks... */
	if (rid == DOMAIN_USER_RID_GUEST) {
		const char *guest_account = lp_guestaccount();
		if (!(guest_account && *guest_account)) {
			DEBUG(1, ("Guest account not specfied!\n"));
			return nt_status;
		}
		return smbpasswd_getsampwnam(my_methods, sam_acct, guest_account);
	}

	/* Open the sam password file - not for update. */
	fp = startsmbfilepwent(smbpasswd_state->smbpasswd_file, PWF_READ, &(smbpasswd_state->pw_file_lock_depth));

	if (fp == NULL) {
		DEBUG(0, ("Unable to open passdb database.\n"));
		return nt_status;
	}

	while ( ((smb_pw=getsmbfilepwent(smbpasswd_state, fp)) != NULL) && (algorithmic_pdb_uid_to_user_rid(smb_pw->smb_userid) != rid) )
      		/* do nothing */ ;

	endsmbfilepwent(fp, &(smbpasswd_state->pw_file_lock_depth));


	/* did we locate the username in smbpasswd  */
	if (smb_pw == NULL)
		return nt_status;
	
	DEBUG(10, ("getsampwrid (smbpasswd): found by name: %s\n", smb_pw->smb_name));
		
	if (!sam_acct) {
		DEBUG(10,("getsampwrid: (smbpasswd) struct samu is NULL\n"));
		return nt_status;
	}

	/* now build the struct samu */
	if (!build_sam_account (smbpasswd_state, sam_acct, smb_pw))
		return nt_status;

	/* build_sam_account might change the SID on us, if the name was for the guest account */
	if (NT_STATUS_IS_OK(nt_status) && !sid_equal(pdb_get_user_sid(sam_acct), sid)) {
		fstring sid_string1, sid_string2;
		DEBUG(1, ("looking for user with sid %s instead returned %s for account %s!?!\n",
			  sid_to_string(sid_string1, sid), sid_to_string(sid_string2, pdb_get_user_sid(sam_acct)), pdb_get_username(sam_acct)));
		return NT_STATUS_NO_SUCH_USER;
	}

	/* success */
	return NT_STATUS_OK;
}

static NTSTATUS smbpasswd_add_sam_account(struct pdb_methods *my_methods, struct samu *sampass)
{
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	struct smb_passwd smb_pw;
	
	/* convert the struct samu */
	if (!build_smb_pass(&smb_pw, sampass)) {
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	/* add the entry */
	if(!add_smbfilepwd_entry(smbpasswd_state, &smb_pw)) {
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	return NT_STATUS_OK;
}

static NTSTATUS smbpasswd_update_sam_account(struct pdb_methods *my_methods, struct samu *sampass)
{
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;
	struct smb_passwd smb_pw;
	
	/* convert the struct samu */
	if (!build_smb_pass(&smb_pw, sampass)) {
		DEBUG(0, ("smbpasswd_update_sam_account: build_smb_pass failed!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	/* update the entry */
	if(!mod_smbfilepwd_entry(smbpasswd_state, &smb_pw)) {
		DEBUG(0, ("smbpasswd_update_sam_account: mod_smbfilepwd_entry failed!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	return NT_STATUS_OK;
}

static NTSTATUS smbpasswd_delete_sam_account (struct pdb_methods *my_methods, struct samu *sampass)
{
	struct smbpasswd_privates *smbpasswd_state = (struct smbpasswd_privates*)my_methods->private_data;

	const char *username = pdb_get_username(sampass);

	if (del_smbfilepwd_entry(smbpasswd_state, username))
		return NT_STATUS_OK;

	return NT_STATUS_UNSUCCESSFUL;
}

static NTSTATUS smbpasswd_rename_sam_account (struct pdb_methods *my_methods, 
					      struct samu *old_acct,
					      const char *newname)
{
	pstring rename_script;
	struct samu *new_acct = NULL;
	BOOL interim_account = False;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;

	if (!*(lp_renameuser_script()))
		goto done;

	if ( !(new_acct = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}
	
	if ( !pdb_copy_sam_account( new_acct, old_acct ) 
		|| !pdb_set_username(new_acct, newname, PDB_CHANGED)) 
	{
		goto done;
	}
	
	ret = smbpasswd_add_sam_account(my_methods, new_acct);
	if (!NT_STATUS_IS_OK(ret))
		goto done;

	interim_account = True;

	/* rename the posix user */
	pstrcpy(rename_script, lp_renameuser_script());

	if (*rename_script) {
	        int rename_ret;

		string_sub2(rename_script, "%unew", newname, sizeof(pstring), 
			    True, False, True);
		string_sub2(rename_script, "%uold", pdb_get_username(old_acct), 
			    sizeof(pstring), True, False, True);

		rename_ret = smbrun(rename_script, NULL);

		DEBUG(rename_ret ? 0 : 3,("Running the command `%s' gave %d\n", rename_script, rename_ret));

		if (rename_ret == 0) {
			smb_nscd_flush_user_cache();
		}

		if (rename_ret) 
			goto done; 
        } else {
		goto done;
	}

	smbpasswd_delete_sam_account(my_methods, old_acct);
	interim_account = False;

done:	
	/* cleanup */
	if (interim_account)
		smbpasswd_delete_sam_account(my_methods, new_acct);

	if (new_acct)
		TALLOC_FREE(new_acct);
	
	return (ret);	
}

static BOOL smbpasswd_rid_algorithm(struct pdb_methods *methods)
{
	return True;
}

static void free_private_data(void **vp) 
{
	struct smbpasswd_privates **privates = (struct smbpasswd_privates**)vp;
	
	endsmbfilepwent((*privates)->pw_file, &((*privates)->pw_file_lock_depth));
	
	*privates = NULL;
	/* No need to free any further, as it is talloc()ed */
}

static NTSTATUS pdb_init_smbpasswd( struct pdb_methods **pdb_method, const char *location )
{
	NTSTATUS nt_status;
	struct smbpasswd_privates *privates;

	if ( !NT_STATUS_IS_OK(nt_status = make_pdb_method( pdb_method )) ) {
		return nt_status;
	}

	(*pdb_method)->name = "smbpasswd";

	(*pdb_method)->setsampwent = smbpasswd_setsampwent;
	(*pdb_method)->endsampwent = smbpasswd_endsampwent;
	(*pdb_method)->getsampwent = smbpasswd_getsampwent;
	(*pdb_method)->getsampwnam = smbpasswd_getsampwnam;
	(*pdb_method)->getsampwsid = smbpasswd_getsampwsid;
	(*pdb_method)->add_sam_account = smbpasswd_add_sam_account;
	(*pdb_method)->update_sam_account = smbpasswd_update_sam_account;
	(*pdb_method)->delete_sam_account = smbpasswd_delete_sam_account;
	(*pdb_method)->rename_sam_account = smbpasswd_rename_sam_account;

	(*pdb_method)->rid_algorithm = smbpasswd_rid_algorithm;

	/* Setup private data and free function */

	if ( !(privates = TALLOC_ZERO_P( *pdb_method, struct smbpasswd_privates )) ) {
		DEBUG(0, ("talloc() failed for smbpasswd private_data!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	/* Store some config details */

	if (location) {
		privates->smbpasswd_file = talloc_strdup(*pdb_method, location);
	} else {
		privates->smbpasswd_file = talloc_strdup(*pdb_method, lp_smb_passwd_file());
	}
	
	if (!privates->smbpasswd_file) {
		DEBUG(0, ("talloc_strdp() failed for storing smbpasswd location!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	(*pdb_method)->private_data = privates;

	(*pdb_method)->free_private_data = free_private_data;

	return NT_STATUS_OK;
}

NTSTATUS pdb_smbpasswd_init(void) 
{
	return smb_register_passdb(PASSDB_INTERFACE_VERSION, "smbpasswd", pdb_init_smbpasswd);
}
