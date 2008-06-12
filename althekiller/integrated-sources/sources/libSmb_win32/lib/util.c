/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison 2001-2002
   Copyright (C) Simo Sorce 2001
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2003
   Copyright (C) James Peach 2006

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

extern fstring local_machine;
extern char *global_clobber_region_function;
extern unsigned int global_clobber_region_line;
extern fstring remote_arch;

/* Max allowable allococation - 256mb - 0x10000000 */
#define MAX_ALLOC_SIZE (1024*1024*256)

#if (defined(HAVE_NETGROUP) && defined (WITH_AUTOMOUNT))
#ifdef WITH_NISPLUS_HOME
#ifdef BROKEN_NISPLUS_INCLUDE_FILES
/*
 * The following lines are needed due to buggy include files
 * in Solaris 2.6 which define GROUP in both /usr/include/sys/acl.h and
 * also in /usr/include/rpcsvc/nis.h. The definitions conflict. JRA.
 * Also GROUP_OBJ is defined as 0x4 in /usr/include/sys/acl.h and as
 * an enum in /usr/include/rpcsvc/nis.h.
 */

#if defined(GROUP)
#undef GROUP
#endif

#if defined(GROUP_OBJ)
#undef GROUP_OBJ
#endif

#endif /* BROKEN_NISPLUS_INCLUDE_FILES */

#include <rpcsvc/nis.h>

#endif /* WITH_NISPLUS_HOME */
#endif /* HAVE_NETGROUP && WITH_AUTOMOUNT */

enum protocol_types Protocol = PROTOCOL_COREPLUS;

/* a default finfo structure to ensure all fields are sensible */
file_info def_finfo = {-1,0,0,0,0,0,0,"",""};

/* this is used by the chaining code */
int chain_size = 0;

int trans_num = 0;

static enum remote_arch_types ra_type = RA_UNKNOWN;
pstring user_socket_options=DEFAULT_SOCKET_OPTIONS;   

/***********************************************************************
 Definitions for all names.
***********************************************************************/

static char *smb_myname;
static char *smb_myworkgroup;
static char *smb_scope;
static int smb_num_netbios_names;
static char **smb_my_netbios_names;

/***********************************************************************
 Allocate and set myname. Ensure upper case.
***********************************************************************/

BOOL set_global_myname(const char *myname)
{
	SAFE_FREE(smb_myname);
	smb_myname = SMB_STRDUP(myname);
	if (!smb_myname)
		return False;
	strupper_m(smb_myname);
	return True;
}

const char *global_myname(void)
{
	return smb_myname;
}

/***********************************************************************
 Allocate and set myworkgroup. Ensure upper case.
***********************************************************************/

BOOL set_global_myworkgroup(const char *myworkgroup)
{
	SAFE_FREE(smb_myworkgroup);
	smb_myworkgroup = SMB_STRDUP(myworkgroup);
	if (!smb_myworkgroup)
		return False;
	strupper_m(smb_myworkgroup);
	return True;
}

const char *lp_workgroup(void)
{
	return smb_myworkgroup;
}

/***********************************************************************
 Allocate and set scope. Ensure upper case.
***********************************************************************/

BOOL set_global_scope(const char *scope)
{
	SAFE_FREE(smb_scope);
	smb_scope = SMB_STRDUP(scope);
	if (!smb_scope)
		return False;
	strupper_m(smb_scope);
	return True;
}

/*********************************************************************
 Ensure scope is never null string.
*********************************************************************/

const char *global_scope(void)
{
	if (!smb_scope)
		set_global_scope("");
	return smb_scope;
}

static void free_netbios_names_array(void)
{
	int i;

	for (i = 0; i < smb_num_netbios_names; i++)
		SAFE_FREE(smb_my_netbios_names[i]);

	SAFE_FREE(smb_my_netbios_names);
	smb_num_netbios_names = 0;
}

static BOOL allocate_my_netbios_names_array(size_t number)
{
	free_netbios_names_array();

	smb_num_netbios_names = number + 1;
	smb_my_netbios_names = SMB_MALLOC_ARRAY( char *, smb_num_netbios_names );

	if (!smb_my_netbios_names)
		return False;

	memset(smb_my_netbios_names, '\0', sizeof(char *) * smb_num_netbios_names);
	return True;
}

static BOOL set_my_netbios_names(const char *name, int i)
{
	SAFE_FREE(smb_my_netbios_names[i]);

	smb_my_netbios_names[i] = SMB_STRDUP(name);
	if (!smb_my_netbios_names[i])
		return False;
	strupper_m(smb_my_netbios_names[i]);
	return True;
}

/***********************************************************************
 Free memory allocated to global objects
***********************************************************************/

void gfree_names(void)
{
	SAFE_FREE( smb_myname );
	SAFE_FREE( smb_myworkgroup );
	SAFE_FREE( smb_scope );
	free_netbios_names_array();
}

void gfree_all( void )
{
	gfree_names();	
	gfree_loadparm();
	gfree_case_tables();
	gfree_debugsyms();
	gfree_charcnv();
	gfree_messsges();

	/* release the talloc null_context memory last */
	talloc_nc_free();
}

const char *my_netbios_names(int i)
{
	return smb_my_netbios_names[i];
}

BOOL set_netbios_aliases(const char **str_array)
{
	size_t namecount;

	/* Work out the max number of netbios aliases that we have */
	for( namecount=0; str_array && (str_array[namecount] != NULL); namecount++ )
		;

	if ( global_myname() && *global_myname())
		namecount++;

	/* Allocate space for the netbios aliases */
	if (!allocate_my_netbios_names_array(namecount))
		return False;

	/* Use the global_myname string first */
	namecount=0;
	if ( global_myname() && *global_myname()) {
		set_my_netbios_names( global_myname(), namecount );
		namecount++;
	}

	if (str_array) {
		size_t i;
		for ( i = 0; str_array[i] != NULL; i++) {
			size_t n;
			BOOL duplicate = False;

			/* Look for duplicates */
			for( n=0; n<namecount; n++ ) {
				if( strequal( str_array[i], my_netbios_names(n) ) ) {
					duplicate = True;
					break;
				}
			}
			if (!duplicate) {
				if (!set_my_netbios_names(str_array[i], namecount))
					return False;
				namecount++;
			}
		}
	}
	return True;
}

/****************************************************************************
  Common name initialization code.
****************************************************************************/

BOOL init_names(void)
{
	char *p;
	int n;

	if (global_myname() == NULL || *global_myname() == '\0') {
		if (!set_global_myname(myhostname())) {
			DEBUG( 0, ( "init_structs: malloc fail.\n" ) );
			return False;
		}
	}

	if (!set_netbios_aliases(lp_netbios_aliases())) {
		DEBUG( 0, ( "init_structs: malloc fail.\n" ) );
		return False;
	}			

	fstrcpy( local_machine, global_myname() );
	trim_char( local_machine, ' ', ' ' );
	p = strchr( local_machine, ' ' );
	if (p)
		*p = 0;
	strlower_m( local_machine );

	DEBUG( 5, ("Netbios name list:-\n") );
	for( n=0; my_netbios_names(n); n++ )
		DEBUGADD( 5, ( "my_netbios_names[%d]=\"%s\"\n", n, my_netbios_names(n) ) );

	return( True );
}

/**************************************************************************n
 Find a suitable temporary directory. The result should be copied immediately
 as it may be overwritten by a subsequent call.
****************************************************************************/

const char *tmpdir(void)
{
	char *p;
	if ((p = getenv("TMPDIR")))
		return p;
	return "/tmp";
}

/****************************************************************************
 Add a gid to an array of gids if it's not already there.
****************************************************************************/

void add_gid_to_array_unique(TALLOC_CTX *mem_ctx, gid_t gid,
			     gid_t **gids, size_t *num_gids)
{
	int i;

	if ((*num_gids != 0) && (*gids == NULL)) {
		/*
		 * A former call to this routine has failed to allocate memory
		 */
		return;
	}

	for (i=0; i<*num_gids; i++) {
		if ((*gids)[i] == gid)
			return;
	}

	if (mem_ctx != NULL) {
		*gids = TALLOC_REALLOC_ARRAY(mem_ctx, *gids, gid_t, *num_gids+1);
	} else {
		*gids = SMB_REALLOC_ARRAY(*gids, gid_t, *num_gids+1);
	}

	if (*gids == NULL) {
		return;
	}

	(*gids)[*num_gids] = gid;
	*num_gids += 1;
}

/****************************************************************************
 Like atoi but gets the value up to the separator character.
****************************************************************************/

static const char *Atoic(const char *p, int *n, const char *c)
{
	if (!isdigit((int)*p)) {
		DEBUG(5, ("Atoic: malformed number\n"));
		return NULL;
	}

	(*n) = atoi(p);

	while ((*p) && isdigit((int)*p))
		p++;

	if (strchr_m(c, *p) == NULL) {
		DEBUG(5, ("Atoic: no separator characters (%s) not found\n", c));
		return NULL;
	}

	return p;
}

/*************************************************************************
 Reads a list of numbers.
 *************************************************************************/

const char *get_numlist(const char *p, uint32 **num, int *count)
{
	int val;

	if (num == NULL || count == NULL)
		return NULL;

	(*count) = 0;
	(*num  ) = NULL;

	while ((p = Atoic(p, &val, ":,")) != NULL && (*p) != ':') {
		*num = SMB_REALLOC_ARRAY((*num), uint32, (*count)+1);
		if (!(*num)) {
			return NULL;
		}
		(*num)[(*count)] = val;
		(*count)++;
		p++;
	}

	return p;
}

/*******************************************************************
 Check if a file exists - call vfs_file_exist for samba files.
********************************************************************/

BOOL file_exist(const char *fname,SMB_STRUCT_STAT *sbuf)
{
	OutputDebugString("file_exist not supported\n");
#ifndef _XBOX
	SMB_STRUCT_STAT st;
	if (!sbuf)
		sbuf = &st;
  
	if (sys_stat(fname,sbuf) != 0) 
		return(False);

	return((S_ISREG(sbuf->st_mode)) || (S_ISFIFO(sbuf->st_mode)));
#endif //_XBOX
	return False;
}

/*******************************************************************
 Check a files mod time.
********************************************************************/

time_t file_modtime(const char *fname)
{
	SMB_STRUCT_STAT st;
  
	if (sys_stat(fname,&st) != 0) 
		return(0);

	return(st.st_mtime);
}

/*******************************************************************
 Check if a directory exists.
********************************************************************/

BOOL directory_exist(char *dname,SMB_STRUCT_STAT *st)
{
	SMB_STRUCT_STAT st2;
	BOOL ret;

	if (!st)
		st = &st2;

	if (sys_stat(dname,st) != 0) 
		return(False);

	ret = S_ISDIR(st->st_mode);
	if(!ret)
		errno = ENOTDIR;
	return ret;
}

/*******************************************************************
 Returns the size in bytes of the named file.
********************************************************************/

SMB_OFF_T get_file_size(char *file_name)
{
	SMB_STRUCT_STAT buf;
	buf.st_size = 0;
	if(sys_stat(file_name,&buf) != 0)
		return (SMB_OFF_T)-1;
	return(buf.st_size);
}

/*******************************************************************
 Return a string representing an attribute for a file.
********************************************************************/

char *attrib_string(uint16 mode)
{
	static fstring attrstr;

	attrstr[0] = 0;

	if (mode & aVOLID) fstrcat(attrstr,"V");
	if (mode & aDIR) fstrcat(attrstr,"D");
	if (mode & aARCH) fstrcat(attrstr,"A");
	if (mode & aHIDDEN) fstrcat(attrstr,"H");
	if (mode & aSYSTEM) fstrcat(attrstr,"S");
	if (mode & aRONLY) fstrcat(attrstr,"R");	  

	return(attrstr);
}

/*******************************************************************
 Show a smb message structure.
********************************************************************/

void show_msg(char *buf)
{
	int i;
	int bcc=0;

	if (!DEBUGLVL(5))
		return;
	
	DEBUG(5,("size=%d\nsmb_com=0x%x\nsmb_rcls=%d\nsmb_reh=%d\nsmb_err=%d\nsmb_flg=%d\nsmb_flg2=%d\n",
			smb_len(buf),
			(int)CVAL(buf,smb_com),
			(int)CVAL(buf,smb_rcls),
			(int)CVAL(buf,smb_reh),
			(int)SVAL(buf,smb_err),
			(int)CVAL(buf,smb_flg),
			(int)SVAL(buf,smb_flg2)));
	DEBUGADD(5,("smb_tid=%d\nsmb_pid=%d\nsmb_uid=%d\nsmb_mid=%d\n",
			(int)SVAL(buf,smb_tid),
			(int)SVAL(buf,smb_pid),
			(int)SVAL(buf,smb_uid),
			(int)SVAL(buf,smb_mid)));
	DEBUGADD(5,("smt_wct=%d\n",(int)CVAL(buf,smb_wct)));

	for (i=0;i<(int)CVAL(buf,smb_wct);i++)
		DEBUGADD(5,("smb_vwv[%2d]=%5d (0x%X)\n",i,
			SVAL(buf,smb_vwv+2*i),SVAL(buf,smb_vwv+2*i)));
	
	bcc = (int)SVAL(buf,smb_vwv+2*(CVAL(buf,smb_wct)));

	DEBUGADD(5,("smb_bcc=%d\n",bcc));

	if (DEBUGLEVEL < 10)
		return;

	if (DEBUGLEVEL < 50)
		bcc = MIN(bcc, 512);

	dump_data(10, smb_buf(buf), bcc);	
}

/*******************************************************************
 Set the length and marker of an smb packet.
********************************************************************/

void smb_setlen(char *buf,int len)
{
	_smb_setlen(buf,len);

	SCVAL(buf,4,0xFF);
	SCVAL(buf,5,'S');
	SCVAL(buf,6,'M');
	SCVAL(buf,7,'B');
}

/*******************************************************************
 Setup the word count and byte count for a smb message.
********************************************************************/

int set_message(char *buf,int num_words,int num_bytes,BOOL zero)
{
	if (zero && (num_words || num_bytes)) {
		memset(buf + smb_size,'\0',num_words*2 + num_bytes);
	}
	SCVAL(buf,smb_wct,num_words);
	SSVAL(buf,smb_vwv + num_words*SIZEOFWORD,num_bytes);  
	smb_setlen(buf,smb_size + num_words*2 + num_bytes - 4);
	return (smb_size + num_words*2 + num_bytes);
}

/*******************************************************************
 Setup only the byte count for a smb message.
********************************************************************/

int set_message_bcc(char *buf,int num_bytes)
{
	int num_words = CVAL(buf,smb_wct);
	SSVAL(buf,smb_vwv + num_words*SIZEOFWORD,num_bytes);  
	smb_setlen(buf,smb_size + num_words*2 + num_bytes - 4);
	return (smb_size + num_words*2 + num_bytes);
}

/*******************************************************************
 Setup only the byte count for a smb message, using the end of the
 message as a marker.
********************************************************************/

int set_message_end(void *outbuf,void *end_ptr)
{
	return set_message_bcc((char *)outbuf,PTR_DIFF(end_ptr,smb_buf((char *)outbuf)));
}

/*******************************************************************
 Reduce a file name, removing .. elements.
********************************************************************/

void dos_clean_name(char *s)
{
	char *p=NULL;

	DEBUG(3,("dos_clean_name [%s]\n",s));

	/* remove any double slashes */
	all_string_sub(s, "\\\\", "\\", 0);

	while ((p = strstr_m(s,"\\..\\")) != NULL) {
		pstring s1;

		*p = 0;
		pstrcpy(s1,p+3);

		if ((p=strrchr_m(s,'\\')) != NULL)
			*p = 0;
		else
			*s = 0;
		pstrcat(s,s1);
	}  

	trim_string(s,NULL,"\\..");

	all_string_sub(s, "\\.\\", "\\", 0);
}

/*******************************************************************
 Reduce a file name, removing .. elements. 
********************************************************************/

void unix_clean_name(char *s)
{
	char *p=NULL;

	DEBUG(3,("unix_clean_name [%s]\n",s));

	/* remove any double slashes */
	all_string_sub(s, "//","/", 0);

	/* Remove leading ./ characters */
	if(strncmp(s, "./", 2) == 0) {
		trim_string(s, "./", NULL);
		if(*s == 0)
			pstrcpy(s,"./");
	}

	while ((p = strstr_m(s,"/../")) != NULL) {
		pstring s1;

		*p = 0;
		pstrcpy(s1,p+3);

		if ((p=strrchr_m(s,'/')) != NULL)
			*p = 0;
		else
			*s = 0;
		pstrcat(s,s1);
	}  

	trim_string(s,NULL,"/..");
}

/*******************************************************************
 Close the low 3 fd's and open dev/null in their place.
********************************************************************/

void close_low_fds(BOOL stderr_too)
{
#ifndef VALGRIND
	int fd;
	int i;

	close(0);
	close(1); 

	if (stderr_too)
		close(2);

	/* try and use up these file descriptors, so silly
		library routines writing to stdout etc won't cause havoc */
	for (i=0;i<3;i++) {
		if (i == 2 && !stderr_too)
			continue;

		fd = sys_open("/dev/null",O_RDWR,0);
		if (fd < 0)
			fd = sys_open("/dev/null",O_WRONLY,0);
		if (fd < 0) {
			DEBUG(0,("Can't open /dev/null\n"));
			return;
		}
		if (fd != i) {
			DEBUG(0,("Didn't get file descriptor %d\n",i));
			return;
		}
	}
#endif
}

/*******************************************************************
 Write data into an fd at a given offset. Ignore seek errors.
********************************************************************/

ssize_t write_data_at_offset(int fd, const char *buffer, size_t N, SMB_OFF_T pos)
{
	size_t total=0;
	ssize_t ret;

	if (pos == (SMB_OFF_T)-1) {
		return write_data(fd, buffer, N);
	}
#if defined(HAVE_PWRITE) || defined(HAVE_PRWITE64)
	while (total < N) {
		ret = sys_pwrite(fd,buffer + total,N - total, pos);
		if (ret == -1 && errno == ESPIPE) {
			return write_data(fd, buffer + total,N - total);
		}
		if (ret == -1) {
			DEBUG(0,("write_data_at_offset: write failure. Error = %s\n", strerror(errno) ));
			return -1;
		}
		if (ret == 0) {
			return total;
		}
		total += ret;
		pos += ret;
	}
	return (ssize_t)total;
#else
	/* Use lseek and write_data. */
	if (sys_lseek(fd, pos, SEEK_SET) == -1) {
		if (errno != ESPIPE) {
			return -1;
		}
	}
	return write_data(fd, buffer, N);
#endif
}

/****************************************************************************
 Set a fd into blocking/nonblocking mode. Uses POSIX O_NONBLOCK if available,
 else
  if SYSV use O_NDELAY
  if BSD use FNDELAY
****************************************************************************/

int set_blocking(int fd, BOOL set)
{
#ifdef _XBOX
  unsigned long arg;

  if(set)
    arg = 0;
  else
    arg = 1;

  ioctlsocket(fd, FIONBIO, &arg);

  return 0;

#elif
	int val;
#ifdef O_NONBLOCK
#define FLAG_TO_SET O_NONBLOCK
#else
#ifdef SYSV
#define FLAG_TO_SET O_NDELAY
#else /* BSD */
#define FLAG_TO_SET FNDELAY
#endif
#endif

	if((val = sys_fcntl_long(fd, F_GETFL, 0)) == -1)
		return -1;
	if(set) /* Turn blocking on - ie. clear nonblock flag */
		val &= ~FLAG_TO_SET;
	else
		val |= FLAG_TO_SET;
	return sys_fcntl_long( fd, F_SETFL, val);
#undef FLAG_TO_SET
#endif //_XBOX
}

/****************************************************************************
 Transfer some data between two fd's.
****************************************************************************/

#ifndef TRANSFER_BUF_SIZE
#define TRANSFER_BUF_SIZE 65536
#endif

ssize_t transfer_file_internal(int infd, int outfd, size_t n, ssize_t (*read_fn)(int, void *, size_t),
						ssize_t (*write_fn)(int, const void *, size_t))
{
	char *buf;
	size_t total = 0;
	ssize_t read_ret;
	ssize_t write_ret;
	size_t num_to_read_thistime;
	size_t num_written = 0;

	if ((buf = SMB_MALLOC(TRANSFER_BUF_SIZE)) == NULL)
		return -1;

	while (total < n) {
		num_to_read_thistime = MIN((n - total), TRANSFER_BUF_SIZE);

		read_ret = (*read_fn)(infd, buf, num_to_read_thistime);
		if (read_ret == -1) {
			DEBUG(0,("transfer_file_internal: read failure. Error = %s\n", strerror(errno) ));
			SAFE_FREE(buf);
			return -1;
		}
		if (read_ret == 0)
			break;

		num_written = 0;
 
		while (num_written < read_ret) {
			write_ret = (*write_fn)(outfd,buf + num_written, read_ret - num_written);
 
			if (write_ret == -1) {
				DEBUG(0,("transfer_file_internal: write failure. Error = %s\n", strerror(errno) ));
				SAFE_FREE(buf);
				return -1;
			}
			if (write_ret == 0)
				return (ssize_t)total;
 
			num_written += (size_t)write_ret;
		}

		total += (size_t)read_ret;
	}

	SAFE_FREE(buf);
	return (ssize_t)total;		
}

SMB_OFF_T transfer_file(int infd,int outfd,SMB_OFF_T n)
{
	return (SMB_OFF_T)transfer_file_internal(infd, outfd, (size_t)n, sys_read, sys_write);
}

/*******************************************************************
 Sleep for a specified number of milliseconds.
********************************************************************/

void smb_msleep(unsigned int t)
{
#if defined(HAVE_NANOSLEEP)
	struct timespec tval;
	int ret;

	tval.tv_sec = t/1000;
	tval.tv_nsec = 1000000*(t%1000);

	do {
		errno = 0;
		ret = nanosleep(&tval, &tval);
	} while (ret < 0 && errno == EINTR && (tval.tv_sec > 0 || tval.tv_nsec > 0));
#elif _XBOX
  Sleep(t);
#else
	unsigned int tdiff=0;
	struct timeval tval,t1,t2;  
	fd_set fds;

	GetTimeOfDay(&t1);
	t2 = t1;
  
	while (tdiff < t) {
		tval.tv_sec = (t-tdiff)/1000;
		tval.tv_usec = 1000*((t-tdiff)%1000);

		/* Never wait for more than 1 sec. */
		if (tval.tv_sec > 1) {
			tval.tv_sec = 1; 
			tval.tv_usec = 0;
		}

		FD_ZERO(&fds);
		errno = 0;
		sys_select_intr(0,&fds,NULL,NULL,&tval);

		GetTimeOfDay(&t2);
		if (t2.tv_sec < t1.tv_sec) {
			/* Someone adjusted time... */
			t1 = t2;
		}

		tdiff = TvalDiff(&t1,&t2);
	}
#endif
}

/****************************************************************************
 Become a daemon, discarding the controlling terminal.
****************************************************************************/

void become_daemon(BOOL Fork, BOOL no_process_group)
{
	if (Fork) {
		if (sys_fork()) {
			_exit(0);
		}
	}

  /* detach from the terminal */
#ifdef HAVE_SETSID
	if (!no_process_group) setsid();
#elif defined(TIOCNOTTY)
	if (!no_process_group) {
		int i = sys_open("/dev/tty", O_RDWR, 0);
		if (i != -1) {
			ioctl(i, (int) TIOCNOTTY, (char *)0);      
			close(i);
		}
	}
#endif /* HAVE_SETSID */

	/* Close fd's 0,1,2. Needed if started by rsh */
	close_low_fds(False);  /* Don't close stderr, let the debug system
				  attach it to the logfile */
}

/****************************************************************************
 Put up a yes/no prompt.
****************************************************************************/

BOOL yesno(char *p)
{
	pstring ans;
	printf("%s",p);

	if (!fgets(ans,sizeof(ans)-1,stdin))
		return(False);

	if (*ans == 'y' || *ans == 'Y')
		return(True);

	return(False);
}

#if defined(PARANOID_MALLOC_CHECKER)

/****************************************************************************
 Internal malloc wrapper. Externally visible.
****************************************************************************/

void *malloc_(size_t size)
{
#undef malloc
	return malloc(size);
#define malloc(s) __ERROR_DONT_USE_MALLOC_DIRECTLY
}

/****************************************************************************
 Internal calloc wrapper. Not externally visible.
****************************************************************************/

static void *calloc_(size_t count, size_t size)
{
#undef calloc
	return calloc(count, size);
#define calloc(n,s) __ERROR_DONT_USE_CALLOC_DIRECTLY
}

/****************************************************************************
 Internal realloc wrapper. Not externally visible.
****************************************************************************/

static void *realloc_(void *ptr, size_t size)
{
#undef realloc
	return realloc(ptr, size);
#define realloc(p,s) __ERROR_DONT_USE_RELLOC_DIRECTLY
}

#endif /* PARANOID_MALLOC_CHECKER */

/****************************************************************************
 Type-safe malloc.
****************************************************************************/

void *malloc_array(size_t el_size, unsigned int count)
{
	if (count >= MAX_ALLOC_SIZE/el_size) {
		return NULL;
	}

#if defined(PARANOID_MALLOC_CHECKER)
	return malloc_(el_size*count);
#else
	return malloc(el_size*count);
#endif
}

/****************************************************************************
 Type-safe calloc.
****************************************************************************/

void *calloc_array(size_t size, size_t nmemb)
{
	if (nmemb >= MAX_ALLOC_SIZE/size) {
		return NULL;
	}
#if defined(PARANOID_MALLOC_CHECKER)
	return calloc_(nmemb, size);
#else
	return calloc(nmemb, size);
#endif
}

/****************************************************************************
 Expand a pointer to be a particular size.
 Note that this version of Realloc has an extra parameter that decides
 whether to free the passed in storage on allocation failure or if the
 new size is zero.

 This is designed for use in the typical idiom of :

 p = SMB_REALLOC(p, size)
 if (!p) {
    return error;
 }

 and not to have to keep track of the old 'p' contents to free later, nor
 to worry if the size parameter was zero. In the case where NULL is returned
 we guarentee that p has been freed.

 If free later semantics are desired, then pass 'free_old_on_error' as False which
 guarentees that the old contents are not freed on error, even if size == 0. To use
 this idiom use :

 tmp = SMB_REALLOC_KEEP_OLD_ON_ERROR(p, size);
 if (!tmp) {
    SAFE_FREE(p);
    return error;
 } else {
    p = tmp;
 }

 Changes were instigated by Coverity error checking. JRA.
****************************************************************************/

void *Realloc(void *p, size_t size, BOOL free_old_on_error)
{
	void *ret=NULL;

	if (size == 0) {
		if (free_old_on_error) {
			SAFE_FREE(p);
		}
		DEBUG(2,("Realloc asked for 0 bytes\n"));
		return NULL;
	}

#if defined(PARANOID_MALLOC_CHECKER)
	if (!p) {
		ret = (void *)malloc_(size);
	} else {
		ret = (void *)realloc_(p,size);
	}
#else
	if (!p) {
		ret = (void *)malloc(size);
	} else {
		ret = (void *)realloc(p,size);
	}
#endif

	if (!ret) {
		if (free_old_on_error && p) {
			SAFE_FREE(p);
		}
		DEBUG(0,("Memory allocation error: failed to expand to %d bytes\n",(int)size));
	}

	return(ret);
}

/****************************************************************************
 Type-safe realloc.
****************************************************************************/

void *realloc_array(void *p, size_t el_size, unsigned int count, BOOL free_old_on_error)
{
	if (count >= MAX_ALLOC_SIZE/el_size) {
		if (free_old_on_error) {
			SAFE_FREE(p);
		}
		return NULL;
	}
	return Realloc(p, el_size*count, free_old_on_error);
}

/****************************************************************************
 (Hopefully) efficient array append.
****************************************************************************/

void add_to_large_array(TALLOC_CTX *mem_ctx, size_t element_size,
			void *element, void **array, uint32 *num_elements,
			ssize_t *array_size)
{
	if (*array_size < 0) {
		return;
	}

	if (*array == NULL) {
		if (*array_size == 0) {
			*array_size = 128;
		}

		if (*array_size >= MAX_ALLOC_SIZE/element_size) {
			goto error;
		}

		if (mem_ctx != NULL) {
			*array = TALLOC(mem_ctx, element_size * (*array_size));
		} else {
			*array = SMB_MALLOC(element_size * (*array_size));
		}

		if (*array == NULL) {
			goto error;
		}
	}

	if (*num_elements == *array_size) {
		*array_size *= 2;

		if (*array_size >= MAX_ALLOC_SIZE/element_size) {
			goto error;
		}

		if (mem_ctx != NULL) {
			*array = TALLOC_REALLOC(mem_ctx, *array,
						element_size * (*array_size));
		} else {
			*array = SMB_REALLOC(*array,
					     element_size * (*array_size));
		}

		if (*array == NULL) {
			goto error;
		}
	}

	memcpy((char *)(*array) + element_size*(*num_elements),
	       element, element_size);
	*num_elements += 1;

	return;

 error:
	*num_elements = 0;
	*array_size = -1;
}

/****************************************************************************
 Free memory, checks for NULL.
 Use directly SAFE_FREE()
 Exists only because we need to pass a function pointer somewhere --SSS
****************************************************************************/

void safe_free(void *p)
{
	SAFE_FREE(p);
}

/****************************************************************************
 Get my own name and IP.
****************************************************************************/

BOOL get_myname(char *my_name)
{
	pstring hostname;

	*hostname = 0;

	/* get my host name */
	if (gethostname(hostname, sizeof(hostname)) == -1) {
		DEBUG(0,("gethostname failed\n"));
		return False;
	} 

	/* Ensure null termination. */
	hostname[sizeof(hostname)-1] = '\0';

	if (my_name) {
		/* split off any parts after an initial . */
		char *p = strchr_m(hostname,'.');

		if (p)
			*p = 0;
		
		fstrcpy(my_name,hostname);
	}
	
	return(True);
}

/****************************************************************************
 Get my own canonical name, including domain.
****************************************************************************/

BOOL get_mydnsfullname(fstring my_dnsname)
{
	static fstring dnshostname;
	struct hostent *hp;

	if (!*dnshostname) {
		/* get my host name */
		if (gethostname(dnshostname, sizeof(dnshostname)) == -1) {
			*dnshostname = '\0';
			DEBUG(0,("gethostname failed\n"));
			return False;
		} 

		/* Ensure null termination. */
		dnshostname[sizeof(dnshostname)-1] = '\0';

		/* Ensure we get the cannonical name. */
		if (!(hp = sys_gethostbyname(dnshostname))) {
			*dnshostname = '\0';
			return False;
		}
		fstrcpy(dnshostname, hp->h_name);
	}
	fstrcpy(my_dnsname, dnshostname);
	return True;
}

/****************************************************************************
 Get my own domain name.
****************************************************************************/

BOOL get_mydnsdomname(fstring my_domname)
{
	fstring domname;
	char *p;

	*my_domname = '\0';
	if (!get_mydnsfullname(domname)) {
		return False;
	}	
	p = strchr_m(domname, '.');
	if (p) {
		p++;
		fstrcpy(my_domname, p);
	}

	return False;
}

/****************************************************************************
 Interpret a protocol description string, with a default.
****************************************************************************/

int interpret_protocol(const char *str,int def)
{
	if (strequal(str,"NT1"))
		return(PROTOCOL_NT1);
	if (strequal(str,"LANMAN2"))
		return(PROTOCOL_LANMAN2);
	if (strequal(str,"LANMAN1"))
		return(PROTOCOL_LANMAN1);
	if (strequal(str,"CORE"))
		return(PROTOCOL_CORE);
	if (strequal(str,"COREPLUS"))
		return(PROTOCOL_COREPLUS);
	if (strequal(str,"CORE+"))
		return(PROTOCOL_COREPLUS);
  
	DEBUG(0,("Unrecognised protocol level %s\n",str));
  
	return(def);
}

/****************************************************************************
 Return true if a string could be a pure IP address.
****************************************************************************/

BOOL is_ipaddress(const char *str)
{
	BOOL pure_address = True;
	int i;
  
	for (i=0; pure_address && str[i]; i++)
		if (!(isdigit((int)str[i]) || str[i] == '.'))
			pure_address = False;

	/* Check that a pure number is not misinterpreted as an IP */
	pure_address = pure_address && (strchr_m(str, '.') != NULL);

	return pure_address;
}

/****************************************************************************
 Interpret an internet address or name into an IP address in 4 byte form.
****************************************************************************/

uint32 interpret_addr(const char *str)
{
	struct hostent *hp;
	uint32 res;

	if (strcmp(str,"0.0.0.0") == 0)
		return(0);
	if (strcmp(str,"255.255.255.255") == 0)
		return(0xFFFFFFFF);

  /* if it's in the form of an IP address then get the lib to interpret it */
	if (is_ipaddress(str)) {
		res = inet_addr(str);
	} else {
		/* otherwise assume it's a network name of some sort and use 
			sys_gethostbyname */
		if ((hp = sys_gethostbyname(str)) == 0) {
			DEBUG(3,("sys_gethostbyname: Unknown host. %s\n",str));
			return 0;
		}

		if(hp->h_addr == NULL) {
			DEBUG(3,("sys_gethostbyname: host address is invalid for host %s\n",str));
			return 0;
		}
		putip((char *)&res,(char *)hp->h_addr);
	}

	if (res == (uint32)-1)
		return(0);

	return(res);
}

/*******************************************************************
 A convenient addition to interpret_addr().
******************************************************************/

struct in_addr *interpret_addr2(const char *str)
{
	static struct in_addr ret;
	uint32 a = interpret_addr(str);
	ret.s_addr = a;
	return(&ret);
}

/*******************************************************************
 Check if an IP is the 0.0.0.0.
******************************************************************/

BOOL is_zero_ip(struct in_addr ip)
{
	uint32 a;
	putip((char *)&a,(char *)&ip);
	return(a == 0);
}

/*******************************************************************
 Set an IP to 0.0.0.0.
******************************************************************/

void zero_ip(struct in_addr *ip)
{
        static BOOL init;
        static struct in_addr ipzero;

        if (!init) {
                ipzero = *interpret_addr2("0.0.0.0");
                init = True;
        }

        *ip = ipzero;
}

#if (defined(HAVE_NETGROUP) && defined(WITH_AUTOMOUNT))
/******************************************************************
 Remove any mount options such as -rsize=2048,wsize=2048 etc.
 Based on a fix from <Thomas.Hepper@icem.de>.
*******************************************************************/

static void strip_mount_options( pstring *str)
{
	if (**str == '-') { 
		char *p = *str;
		while(*p && !isspace(*p))
			p++;
		while(*p && isspace(*p))
			p++;
		if(*p) {
			pstring tmp_str;

			pstrcpy(tmp_str, p);
			pstrcpy(*str, tmp_str);
		}
	}
}

/*******************************************************************
 Patch from jkf@soton.ac.uk
 Split Luke's automount_server into YP lookup and string splitter
 so can easily implement automount_path(). 
 As we may end up doing both, cache the last YP result. 
*******************************************************************/

#ifdef WITH_NISPLUS_HOME
char *automount_lookup(const char *user_name)
{
	static fstring last_key = "";
	static pstring last_value = "";
 
	char *nis_map = (char *)lp_nis_home_map_name();
 
	char buffer[NIS_MAXATTRVAL + 1];
	nis_result *result;
	nis_object *object;
	entry_obj  *entry;
 
	if (strcmp(user_name, last_key)) {
		slprintf(buffer, sizeof(buffer)-1, "[key=%s],%s", user_name, nis_map);
		DEBUG(5, ("NIS+ querystring: %s\n", buffer));
 
		if (result = nis_list(buffer, FOLLOW_PATH|EXPAND_NAME|HARD_LOOKUP, NULL, NULL)) {
			if (result->status != NIS_SUCCESS) {
				DEBUG(3, ("NIS+ query failed: %s\n", nis_sperrno(result->status)));
				fstrcpy(last_key, ""); pstrcpy(last_value, "");
			} else {
				object = result->objects.objects_val;
				if (object->zo_data.zo_type == ENTRY_OBJ) {
					entry = &object->zo_data.objdata_u.en_data;
					DEBUG(5, ("NIS+ entry type: %s\n", entry->en_type));
					DEBUG(3, ("NIS+ result: %s\n", entry->en_cols.en_cols_val[1].ec_value.ec_value_val));
 
					pstrcpy(last_value, entry->en_cols.en_cols_val[1].ec_value.ec_value_val);
					pstring_sub(last_value, "&", user_name);
					fstrcpy(last_key, user_name);
				}
			}
		}
		nis_freeresult(result);
	}

	strip_mount_options(&last_value);

	DEBUG(4, ("NIS+ Lookup: %s resulted in %s\n", user_name, last_value));
	return last_value;
}
#else /* WITH_NISPLUS_HOME */

char *automount_lookup(const char *user_name)
{
	static fstring last_key = "";
	static pstring last_value = "";

	int nis_error;        /* returned by yp all functions */
	char *nis_result;     /* yp_match inits this */
	int nis_result_len;  /* and set this */
	char *nis_domain;     /* yp_get_default_domain inits this */
	char *nis_map = (char *)lp_nis_home_map_name();

	if ((nis_error = yp_get_default_domain(&nis_domain)) != 0) {
		DEBUG(3, ("YP Error: %s\n", yperr_string(nis_error)));
		return last_value;
	}

	DEBUG(5, ("NIS Domain: %s\n", nis_domain));

	if (!strcmp(user_name, last_key)) {
		nis_result = last_value;
		nis_result_len = strlen(last_value);
		nis_error = 0;
  	} else {
		if ((nis_error = yp_match(nis_domain, nis_map, user_name, strlen(user_name),
				&nis_result, &nis_result_len)) == 0) {
			fstrcpy(last_key, user_name);
			pstrcpy(last_value, nis_result);
			strip_mount_options(&last_value);

		} else if(nis_error == YPERR_KEY) {

			/* If Key lookup fails user home server is not in nis_map 
				use default information for server, and home directory */
			last_value[0] = 0;
			DEBUG(3, ("YP Key not found:  while looking up \"%s\" in map \"%s\"\n", 
					user_name, nis_map));
			DEBUG(3, ("using defaults for server and home directory\n"));
		} else {
			DEBUG(3, ("YP Error: \"%s\" while looking up \"%s\" in map \"%s\"\n", 
					yperr_string(nis_error), user_name, nis_map));
		}
	}

	DEBUG(4, ("YP Lookup: %s resulted in %s\n", user_name, last_value));
	return last_value;
}
#endif /* WITH_NISPLUS_HOME */
#endif

/*******************************************************************
 Are two IPs on the same subnet?
********************************************************************/

BOOL same_net(struct in_addr ip1,struct in_addr ip2,struct in_addr mask)
{
	uint32 net1,net2,nmask;

	nmask = ntohl(mask.s_addr);
	net1  = ntohl(ip1.s_addr);
	net2  = ntohl(ip2.s_addr);
            
	return((net1 & nmask) == (net2 & nmask));
}


/****************************************************************************
 Check if a process exists. Does this work on all unixes?
****************************************************************************/

BOOL process_exists(const struct process_id pid)
{
#ifdef _XBOX
	if (getpid() == pid.pid) return True;
	else return False;
#elif

	if (!procid_is_local(&pid)) {
		/* This *SEVERELY* needs fixing. */
		return True;
	}

	/* Doing kill with a non-positive pid causes messages to be
	 * sent to places we don't want. */
	SMB_ASSERT(pid.pid > 0);
	return(kill(pid.pid,0) == 0 || errno != ESRCH);
#endif
}

BOOL process_exists_by_pid(pid_t pid)
{
	return process_exists(pid_to_procid(pid));
}

/*******************************************************************
 Convert a uid into a user name.
********************************************************************/

const char *uidtoname(uid_t uid)
{
	static fstring name;
	struct passwd *pass;

	pass = getpwuid_alloc(NULL, uid);
	if (pass) {
		fstrcpy(name, pass->pw_name);
		TALLOC_FREE(pass);
	} else {
		slprintf(name, sizeof(name) - 1, "%ld",(long int)uid);
	}
	return name;
}


/*******************************************************************
 Convert a gid into a group name.
********************************************************************/

char *gidtoname(gid_t gid)
{
	OutputDebugString("gidtoname not supported\n");
#ifndef _XBOX
	static fstring name;
	struct group *grp;

	grp = getgrgid(gid);
	if (grp)
		return(grp->gr_name);
	slprintf(name,sizeof(name) - 1, "%d",(int)gid);
	return(name);
#endif //_XBOX
	return "";
}

/*******************************************************************
 Convert a user name into a uid. 
********************************************************************/

uid_t nametouid(const char *name)
{
	struct passwd *pass;
	char *p;
	uid_t u;

	pass = getpwnam_alloc(NULL, name);
	if (pass) {
		u = pass->pw_uid;
		TALLOC_FREE(pass);
		return u;
	}

	u = (uid_t)strtol(name, &p, 0);
	if ((p != name) && (*p == '\0'))
		return u;

	return (uid_t)-1;
}

/*******************************************************************
 Convert a name to a gid_t if possible. Return -1 if not a group. 
********************************************************************/

gid_t nametogid(const char *name)
{
	OutputDebugString("nametogid not supported\n");
#ifndef _XBOX
	struct group *grp;
	char *p;
	gid_t g;

	g = (gid_t)strtol(name, &p, 0);
	if ((p != name) && (*p == '\0'))
		return g;

	grp = sys_getgrnam(name);
	if (grp)
		return(grp->gr_gid);
	return (gid_t)-1;
#endif //_XBOX
	return -1;
}


/*******************************************************************
 Something really nasty happened - panic !
********************************************************************/

void smb_panic(const char *const why)
{
	OutputDebugString("Todo: smb_panic\n");
#ifndef _XBOX
	char *cmd;
	int result;

#ifdef DEVELOPER
	{

		if (global_clobber_region_function) {
			DEBUG(0,("smb_panic: clobber_region() last called from [%s(%u)]\n",
					 global_clobber_region_function,
					 global_clobber_region_line));
		} 
	}
#endif

	DEBUG(0,("PANIC (pid %llu): %s\n",
		    (unsigned long long)sys_getpid(), why));
	log_stack_trace();

	/* only smbd needs to decrement the smbd counter in connections.tdb */
	decrement_smbd_process_count();

	cmd = lp_panic_action();
	if (cmd && *cmd) {
		DEBUG(0, ("smb_panic(): calling panic action [%s]\n", cmd));
		result = system(cmd);

		if (result == -1)
			DEBUG(0, ("smb_panic(): fork failed in panic action: %s\n",
					  strerror(errno)));
		else
			DEBUG(0, ("smb_panic(): action returned status %d\n",
					  WEXITSTATUS(result)));
	}

	dump_core();
}

/*******************************************************************
 Print a backtrace of the stack to the debug log. This function
 DELIBERATELY LEAKS MEMORY. The expectation is that you should
 exit shortly after calling it.
********************************************************************/

#ifdef HAVE_LIBUNWIND_H
#include <libunwind.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_LIBEXC_H
#include <libexc.h>
#endif

void log_stack_trace(void)
{
#ifdef HAVE_LIBUNWIND
	/* Try to use libunwind before any other technique since on ia64
	 * libunwind correctly walks the stack in more circumstances than
	 * backtrace.
	 */ 
	unw_cursor_t cursor;
	unw_context_t uc;
	unsigned i = 0;

	char procname[256];
	unw_word_t ip, sp, off;

	procname[sizeof(procname) - 1] = '\0';

	if (unw_getcontext(&uc) != 0) {
		goto libunwind_failed;
	}

	if (unw_init_local(&cursor, &uc) != 0) {
		goto libunwind_failed;
	}

	DEBUG(0, ("BACKTRACE:\n"));

	do {
	    ip = sp = 0;
	    unw_get_reg(&cursor, UNW_REG_IP, &ip);
	    unw_get_reg(&cursor, UNW_REG_SP, &sp);

	    switch (unw_get_proc_name(&cursor,
			procname, sizeof(procname) - 1, &off) ) {
	    case 0:
		    /* Name found. */
	    case -UNW_ENOMEM:
		    /* Name truncated. */
		    DEBUGADD(0, (" #%u %s + %#llx [ip=%#llx] [sp=%#llx]\n",
			    i, procname, (long long)off,
			    (long long)ip, (long long) sp));
		    break;
	    default:
	    /* case -UNW_ENOINFO: */
	    /* case -UNW_EUNSPEC: */
		    /* No symbol name found. */
		    DEBUGADD(0, (" #%u %s [ip=%#llx] [sp=%#llx]\n",
			    i, "<unknown symbol>",
			    (long long)ip, (long long) sp));
	    }
	    ++i;
	} while (unw_step(&cursor) > 0);

	return;

libunwind_failed:
	DEBUG(0, ("unable to produce a stack trace with libunwind\n"));

#elif HAVE_BACKTRACE_SYMBOLS
	void *backtrace_stack[BACKTRACE_STACK_SIZE];
	size_t backtrace_size;
	char **backtrace_strings;

	/* get the backtrace (stack frames) */
	backtrace_size = backtrace(backtrace_stack,BACKTRACE_STACK_SIZE);
	backtrace_strings = backtrace_symbols(backtrace_stack, backtrace_size);

	DEBUG(0, ("BACKTRACE: %lu stack frames:\n", 
		  (unsigned long)backtrace_size));
	
	if (backtrace_strings) {
		int i;

		for (i = 0; i < backtrace_size; i++)
			DEBUGADD(0, (" #%u %s\n", i, backtrace_strings[i]));

		/* Leak the backtrace_strings, rather than risk what free() might do */
	}

#elif HAVE_LIBEXC

	/* The IRIX libexc library provides an API for unwinding the stack. See
	 * libexc(3) for details. Apparantly trace_back_stack leaks memory, but
	 * since we are about to abort anyway, it hardly matters.
	 */

#define NAMESIZE 32 /* Arbitrary */

	__uint64_t	addrs[BACKTRACE_STACK_SIZE];
	char *      	names[BACKTRACE_STACK_SIZE];
	char		namebuf[BACKTRACE_STACK_SIZE * NAMESIZE];

	int		i;
	int		levels;

	ZERO_ARRAY(addrs);
	ZERO_ARRAY(names);
	ZERO_ARRAY(namebuf);

	/* We need to be root so we can open our /proc entry to walk
	 * our stack. It also helps when we want to dump core.
	 */
	become_root();

	for (i = 0; i < BACKTRACE_STACK_SIZE; i++) {
		names[i] = namebuf + (i * NAMESIZE);
	}

	levels = trace_back_stack(0, addrs, names,
			BACKTRACE_STACK_SIZE, NAMESIZE - 1);

	DEBUG(0, ("BACKTRACE: %d stack frames:\n", levels));
	for (i = 0; i < levels; i++) {
		DEBUGADD(0, (" #%d 0x%llx %s\n", i, addrs[i], names[i]));
	}
#undef NAMESIZE

#else
	DEBUG(0, ("unable to produce a stack trace on this platform\n"));
#endif
#endif //_XBOX
}

/*******************************************************************
  A readdir wrapper which just returns the file name.
 ********************************************************************/

const char *readdirname(SMB_STRUCT_DIR *p)
{
	SMB_STRUCT_DIRENT *ptr;
	char *dname;

	if (!p)
		return(NULL);
  
	ptr = (SMB_STRUCT_DIRENT *)sys_readdir(p);
	if (!ptr)
		return(NULL);

	dname = ptr->d_name;

#ifdef NEXT2
	if (telldir(p) < 0)
		return(NULL);
#endif

#ifdef HAVE_BROKEN_READDIR_NAME
	/* using /usr/ucb/cc is BAD */
	dname = dname - 2;
#endif

	{
		static pstring buf;
		int len = NAMLEN(ptr);
		memcpy(buf, dname, len);
		buf[len] = 0;
		dname = buf;
	}

	return(dname);
}

/*******************************************************************
 Utility function used to decide if the last component 
 of a path matches a (possibly wildcarded) entry in a namelist.
********************************************************************/

BOOL is_in_path(const char *name, name_compare_entry *namelist, BOOL case_sensitive)
{
	pstring last_component;
	char *p;

	/* if we have no list it's obviously not in the path */
	if((namelist == NULL ) || ((namelist != NULL) && (namelist[0].name == NULL))) {
		return False;
	}

	DEBUG(8, ("is_in_path: %s\n", name));

	/* Get the last component of the unix name. */
	p = strrchr_m(name, '/');
	pstrcpy(last_component, p ? ++p : name);

	for(; namelist->name != NULL; namelist++) {
		if(namelist->is_wild) {
			if (mask_match(last_component, namelist->name, case_sensitive)) {
				DEBUG(8,("is_in_path: mask match succeeded\n"));
				return True;
			}
		} else {
			if((case_sensitive && (strcmp(last_component, namelist->name) == 0))||
						(!case_sensitive && (StrCaseCmp(last_component, namelist->name) == 0))) {
				DEBUG(8,("is_in_path: match succeeded\n"));
				return True;
			}
		}
	}
	DEBUG(8,("is_in_path: match not found\n"));
 
	return False;
}

/*******************************************************************
 Strip a '/' separated list into an array of 
 name_compare_enties structures suitable for 
 passing to is_in_path(). We do this for
 speed so we can pre-parse all the names in the list 
 and don't do it for each call to is_in_path().
 namelist is modified here and is assumed to be 
 a copy owned by the caller.
 We also check if the entry contains a wildcard to
 remove a potentially expensive call to mask_match
 if possible.
********************************************************************/
 
void set_namearray(name_compare_entry **ppname_array, char *namelist)
{
	char *name_end;
	char *nameptr = namelist;
	int num_entries = 0;
	int i;

	(*ppname_array) = NULL;

	if((nameptr == NULL ) || ((nameptr != NULL) && (*nameptr == '\0'))) 
		return;

	/* We need to make two passes over the string. The
		first to count the number of elements, the second
		to split it.
	*/

	while(*nameptr) {
		if ( *nameptr == '/' ) {
			/* cope with multiple (useless) /s) */
			nameptr++;
			continue;
		}
		/* find the next / */
		name_end = strchr_m(nameptr, '/');

		/* oops - the last check for a / didn't find one. */
		if (name_end == NULL)
			break;

		/* next segment please */
		nameptr = name_end + 1;
		num_entries++;
	}

	if(num_entries == 0)
		return;

	if(( (*ppname_array) = SMB_MALLOC_ARRAY(name_compare_entry, num_entries + 1)) == NULL) {
		DEBUG(0,("set_namearray: malloc fail\n"));
		return;
	}

	/* Now copy out the names */
	nameptr = namelist;
	i = 0;
	while(*nameptr) {
		if ( *nameptr == '/' ) {
			/* cope with multiple (useless) /s) */
			nameptr++;
			continue;
		}
		/* find the next / */
		if ((name_end = strchr_m(nameptr, '/')) != NULL)
			*name_end = 0;

		/* oops - the last check for a / didn't find one. */
		if(name_end == NULL) 
			break;

		(*ppname_array)[i].is_wild = ms_has_wild(nameptr);
		if(((*ppname_array)[i].name = SMB_STRDUP(nameptr)) == NULL) {
			DEBUG(0,("set_namearray: malloc fail (1)\n"));
			return;
		}

		/* next segment please */
		nameptr = name_end + 1;
		i++;
	}
  
	(*ppname_array)[i].name = NULL;

	return;
}

/****************************************************************************
 Routine to free a namearray.
****************************************************************************/

void free_namearray(name_compare_entry *name_array)
{
	int i;

	if(name_array == NULL)
		return;

	for(i=0; name_array[i].name!=NULL; i++)
		SAFE_FREE(name_array[i].name);
	SAFE_FREE(name_array);
}

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_LOCKING

/****************************************************************************
 Simple routine to do POSIX file locking. Cruft in NFS and 64->32 bit mapping
 is dealt with in posix.c
 Returns True if the lock was granted, False otherwise.
****************************************************************************/

BOOL fcntl_lock(int fd, int op, SMB_OFF_T offset, SMB_OFF_T count, int type)
{
	OutputDebugString("fcntl_lock not supported\n");
#ifndef _XBOX
	SMB_STRUCT_FLOCK lock;
	int ret;

	DEBUG(8,("fcntl_lock fd=%d op=%d offset=%.0f count=%.0f type=%d\n",
		fd,op,(double)offset,(double)count,type));

	lock.l_type = type;
	lock.l_whence = SEEK_SET;
	lock.l_start = offset;
	lock.l_len = count;
	lock.l_pid = 0;

	ret = sys_fcntl_ptr(fd,op,&lock);

	if (ret == -1) {
		int sav = errno;
		DEBUG(3,("fcntl_lock: lock failed at offset %.0f count %.0f op %d type %d (%s)\n",
			(double)offset,(double)count,op,type,strerror(errno)));
		errno = sav;
		return False;
	}

	/* everything went OK */
	DEBUG(8,("fcntl_lock: Lock call successful\n"));
#endif //_XBOX
	return True;
}

/****************************************************************************
 Simple routine to query existing file locks. Cruft in NFS and 64->32 bit mapping
 is dealt with in posix.c
 Returns True if we have information regarding this lock region (and returns
 F_UNLCK in *ptype if the region is unlocked). False if the call failed.
****************************************************************************/

BOOL fcntl_getlock(int fd, SMB_OFF_T *poffset, SMB_OFF_T *pcount, int *ptype, pid_t *ppid)
{
	OutputDebugString("fcntl_getlock not supported\n");
#ifndef _XBOX

	SMB_STRUCT_FLOCK lock;
	int ret;

	DEBUG(8,("fcntl_getlock fd=%d offset=%.0f count=%.0f type=%d\n",
		    fd,(double)*poffset,(double)*pcount,*ptype));

	lock.l_type = *ptype;
	lock.l_whence = SEEK_SET;
	lock.l_start = *poffset;
	lock.l_len = *pcount;
	lock.l_pid = 0;

	ret = sys_fcntl_ptr(fd,SMB_F_GETLK,&lock);

	if (ret == -1) {
		int sav = errno;
		DEBUG(3,("fcntl_getlock: lock request failed at offset %.0f count %.0f type %d (%s)\n",
			(double)*poffset,(double)*pcount,*ptype,strerror(errno)));
		errno = sav;
		return False;
	}

	*ptype = lock.l_type;
	*poffset = lock.l_start;
	*pcount = lock.l_len;
	*ppid = lock.l_pid;
	
	DEBUG(3,("fcntl_getlock: fd %d is returned info %d pid %u\n",
			fd, (int)lock.l_type, (unsigned int)lock.l_pid));

#endif //XBOX
	return True;
}

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_ALL

/*******************************************************************
 Is the name specified one of my netbios names.
 Returns true if it is equal, false otherwise.
********************************************************************/

BOOL is_myname(const char *s)
{
	int n;
	BOOL ret = False;

	for (n=0; my_netbios_names(n); n++) {
		if (strequal(my_netbios_names(n), s)) {
			ret=True;
			break;
		}
	}
	DEBUG(8, ("is_myname(\"%s\") returns %d\n", s, ret));
	return(ret);
}

BOOL is_myname_or_ipaddr(const char *s)
{
	fstring name, dnsname;
	char *servername;

	if ( !s )
		return False;

	/* santize the string from '\\name' */

	fstrcpy( name, s );

	servername = strrchr_m( name, '\\' );
	if ( !servername )
		servername = name;
	else
		servername++;

	/* optimize for the common case */

	if (strequal(servername, global_myname())) 
		return True;

	/* check for an alias */

	if (is_myname(servername))
		return True;

	/* check for loopback */

	if (strequal(servername, "localhost")) 
		return True;

	/* maybe it's my dns name */

	if ( get_mydnsfullname( dnsname ) )
		if ( strequal( servername, dnsname ) )
			return True;
		
	/* handle possible CNAME records */

	if ( !is_ipaddress( servername ) ) {
		/* use DNS to resolve the name, but only the first address */
		struct hostent *hp;

		if (((hp = sys_gethostbyname(name)) != NULL) && (hp->h_addr != NULL)) {
			struct in_addr return_ip;
			putip( (char*)&return_ip, (char*)hp->h_addr );
			fstrcpy( name, inet_ntoa( return_ip ) );
			servername = name;
		}	
	}
		
	/* maybe its an IP address? */
	if (is_ipaddress(servername)) {
		struct iface_struct nics[MAX_INTERFACES];
		int i, n;
		uint32 ip;
		
		ip = interpret_addr(servername);
		if ((ip==0) || (ip==0xffffffff))
			return False;
			
		n = get_interfaces(nics, MAX_INTERFACES);
		for (i=0; i<n; i++) {
			if (ip == nics[i].ip.s_addr)
				return True;
		}
	}	

	/* no match */
	return False;
}

/*******************************************************************
 Is the name specified our workgroup/domain.
 Returns true if it is equal, false otherwise.
********************************************************************/

BOOL is_myworkgroup(const char *s)
{
	BOOL ret = False;

	if (strequal(s, lp_workgroup())) {
		ret=True;
	}

	DEBUG(8, ("is_myworkgroup(\"%s\") returns %d\n", s, ret));
	return(ret);
}

/*******************************************************************
 we distinguish between 2K and XP by the "Native Lan Manager" string
   WinXP => "Windows 2002 5.1"
   Win2k => "Windows 2000 5.0"
   NT4   => "Windows NT 4.0" 
   Win9x => "Windows 4.0"
 Windows 2003 doesn't set the native lan manager string but 
 they do set the domain to "Windows 2003 5.2" (probably a bug).
********************************************************************/

void ra_lanman_string( const char *native_lanman )
{		 
	if ( strcmp( native_lanman, "Windows 2002 5.1" ) == 0 )
		set_remote_arch( RA_WINXP );
	else if ( strcmp( native_lanman, "Windows Server 2003 5.2" ) == 0 )
		set_remote_arch( RA_WIN2K3 );
}

/*******************************************************************
 Set the horrid remote_arch string based on an enum.
********************************************************************/

void set_remote_arch(enum remote_arch_types type)
{
	ra_type = type;
	switch( type ) {
	case RA_WFWG:
		fstrcpy(remote_arch, "WfWg");
		break;
	case RA_OS2:
		fstrcpy(remote_arch, "OS2");
		break;
	case RA_WIN95:
		fstrcpy(remote_arch, "Win95");
		break;
	case RA_WINNT:
		fstrcpy(remote_arch, "WinNT");
		break;
	case RA_WIN2K:
		fstrcpy(remote_arch, "Win2K");
		break;
	case RA_WINXP:
		fstrcpy(remote_arch, "WinXP");
		break;
	case RA_WIN2K3:
		fstrcpy(remote_arch, "Win2K3");
		break;
	case RA_SAMBA:
		fstrcpy(remote_arch,"Samba");
		break;
	case RA_CIFSFS:
		fstrcpy(remote_arch,"CIFSFS");
		break;
	default:
		ra_type = RA_UNKNOWN;
		fstrcpy(remote_arch, "UNKNOWN");
		break;
	}

	DEBUG(10,("set_remote_arch: Client arch is \'%s\'\n", remote_arch));
}

/*******************************************************************
 Get the remote_arch type.
********************************************************************/

enum remote_arch_types get_remote_arch(void)
{
	return ra_type;
}

void print_asc(int level, const unsigned char *buf,int len)
{
	int i;
	for (i=0;i<len;i++)
		DEBUG(level,("%c", isprint(buf[i])?buf[i]:'.'));
}

void dump_data(int level, const char *buf1,int len)
{
	const unsigned char *buf = (const unsigned char *)buf1;
	int i=0;
	if (len<=0) return;

	if (!DEBUGLVL(level)) return;
	
	DEBUGADD(level,("[%03X] ",i));
	for (i=0;i<len;) {
		DEBUGADD(level,("%02X ",(int)buf[i]));
		i++;
		if (i%8 == 0) DEBUGADD(level,(" "));
		if (i%16 == 0) {      
			print_asc(level,&buf[i-16],8); DEBUGADD(level,(" "));
			print_asc(level,&buf[i-8],8); DEBUGADD(level,("\n"));
			if (i<len) DEBUGADD(level,("[%03X] ",i));
		}
	}
	if (i%16) {
		int n;
		n = 16 - (i%16);
		DEBUGADD(level,(" "));
		if (n>8) DEBUGADD(level,(" "));
		while (n--) DEBUGADD(level,("   "));
		n = MIN(8,i%16);
		print_asc(level,&buf[i-(i%16)],n); DEBUGADD(level,( " " ));
		n = (i%16) - n;
		if (n>0) print_asc(level,&buf[i-n],n); 
		DEBUGADD(level,("\n"));    
	}	
}

void dump_data_pw(const char *msg, const uchar * data, size_t len)
{
#ifdef DEBUG_PASSWORD
	DEBUG(11, ("%s", msg));
	if (data != NULL && len > 0)
	{
		dump_data(11, (const char *)data, len);
	}
#endif
}

char *tab_depth(int depth)
{
	static pstring spaces;
	memset(spaces, ' ', depth * 4);
	spaces[depth * 4] = 0;
	return spaces;
}

/*****************************************************************************
 Provide a checksum on a string

 Input:  s - the null-terminated character string for which the checksum
             will be calculated.

  Output: The checksum value calculated for s.
*****************************************************************************/

int str_checksum(const char *s)
{
	int res = 0;
	int c;
	int i=0;
	
	while(*s) {
		c = *s;
		res ^= (c << (i % 15)) ^ (c >> (15-(i%15)));
		s++;
		i++;
	}
	return(res);
}

/*****************************************************************
 Zero a memory area then free it. Used to catch bugs faster.
*****************************************************************/  

void zero_free(void *p, size_t size)
{
	memset(p, 0, size);
	SAFE_FREE(p);
}

/*****************************************************************
 Set our open file limit to a requested max and return the limit.
*****************************************************************/  

int set_maxfiles(int requested_max)
{
#if (defined(HAVE_GETRLIMIT) && defined(RLIMIT_NOFILE))
	struct rlimit rlp;
	int saved_current_limit;

	if(getrlimit(RLIMIT_NOFILE, &rlp)) {
		DEBUG(0,("set_maxfiles: getrlimit (1) for RLIMIT_NOFILE failed with error %s\n",
			strerror(errno) ));
		/* just guess... */
		return requested_max;
	}

	/* 
	 * Set the fd limit to be real_max_open_files + MAX_OPEN_FUDGEFACTOR to
	 * account for the extra fd we need 
	 * as well as the log files and standard
	 * handles etc. Save the limit we want to set in case
	 * we are running on an OS that doesn't support this limit (AIX)
	 * which always returns RLIM_INFINITY for rlp.rlim_max.
	 */

	/* Try raising the hard (max) limit to the requested amount. */

#if defined(RLIM_INFINITY)
	if (rlp.rlim_max != RLIM_INFINITY) {
		int orig_max = rlp.rlim_max;

		if ( rlp.rlim_max < requested_max )
			rlp.rlim_max = requested_max;

		/* This failing is not an error - many systems (Linux) don't
			support our default request of 10,000 open files. JRA. */

		if(setrlimit(RLIMIT_NOFILE, &rlp)) {
			DEBUG(3,("set_maxfiles: setrlimit for RLIMIT_NOFILE for %d max files failed with error %s\n", 
				(int)rlp.rlim_max, strerror(errno) ));

			/* Set failed - restore original value from get. */
			rlp.rlim_max = orig_max;
		}
	}
#endif

	/* Now try setting the soft (current) limit. */

	saved_current_limit = rlp.rlim_cur = MIN(requested_max,rlp.rlim_max);

	if(setrlimit(RLIMIT_NOFILE, &rlp)) {
		DEBUG(0,("set_maxfiles: setrlimit for RLIMIT_NOFILE for %d files failed with error %s\n", 
			(int)rlp.rlim_cur, strerror(errno) ));
		/* just guess... */
		return saved_current_limit;
	}

	if(getrlimit(RLIMIT_NOFILE, &rlp)) {
		DEBUG(0,("set_maxfiles: getrlimit (2) for RLIMIT_NOFILE failed with error %s\n",
			strerror(errno) ));
		/* just guess... */
		return saved_current_limit;
    }

#if defined(RLIM_INFINITY)
	if(rlp.rlim_cur == RLIM_INFINITY)
		return saved_current_limit;
#endif

	if((int)rlp.rlim_cur > saved_current_limit)
		return saved_current_limit;

	return rlp.rlim_cur;
#else /* !defined(HAVE_GETRLIMIT) || !defined(RLIMIT_NOFILE) */
	/*
	 * No way to know - just guess...
	 */
	return requested_max;
#endif
}

/*****************************************************************
 Possibly replace mkstemp if it is broken.
*****************************************************************/  

int smb_mkstemp(char *name_template)
{
#if HAVE_SECURE_MKSTEMP
	return mkstemp(name_template);
#else
	/* have a reasonable go at emulating it. Hope that
	   the system mktemp() isn't completly hopeless */
	char *p = mktemp(name_template);
	if (!p)
		return -1;
	return open(p, O_CREAT|O_EXCL|O_RDWR, 0600);
#endif
}

/*****************************************************************
 malloc that aborts with smb_panic on fail or zero size.
 *****************************************************************/  

void *smb_xmalloc_array(size_t size, unsigned int count)
{
	void *p;
	if (size == 0)
		smb_panic("smb_xmalloc_array: called with zero size.\n");
        if (count >= MAX_ALLOC_SIZE/size) {
                smb_panic("smb_xmalloc: alloc size too large.\n");
        }
	if ((p = SMB_MALLOC(size*count)) == NULL) {
		DEBUG(0, ("smb_xmalloc_array failed to allocate %lu * %lu bytes\n",
			(unsigned long)size, (unsigned long)count));
		smb_panic("smb_xmalloc_array: malloc fail.\n");
	}
	return p;
}

/**
 Memdup with smb_panic on fail.
**/

void *smb_xmemdup(const void *p, size_t size)
{
	void *p2;
	p2 = SMB_XMALLOC_ARRAY(unsigned char,size);
	memcpy(p2, p, size);
	return p2;
}

/**
 strdup that aborts on malloc fail.
**/

char *smb_xstrdup(const char *s)
{
#if defined(PARANOID_MALLOC_CHECKER)
#ifdef strdup
#undef strdup
#endif
#endif
	char *s1 = strdup(s);
#if defined(PARANOID_MALLOC_CHECKER)
#define strdup(s) __ERROR_DONT_USE_STRDUP_DIRECTLY
#endif
	if (!s1)
		smb_panic("smb_xstrdup: malloc fail\n");
	return s1;

}

/**
 strndup that aborts on malloc fail.
**/

char *smb_xstrndup(const char *s, size_t n)
{
#if defined(PARANOID_MALLOC_CHECKER)
#ifdef strndup
#undef strndup
#endif
#endif
	char *s1 = strndup(s, n);
#if defined(PARANOID_MALLOC_CHECKER)
#define strndup(s,n) __ERROR_DONT_USE_STRNDUP_DIRECTLY
#endif
	if (!s1)
		smb_panic("smb_xstrndup: malloc fail\n");
	return s1;
}

/*
  vasprintf that aborts on malloc fail
*/

 int smb_xvasprintf(char **ptr, const char *format, va_list ap)
{
	int n;
	va_list ap2;

	VA_COPY(ap2, ap);

	n = vasprintf(ptr, format, ap2);
	if (n == -1 || ! *ptr)
		smb_panic("smb_xvasprintf: out of memory");
	return n;
}

/*****************************************************************
 Like strdup but for memory.
*****************************************************************/  

void *memdup(const void *p, size_t size)
{
	void *p2;
	if (size == 0)
		return NULL;
	p2 = SMB_MALLOC(size);
	if (!p2)
		return NULL;
	memcpy(p2, p, size);
	return p2;
}

/*****************************************************************
 Get local hostname and cache result.
*****************************************************************/  

char *myhostname(void)
{
	static pstring ret;
	if (ret[0] == 0)
		get_myname(ret);
	return ret;
}

/*****************************************************************
 A useful function for returning a path in the Samba lock directory.
*****************************************************************/  

char *lock_path(const char *name)
{
	static pstring fname;

	pstrcpy(fname,lp_lockdir());
	trim_char(fname,'\0','/');
	
	if (!directory_exist(fname,NULL))
#ifdef _XBOX
		CreateDirectory(fname, NULL);
	pstrcat(fname,"\\");
#elif
		mkdir(fname,0755);
	pstrcat(fname,"/");
#endif //_XBOX
	
	pstrcat(fname,"/");
	pstrcat(fname,name);

	return fname;
}

/*****************************************************************
 A useful function for returning a path in the Samba pid directory.
*****************************************************************/

char *pid_path(const char *name)
{
	static pstring fname;

	pstrcpy(fname,lp_piddir());
	trim_char(fname,'\0','/');

	if (!directory_exist(fname,NULL))
#ifdef _XBOX
		CreateDirectory(fname, NULL);
#elif
		mkdir(fname,0755);
#endif //_XBOX

	pstrcat(fname,"/");
	pstrcat(fname,name);

	return fname;
}

/**
 * @brief Returns an absolute path to a file in the Samba lib directory.
 *
 * @param name File to find, relative to LIBDIR.
 *
 * @retval Pointer to a static #pstring containing the full path.
 **/

char *lib_path(const char *name)
{
	static pstring fname;
#ifdef _XBOX
	fstr_sprintf(fname, "%s\\%s", dyn_LIBDIR, name);
#elif
	fstr_sprintf(fname, "%s/%s", dyn_LIBDIR, name);
#endif
	return fname;
}

/**
 * @brief Returns the platform specific shared library extension.
 *
 * @retval Pointer to a static #fstring containing the extension.
 **/

const char *shlib_ext(void)
{
  return dyn_SHLIBEXT;
}

/*******************************************************************
 Given a filename - get its directory name
 NB: Returned in static storage.  Caveats:
 o  Not safe in thread environment.
 o  Caller must not free.
 o  If caller wishes to preserve, they should copy.
********************************************************************/

char *parent_dirname(const char *path)
{
	static pstring dirpath;
	char *p;

	if (!path)
		return(NULL);

	pstrcpy(dirpath, path);
	p = strrchr_m(dirpath, '/');  /* Find final '/', if any */
	if (!p) {
		pstrcpy(dirpath, ".");    /* No final "/", so dir is "." */
	} else {
		if (p == dirpath)
			++p;    /* For root "/", leave "/" in place */
		*p = '\0';
	}
	return dirpath;
}


/*******************************************************************
 Determine if a pattern contains any Microsoft wildcard characters.
*******************************************************************/

BOOL ms_has_wild(const char *s)
{
	char c;

	if (lp_posix_pathnames()) {
		/* With posix pathnames no characters are wild. */
		return False;
	}

	while ((c = *s++)) {
		switch (c) {
		case '*':
		case '?':
		case '<':
		case '>':
		case '"':
			return True;
		}
	}
	return False;
}

BOOL ms_has_wild_w(const smb_ucs2_t *s)
{
	smb_ucs2_t c;
	if (!s) return False;
	while ((c = *s++)) {
		switch (c) {
		case UCS2_CHAR('*'):
		case UCS2_CHAR('?'):
		case UCS2_CHAR('<'):
		case UCS2_CHAR('>'):
		case UCS2_CHAR('"'):
			return True;
		}
	}
	return False;
}

/*******************************************************************
 A wrapper that handles case sensitivity and the special handling
 of the ".." name.
*******************************************************************/

BOOL mask_match(const char *string, char *pattern, BOOL is_case_sensitive)
{
	if (strcmp(string,"..") == 0)
		string = ".";
	if (strcmp(pattern,".") == 0)
		return False;
	
	return ms_fnmatch(pattern, string, Protocol <= PROTOCOL_LANMAN2, is_case_sensitive) == 0;
}

/*******************************************************************
 A wrapper that handles case sensitivity and the special handling
 of the ".." name. Varient that is only called by old search code which requires
 pattern translation.
*******************************************************************/

BOOL mask_match_search(const char *string, char *pattern, BOOL is_case_sensitive)
{
	if (strcmp(string,"..") == 0)
		string = ".";
	if (strcmp(pattern,".") == 0)
		return False;
	
	return ms_fnmatch(pattern, string, True, is_case_sensitive) == 0;
}

/*******************************************************************
 A wrapper that handles a list of patters and calls mask_match()
 on each.  Returns True if any of the patterns match.
*******************************************************************/

BOOL mask_match_list(const char *string, char **list, int listLen, BOOL is_case_sensitive)
{
       while (listLen-- > 0) {
               if (mask_match(string, *list++, is_case_sensitive))
                       return True;
       }
       return False;
}

/*********************************************************
 Recursive routine that is called by unix_wild_match.
*********************************************************/

static BOOL unix_do_match(const char *regexp, const char *str)
{
	const char *p;

	for( p = regexp; *p && *str; ) {

		switch(*p) {
			case '?':
				str++;
				p++;
				break;

			case '*':

				/*
				 * Look for a character matching 
				 * the one after the '*'.
				 */
				p++;
				if(!*p)
					return True; /* Automatic match */
				while(*str) {

					while(*str && (*p != *str))
						str++;

					/*
					 * Patch from weidel@multichart.de. In the case of the regexp
					 * '*XX*' we want to ensure there are at least 2 'X' characters
					 * in the string after the '*' for a match to be made.
					 */

					{
						int matchcount=0;

						/*
						 * Eat all the characters that match, but count how many there were.
						 */

						while(*str && (*p == *str)) {
							str++;
							matchcount++;
						}

						/*
						 * Now check that if the regexp had n identical characters that
						 * matchcount had at least that many matches.
						 */

						while ( *(p+1) && (*(p+1) == *p)) {
							p++;
							matchcount--;
						}

						if ( matchcount <= 0 )
							return False;
					}

					str--; /* We've eaten the match char after the '*' */

					if(unix_do_match(p, str))
						return True;

					if(!*str)
						return False;
					else
						str++;
				}
				return False;

			default:
				if(*str != *p)
					return False;
				str++;
				p++;
				break;
		}
	}

	if(!*p && !*str)
		return True;

	if (!*p && str[0] == '.' && str[1] == 0)
		return(True);
  
	if (!*str && *p == '?') {
		while (*p == '?')
			p++;
		return(!*p);
	}

	if(!*str && (*p == '*' && p[1] == '\0'))
		return True;

	return False;
}

/*******************************************************************
 Simple case insensitive interface to a UNIX wildcard matcher.
 Returns True if match, False if not.
*******************************************************************/

BOOL unix_wild_match(const char *pattern, const char *string)
{
	pstring p2, s2;
	char *p;

	pstrcpy(p2, pattern);
	pstrcpy(s2, string);
	strlower_m(p2);
	strlower_m(s2);

	/* Remove any *? and ** from the pattern as they are meaningless */
	for(p = p2; *p; p++)
		while( *p == '*' && (p[1] == '?' ||p[1] == '*'))
			pstrcpy( &p[1], &p[2]);
 
	if (strequal(p2,"*"))
		return True;

	return unix_do_match(p2, s2);
}

/**********************************************************************
 Converts a name to a fully qalified domain name.
***********************************************************************/
                                                                                                                                                   
void name_to_fqdn(fstring fqdn, const char *name)
{
	struct hostent *hp = sys_gethostbyname(name);
	if ( hp && hp->h_name && *hp->h_name ) {
		char *full = NULL;

		/* find out if the fqdn is returned as an alias
		 * to cope with /etc/hosts files where the first
		 * name is not the fqdn but the short name */
		if (hp->h_aliases && (! strchr_m(hp->h_name, '.'))) {
			int i;
			for (i = 0; hp->h_aliases[i]; i++) {
				if (strchr_m(hp->h_aliases[i], '.')) {
					full = hp->h_aliases[i];
					break;
				}
			}
		}
		if (full && (StrCaseCmp(full, "localhost.localdomain") == 0)) {
			DEBUG(1, ("WARNING: your /etc/hosts file may be broken!\n"));
			DEBUGADD(1, ("    Specifing the machine hostname for address 127.0.0.1 may lead\n"));
			DEBUGADD(1, ("    to Kerberos authentication probelms as localhost.localdomain\n"));
			DEBUGADD(1, ("    may end up being used instead of the real machine FQDN.\n"));
			full = hp->h_name;
		}
			
		if (!full) {
			full = hp->h_name;
		}

		DEBUG(10,("name_to_fqdn: lookup for %s -> %s.\n", name, full));
		fstrcpy(fqdn, full);
	} else {
		DEBUG(10,("name_to_fqdn: lookup for %s failed.\n", name));
		fstrcpy(fqdn, name);
	}
}

/**********************************************************************
 Extension to talloc_get_type: Abort on type mismatch
***********************************************************************/

void *talloc_check_name_abort(const void *ptr, const char *name)
{
	void *result;

	result = talloc_check_name(ptr, name);
	if (result != NULL)
		return result;

	DEBUG(0, ("Talloc type mismatch, expected %s, got %s\n",
		  name, talloc_get_name(ptr)));
	smb_panic("aborting");
	/* Keep the compiler happy */
	return NULL;
}


#ifdef __INSURE__

/*******************************************************************
This routine is a trick to immediately catch errors when debugging
with insure. A xterm with a gdb is popped up when insure catches
a error. It is Linux specific.
********************************************************************/

int _Insure_trap_error(int a1, int a2, int a3, int a4, int a5, int a6)
{
	static int (*fn)();
	int ret;
	char pidstr[10];
	/* you can get /usr/bin/backtrace from 
           http://samba.org/ftp/unpacked/junkcode/backtrace */
	pstring cmd = "/usr/bin/backtrace %d";

	slprintf(pidstr, sizeof(pidstr)-1, "%d", sys_getpid());
	pstring_sub(cmd, "%d", pidstr);

	if (!fn) {
		static void *h;
		h = dlopen("/usr/local/parasoft/insure++lite/lib.linux2/libinsure.so", RTLD_LAZY);
		fn = dlsym(h, "_Insure_trap_error");

		if (!h || h == _Insure_trap_error) {
			h = dlopen("/usr/local/parasoft/lib.linux2/libinsure.so", RTLD_LAZY);
			fn = dlsym(h, "_Insure_trap_error");
		}		
	}

	ret = fn(a1, a2, a3, a4, a5, a6);

	system(cmd);

	return ret;
}
#endif

uint32 map_share_mode_to_deny_mode(uint32 share_access, uint32 private_options)
{
	switch (share_access & ~FILE_SHARE_DELETE) {
		case FILE_SHARE_NONE:
			return DENY_ALL;
		case FILE_SHARE_READ:
			return DENY_WRITE;
		case FILE_SHARE_WRITE:
			return DENY_READ;
		case FILE_SHARE_READ|FILE_SHARE_WRITE:
			return DENY_NONE;
	}
	if (private_options & NTCREATEX_OPTIONS_PRIVATE_DENY_DOS) {
		return DENY_DOS;
	} else if (private_options & NTCREATEX_OPTIONS_PRIVATE_DENY_FCB) {
		return DENY_FCB;
	}

	return (uint32)-1;
}

pid_t procid_to_pid(const struct process_id *proc)
{
	return proc->pid;
}

struct process_id pid_to_procid(pid_t pid)
{
	struct process_id result;
	result.pid = pid;
	return result;
}

struct process_id procid_self(void)
{
	return pid_to_procid(sys_getpid());
}

BOOL procid_equal(const struct process_id *p1, const struct process_id *p2)
{
	return (p1->pid == p2->pid);
}

BOOL procid_is_me(const struct process_id *pid)
{
	return (pid->pid == sys_getpid());
}

struct process_id interpret_pid(const char *pid_string)
{
	return pid_to_procid(atoi(pid_string));
}

char *procid_str_static(const struct process_id *pid)
{
	static fstring str;
	fstr_sprintf(str, "%d", pid->pid);
	return str;
}

char *procid_str(TALLOC_CTX *mem_ctx, const struct process_id *pid)
{
	return talloc_strdup(mem_ctx, procid_str_static(pid));
}

BOOL procid_valid(const struct process_id *pid)
{
	return (pid->pid != -1);
}

BOOL procid_is_local(const struct process_id *pid)
{
	return True;
}

int this_is_smp(void)
{
#if defined(HAVE_SYSCONF)

#if defined(SYSCONF_SC_NPROC_ONLN)
        return (sysconf(_SC_NPROC_ONLN) > 1) ? 1 : 0;
#elif defined(SYSCONF_SC_NPROCESSORS_ONLN)
        return (sysconf(_SC_NPROCESSORS_ONLN) > 1) ? 1 : 0;
#else
	return 0;
#endif

#else
	return 0;
#endif
}

