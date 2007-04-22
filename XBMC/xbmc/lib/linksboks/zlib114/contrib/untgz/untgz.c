/*
 * untgz.c -- Display contents and/or extract file from
 * a gzip'd TAR file
 * written by "Pedro A. Aranda Guti\irrez" <paag@tid.es>
 * adaptation to Unix by Jean-loup Gailly <jloup@gzip.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#ifdef unix
# include <unistd.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "zlib.h"

#ifdef WIN32
#  ifndef F_OK
#    define F_OK (0)
#  endif
#  ifdef _MSC_VER
#    define mkdir(dirname,mode) _mkdir(dirname)
#    define strdup(str)         _strdup(str)
#    define unlink(fn)          _unlink(fn)
#    define access(path,mode)   _access(path,mode)
#  else
#    define mkdir(dirname,mode) _mkdir(dirname)
#  endif
#else
#  include <utime.h>
#endif


/* Values used in typeflag field.  */

#define REGTYPE	 '0'		/* regular file */
#define AREGTYPE '\0'		/* regular file */
#define LNKTYPE  '1'		/* link */
#define SYMTYPE  '2'		/* reserved */
#define CHRTYPE  '3'		/* character special */
#define BLKTYPE  '4'		/* block special */
#define DIRTYPE  '5'		/* directory */
#define FIFOTYPE '6'		/* FIFO special */
#define CONTTYPE '7'		/* reserved */

#define BLOCKSIZE 512

struct tar_header
{				/* byte offset */
  char name[100];		/*   0 */
  char mode[8];			/* 100 */
  char uid[8];			/* 108 */
  char gid[8];			/* 116 */
  char size[12];		/* 124 */
  char mtime[12];		/* 136 */
  char chksum[8];		/* 148 */
  char typeflag;		/* 156 */
  char linkname[100];		/* 157 */
  char magic[6];		/* 257 */
  char version[2];		/* 263 */
  char uname[32];		/* 265 */
  char gname[32];		/* 297 */
  char devmajor[8];		/* 329 */
  char devminor[8];		/* 337 */
  char prefix[155];		/* 345 */
				/* 500 */
};

union tar_buffer {
  char               buffer[BLOCKSIZE];
  struct tar_header  header;
};

enum { TGZ_EXTRACT = 0, TGZ_LIST };

static char *TGZfname	OF((const char *));
void TGZnotfound	OF((const char *));

int getoct		OF((char *, int));
char *strtime		OF((time_t *));
int ExprMatch		OF((char *,char *));

int makedir		OF((char *));
int matchname		OF((int,int,char **,char *));

void error		OF((const char *));
int  tar		OF((gzFile, int, int, int, char **));

void help		OF((int));
int main		OF((int, char **));

char *prog;

/* This will give a benign warning */

static char *TGZprefix[] = { "\0", ".tgz", ".tar.gz", ".tar", NULL };

/* Return the real name of the TGZ archive */
/* or NULL if it does not exist. */

static char *TGZfname OF((const char *fname))
{
  static char buffer[1024];
  int origlen,i;
  
  strcpy(buffer,fname);
  origlen = strlen(buffer);

  for (i=0; TGZprefix[i]; i++)
    {
       strcpy(buffer+origlen,TGZprefix[i]);
       if (access(buffer,F_OK) == 0)
         return buffer;
    }
  return NULL;
}

/* error message for the filename */

void TGZnotfound OF((const char *fname))
{
  int i;

  fprintf(stderr,"%s : couldn't find ",prog);
  for (i=0;TGZprefix[i];i++)
    fprintf(stderr,(TGZprefix[i+1]) ? "%s%s, " : "or %s%s\n",
            fname,
            TGZprefix[i]);
  exit(1);
}


/* help functions */

int getoct(char *p,int width)
{
  int result = 0;
  char c;
  
  while (width --)
    {
      c = *p++;
      if (c == ' ')
	continue;
      if (c == 0)
	break;
      result = result * 8 + (c - '0');
    }
  return result;
}

char *strtime (time_t *t)
{
  struct tm   *local;
  static char result[32];

  local = localtime(t);
  sprintf(result,"%2d/%02d/%4d %02d:%02d:%02d",
	  local->tm_mday, local->tm_mon+1, local->tm_year+1900,
	  local->tm_hour, local->tm_min,   local->tm_sec);
  return result;
}


/* regular expression matching */

#define ISSPECIAL(c) (((c) == '*') || ((c) == '/'))

int ExprMatch(char *string,char *expr)
{
  while (1)
    {
      if (ISSPECIAL(*expr))
	{
	  if (*expr == '/')
	    {
	      if (*string != '\\' && *string != '/')
		return 0;
	      string ++; expr++;
	    }
	  else if (*expr == '*')
	    {
	      if (*expr ++ == 0)
		return 1;
	      while (*++string != *expr)
		if (*string == 0)
		  return 0;
	    }
	}
      else
	{
	  if (*string != *expr)
	    return 0;
	  if (*expr++ == 0)
	    return 1;
	  string++;
	}
    }
}

/* recursive make directory */
/* abort if you get an ENOENT errno somewhere in the middle */
/* e.g. ignore error "mkdir on existing directory" */
/* */
/* return 1 if OK */
/*        0 on error */

int makedir (char *newdir)
{
  char *buffer = strdup(newdir);
  char *p;
  int  len = strlen(buffer);
  
  if (len <= 0) {
    free(buffer);
    return 0;
  }
  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (mkdir(buffer, 0775) == 0)
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;
      
      while(*p && *p != '\\' && *p != '/')
	p++;
      hold = *p;
      *p = 0;
      if ((mkdir(buffer, 0775) == -1) && (errno == ENOENT))
	{
	  fprintf(stderr,"%s: couldn't create directory %s\n",prog,buffer);
	  free(buffer);
	  return 0;
	}
      if (hold == 0)
	break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}

int matchname (int arg,int argc,char **argv,char *fname)
{
  if (arg == argc)		/* no arguments given (untgz tgzarchive) */
    return 1;

  while (arg < argc)
    if (ExprMatch(fname,argv[arg++]))
      return 1;

  return 0; /* ignore this for the moment being */
}


/* Tar file list or extract */

int tar (gzFile in,int action,int arg,int argc,char **argv)
{
  union  tar_buffer buffer;
  int    len;
  int    err;
  int    getheader = 1;
  int    remaining = 0;
  FILE   *outfile = NULL;
  char   fname[BLOCKSIZE];
  time_t tartime;
  
  if (action == TGZ_LIST)
    printf("     day      time     size                       file\n"
	   " ---------- -------- --------- -------------------------------------\n");
  while (1)
    {
      len = gzread(in, &buffer, BLOCKSIZE);
      if (len < 0)
	error (gzerror(in, &err));
      /*
       * Always expect complete blocks to process
       * the tar information.
       */
      if (len != BLOCKSIZE)
	error("gzread: incomplete block read");
      
      /*
       * If we have to get a tar header
       */
      if (getheader == 1)
	{
	  /*
	   * if we met the end of the tar
	   * or the end-of-tar block,
	   * we are done
	   */
	  if ((len == 0)  || (buffer.header.name[0]== 0)) break;

	  tartime = (time_t)getoct(buffer.header.mtime,12);
	  strcpy(fname,buffer.header.name);
	  
	  switch (buffer.header.typeflag)
	    {
	    case DIRTYPE:
	      if (action == TGZ_LIST)
		printf(" %s     <dir> %s\n",strtime(&tartime),fname);
	      if (action == TGZ_EXTRACT)
		makedir(fname);
	      break;
	    case REGTYPE:
	    case AREGTYPE:
	      remaining = getoct(buffer.header.size,12);
	      if (action == TGZ_LIST)
		printf(" %s %9d %s\n",strtime(&tartime),remaining,fname);
	      if (action == TGZ_EXTRACT)
		{
		  if ((remaining) && (matchname(arg,argc,argv,fname)))
		    {
		      outfile = fopen(fname,"wb");
		      if (outfile == NULL) {
			/* try creating directory */
			char *p = strrchr(fname, '/');
			if (p != NULL) {
			  *p = '\0';
			  makedir(fname);
			  *p = '/';
			  outfile = fopen(fname,"wb");
			}
		      }
		      fprintf(stderr,
			      "%s %s\n",
			      (outfile) ? "Extracting" : "Couldn't create",
			      fname);
		    }
		  else
		    outfile = NULL;
		}
	      /*
	       * could have no contents
	       */
	      getheader = (remaining) ? 0 : 1;
	      break;
	    default:
	      if (action == TGZ_LIST)
		printf(" %s     <---> %s\n",strtime(&tartime),fname);
	      break;
	    }
	}
      else
	{
	  unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;

	  if ((action == TGZ_EXTRACT) && (outfile != NULL))
	    {
	      if (fwrite(&buffer,sizeof(char),bytes,outfile) != bytes)
		{
		  fprintf(stderr,"%s : error writing %s skipping...\n",prog,fname);
		  fclose(outfile);
		  unlink(fname);
		}
	    }
	  remaining -= bytes;
	  if (remaining == 0)
	    {
	      getheader = 1;
	      if ((action == TGZ_EXTRACT) && (outfile != NULL))
		{
#ifdef WIN32
		  HANDLE hFile;
		  FILETIME ftm,ftLocal;
		  SYSTEMTIME st;
		  struct tm localt;
 
		  fclose(outfile);

		  localt = *localtime(&tartime);

		  hFile = CreateFile(fname, GENERIC_READ | GENERIC_WRITE,
				     0, NULL, OPEN_EXISTING, 0, NULL);
		  
		  st.wYear = (WORD)localt.tm_year+1900;
		  st.wMonth = (WORD)localt.tm_mon;
		  st.wDayOfWeek = (WORD)localt.tm_wday;
		  st.wDay = (WORD)localt.tm_mday;
		  st.wHour = (WORD)localt.tm_hour;
		  st.wMinute = (WORD)localt.tm_min;
		  st.wSecond = (WORD)localt.tm_sec;
		  st.wMilliseconds = 0;
		  SystemTimeToFileTime(&st,&ftLocal);
		  LocalFileTimeToFileTime(&ftLocal,&ftm);
		  SetFileTime(hFile,&ftm,NULL,&ftm);
		  CloseHandle(hFile);

		  outfile = NULL;
#else
		  struct utimbuf settime;

		  settime.actime = settime.modtime = tartime;

		  fclose(outfile);
		  outfile = NULL;
		  utime(fname,&settime);
#endif
		}
	    }
	}
    }
  
  if (gzclose(in) != Z_OK)
    error("failed gzclose");

  return 0;
}


/* =========================================================== */

void help(int exitval)
{
  fprintf(stderr,
	  "untgz v 0.1\n"
	  " an sample application of zlib 1.0.4\n\n"
          "Usage : untgz TGZfile            to extract all files\n"
          "        untgz TGZfile fname ...  to extract selected files\n"
          "        untgz -l TGZfile         to list archive contents\n"
          "        untgz -h                 to display this help\n\n");
  exit(exitval);
}

void error(const char *msg)
{
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}


/* ====================================================================== */

int _CRT_glob = 0;	/* disable globbing of the arguments */

int main(int argc,char **argv)
{
    int 	action = TGZ_EXTRACT;
    int 	arg = 1;
    char	*TGZfile;
    gzFile	*f;
    

    prog = strrchr(argv[0],'\\');
    if (prog == NULL)
      {
	prog = strrchr(argv[0],'/');
	if (prog == NULL)
	  {
	    prog = strrchr(argv[0],':');
	    if (prog == NULL)
	      prog = argv[0];
	    else
	      prog++;
	  }
	else
	  prog++;
      }
    else
      prog++;
    
    if (argc == 1)
      help(0);

    if (strcmp(argv[arg],"-l") == 0)
      {
	action = TGZ_LIST;
	if (argc == ++arg)
	  help(0);
      }
    else if (strcmp(argv[arg],"-h") == 0)
      {
	help(0);
      }

    if ((TGZfile = TGZfname(argv[arg])) == NULL)
      TGZnotfound(argv[arg]);            

    ++arg;
    if ((action == TGZ_LIST) && (arg != argc))
      help(1);

/*
 *  Process the TGZ file
 */
    switch(action)
      {
      case TGZ_LIST:
      case TGZ_EXTRACT:
	f = gzopen(TGZfile,"rb");
	if (f == NULL)
	  {
	    fprintf(stderr,"%s: Couldn't gzopen %s\n",
		    prog,
		    TGZfile);
	    return 1;
	  }
	exit(tar(f, action, arg, argc, argv));
      break;
	
      default:
	error("Unknown option!");
	exit(1);
      }

    return 0;
}
