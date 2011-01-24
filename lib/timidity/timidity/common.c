/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   common.c

   */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>
#ifndef __W32__
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#else
#include <process.h>
#include <io.h>
#endif /* __W32__ */
#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "wrd.h"
#include "strtab.h"
#include "support.h"

/* RAND_MAX must defined in stdlib.h
 * Why RAND_MAX is not defined at SunOS?
 */
#if defined(sun) && !defined(SOLARIS) && !defined(RAND_MAX)
#define RAND_MAX ((1<<15)-1)
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* #define MIME_CONVERSION */
#undef DECOMPRESSOR_LIST
#undef PATCH_CONVERTERS
#undef JAPANESE

char *program_name, current_filename[1024];
MBlockList tmpbuffer;
char *output_text_code = NULL;
int open_file_noise_mode = OF_NORMAL;

#ifdef DEFAULT_PATH
    /* The paths in this list will be tried whenever we're reading a file */
    static PathList defaultpathlist={DEFAULT_PATH,0};
    static PathList *pathlist=&defaultpathlist; /* This is a linked list */
#else
    static PathList *pathlist=0;
#endif

const char *note_name[] =
{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};


#ifndef TMP_MAX
#define TMP_MAX 238328
#endif

int
tmdy_mkstemp(char *tmpl)
{
  char *XXXXXX;
  static uint32 value;
  uint32 random_time_bits;
  int count, fd = -1;
  int save_errno = errno;

  /* These are the characters used in temporary filenames.  */
  static const char letters[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  /* This is where the Xs start.  */
  XXXXXX = strstr(tmpl, "XXXXXX");
  if (XXXXXX == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Get some more or less random data.  */
#if HAVE_GETTIMEOFDAY
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    random_time_bits = (uint32)((tv.tv_usec << 16) ^ tv.tv_sec);
  }
#else
  random_time_bits = (uint32)time(NULL);
#endif

  value += random_time_bits ^ getpid();

  for (count = 0; count < TMP_MAX; value += 7777, ++count) {
    uint32 v = value;

    /* Fill in the random bits.  */
    XXXXXX[0] = letters[v % 62];
    v /= 62;
    XXXXXX[1] = letters[v % 62];
    v /= 62;
    XXXXXX[2] = letters[v % 62];

    v = (v << 16) ^ value;
    XXXXXX[3] = letters[v % 62];
    v /= 62;
    XXXXXX[4] = letters[v % 62];
    v /= 62;
    XXXXXX[5] = letters[v % 62];

#if defined(_MSC_VER)
#define S_IRUSR 0
#define S_IWUSR 0
#endif

	fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL | O_BINARY, S_IRUSR | S_IWUSR);

    if (fd >= 0) {
      errno = save_errno;
      return fd;
    }
    if (errno != EEXIST)
      return -1;
  }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return -1;
}


static char *
url_dumpfile(URL url, const char *ext)
{
  char filename[1024];
  char *tmpdir;
  int fd;
  FILE *fp;
  int n;
  char buff[BUFSIZ];

#ifdef TMPDIR
  tmpdir = TMPDIR;
#else
  tmpdir = getenv("TMPDIR");
#endif
  if(tmpdir == NULL || strlen(tmpdir) == 0)
    tmpdir = PATH_STRING "tmp" PATH_STRING;
  if(IS_PATH_SEP(tmpdir[strlen(tmpdir) - 1]))
    snprintf(filename, sizeof(filename), "%sXXXXXX.%s", tmpdir, ext);
  else
    snprintf(filename, sizeof(filename), "%s" PATH_STRING "XXXXXX.%s",
	     tmpdir, ext);

  fd = tmdy_mkstemp(filename);

  if (fd == -1)
    return NULL;

  if ((fp = fdopen(fd, "w")) == NULL) {
    close(fd);
    unlink(filename);
    return NULL;
  }

  while((n = url_read(url, buff, sizeof(buff))) > 0)
    fwrite(buff, 1, n, fp);
  fclose(fp);
  return safe_strdup(filename);
}


/* Try to open a file for reading. If the filename ends in one of the
   defined compressor extensions, pipe the file through the decompressor */
struct timidity_file *try_to_open(char *name, int decompress)
{
    struct timidity_file *tf;
    URL url;
    int len;

    //if((url = url_arc_open(name)) == NULL)
      if((url = url_open(name)) == NULL)
	    return NULL;

    tf = (struct timidity_file *)safe_malloc(sizeof(struct timidity_file));
    tf->url = url;
    tf->tmpname = NULL;

    len = strlen(name);
/*
    if(decompress && len >= 3 && strcasecmp(name + len - 3, ".gz") == 0)
    {
	int method;

	if(!IS_URL_SEEK_SAFE(tf->url))
	{
	    if((tf->url = url_cache_open(tf->url, 1)) == NULL)
	    {
		close_file(tf);
		return NULL;
	    }
	}

	method = skip_gzip_header(tf->url);
	if(method == ARCHIVEC_DEFLATED)
	{
	    url_cache_disable(tf->url);
	    if((tf->url = url_inflate_open(tf->url, -1, 1)) == NULL)
	    {
		close_file(tf);
		return NULL;
	    }

	    // success
	    return tf;
	}
	// fail 
	url_rewind(tf->url);
	url_cache_disable(tf->url);
    }
*/

#ifdef __W32__
    /* Sorry, DECOMPRESSOR_LIST and PATCH_CONVERTERS are not worked yet. */
    return tf;
#endif /* __W32__ */

#if defined(DECOMPRESSOR_LIST)
    if(decompress)
    {
	static char *decompressor_list[] = DECOMPRESSOR_LIST, **dec;
	char tmp[1024];

	/* Check if it's a compressed file */
	for(dec = decompressor_list; *dec; dec += 2)
	{
	    if(!check_file_extension(name, *dec, 0))
		continue;

	    tf->tmpname = url_dumpfile(tf->url, *dec);
	    if (tf->tmpname == NULL) {
		close_file(tf);
		return NULL;
	    }

	    url_close(tf->url);
	    snprintf(tmp, sizeof(tmp), *(dec+1), tf->tmpname);
	    if((tf->url = url_pipe_open(tmp)) == NULL)
	    {
		close_file(tf);
		return NULL;
	    }

	    break;
	}
    }
#endif /* DECOMPRESSOR_LIST */

#if defined(PATCH_CONVERTERS)
    if(decompress == 2)
    {
	static char *decompressor_list[] = PATCH_CONVERTERS, **dec;
	char tmp[1024];

	/* Check if it's a compressed file */
	for(dec = decompressor_list; *dec; dec += 2)
	{
	    if(!check_file_extension(name, *dec, 0))
		continue;

	    tf->tmpname = url_dumpfile(tf->url, *dec);
	    if (tf->tmpname == NULL) {
		close_file(tf);
		return NULL;
	    }

	    url_close(tf->url);
	    sprintf(tmp, *(dec+1), tf->tmpname);
	    if((tf->url = url_pipe_open(tmp)) == NULL)
	    {
		close_file(tf);
		return NULL;
	    }

	    break;
	}
    }
#endif /* PATCH_CONVERTERS */
    
    return tf;
}

int is_url_prefix(const char *name)
{
    int i;

    static char *url_proto_names[] =
    {
	"file:",
#ifdef SUPPORT_SOCKET
	"http://",
	"ftp://",
	"news://",
#endif /* SUPPORT_SOCKET */
	"mime:",
	NULL
    };
    for(i = 0; url_proto_names[i]; i++)
	if(strncmp(name, url_proto_names[i], strlen(url_proto_names[i])) == 0)
	    return 1;
    return 0;
}

static int is_abs_path(const char *name)
{
#ifndef __MACOS__
	if (IS_PATH_SEP(name[0]))
		return 1;
#else
	if (!IS_PATH_SEP(name[0]) && strchr(name, PATH_SEP) != NULL)
		return 1;
#endif /* __MACOS__ */
#ifdef __W32__
    /* [A-Za-z]: (for Windows) */
    if (isalpha(name[0]) && name[1] == ':')
		return 1;
#endif /* __W32__ */
	if (is_url_prefix(name))
		return 1;	/* assuming relative notation is excluded */
	return 0;
}

struct timidity_file *open_with_mem(char *mem, int32 memlen, int noise_mode)
{
    URL url;
    struct timidity_file *tf;

    errno = 0;
    if((url = url_mem_open(mem, memlen, 0)) == NULL)
    {
	if(noise_mode >= 2)
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't open.");
	return NULL;
    }
    tf = (struct timidity_file *)safe_malloc(sizeof(struct timidity_file));
    tf->url = url;
    tf->tmpname = NULL;
    return tf;
}

/* This is meant to find and open files for reading, possibly piping
   them through a decompressor. */
struct timidity_file *open_file(char *name, int decompress, int noise_mode)
{
  struct stat st;
  struct timidity_file *tf;
  PathList *plp=pathlist;
  int l;

  open_file_noise_mode = noise_mode;
  if (!name || !(*name))
    {
      if(noise_mode)
        ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Attempted to open nameless file.");
      return 0;
    }

  /* First try the given name */
  strncpy(current_filename, name, 1023);
  current_filename[1023]='\0';

  if(noise_mode)
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Trying to open %s", current_filename);

  if( stat(current_filename, &st) == 0 && !S_ISDIR(st.st_mode) )
    if ((tf=try_to_open(current_filename, decompress)))
      return tf;

#ifdef __MACOS__
  if(errno)
#else
  if(errno && errno != ENOENT)
#endif
    {
	if(noise_mode)
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		      current_filename, strerror(errno));
	return 0;
    }

  if (!is_abs_path(name))
    while (plp)  /* Try along the path then */
      {
	*current_filename=0;
	l=strlen(plp->path);
	if(l)
	  {
              strncpy(current_filename, plp->path, sizeof(current_filename));
	    if(!IS_PATH_SEP(current_filename[l-1]) &&
	       current_filename[l-1] != '#' &&
	       name[0] != '#')
		strncat(current_filename, PATH_STRING, sizeof(current_filename) - strlen(current_filename) - 1);
	  }
	strncat(current_filename, name, sizeof(current_filename) - strlen(current_filename) - 1);
	if(noise_mode)
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		      "Trying to open %s", current_filename);
	stat(current_filename, &st);
	if(!S_ISDIR(st.st_mode))
	  if ((tf=try_to_open(current_filename, decompress)))
	    return tf;
#ifdef __MACOS__
	if(errno)
#else
	if(errno && errno != ENOENT)
#endif
	{
	    if(noise_mode)
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
			  current_filename, strerror(errno));
	    return 0;
	  }
	plp=plp->next;
      }

  /* Nothing could be opened. */

  *current_filename=0;

  if (noise_mode>=2)
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s", name,
		errno ? strerror(errno) : "Can't open file");

  return 0;
}

/* This closes files opened with open_file */
void close_file(struct timidity_file *tf)
{
    int save_errno = errno;
    if(tf->url != NULL)
    {
#ifndef __W32__
	if(tf->tmpname != NULL)
	{
	    int i;
	    /* dispose the pipe garbage */
	    for(i = 0; tf_getc(tf) != EOF && i < 0xFFFF; i++)
		;
	}
#endif /* __W32__ */
	url_close(tf->url);
    }
    if(tf->tmpname != NULL)
    {
	unlink(tf->tmpname); /* remove temporary file */
	free(tf->tmpname);
    }
    free(tf);
    errno = save_errno;
}

/* This is meant for skipping a few bytes. */
void skip(struct timidity_file *tf, size_t len)
{
    url_skip(tf->url, (long)len);
}

char *tf_gets(char *buff, int n, struct timidity_file *tf)
{
    return url_gets(tf->url, buff, n);
}

long tf_read(void *buff, int32 size, int32 nitems, struct timidity_file *tf)
{
    return url_nread(tf->url, buff, size * nitems) / size;
}

long tf_seek(struct timidity_file *tf, long offset, int whence)
{
    long prevpos;

    prevpos = url_seek(tf->url, offset, whence);
    if(prevpos == -1)
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "Warning: Can't seek file position");
    return prevpos;
}

long tf_tell(struct timidity_file *tf)
{
    long pos;

    pos = url_tell(tf->url);
    if(pos == -1)
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "Warning: Can't get current file position");
	return (long)tf->url->nread;
    }

    return pos;
}

void safe_exit(int status)
{
    if(play_mode->fd != -1)
    {
	play_mode->acntl(PM_REQ_DISCARD, NULL);
	play_mode->close_output();
    }
    ctl->close();
    wrdt->close();
    exit(status);
    /*NOTREACHED*/
}

/* This'll allocate memory or die. */
void *safe_malloc(size_t count)
{
    void *p;
    static int errflag = 0;

    if(errflag)
	safe_exit(10);
    if(count > MAX_SAFE_MALLOC_SIZE)
    {
	errflag = 1;
	ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		  "Strange, I feel like allocating %d bytes. "
		  "This must be a bug.", count);
    }
    else {
      if(count == 0)
	/* Some malloc routine return NULL if count is zero, such as
	 * malloc routine from libmalloc.a of Solaris.
	 * But TiMidity doesn't want to return NULL even if count is zero.
	 */
	count = 1;
      if((p = (void *)malloc(count)) != NULL)
	return p;
      errflag = 1;
      ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		"Sorry. Couldn't malloc %d bytes.", count);
    }
#ifdef ABORT_AT_FATAL
    abort();
#endif /* ABORT_AT_FATAL */
    safe_exit(10);
    /*NOTREACHED*/
}

void *safe_large_malloc(size_t count)
{
    void *p;
    static int errflag = 0;

    if(errflag)
	safe_exit(10);
    if(count == 0)
      /* Some malloc routine return NULL if count is zero, such as
       * malloc routine from libmalloc.a of Solaris.
       * But TiMidity doesn't want to return NULL even if count is zero.
       */
      count = 1;
    if((p = (void *)malloc(count)) != NULL)
      return p;
    errflag = 1;
    ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
	      "Sorry. Couldn't malloc %d bytes.", count);

#ifdef ABORT_AT_FATAL
    abort();
#endif /* ABORT_AT_FATAL */
    safe_exit(10);
    /*NOTREACHED*/
}

void *safe_realloc(void *ptr, size_t count)
{
    void *p;
    static int errflag = 0;

    if(errflag)
	safe_exit(10);
    if(count > MAX_SAFE_MALLOC_SIZE)
    {
	errflag = 1;
	ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		  "Strange, I feel like allocating %d bytes. "
		  "This must be a bug.", count);
    }
    else {
      if (ptr == NULL)
	return safe_malloc(count);
      if(count == 0)
	/* Some malloc routine return NULL if count is zero, such as
	 * malloc routine from libmalloc.a of Solaris.
	 * But TiMidity doesn't want to return NULL even if count is zero.
	 */
	count = 1;
      if((p = (void *)realloc(ptr, count)) != NULL)
	return p;
      errflag = 1;
      ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		"Sorry. Couldn't malloc %d bytes.", count);
    }
#ifdef ABORT_AT_FATAL
    abort();
#endif /* ABORT_AT_FATAL */
    safe_exit(10);
    /*NOTREACHED*/
}

/* This'll allocate memory or die. */
char *safe_strdup(const char *s)
{
    char *p;
    static int errflag = 0;

    if(errflag)
	safe_exit(10);

    if(s == NULL)
	p = strdup("");
    else
	p = strdup(s);
    if(p != NULL)
	return p;
    errflag = 1;
    ctl->cmsg(CMSG_FATAL, VERB_NORMAL, "Sorry. Couldn't alloc memory.");
#ifdef ABORT_AT_FATAL
    abort();
#endif /* ABORT_AT_FATAL */
    safe_exit(10);
    /*NOTREACHED*/
}

/* free ((void **)ptr_list)[0..count-1] and ptr_list itself */
void free_ptr_list(void *ptr_list, int count)
{
	int i;
	for(i = 0; i < count; i++)
		free(((void **)ptr_list)[i]);
	free(ptr_list);
}

static int atoi_limited(const char *string, int v_min, int v_max)
{
	int value = atoi(string);
	
	if (value <= v_min)
		value = v_min;
	else if (value > v_max)
		value = v_max;
	return value;
}

int string_to_7bit_range(const char *string_, int *start, int *end)
{
	const char *string = string_;
	
	if(isdigit(*string)) {
		*start = atoi_limited(string, 0, 127);
		while(isdigit(*++string)) ;
	} else
		*start = 0;
	if (*string == '-') {
		string++;
		*end = isdigit(*string) ? atoi_limited(string, 0, 127) : 127;
		if(*start > *end)
			*end = *start;
	} else
		*end = *start;
	return string != string_;
}

/* This adds a directory to the path list */
void add_to_pathlist(char *s)
{
    PathList *cur, *prev, *plp;

    /* Check duplicated path in the pathlist. */
    plp = prev = NULL;
    for(cur = pathlist; cur; prev = cur, cur = cur->next)
	if(pathcmp(s, cur->path, 0) == 0)
	{
	    plp = cur;
	    break;
	}

    if(plp) /* found */
    {
	if(prev == NULL) /* first */
	    pathlist = pathlist->next;
	else
	    prev->next = plp->next;
    }
    else
    {
	/* Allocate new path */
	plp = safe_malloc(sizeof(PathList));
	plp->path = safe_strdup(s);
    }

    plp->next = pathlist;
    pathlist = plp;
}

void clean_up_pathlist(void)
{
  PathList *cur, *next;

  cur = pathlist;
  while (cur) {
    next = cur->next;
#ifdef DEFAULT_PATH
    if (cur == &defaultpathlist) {
      cur = next;
      continue;
    }
#endif
    free(cur->path);
    free(cur);
    cur = next;
  }

#ifdef DEFAULT_PATH
  pathlist = &defaultpathlist;
#else
  pathlist = NULL;
#endif
}

#ifndef HAVE_VOLATILE
/*ARGSUSED*/
int volatile_touch(void *dmy) {return 1;}
#endif /* HAVE_VOLATILE */

/* code converters */
static unsigned char
      w2k[] = {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
               144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
               160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
               176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
               225,226,247,231,228,229,246,250,233,234,235,236,237,238,239,240,
               242,243,244,245,230,232,227,254,251,253,255,249,248,252,224,241,
               193,194,215,199,196,197,214,218,201,202,203,204,205,206,207,208,
               210,211,212,213,198,200,195,222,219,221,223,217,216,220,192,209};

static void code_convert_cp1251(char *in, char *out, int maxlen)
{
    int i;
    if(out == NULL)
        out = in;
    for(i = 0; i < maxlen && in[i]; i++)
    {
	if(in[i] & 0200)
	    out[i] = w2k[in[i] & 0177];
	else
	    out[i] = in[i];
    }
    out[i]='\0';
}

static void code_convert_dump(char *in, char *out, int maxlen, char *ocode)
{
    if(ocode == NULL)
	ocode = output_text_code;

    if(ocode != NULL && ocode != (char *)-1
	&& (strstr(ocode, "ASCII") || strstr(ocode, "ascii")))
    {
	int i;

	if(out == NULL)
	    out = in;
	for(i = 0; i < maxlen && in[i]; i++)
	    if(in[i] < ' ' || in[i] >= 127)
		out[i] = '.';
	    else
		out[i] = in[i];
	out[i]='\0';
    }
    else /* "NOCNV" */
    {
	if(out == NULL)
	    return;
	strncpy(out, in, maxlen);
	out[maxlen] = '\0';
    }
}

#ifdef JAPANESE
static void code_convert_japan(char *in, char *out, int maxlen,
			       char *icode, char *ocode)
{
    static char *mode = NULL, *wrd_mode = NULL;

    if(ocode != NULL && ocode != (char *)-1)
    {
	nkf_convert(in, out, maxlen, icode, ocode);
	if(out != NULL)
	    out[maxlen] = '\0';
	return;
    }

    if(mode == NULL || wrd_mode == NULL)
    {
	mode = output_text_code;
	if(mode == NULL || strstr(mode, "AUTO"))
	{
#ifndef __W32__
	    mode = getenv("LANG");
#else
	    mode = "SJIS";
	    wrd_mode = "SJISK";
#endif
	    if(mode == NULL || *mode == '\0')
	    {
		mode = "ASCII";
		wrd_mode = mode;
	    }
	}

	if(strstr(mode, "ASCII") ||
	   strstr(mode, "ascii"))
	{
	    mode = "ASCII";
	    wrd_mode = mode;
	}
	else if(strstr(mode, "NOCNV") ||
		strstr(mode, "nocnv"))
	{
	    mode = "NOCNV";
	    wrd_mode = mode;
	}
#ifndef	HPUX
	else if(strstr(mode, "EUC") ||
		strstr(mode, "euc") ||
		strstr(mode, "ujis") ||
		strcmp(mode, "japanese") == 0)
	{
	    mode = "EUC";
	    wrd_mode = "EUCK";
	}
	else if(strstr(mode, "SJIS") ||
		strstr(mode, "sjis"))
	{
	    mode = "SJIS";
	    wrd_mode = "SJISK";
	}
#else
	else if(strstr(mode, "EUC") ||
		strstr(mode, "euc") ||
		strstr(mode, "ujis"))
	{
	    mode = "EUC";
	    wrd_mode = "EUCK";
	}
	else if(strstr(mode, "SJIS") ||
		strstr(mode, "sjis") ||
		strcmp(mode, "japanese") == 0)
	{
	    mode = "SJIS";
	    wrd_mode = "SJISK";
	}
#endif	/* HPUX */
	else if(strstr(mode,"JISk")||
		strstr(mode,"jisk"))
	{
	    mode = "JISK";
	    wrd_mode = mode;
	}
	else if(strstr(mode, "JIS") ||
		strstr(mode, "jis"))
	{
	    mode = "JIS";
	    wrd_mode = "JISK";
	}
	else if(strcmp(mode, "ja") == 0)
	{
	    mode = "EUC";
	    wrd_mode = "EUCK";
	}
	else
	{
	    mode = "NOCNV";
	    wrd_mode = mode;
	}
    }

    if(ocode == NULL)
    {
	if(strcmp(mode, "NOCNV") == 0)
	{
	    if(out == NULL)
		return;
	    strncpy(out, in, maxlen);
	    out[maxlen] = '\0';
	}
	else if(strcmp(mode, "ASCII") == 0)
	    code_convert_dump(in, out, maxlen, "ASCII");
	else
	{
	    nkf_convert(in, out, maxlen, icode, mode);
	    if(out != NULL)
		out[maxlen] = '\0';
	}
    }
    else if(ocode == (char *)-1)
    {
	if(strcmp(wrd_mode, "NOCNV") == 0)
	{
	    if(out == NULL)
		return;
	    strncpy(out, in, maxlen);
	    out[maxlen] = '\0';
	}
	else if(strcmp(wrd_mode, "ASCII") == 0)
	    code_convert_dump(in, out, maxlen, "ASCII");
	else
	{
	    nkf_convert(in, out, maxlen, icode, wrd_mode);
	    if(out != NULL)
		out[maxlen] = '\0';
	}
    }
}
#endif /* JAPANESE */

void code_convert(char *in, char *out, int outsiz, char *icode, char *ocode)
{
#if !defined(MIME_CONVERSION) && defined(JAPANESE)
    int i;
    /* check ASCII string */
    for(i = 0; in[i]; i++)
	if(in[i] < ' ' || in[i] >= 127)
	    break;
    if(!in[i])
    {
	if(out == NULL)
	    return;
	strncpy(out, in, outsiz - 1);
	out[outsiz - 1] = '\0';
	return; /* All ASCII string */
    }
#endif /* MIME_CONVERSION */

    if(ocode != NULL && ocode != (char *)-1)
    {
	if(strcasecmp(ocode, "nocnv") == 0)
	{
	    if(out == NULL)
		return;
	    outsiz--;
	    strncpy(out, in, outsiz);
	    out[outsiz] = '\0';
	    return;
	}

	if(strcasecmp(ocode, "ascii") == 0)
	{
	    code_convert_dump(in, out, outsiz - 1, "ASCII");
	    return;
	}

	if(strcasecmp(ocode, "1251") == 0)
	{
	    code_convert_cp1251(in, out, outsiz - 1);
	    return;
	}
    }

#if defined(JAPANESE)
    code_convert_japan(in, out, outsiz - 1, icode, ocode);
#else
    code_convert_dump(in, out, outsiz - 1, ocode);
#endif
}

/* EAW -- insert stuff from playlist files
 *
 * Tue Apr 6 1999: Modified by Masanao Izumo <mo@goice.co.jp>
 *                 One pass implemented.
 */
static char **expand_file_lists(char **files, int *nfiles_in_out)
{
    int nfiles;
    int i;
    char input_line[256];
    char *pfile;
    static const char *testext = ".m3u.pls.asx.M3U.PLS.ASX";
    struct timidity_file *list_file;
    char *one_file[1];
    int one;

    /* Recusive global */
    static StringTable st;
    static int error_outflag = 0;
    static int depth = 0;

    if(depth >= 16)
    {
	if(!error_outflag)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "Probable loop in playlist files");
	    error_outflag = 1;
	}
	return NULL;
    }

    if(depth == 0)
    {
	error_outflag = 0;
	init_string_table(&st);
    }
    nfiles = *nfiles_in_out;

    /* Expand playlist recursively */
    for(i = 0; i < nfiles; i++)
    {
	/* extract the file extension */
	pfile = strrchr(files[i], '.');

	if(*files[i] == '@' || (pfile != NULL && strstr(testext, pfile)))
	{
	    /* Playlist file */
            if(*files[i] == '@')
		list_file = open_file(files[i] + 1, 1, 1);
            else
		list_file = open_file(files[i], 1, 1);
            if(list_file)
	    {
                while(tf_gets(input_line, sizeof(input_line), list_file)
		      != NULL) {
            	    if(*input_line == '\n' || *input_line == '\r')
			continue;
		    if((pfile = strchr(input_line, '\r')))
		    	*pfile = '\0';
		    if((pfile = strchr(input_line, '\n')))
		    	*pfile = '\0';
		    one_file[0] = input_line;
		    one = 1;
		    depth++;
		    expand_file_lists(one_file, &one);
		    depth--;
		}
                close_file(list_file);
            }
        }
	else /* Other file */
	    put_string_table(&st, files[i], strlen(files[i]));
    }

    if(depth)
	return NULL;
    *nfiles_in_out = st.nstring;
    return make_string_array(&st);
}


#ifdef RAND_MAX
int int_rand(int n)
{
    if(n < 0)
    {
	if(n == -1)
	    srand(time(NULL));
	else
	    srand(-n);
	return n;
    }
    return (int)(n * (double)rand() * (1.0 / (RAND_MAX + 1.0)));
}
#else
int int_rand(int n)
{
    static unsigned long rnd_seed = 0xabcd0123;

    if(n < 0)
    {
	if(n == -1)
	    rnd_seed = time(NULL);
	else
	    rnd_seed = -n;
	return n;
    }

    rnd_seed *= 69069UL;
    return (int)(n * (double)(rnd_seed & 0xffffffff) *
		 (1.0 / (0xffffffff + 1.0)));
}
#endif /* RAND_MAX */

int check_file_extension(char *filename, char *ext, int decompress)
{
    int len, elen, i;
#if defined(DECOMPRESSOR_LIST)
    char *dlist[] = DECOMPRESSOR_LIST;
#endif /* DECOMPRESSOR_LIST */

    len = strlen(filename);
    elen = strlen(ext);
    if(len > elen && strncasecmp(filename + len - elen, ext, elen) == 0)
	return 1;

    if(decompress)
    {
	/* Check gzip'ed file name */

	if(len > 3 + elen &&
	   strncasecmp(filename + len - elen - 3 , ext, elen) == 0 &&
	   strncasecmp(filename + len - 3, ".gz", 3) == 0)
	    return 1;

#if defined(DECOMPRESSOR_LIST)
	for(i = 0; dlist[i]; i += 2)
	{
	    int dlen;

	    dlen = strlen(dlist[i]);
	    if(len > dlen + elen &&
	       strncasecmp(filename + len - elen - dlen , ext, elen) == 0 &&
	       strncasecmp(filename + len - dlen, dlist[i], dlen) == 0)
		return 1;
	}
#endif /* DECOMPRESSOR_LIST */
    }
    return 0;
}

void randomize_string_list(char **strlist, int n)
{
    int i, j;
    char *tmp;
    for(i = 0; i < n; i++)
    {
	j = int_rand(n - i);
	tmp = strlist[j];
	strlist[j] = strlist[n - i - 1];
	strlist[n - i - 1] = tmp;
    }
}

int pathcmp(const char *p1, const char *p2, int ignore_case)
{
    int c1, c2;

#ifdef __W32__
    ignore_case = 1;	/* Always ignore the case */
#endif

    do {
	c1 = *p1++ & 0xff;
	c2 = *p2++ & 0xff;
	if(ignore_case)
	{
	    c1 = tolower(c1);
	    c2 = tolower(c2);
	}
	if(IS_PATH_SEP(c1)) c1 = *p1 ? 0x100 : 0;
	if(IS_PATH_SEP(c2)) c2 = *p2 ? 0x100 : 0;
    } while(c1 == c2 && c1 /* && c2 */);

    return c1 - c2;
}

static int pathcmp_qsort(const char **p1,
			 const char **p2)
{
    return pathcmp(*(const char **)p1, *(const char **)p2, 1);
}

void sort_pathname(char **files, int nfiles)
{
    qsort(files, nfiles, sizeof(char *),
	  (int (*)(const void *, const void *))pathcmp_qsort);
}

char *pathsep_strchr(const char *path)
{
#ifdef PATH_SEP2
    while(*path)
    {
        if(*path == PATH_SEP || *path == PATH_SEP2)
	    return (char *)path;
	path++;
    }
    return NULL;
#else
    return strchr(path, PATH_SEP);
#endif
}

char *pathsep_strrchr(const char *path)
{
#ifdef PATH_SEP2
    char *last_sep = NULL;
    while(*path)
    {
        if(*path == PATH_SEP || *path == PATH_SEP2)
	    last_sep = (char *)path;
	path++;
    }
    return last_sep;
#else
    return strrchr(path, PATH_SEP);
#endif
}

int str2mID(char *str)
{
    int val;

    if(strncasecmp(str, "gs", 2) == 0)
	val = 0x41;
    else if(strncasecmp(str, "xg", 2) == 0)
	val = 0x43;
    else if(strncasecmp(str, "gm", 2) == 0)
	val = 0x7e;
    else
    {
	int i, v;
	val = 0;
	for(i = 0; i < 2; i++)
	{
	    v = str[i];
	    if('0' <= v && v <= '9')
		v = v - '0';
	    else if('A' <= v && v <= 'F')
		v = v - 'A' + 10;
	    else if('a' <= v && v <= 'f')
		v = v - 'a' + 10;
	    else
		return 0;
	    val = (val << 4 | v);
	}
    }
    return val;
}
