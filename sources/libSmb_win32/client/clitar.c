/* 
   Unix SMB/CIFS implementation.
   Tar Extensions
   Copyright (C) Ricky Poulten 1995-1998
   Copyright (C) Richard Sharpe 1998
   
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
/* The following changes developed by Richard Sharpe for Canon Information
   Systems Research Australia (CISRA)

   1. Restore can now restore files with long file names
   2. Save now saves directory information so that we can restore 
      directory creation times
   3. tar now accepts both UNIX path names and DOS path names. I prefer
      those lovely /'s to those UGLY \'s :-)
   4. the files to exclude can be specified as a regular expression by adding
      an r flag to the other tar flags. Eg:

         -TcrX file.tar "*.(obj|exe)"

      will skip all .obj and .exe files
*/


#include "includes.h"
#include "clitar.h"
#include "client/client_proto.h"

static int clipfind(char **aret, int ret, char *tok);

typedef struct file_info_struct file_info2;

struct file_info_struct {
	SMB_OFF_T size;
	uint16 mode;
	uid_t uid;
	gid_t gid;
	/* These times are normally kept in GMT */
	time_t mtime;
	time_t atime;
	time_t ctime;
	char *name;     /* This is dynamically allocate */

	file_info2 *next, *prev;  /* Used in the stack ... */
};

typedef struct {
	file_info2 *top;
	int items;
} stack;

#define SEPARATORS " \t\n\r"
extern time_t newer_than;
extern struct cli_state *cli;

/* These defines are for the do_setrattr routine, to indicate
 * setting and reseting of file attributes in the function call */
#define ATTRSET 1
#define ATTRRESET 0

static uint16 attribute = aDIR | aSYSTEM | aHIDDEN;

#ifndef CLIENT_TIMEOUT
#define CLIENT_TIMEOUT (30*1000)
#endif

static char *tarbuf, *buffer_p;
static int tp, ntarf, tbufsiz;
static double ttarf;
/* Incremental mode */
static BOOL tar_inc=False;
/* Reset archive bit */
static BOOL tar_reset=False;
/* Include / exclude mode (true=include, false=exclude) */
static BOOL tar_excl=True;
/* use regular expressions for search on file names */
static BOOL tar_re_search=False;
/* Do not dump anything, just calculate sizes */
static BOOL dry_run=False;
/* Dump files with System attribute */
static BOOL tar_system=True;
/* Dump files with Hidden attribute */
static BOOL tar_hidden=True;
/* Be noisy - make a catalogue */
static BOOL tar_noisy=True;
static BOOL tar_real_noisy=False;  /* Don't want to be really noisy by default */

char tar_type='\0';
static char **cliplist=NULL;
static int clipn=0;
static BOOL must_free_cliplist = False;

extern file_info def_finfo;
extern BOOL lowercase;
extern uint16 cnum;
extern BOOL readbraw_supported;
extern int max_xmit;
extern pstring cur_dir;
extern int get_total_time_ms;
extern int get_total_size;

static int blocksize=20;
static int tarhandle;

static void writetarheader(int f,  const char *aname, SMB_BIG_UINT size, time_t mtime,
			   const char *amode, unsigned char ftype);
static void do_atar(char *rname,char *lname,file_info *finfo1);
static void do_tar(file_info *finfo);
static void oct_it(SMB_BIG_UINT value, int ndgs, char *p);
static void fixtarname(char *tptr, const char *fp, size_t l);
static int dotarbuf(int f, char *b, int n);
static void dozerobuf(int f, int n);
static void dotareof(int f);
static void initarbuf(void);

/* restore functions */
static long readtarheader(union hblock *hb, file_info2 *finfo, char *prefix);
static long unoct(char *p, int ndgs);
static void do_tarput(void);
static void unfixtarname(char *tptr, char *fp, int l, BOOL first);

/*
 * tar specific utitlities
 */

/*******************************************************************
Create  a string of size size+1 (for the null)
*******************************************************************/

static char *string_create_s(int size)
{
	char *tmp;

	tmp = (char *)SMB_MALLOC(size+1);

	if (tmp == NULL) {
		DEBUG(0, ("Out of memory in string_create_s\n"));
	}

	return(tmp);
}

/****************************************************************************
Write a tar header to buffer
****************************************************************************/

static void writetarheader(int f, const char *aname, SMB_BIG_UINT size, time_t mtime,
			   const char *amode, unsigned char ftype)
{
	union hblock hb;
	int i, chk, l;
	char *jp;

	DEBUG(5, ("WriteTarHdr, Type = %c, Size= %.0f, Name = %s\n", ftype, (double)size, aname));

	memset(hb.dummy, 0, sizeof(hb.dummy));
  
	l=strlen(aname);
	/* We will be prepending a '.' in fixtarheader so use +2 to
	 * take care of the . and terminating zero. JRA.
	 */
	if (l+2 >= NAMSIZ) {
		/* write a GNU tar style long header */
		char *b;
		b = (char *)SMB_MALLOC(l+TBLOCK+100);
		if (!b) {
			DEBUG(0,("out of memory\n"));
			exit(1);
		}
		writetarheader(f, "/./@LongLink", l+2, 0, "     0 \0", 'L');
		memset(b, 0, l+TBLOCK+100);
		fixtarname(b, aname, l+2);
		i = strlen(b)+1;
		DEBUG(5, ("File name in tar file: %s, size=%d, \n", b, (int)strlen(b)));
		dotarbuf(f, b, TBLOCK*(((i-1)/TBLOCK)+1));
		SAFE_FREE(b);
	}

	fixtarname(hb.dbuf.name, aname, (l+2 >= NAMSIZ) ? NAMSIZ : l + 2);

	if (lowercase)
		strlower_m(hb.dbuf.name);

	/* write out a "standard" tar format header */

	hb.dbuf.name[NAMSIZ-1]='\0';
	safe_strcpy(hb.dbuf.mode, amode, sizeof(hb.dbuf.mode)-1);
	oct_it((SMB_BIG_UINT)0, 8, hb.dbuf.uid);
	oct_it((SMB_BIG_UINT)0, 8, hb.dbuf.gid);
	oct_it((SMB_BIG_UINT) size, 13, hb.dbuf.size);
	if (size > (SMB_BIG_UINT)077777777777LL) {    

		/* This is a non-POSIX compatible extention to store files
			greater than 8GB. */

		memset(hb.dbuf.size, 0, 4);
		hb.dbuf.size[0]=128;
		for (i = 8, jp=(char*)&size; i; i--)
			hb.dbuf.size[i+3] = *(jp++);
	}
	oct_it((SMB_BIG_UINT) mtime, 13, hb.dbuf.mtime);
	memcpy(hb.dbuf.chksum, "        ", sizeof(hb.dbuf.chksum));
	memset(hb.dbuf.linkname, 0, NAMSIZ);
	hb.dbuf.linkflag=ftype;
  
	for (chk=0, i=sizeof(hb.dummy), jp=hb.dummy; --i>=0;)
		chk+=(0xFF & *jp++);

	oct_it((SMB_BIG_UINT) chk, 8, hb.dbuf.chksum);
	hb.dbuf.chksum[6] = '\0';

	(void) dotarbuf(f, hb.dummy, sizeof(hb.dummy));
}

/****************************************************************************
Read a tar header into a hblock structure, and validate
***************************************************************************/

static long readtarheader(union hblock *hb, file_info2 *finfo, char *prefix)
{
	long chk, fchk;
	int i;
	char *jp;

	/*
	 * read in a "standard" tar format header - we're not that interested
	 * in that many fields, though
	 */

	/* check the checksum */
	for (chk=0, i=sizeof(hb->dummy), jp=hb->dummy; --i>=0;)
		chk+=(0xFF & *jp++);

	if (chk == 0)
		return chk;

	/* compensate for blanks in chksum header */
	for (i=sizeof(hb->dbuf.chksum), jp=hb->dbuf.chksum; --i>=0;)
		chk-=(0xFF & *jp++);

	chk += ' ' * sizeof(hb->dbuf.chksum);

	fchk=unoct(hb->dbuf.chksum, sizeof(hb->dbuf.chksum));

	DEBUG(5, ("checksum totals chk=%ld fchk=%ld chksum=%s\n",
			chk, fchk, hb->dbuf.chksum));

	if (fchk != chk) {
		DEBUG(0, ("checksums don't match %ld %ld\n", fchk, chk));
		dump_data(5, (char *)hb - TBLOCK, TBLOCK *3);
		return -1;
	}

	if ((finfo->name = string_create_s(strlen(prefix) + strlen(hb -> dbuf.name) + 3)) == NULL) {
		DEBUG(0, ("Out of space creating file_info2 for %s\n", hb -> dbuf.name));
		return(-1);
	}

	safe_strcpy(finfo->name, prefix, strlen(prefix) + strlen(hb -> dbuf.name) + 3);

	/* use l + 1 to do the null too; do prefix - prefcnt to zap leading slash */
	unfixtarname(finfo->name + strlen(prefix), hb->dbuf.name,
		strlen(hb->dbuf.name) + 1, True);

	/* can't handle some links at present */
	if ((hb->dbuf.linkflag != '0') && (hb -> dbuf.linkflag != '5')) {
		if (hb->dbuf.linkflag == 0) {
			DEBUG(6, ("Warning: NULL link flag (gnu tar archive ?) %s\n",
				finfo->name));
		} else { 
			if (hb -> dbuf.linkflag == 'L') { /* We have a longlink */
				/* Do nothing here at the moment. do_tarput will handle this
					as long as the longlink gets back to it, as it has to advance 
					the buffer pointer, etc */
			} else {
				DEBUG(0, ("this tar file appears to contain some kind \
of link other than a GNUtar Longlink - ignoring\n"));
				return -2;
			}
		}
	}
    
	if ((unoct(hb->dbuf.mode, sizeof(hb->dbuf.mode)) & S_IFDIR) ||
				(*(finfo->name+strlen(finfo->name)-1) == '\\')) {
		finfo->mode=aDIR;
	} else {
		finfo->mode=0; /* we don't care about mode at the moment, we'll
				* just make it a regular file */
	}

	/*
	 * Bug fix by richard@sj.co.uk
	 *
	 * REC: restore times correctly (as does tar)
	 * We only get the modification time of the file; set the creation time
	 * from the mod. time, and the access time to current time
	 */
	finfo->mtime = finfo->ctime = strtol(hb->dbuf.mtime, NULL, 8);
	finfo->atime = time(NULL);
	finfo->size = unoct(hb->dbuf.size, sizeof(hb->dbuf.size));

	return True;
}

/****************************************************************************
Write out the tar buffer to tape or wherever
****************************************************************************/

static int dotarbuf(int f, char *b, int n)
{
	int fail=1, writ=n;

	if (dry_run) {
		return writ;
	}
	/* This routine and the next one should be the only ones that do write()s */
	if (tp + n >= tbufsiz) {
		int diff;

		diff=tbufsiz-tp;
		memcpy(tarbuf + tp, b, diff);
		fail=fail && (1+write(f, tarbuf, tbufsiz));
		n-=diff;
		b+=diff;
		tp=0;

		while (n >= tbufsiz) {
			fail=fail && (1 + write(f, b, tbufsiz));
			n-=tbufsiz;
			b+=tbufsiz;
		}
	}

	if (n>0) {
		memcpy(tarbuf+tp, b, n);
		tp+=n;
	}

	return(fail ? writ : 0);
}

/****************************************************************************
Write zeros to buffer / tape
****************************************************************************/

static void dozerobuf(int f, int n)
{
	/* short routine just to write out n zeros to buffer -
	 * used to round files to nearest block
	 * and to do tar EOFs */

	if (dry_run)
		return;
  
	if (n+tp >= tbufsiz) {
		memset(tarbuf+tp, 0, tbufsiz-tp);
		write(f, tarbuf, tbufsiz);
		memset(tarbuf, 0, (tp+=n-tbufsiz));
	} else {
		memset(tarbuf+tp, 0, n);
		tp+=n;
	}
}

/****************************************************************************
Malloc tape buffer
****************************************************************************/

static void initarbuf(void)
{
	/* initialize tar buffer */
	tbufsiz=blocksize*TBLOCK;
	tarbuf=SMB_MALLOC(tbufsiz);      /* FIXME: We might not get the buffer */

	/* reset tar buffer pointer and tar file counter and total dumped */
	tp=0; ntarf=0; ttarf=0;
}

/****************************************************************************
Write two zero blocks at end of file
****************************************************************************/

static void dotareof(int f)
{
	SMB_STRUCT_STAT stbuf;
	/* Two zero blocks at end of file, write out full buffer */

	if (dry_run)
		return;

	(void) dozerobuf(f, TBLOCK);
	(void) dozerobuf(f, TBLOCK);

	if (sys_fstat(f, &stbuf) == -1) {
		DEBUG(0, ("Couldn't stat file handle\n"));
		return;
	}

	/* Could be a pipe, in which case S_ISREG should fail,
		* and we should write out at full size */
	if (tp > 0)
		write(f, tarbuf, S_ISREG(stbuf.st_mode) ? tp : tbufsiz);
}

/****************************************************************************
(Un)mangle DOS pathname, make nonabsolute
****************************************************************************/

static void fixtarname(char *tptr, const char *fp, size_t l)
{
	/* add a '.' to start of file name, convert from ugly dos \'s in path
	 * to lovely unix /'s :-} */
	*tptr++='.';
	l--;

	StrnCpy(tptr, fp, l-1);
	string_replace(tptr, '\\', '/');
}

/****************************************************************************
Convert from decimal to octal string
****************************************************************************/

static void oct_it (SMB_BIG_UINT value, int ndgs, char *p)
{
	/* Converts long to octal string, pads with leading zeros */

	/* skip final null, but do final space */
	--ndgs;
	p[--ndgs] = ' ';
 
	/* Loop does at least one digit */
	do {
		p[--ndgs] = '0' + (char) (value & 7);
		value >>= 3;
	} while (ndgs > 0 && value != 0);
 
	/* Do leading zeros */
	while (ndgs > 0)
		p[--ndgs] = '0';
}

/****************************************************************************
Convert from octal string to long
***************************************************************************/

static long unoct(char *p, int ndgs)
{
	long value=0;
	/* Converts octal string to long, ignoring any non-digit */

	while (--ndgs) {
		if (isdigit((int)*p))
			value = (value << 3) | (long) (*p - '0');

		p++;
	}

	return value;
}

/****************************************************************************
Compare two strings in a slash insensitive way, allowing s1 to match s2 
if s1 is an "initial" string (up to directory marker).  Thus, if s2 is 
a file in any subdirectory of s1, declare a match.
***************************************************************************/

static int strslashcmp(char *s1, char *s2)
{
	char *s1_0=s1;

	while(*s1 && *s2 && (*s1 == *s2 || tolower_ascii(*s1) == tolower_ascii(*s2) ||
				(*s1 == '\\' && *s2=='/') || (*s1 == '/' && *s2=='\\'))) {
		s1++; s2++;
	}

	/* if s1 has a trailing slash, it compared equal, so s1 is an "initial" 
		string of s2.
	*/
	if (!*s1 && s1 != s1_0 && (*(s1-1) == '/' || *(s1-1) == '\\'))
		return 0;

	/* ignore trailing slash on s1 */
	if (!*s2 && (*s1 == '/' || *s1 == '\\') && !*(s1+1))
		return 0;

	/* check for s1 is an "initial" string of s2 */
	if ((*s2 == '/' || *s2 == '\\') && !*s1)
		return 0;

	return *s1-*s2;
}

/****************************************************************************
Ensure a remote path exists (make if necessary)
***************************************************************************/

static BOOL ensurepath(char *fname)
{
	/* *must* be called with buffer ready malloc'ed */
	/* ensures path exists */

	char *partpath, *ffname;
	char *p=fname, *basehack;

	DEBUG(5, ( "Ensurepath called with: %s\n", fname));

	partpath = string_create_s(strlen(fname));
	ffname = string_create_s(strlen(fname));

	if ((partpath == NULL) || (ffname == NULL)){
		DEBUG(0, ("Out of memory in ensurepath: %s\n", fname));
		SAFE_FREE(partpath);
		SAFE_FREE(ffname);
		return(False);
	}

	*partpath = 0;

	/* fname copied to ffname so can strtok */

	safe_strcpy(ffname, fname, strlen(fname));

	/* do a `basename' on ffname, so don't try and make file name directory */
	if ((basehack=strrchr_m(ffname, '\\')) == NULL)
		return True;
	else
		*basehack='\0';

	p=strtok(ffname, "\\");

	while (p) {
		safe_strcat(partpath, p, strlen(fname) + 1);

		if (!cli_chkpath(cli, partpath)) {
			if (!cli_mkdir(cli, partpath)) {
				DEBUG(0, ("Error mkdirhiering\n"));
				return False;
			} else {
				DEBUG(3, ("mkdirhiering %s\n", partpath));
			}
		}

		safe_strcat(partpath, "\\", strlen(fname) + 1);
		p = strtok(NULL,"/\\");
	}

	return True;
}

static int padit(char *buf, SMB_BIG_UINT bufsize, SMB_BIG_UINT padsize)
{
	int berr= 0;
	int bytestowrite;
  
	DEBUG(5, ("Padding with %0.f zeros\n", (double)padsize));
	memset(buf, 0, (size_t)bufsize);
	while( !berr && padsize > 0 ) {
		bytestowrite= (int)MIN(bufsize, padsize);
		berr = dotarbuf(tarhandle, buf, bytestowrite) != bytestowrite;
		padsize -= bytestowrite;
	}
  
	return berr;
}

static void do_setrattr(char *name, uint16 attr, int set)
{
	uint16 oldattr;

	if (!cli_getatr(cli, name, &oldattr, NULL, NULL)) return;

	if (set == ATTRSET) {
		attr |= oldattr;
	} else {
		attr = oldattr & ~attr;
	}

	if (!cli_setatr(cli, name, attr, 0)) {
		DEBUG(1,("setatr failed: %s\n", cli_errstr(cli)));
	}
}

/****************************************************************************
append one remote file to the tar file
***************************************************************************/

static void do_atar(char *rname,char *lname,file_info *finfo1)
{
	int fnum;
	SMB_BIG_UINT nread=0;
	char ftype;
	file_info2 finfo;
	BOOL close_done = False;
	BOOL shallitime=True;
	char data[65520];
	int read_size = 65520;
	int datalen=0;

	struct timeval tp_start;

	GetTimeOfDay(&tp_start);

	ftype = '0'; /* An ordinary file ... */

	if (finfo1) {
		finfo.size  = finfo1 -> size;
		finfo.mode  = finfo1 -> mode;
		finfo.uid   = finfo1 -> uid;
		finfo.gid   = finfo1 -> gid;
		finfo.mtime = finfo1 -> mtime;
		finfo.atime = finfo1 -> atime;
		finfo.ctime = finfo1 -> ctime;
		finfo.name  = finfo1 -> name;
	} else {
		finfo.size  = def_finfo.size;
		finfo.mode  = def_finfo.mode;
		finfo.uid   = def_finfo.uid;
		finfo.gid   = def_finfo.gid;
		finfo.mtime = def_finfo.mtime;
		finfo.atime = def_finfo.atime;
		finfo.ctime = def_finfo.ctime;
		finfo.name  = def_finfo.name;
	}

	if (dry_run) {
		DEBUG(3,("skipping file %s of size %12.0f bytes\n", finfo.name,
				(double)finfo.size));
		shallitime=0;
		ttarf+=finfo.size + TBLOCK - (finfo.size % TBLOCK);
		ntarf++;
		return;
	}

	fnum = cli_open(cli, rname, O_RDONLY, DENY_NONE);

	dos_clean_name(rname);

	if (fnum == -1) {
		DEBUG(0,("%s opening remote file %s (%s)\n",
				cli_errstr(cli),rname, cur_dir));
		return;
	}

	finfo.name = string_create_s(strlen(rname));
	if (finfo.name == NULL) {
		DEBUG(0, ("Unable to allocate space for finfo.name in do_atar\n"));
		return;
	}

	safe_strcpy(finfo.name,rname, strlen(rname));
	if (!finfo1) {
		if (!cli_getattrE(cli, fnum, &finfo.mode, &finfo.size, NULL, &finfo.atime, &finfo.mtime)) {
			DEBUG(0, ("getattrE: %s\n", cli_errstr(cli)));
			return;
		}
		finfo.ctime = finfo.mtime;
	}

	DEBUG(3,("file %s attrib 0x%X\n",finfo.name,finfo.mode));

	if (tar_inc && !(finfo.mode & aARCH)) {
		DEBUG(4, ("skipping %s - archive bit not set\n", finfo.name));
		shallitime=0;
	} else if (!tar_system && (finfo.mode & aSYSTEM)) {
		DEBUG(4, ("skipping %s - system bit is set\n", finfo.name));
		shallitime=0;
	} else if (!tar_hidden && (finfo.mode & aHIDDEN)) {
		DEBUG(4, ("skipping %s - hidden bit is set\n", finfo.name));
		shallitime=0;
	} else {
		BOOL wrote_tar_header = False;

		DEBUG(3,("getting file %s of size %.0f bytes as a tar file %s",
			finfo.name, (double)finfo.size, lname));
      
		while (nread < finfo.size && !close_done) {
	      
			DEBUG(3,("nread=%.0f\n",(double)nread));
	      
			datalen = cli_read(cli, fnum, data, nread, read_size);
	      
			if (datalen == -1) {
				DEBUG(0,("Error reading file %s : %s\n", rname, cli_errstr(cli)));
				break;
			}
	      
			nread += datalen;

			/* Only if the first read succeeds, write out the tar header. */
			if (!wrote_tar_header) {
				/* write a tar header, don't bother with mode - just set to 100644 */
				writetarheader(tarhandle, rname, finfo.size, finfo.mtime, "100644 \0", ftype);
				wrote_tar_header = True;
			}

			/* if file size has increased since we made file size query, truncate
				read so tar header for this file will be correct.
			*/

			if (nread > finfo.size) {
				datalen -= nread - finfo.size;
				DEBUG(0,("File size change - truncating %s to %.0f bytes\n",
							finfo.name, (double)finfo.size));
			}

			/* add received bits of file to buffer - dotarbuf will
			* write out in 512 byte intervals */

			if (dotarbuf(tarhandle,data,datalen) != datalen) {
				DEBUG(0,("Error writing to tar file - %s\n", strerror(errno)));
				break;
			}
	      
			if (datalen == 0) {
				DEBUG(0,("Error reading file %s. Got 0 bytes\n", rname));
				break;
			}

			datalen=0;
		}

		if (wrote_tar_header) {
			/* pad tar file with zero's if we couldn't get entire file */
			if (nread < finfo.size) {
				DEBUG(0, ("Didn't get entire file. size=%.0f, nread=%d\n",
							(double)finfo.size, (int)nread));
				if (padit(data, (SMB_BIG_UINT)sizeof(data), finfo.size - nread))
					DEBUG(0,("Error writing tar file - %s\n", strerror(errno)));
			}

			/* round tar file to nearest block */
			if (finfo.size % TBLOCK)
				dozerobuf(tarhandle, TBLOCK - (finfo.size % TBLOCK));
      
			ttarf+=finfo.size + TBLOCK - (finfo.size % TBLOCK);
			ntarf++;
		} else {
			DEBUG(4, ("skipping %s - initial read failed (file was locked ?)\n", finfo.name));
			shallitime=0;
		}
	}
  
	cli_close(cli, fnum);

	if (shallitime) {
		struct timeval tp_end;
		int this_time;

		/* if shallitime is true then we didn't skip */
		if (tar_reset && !dry_run)
			(void) do_setrattr(finfo.name, aARCH, ATTRRESET);
      
		GetTimeOfDay(&tp_end);
		this_time = (tp_end.tv_sec - tp_start.tv_sec)*1000 + (tp_end.tv_usec - tp_start.tv_usec)/1000;
		get_total_time_ms += this_time;
		get_total_size += finfo.size;

		if (tar_noisy) {
			DEBUG(0, ("%12.0f (%7.1f kb/s) %s\n",
				(double)finfo.size, finfo.size / MAX(0.001, (1.024*this_time)),
				finfo.name));
		}

		/* Thanks to Carel-Jan Engel (ease@mail.wirehub.nl) for this one */
		DEBUG(3,("(%g kb/s) (average %g kb/s)\n",
				finfo.size / MAX(0.001, (1.024*this_time)),
				get_total_size / MAX(0.001, (1.024*get_total_time_ms))));
	}
}

/****************************************************************************
Append single file to tar file (or not)
***************************************************************************/

static void do_tar(file_info *finfo)
{
	pstring rname;

	if (strequal(finfo->name,"..") || strequal(finfo->name,"."))
		return;

	/* Is it on the exclude list ? */
	if (!tar_excl && clipn) {
		pstring exclaim;

		DEBUG(5, ("Excl: strlen(cur_dir) = %d\n", (int)strlen(cur_dir)));

		pstrcpy(exclaim, cur_dir);
		*(exclaim+strlen(exclaim)-1)='\0';

		pstrcat(exclaim, "\\");
		pstrcat(exclaim, finfo->name);

		DEBUG(5, ("...tar_re_search: %d\n", tar_re_search));

		if ((!tar_re_search && clipfind(cliplist, clipn, exclaim)) ||
				(tar_re_search && mask_match_list(exclaim, cliplist, clipn, True))) {
			DEBUG(3,("Skipping file %s\n", exclaim));
			return;
		}
	}

	if (finfo->mode & aDIR) {
		pstring saved_curdir;
		pstring mtar_mask;

		pstrcpy(saved_curdir, cur_dir);

		DEBUG(5, ("Sizeof(cur_dir)=%d, strlen(cur_dir)=%d, \
strlen(finfo->name)=%d\nname=%s,cur_dir=%s\n",
			(int)sizeof(cur_dir), (int)strlen(cur_dir),
			(int)strlen(finfo->name), finfo->name, cur_dir));

		pstrcat(cur_dir,finfo->name);
		pstrcat(cur_dir,"\\");

		DEBUG(5, ("Writing a dir, Name = %s\n", cur_dir));

		/* write a tar directory, don't bother with mode - just set it to
			* 40755 */
		writetarheader(tarhandle, cur_dir, 0, finfo->mtime, "040755 \0", '5');
		if (tar_noisy) {
			DEBUG(0,("                directory %s\n", cur_dir));
		}
		ntarf++;  /* Make sure we have a file on there */
		pstrcpy(mtar_mask,cur_dir);
		pstrcat(mtar_mask,"*");
		DEBUG(5, ("Doing list with mtar_mask: %s\n", mtar_mask));
		do_list(mtar_mask, attribute, do_tar, False, True);
		pstrcpy(cur_dir,saved_curdir);
	} else {
		pstrcpy(rname,cur_dir);
		pstrcat(rname,finfo->name);
		do_atar(rname,finfo->name,finfo);
	}
}

/****************************************************************************
Convert from UNIX to DOS file names
***************************************************************************/

static void unfixtarname(char *tptr, char *fp, int l, BOOL first)
{
	/* remove '.' from start of file name, convert from unix /'s to
	 * dos \'s in path. Kill any absolute path names. But only if first!
	 */

	DEBUG(5, ("firstb=%lX, secondb=%lX, len=%i\n", (long)tptr, (long)fp, l));

	if (first) {
		if (*fp == '.') {
			fp++;
			l--;
		}
		if (*fp == '\\' || *fp == '/') {
			fp++;
			l--;
		}
	}

	safe_strcpy(tptr, fp, l);
	string_replace(tptr, '/', '\\');
}

/****************************************************************************
Move to the next block in the buffer, which may mean read in another set of
blocks. FIXME, we should allow more than one block to be skipped.
****************************************************************************/

static int next_block(char *ltarbuf, char **bufferp, int bufsiz)
{
	int bufread, total = 0;

	DEBUG(5, ("Advancing to next block: %0lx\n", (unsigned long)*bufferp));
	*bufferp += TBLOCK;
	total = TBLOCK;

	if (*bufferp >= (ltarbuf + bufsiz)) {

		DEBUG(5, ("Reading more data into ltarbuf ...\n"));

		/*
		 * Bugfix from Bob Boehmer <boehmer@worldnet.att.net>
		 * Fixes bug where read can return short if coming from
		 * a pipe.
		 */

		bufread = read(tarhandle, ltarbuf, bufsiz);
		total = bufread;

		while (total < bufsiz) {
			if (bufread < 0) { /* An error, return false */
				return (total > 0 ? -2 : bufread);
			}
			if (bufread == 0) {
				if (total <= 0) {
					return -2;
				}
				break;
			}
			bufread = read(tarhandle, &ltarbuf[total], bufsiz - total);
			total += bufread;
		}

		DEBUG(5, ("Total bytes read ... %i\n", total));

		*bufferp = ltarbuf;
	}

	return(total);
}

/* Skip a file, even if it includes a long file name? */
static int skip_file(int skipsize)
{
	int dsize = skipsize;

	DEBUG(5, ("Skiping file. Size = %i\n", skipsize));

	/* FIXME, we should skip more than one block at a time */

	while (dsize > 0) {
		if (next_block(tarbuf, &buffer_p, tbufsiz) <= 0) {
			DEBUG(0, ("Empty file, short tar file, or read error: %s\n", strerror(errno)));
			return(False);
		}
		dsize -= TBLOCK;
	}

	return(True);
}

/*************************************************************
 Get a file from the tar file and store it.
 When this is called, tarbuf already contains the first
 file block. This is a bit broken & needs fixing.
**************************************************************/

static int get_file(file_info2 finfo)
{
	int fnum = -1, pos = 0, dsize = 0, bpos = 0;
	SMB_BIG_UINT rsize = 0;

	DEBUG(5, ("get_file: file: %s, size %.0f\n", finfo.name, (double)finfo.size));

	if (ensurepath(finfo.name) && 
			(fnum=cli_open(cli, finfo.name, O_RDWR|O_CREAT|O_TRUNC, DENY_NONE)) == -1) {
		DEBUG(0, ("abandoning restore\n"));
		return(False);
	}

	/* read the blocks from the tar file and write to the remote file */

	rsize = finfo.size;  /* This is how much to write */

	while (rsize > 0) {

		/* We can only write up to the end of the buffer */
		dsize = MIN(tbufsiz - (buffer_p - tarbuf) - bpos, 65520); /* Calculate the size to write */
		dsize = MIN(dsize, rsize);  /* Should be only what is left */
		DEBUG(5, ("writing %i bytes, bpos = %i ...\n", dsize, bpos));

		if (cli_write(cli, fnum, 0, buffer_p + bpos, pos, dsize) != dsize) {
			DEBUG(0, ("Error writing remote file\n"));
			return 0;
		}

		rsize -= dsize;
		pos += dsize;

		/* Now figure out how much to move in the buffer */

		/* FIXME, we should skip more than one block at a time */

		/* First, skip any initial part of the part written that is left over */
		/* from the end of the first TBLOCK                                   */

		if ((bpos) && ((bpos + dsize) >= TBLOCK)) {
			dsize -= (TBLOCK - bpos);  /* Get rid of the end of the first block */
			bpos = 0;

			if (next_block(tarbuf, &buffer_p, tbufsiz) <=0) {  /* and skip the block */
				DEBUG(0, ("Empty file, short tar file, or read error: %s\n", strerror(errno)));
				return False;
			}
		}

		/*
		 * Bugfix from Bob Boehmer <boehmer@worldnet.att.net>.
		 * If the file being extracted is an exact multiple of
		 * TBLOCK bytes then we don't want to extract the next
		 * block from the tarfile here, as it will be done in
		 * the caller of get_file().
		 */

		while (((rsize != 0) && (dsize >= TBLOCK)) ||
				((rsize == 0) && (dsize > TBLOCK))) {

			if (next_block(tarbuf, &buffer_p, tbufsiz) <=0) {
				DEBUG(0, ("Empty file, short tar file, or read error: %s\n", strerror(errno)));
				return False;
			}

			dsize -= TBLOCK;
		}
		bpos = dsize;
	}

	/* Now close the file ... */

	if (!cli_close(cli, fnum)) {
		DEBUG(0, ("Error closing remote file\n"));
		return(False);
	}

	/* Now we update the creation date ... */
	DEBUG(5, ("Updating creation date on %s\n", finfo.name));

	if (!cli_setatr(cli, finfo.name, finfo.mode, finfo.mtime)) {
		if (tar_real_noisy) {
			DEBUG(0, ("Could not set time on file: %s\n", finfo.name));
			/*return(False); */ /* Ignore, as Win95 does not allow changes */
		}
	}

	ntarf++;
	DEBUG(0, ("restore tar file %s of size %.0f bytes\n", finfo.name, (double)finfo.size));
	return(True);
}

/* Create a directory.  We just ensure that the path exists and return as there
   is no file associated with a directory 
*/
static int get_dir(file_info2 finfo)
{
	DEBUG(0, ("restore directory %s\n", finfo.name));

	if (!ensurepath(finfo.name)) {
		DEBUG(0, ("Problems creating directory\n"));
		return(False);
	}
	ntarf++;
	return(True);
}

/* Get a file with a long file name ... first file has file name, next file 
   has the data. We only want the long file name, as the loop in do_tarput
   will deal with the rest.
*/
static char *get_longfilename(file_info2 finfo)
{
	/* finfo.size here is the length of the filename as written by the "/./@LongLink" name
	 * header call. */
	int namesize = finfo.size + strlen(cur_dir) + 2;
	char *longname = SMB_MALLOC(namesize);
	int offset = 0, left = finfo.size;
	BOOL first = True;

	DEBUG(5, ("Restoring a long file name: %s\n", finfo.name));
	DEBUG(5, ("Len = %.0f\n", (double)finfo.size));

	if (longname == NULL) {
		DEBUG(0, ("could not allocate buffer of size %d for longname\n", namesize));
		return(NULL);
	}

	/* First, add cur_dir to the long file name */

	if (strlen(cur_dir) > 0) {
		strncpy(longname, cur_dir, namesize);
		offset = strlen(cur_dir);
	}

	/* Loop through the blocks picking up the name */

	while (left > 0) {
		if (next_block(tarbuf, &buffer_p, tbufsiz) <= 0) {
			DEBUG(0, ("Empty file, short tar file, or read error: %s\n", strerror(errno)));
			SAFE_FREE(longname);
			return(NULL);
		}

		unfixtarname(longname + offset, buffer_p, MIN(TBLOCK, finfo.size), first--);
		DEBUG(5, ("UnfixedName: %s, buffer: %s\n", longname, buffer_p));

		offset += TBLOCK;
		left -= TBLOCK;
	}

	return(longname);
}

static void do_tarput(void)
{
	file_info2 finfo;
	struct timeval tp_start;
	char *longfilename = NULL, linkflag;
	int skip = False;

	ZERO_STRUCT(finfo);

	GetTimeOfDay(&tp_start);
	DEBUG(5, ("RJS do_tarput called ...\n"));

	buffer_p = tarbuf + tbufsiz;  /* init this to force first read */

	/* Now read through those files ... */
	while (True) {
		/* Get us to the next block, or the first block first time around */
		if (next_block(tarbuf, &buffer_p, tbufsiz) <= 0) {
			DEBUG(0, ("Empty file, short tar file, or read error: %s\n", strerror(errno)));
			SAFE_FREE(longfilename);
			return;
		}

		DEBUG(5, ("Reading the next header ...\n"));

		switch (readtarheader((union hblock *) buffer_p, &finfo, cur_dir)) {
			case -2:    /* Hmm, not good, but not fatal */
				DEBUG(0, ("Skipping %s...\n", finfo.name));
				if ((next_block(tarbuf, &buffer_p, tbufsiz) <= 0) && !skip_file(finfo.size)) {
					DEBUG(0, ("Short file, bailing out...\n"));
					return;
				}
				break;

			case -1:
				DEBUG(0, ("abandoning restore, -1 from read tar header\n"));
				return;

			case 0: /* chksum is zero - looks like an EOF */
				DEBUG(0, ("tar: restored %d files and directories\n", ntarf));
				return;        /* Hmmm, bad here ... */

			default: 
				/* No action */
				break;
		}

		/* Now, do we have a long file name? */
		if (longfilename != NULL) {
			SAFE_FREE(finfo.name);   /* Free the space already allocated */
			finfo.name = longfilename;
			longfilename = NULL;
		}

		/* Well, now we have a header, process the file ...            */
		/* Should we skip the file? We have the long name as well here */
		skip = clipn && ((!tar_re_search && clipfind(cliplist, clipn, finfo.name) ^ tar_excl) ||
					(tar_re_search && mask_match_list(finfo.name, cliplist, clipn, True)));

		DEBUG(5, ("Skip = %i, cliplist=%s, file=%s\n", skip, (cliplist?cliplist[0]:NULL), finfo.name));
		if (skip) {
			skip_file(finfo.size);
			continue;
		}

		/* We only get this far if we should process the file */
		linkflag = ((union hblock *)buffer_p) -> dbuf.linkflag;
		switch (linkflag) {
			case '0':  /* Should use symbolic names--FIXME */
				/* 
				 * Skip to the next block first, so we can get the file, FIXME, should
				 * be in get_file ...
				 * The 'finfo.size != 0' fix is from Bob Boehmer <boehmer@worldnet.att.net>
				 * Fixes bug where file size in tarfile is zero.
				 */
				if ((finfo.size != 0) && next_block(tarbuf, &buffer_p, tbufsiz) <=0) {
					DEBUG(0, ("Short file, bailing out...\n"));
					return;
				}
				if (!get_file(finfo)) {
					DEBUG(0, ("Abandoning restore\n"));
					return;
				}
				break;
			case '5':
				if (!get_dir(finfo)) {
					DEBUG(0, ("Abandoning restore \n"));
					return;
				}
				break;
			case 'L':
				SAFE_FREE(longfilename);
				longfilename = get_longfilename(finfo);
				if (!longfilename) {
					DEBUG(0, ("abandoning restore\n"));
					return;
				}
				DEBUG(5, ("Long file name: %s\n", longfilename));
				break;

			default:
				skip_file(finfo.size);  /* Don't handle these yet */
				break;
		}
	}
}

/*
 * samba interactive commands
 */

/****************************************************************************
Blocksize command
***************************************************************************/

int cmd_block(void)
{
	fstring buf;
	int block;

	if (!next_token_nr(NULL,buf,NULL,sizeof(buf))) {
		DEBUG(0, ("blocksize <n>\n"));
		return 1;
	}

	block=atoi(buf);
	if (block < 0 || block > 65535) {
		DEBUG(0, ("blocksize out of range"));
		return 1;
	}

	blocksize=block;
	DEBUG(2,("blocksize is now %d\n", blocksize));

	return 0;
}

/****************************************************************************
command to set incremental / reset mode
***************************************************************************/

int cmd_tarmode(void)
{
	fstring buf;

	while (next_token_nr(NULL,buf,NULL,sizeof(buf))) {
		if (strequal(buf, "full"))
			tar_inc=False;
		else if (strequal(buf, "inc"))
			tar_inc=True;
		else if (strequal(buf, "reset"))
			tar_reset=True;
		else if (strequal(buf, "noreset"))
			tar_reset=False;
		else if (strequal(buf, "system"))
			tar_system=True;
		else if (strequal(buf, "nosystem"))
			tar_system=False;
		else if (strequal(buf, "hidden"))
			tar_hidden=True;
		else if (strequal(buf, "nohidden"))
			tar_hidden=False;
		else if (strequal(buf, "verbose") || strequal(buf, "noquiet"))
			tar_noisy=True;
		else if (strequal(buf, "quiet") || strequal(buf, "noverbose"))
			tar_noisy=False;
		else
			DEBUG(0, ("tarmode: unrecognised option %s\n", buf));
	}

	DEBUG(0, ("tarmode is now %s, %s, %s, %s, %s\n",
			tar_inc ? "incremental" : "full",
			tar_system ? "system" : "nosystem",
			tar_hidden ? "hidden" : "nohidden",
			tar_reset ? "reset" : "noreset",
			tar_noisy ? "verbose" : "quiet"));
	return 0;
}

/****************************************************************************
Feeble attrib command
***************************************************************************/

int cmd_setmode(void)
{
	char *q;
	fstring buf;
	pstring fname;
	uint16 attra[2];
	int direct=1;

	attra[0] = attra[1] = 0;

	if (!next_token_nr(NULL,buf,NULL,sizeof(buf))) {
		DEBUG(0, ("setmode <filename> <[+|-]rsha>\n"));
		return 1;
	}

	pstrcpy(fname, cur_dir);
	pstrcat(fname, buf);

	while (next_token_nr(NULL,buf,NULL,sizeof(buf))) {
		q=buf;

		while(*q) {
			switch (*q++) {
				case '+':
					direct=1;
					break;
				case '-':
					direct=0;
					break;
				case 'r':
					attra[direct]|=aRONLY;
					break;
				case 'h':
					attra[direct]|=aHIDDEN;
					break;
				case 's':
					attra[direct]|=aSYSTEM;
					break;
				case 'a':
					attra[direct]|=aARCH;
					break;
				default:
					DEBUG(0, ("setmode <filename> <perm=[+|-]rsha>\n"));
					return 1;
			}
		}
	}

	if (attra[ATTRSET]==0 && attra[ATTRRESET]==0) {
		DEBUG(0, ("setmode <filename> <[+|-]rsha>\n"));
		return 1;
	}

	DEBUG(2, ("\nperm set %d %d\n", attra[ATTRSET], attra[ATTRRESET]));
	do_setrattr(fname, attra[ATTRSET], ATTRSET);
	do_setrattr(fname, attra[ATTRRESET], ATTRRESET);
	return 0;
}

/****************************************************************************
Principal command for creating / extracting
***************************************************************************/

int cmd_tar(void)
{
	fstring buf;
	char **argl = NULL;
	int argcl = 0;
	int ret;

	if (!next_token_nr(NULL,buf,NULL,sizeof(buf))) {
		DEBUG(0,("tar <c|x>[IXbgan] <filename>\n"));
		return 1;
	}

	argl=toktocliplist(&argcl, NULL);
	if (!tar_parseargs(argcl, argl, buf, 0))
		return 1;

	ret = process_tar();
	SAFE_FREE(argl);
	return ret;
}

/****************************************************************************
Command line (option) version
***************************************************************************/

int process_tar(void)
{
	int rc = 0;
	initarbuf();
	switch(tar_type) {
		case 'x':

#if 0
			do_tarput2();
#else
			do_tarput();
#endif
			SAFE_FREE(tarbuf);
			close(tarhandle);
			break;
		case 'r':
		case 'c':
			if (clipn && tar_excl) {
				int i;
				pstring tarmac;

				for (i=0; i<clipn; i++) {
					DEBUG(5,("arg %d = %s\n", i, cliplist[i]));

					if (*(cliplist[i]+strlen(cliplist[i])-1)=='\\') {
						*(cliplist[i]+strlen(cliplist[i])-1)='\0';
					}
	
					if (strrchr_m(cliplist[i], '\\')) {
						pstring saved_dir;
	  
						pstrcpy(saved_dir, cur_dir);
	  
						if (*cliplist[i]=='\\') {
							pstrcpy(tarmac, cliplist[i]);
						} else {
							pstrcpy(tarmac, cur_dir);
							pstrcat(tarmac, cliplist[i]);
						}
						pstrcpy(cur_dir, tarmac);
						*(strrchr_m(cur_dir, '\\')+1)='\0';

						DEBUG(5, ("process_tar, do_list with tarmac: %s\n", tarmac));
						do_list(tarmac,attribute,do_tar, False, True);
						pstrcpy(cur_dir,saved_dir);
					} else {
						pstrcpy(tarmac, cur_dir);
						pstrcat(tarmac, cliplist[i]);
						DEBUG(5, ("process_tar, do_list with tarmac: %s\n", tarmac));
						do_list(tarmac,attribute,do_tar, False, True);
					}
				}
			} else {
				pstring mask;
				pstrcpy(mask,cur_dir);
				DEBUG(5, ("process_tar, do_list with mask: %s\n", mask));
				pstrcat(mask,"\\*");
				do_list(mask,attribute,do_tar,False, True);
			}
    
			if (ntarf)
				dotareof(tarhandle);
			close(tarhandle);
			SAFE_FREE(tarbuf);
    
			DEBUG(0, ("tar: dumped %d files and directories\n", ntarf));
			DEBUG(0, ("Total bytes written: %.0f\n", (double)ttarf));
			break;
	}

	if (must_free_cliplist) {
		int i;
		for (i = 0; i < clipn; ++i) {
			SAFE_FREE(cliplist[i]);
		}
		SAFE_FREE(cliplist);
		cliplist = NULL;
		clipn = 0;
		must_free_cliplist = False;
	}
	return rc;
}

/****************************************************************************
Find a token (filename) in a clip list
***************************************************************************/

static int clipfind(char **aret, int ret, char *tok)
{
	if (aret==NULL)
		return 0;

	/* ignore leading slashes or dots in token */
	while(strchr_m("/\\.", *tok))
		tok++;

	while(ret--) {
		char *pkey=*aret++;

		/* ignore leading slashes or dots in list */
		while(strchr_m("/\\.", *pkey))
			pkey++;

		if (!strslashcmp(pkey, tok))
			return 1;
	}
	return 0;
}

/****************************************************************************
Read list of files to include from the file and initialize cliplist
accordingly.
***************************************************************************/

static int read_inclusion_file(char *filename)
{
	XFILE *inclusion = NULL;
	char buf[PATH_MAX + 1];
	char *inclusion_buffer = NULL;
	int inclusion_buffer_size = 0;
	int inclusion_buffer_sofar = 0;
	char *p;
	char *tmpstr;
	int i;
	int error = 0;

	clipn = 0;
	buf[PATH_MAX] = '\0'; /* guarantee null-termination */
	if ((inclusion = x_fopen(filename, O_RDONLY, 0)) == NULL) {
		/* XXX It would be better to include a reason for failure, but without
		 * autoconf, it's hard to use strerror, sys_errlist, etc.
		 */
		DEBUG(0,("Unable to open inclusion file %s\n", filename));
		return 0;
	}

	while ((! error) && (x_fgets(buf, sizeof(buf)-1, inclusion))) {
		if (inclusion_buffer == NULL) {
			inclusion_buffer_size = 1024;
			if ((inclusion_buffer = SMB_MALLOC(inclusion_buffer_size)) == NULL) {
				DEBUG(0,("failure allocating buffer to read inclusion file\n"));
				error = 1;
				break;
			}
		}
    
		if (buf[strlen(buf)-1] == '\n') {
			buf[strlen(buf)-1] = '\0';
		}
    
		if ((strlen(buf) + 1 + inclusion_buffer_sofar) >= inclusion_buffer_size) {
			inclusion_buffer_size *= 2;
			inclusion_buffer = SMB_REALLOC(inclusion_buffer,inclusion_buffer_size);
			if (!inclusion_buffer) {
				DEBUG(0,("failure enlarging inclusion buffer to %d bytes\n",
						inclusion_buffer_size));
				error = 1;
				break;
			}
		}
    
		safe_strcpy(inclusion_buffer + inclusion_buffer_sofar, buf, inclusion_buffer_size - inclusion_buffer_sofar);
		inclusion_buffer_sofar += strlen(buf) + 1;
		clipn++;
	}
	x_fclose(inclusion);

	if (! error) {
		/* Allocate an array of clipn + 1 char*'s for cliplist */
		cliplist = SMB_MALLOC_ARRAY(char *, clipn + 1);
		if (cliplist == NULL) {
			DEBUG(0,("failure allocating memory for cliplist\n"));
			error = 1;
		} else {
			cliplist[clipn] = NULL;
			p = inclusion_buffer;
			for (i = 0; (! error) && (i < clipn); i++) {
				/* set current item to NULL so array will be null-terminated even if
						* malloc fails below. */
				cliplist[i] = NULL;
				if ((tmpstr = (char *)SMB_MALLOC(strlen(p)+1)) == NULL) {
					DEBUG(0, ("Could not allocate space for a cliplist item, # %i\n", i));
					error = 1;
				} else {
					unfixtarname(tmpstr, p, strlen(p) + 1, True);
					cliplist[i] = tmpstr;
					if ((p = strchr_m(p, '\000')) == NULL) {
						DEBUG(0,("INTERNAL ERROR: inclusion_buffer is of unexpected contents.\n"));
						abort();
					}
				}
				++p;
			}
			must_free_cliplist = True;
		}
	}

	SAFE_FREE(inclusion_buffer);
	if (error) {
		if (cliplist) {
			char **pp;
			/* We know cliplist is always null-terminated */
			for (pp = cliplist; *pp; ++pp) {
				SAFE_FREE(*pp);
			}
			SAFE_FREE(cliplist);
			cliplist = NULL;
			must_free_cliplist = False;
		}
		return 0;
	}
  
	/* cliplist and its elements are freed at the end of process_tar. */
	return 1;
}

/****************************************************************************
Parse tar arguments. Sets tar_type, tar_excl, etc.
***************************************************************************/

int tar_parseargs(int argc, char *argv[], const char *Optarg, int Optind)
{
	int newOptind = Optind;
	char tar_clipfl='\0';

	/* Reset back to defaults - could be from interactive version 
	 * reset mode and archive mode left as they are though
	 */
	tar_type='\0';
	tar_excl=True;
	dry_run=False;

	while (*Optarg) {
		switch(*Optarg++) {
			case 'c':
				tar_type='c';
				break;
			case 'x':
				if (tar_type=='c') {
					printf("Tar must be followed by only one of c or x.\n");
					return 0;
				}
				tar_type='x';
				break;
			case 'b':
				if (Optind>=argc || !(blocksize=atoi(argv[Optind]))) {
					DEBUG(0,("Option b must be followed by valid blocksize\n"));
					return 0;
				} else {
					Optind++;
					newOptind++;
				}
				break;
			case 'g':
				tar_inc=True;
				break;
			case 'N':
				if (Optind>=argc) {
					DEBUG(0,("Option N must be followed by valid file name\n"));
					return 0;
				} else {
					SMB_STRUCT_STAT stbuf;
	
					if (sys_stat(argv[Optind], &stbuf) == 0) {
						newer_than = stbuf.st_mtime;
						DEBUG(1,("Getting files newer than %s",
							time_to_asc(&newer_than)));
						newOptind++;
						Optind++;
					} else {
						DEBUG(0,("Error setting newer-than time\n"));
						return 0;
					}
				}
				break;
			case 'a':
				tar_reset=True;
				break;
			case 'q':
				tar_noisy=False;
				break;
			case 'I':
				if (tar_clipfl) {
					DEBUG(0,("Only one of I,X,F must be specified\n"));
					return 0;
				}
				tar_clipfl='I';
				break;
			case 'X':
				if (tar_clipfl) {
					DEBUG(0,("Only one of I,X,F must be specified\n"));
					return 0;
				}
				tar_clipfl='X';
				break;
			case 'F':
				if (tar_clipfl) {
					DEBUG(0,("Only one of I,X,F must be specified\n"));
					return 0;
				}
				tar_clipfl='F';
				break;
			case 'r':
				DEBUG(0, ("tar_re_search set\n"));
				tar_re_search = True;
				break;
			case 'n':
				if (tar_type == 'c') {
					DEBUG(0, ("dry_run set\n"));
					dry_run = True;
				} else {
					DEBUG(0, ("n is only meaningful when creating a tar-file\n"));
					return 0;
				}
				break;
			default:
				DEBUG(0,("Unknown tar option\n"));
				return 0;
		}
	}

	if (!tar_type) {
		printf("Option T must be followed by one of c or x.\n");
		return 0;
	}

	/* tar_excl is true if cliplist lists files to be included.
	 * Both 'I' and 'F' mean include. */
	tar_excl=tar_clipfl!='X';

	if (tar_clipfl=='F') {
		if (argc-Optind-1 != 1) {
			DEBUG(0,("Option F must be followed by exactly one filename.\n"));
			return 0;
		}
		newOptind++;
		/* Optind points at the tar output file, Optind+1 at the inclusion file. */
		if (! read_inclusion_file(argv[Optind+1])) {
			return 0;
		}
	} else if (Optind+1<argc && !tar_re_search) { /* For backwards compatibility */
		char *tmpstr;
		char **tmplist;
		int clipcount;

		cliplist=argv+Optind+1;
		clipn=argc-Optind-1;
		clipcount = clipn;

		if ((tmplist=SMB_MALLOC_ARRAY(char *,clipn)) == NULL) {
			DEBUG(0, ("Could not allocate space to process cliplist, count = %i\n", clipn));
			return 0;
		}

		for (clipcount = 0; clipcount < clipn; clipcount++) {

			DEBUG(5, ("Processing an item, %s\n", cliplist[clipcount]));

			if ((tmpstr = (char *)SMB_MALLOC(strlen(cliplist[clipcount])+1)) == NULL) {
				DEBUG(0, ("Could not allocate space for a cliplist item, # %i\n", clipcount));
				SAFE_FREE(tmplist);
				return 0;
			}

			unfixtarname(tmpstr, cliplist[clipcount], strlen(cliplist[clipcount]) + 1, True);
			tmplist[clipcount] = tmpstr;
			DEBUG(5, ("Processed an item, %s\n", tmpstr));

			DEBUG(5, ("Cliplist is: %s\n", cliplist[0]));
		}

		cliplist = tmplist;
		must_free_cliplist = True;

		newOptind += clipn;
	}

	if (Optind+1<argc && tar_re_search && tar_clipfl != 'F') {
		/* Doing regular expression seaches not from an inclusion file. */
		clipn=argc-Optind-1;
		cliplist=argv+Optind+1;
		newOptind += clipn;
	}

	if (Optind>=argc || !strcmp(argv[Optind], "-")) {
		/* Sets tar handle to either 0 or 1, as appropriate */
		tarhandle=(tar_type=='c');
		/*
		 * Make sure that dbf points to stderr if we are using stdout for 
		 * tar output
		 */
		if (tarhandle == 1)  {
			dbf = x_stderr;
		}
		if (!argv[Optind]) {
			DEBUG(0,("Must specify tar filename\n"));
			return 0;
		}
		if (!strcmp(argv[Optind], "-")) {
			newOptind++;
		}

	} else {
		if (tar_type=='c' && dry_run) {
			tarhandle=-1;
		} else if ((tar_type=='x' && (tarhandle = sys_open(argv[Optind], O_RDONLY, 0)) == -1)
					|| (tar_type=='c' && (tarhandle=sys_creat(argv[Optind], 0644)) < 0)) {
			DEBUG(0,("Error opening local file %s - %s\n", argv[Optind], strerror(errno)));
			return(0);
		}
		newOptind++;
	}

	return newOptind;
}
