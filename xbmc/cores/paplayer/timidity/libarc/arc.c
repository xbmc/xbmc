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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdlib.h>

#include "timidity.h"
#include "common.h"
#include "arc.h"
#include "strtab.h"
#include "zip.h"
#include "unlzh.h"
#include "explode.h"

char *arc_lib_version = ARC_LIB_VERSION;

#define GZIP_ASCIIFLAG		(1u<<0)
#define GZIP_MULTIPARTFLAG	(1u<<1)
#define GZIP_EXTRAFLAG		(1u<<2)
#define GZIP_FILEFLAG		(1u<<3)
#define GZIP_COMMFLAG		(1u<<4)
#define GZIP_ENCFLAG		(1u<<5)

#ifndef TRUE
#define TRUE			1
#endif /* TRUE */
#ifndef FALSE
#define FALSE			0
#endif /* FALSE */
#define ABORT			-1

ArchiveHandler arc_handler;
static MBlockList arc_buffer;

typedef struct _ArchiveFileList
{
    char *archive_name;
    ArchiveEntryNode *entry_list;
    struct _ArchiveFileList *next;
} ArchiveFileList;
static ArchiveFileList *arc_filelist = NULL;

static struct
{
    char *ext;
    int type;
} archive_ext_list[] =
{
    {".tar",	ARCHIVE_TAR},
    {".tar.gz",	ARCHIVE_TGZ},
    {".tgz",	ARCHIVE_TGZ},
    {".zip",	ARCHIVE_ZIP},
    {".neo",	ARCHIVE_ZIP},
    {".lzh",	ARCHIVE_LZH},
    {".lha",	ARCHIVE_LZH},
    {".mime",	ARCHIVE_MIME},
    {PATH_STRING, ARCHIVE_DIR},
    {NULL, -1}
};

int skip_gzip_header(URL url)
{
    unsigned char flags;
    int m1, method;

    /* magic */
    m1 = url_getc(url);
    if(m1 == 0)
    {
	url_skip(url, 128 - 1);
	m1 = url_getc(url);
    }
    if(m1 != 0x1f)
	return -1;
    if(url_getc(url) != 0x8b)
	return -1;

    /* method */
    method = url_getc(url);
    switch(method)
    {
      case 8:			/* deflated */
	method = ARCHIVEC_DEFLATED;
	break;
      default:
	return -1;
    }
    /* flags */
    flags = url_getc(url);
    if(flags & GZIP_ENCFLAG)
	return -1;
    /* time */
    url_getc(url); url_getc(url); url_getc(url); url_getc(url);

    url_getc(url);		/* extra flags */
    url_getc(url);		/* OS type */

    if(flags & GZIP_MULTIPARTFLAG)
    {
	/* part number */
	url_getc(url); url_getc(url);
    }

    if(flags & GZIP_EXTRAFLAG)
    {
	unsigned short len;
	int i;

	/* extra field */

	len = url_getc(url);
	len |= ((unsigned short)url_getc(url)) << 8;
	for(i = 0; i < len; i++)
	    url_getc(url);
    }

    if(flags & GZIP_FILEFLAG)
    {
	/* file name */
	int c;

	do
	{
	    c = url_getc(url);
	    if(c == EOF)
		return -1;
	} while(c != '\0');
    }

    if(flags & GZIP_COMMFLAG)
    {
	/* comment */
	int c;

	do
	{
	    c = url_getc(url);
	    if(c == EOF)
		return -1;
	} while(c != '\0');
    }

    return method;
}

int parse_gzip_header_bytes(char *gz, long maxparse, int *hdrsiz)
{
    URL url = url_mem_open(gz, maxparse, 0);
    int method;

    if(!url)
	return -1;
    method = skip_gzip_header(url);
    *hdrsiz = url_tell(url);
    url_close(url);
    return method;
}

void (* arc_error_handler)(char *error_message) = NULL;
    
static void arc_cant_open(char *s)
{
    if(arc_error_handler != NULL)
    {
	char buff[BUFSIZ];
	snprintf(buff, sizeof(buff), "%s: Can't open", s);
	arc_error_handler(buff);
    }
}

int get_archive_type(char *archive_name)
{
    int i, len;
    char *p;
    int archive_name_length, delim;

#ifdef SUPPORT_SOCKET
    int type = url_check_type(archive_name);
    if(type == URL_news_t)
	return ARCHIVE_MIME;
    if(type == URL_newsgroup_t)
	return ARCHIVE_NEWSGROUP;
#endif /* SUPPORT_SOCKET */

    if(strncmp(archive_name, "mail:", 5) == 0 ||
       strncmp(archive_name, "mime:", 5) == 0)
	return ARCHIVE_MIME;

    if((p = strrchr(archive_name, '#')) != NULL)
    {
	archive_name_length = p - archive_name;
	delim = '#';
    }
    else
    {
	archive_name_length = strlen(archive_name);
	delim = '\0';
    }

    for(i = 0; archive_ext_list[i].ext; i++)
    {
	len = strlen(archive_ext_list[i].ext);
	if(len <= archive_name_length &&
	   strncasecmp(archive_name + archive_name_length - len,
		       archive_ext_list[i].ext, len) == 0 &&
	   archive_name[archive_name_length] == delim)
	    return archive_ext_list[i].type; /* Found */
    }

    if(url_check_type(archive_name) == URL_dir_t)
	return ARCHIVE_DIR;

    return -1;			/* Not found */
}

static ArchiveFileList *find_arc_filelist(char *basename)
{
    ArchiveFileList *p;

    for(p = arc_filelist; p; p = p->next)
    {
	if(strcmp(basename, p->archive_name) == 0)
	    return p;
    }
    return NULL;
}

ArchiveEntryNode *arc_parse_entry(URL url, int archive_type)
{
    ArchiveEntryNode *entry_first, *entry_last, *entry;
    ArchiveEntryNode *(* next_header_entry)(void);
    int gzip_method;
    URL orig;

    orig = NULL;
    switch(archive_type)
    {
      case ARCHIVE_TAR:
	next_header_entry = next_tar_entry;
	break;
      case ARCHIVE_TGZ:
	gzip_method = skip_gzip_header(url);
	if(gzip_method != ARCHIVEC_DEFLATED)
	{
	    url_close(url);
	    return NULL;
	}
	orig = url;
	if((url = url_inflate_open(orig, -1, 0)) == NULL)
	    return NULL;
	next_header_entry = next_tar_entry;
	break;
      case ARCHIVE_ZIP:
	next_header_entry = next_zip_entry;
	break;
      case ARCHIVE_LZH:
	next_header_entry = next_lzh_entry;
	break;
      case ARCHIVE_MIME:
	if(!IS_URL_SEEK_SAFE(url))
	{
	    orig = url;
	    if((url = url_cache_open(orig, 0)) == NULL)
		return NULL;
	}
	next_header_entry = next_mime_entry;
	break;
      default:
	return NULL;
    }

    arc_handler.isfile = (url->type == URL_file_t);
    arc_handler.url = url;
    arc_handler.counter = 0;
    entry_first = entry_last = NULL;
    arc_handler.pos = 0;
    while((entry = next_header_entry()) != NULL)
    {
	if(entry_first != NULL)
	    entry_last->next = entry;
	else
	    entry_first = entry_last = entry;
	while(entry_last->next)
	    entry_last = entry_last->next;
	arc_handler.counter++;
    }
    url_close(url);
    if(orig)
	url_close(orig);
    return entry_first;		/* Note that NULL if no archive file */
}

static ArchiveFileList *add_arc_filelist(char *basename, int archive_type)
{
    URL url;
    ArchiveFileList *afl;
    ArchiveEntryNode *entry;

    switch(archive_type)
    {
      case ARCHIVE_TAR:
      case ARCHIVE_TGZ:
      case ARCHIVE_ZIP:
      case ARCHIVE_LZH:
      case ARCHIVE_MIME:
	break;
      default:
	return NULL;
    }

    if((url = url_open(basename)) == NULL)
    {
	arc_cant_open(basename);
	return NULL;
    }

    entry = arc_parse_entry(url, archive_type);

    afl = (ArchiveFileList *)safe_malloc(sizeof(ArchiveFileList));
    afl->archive_name = safe_strdup(basename);
    afl->entry_list = entry;
    afl->next = arc_filelist;
    arc_filelist = afl;
    return afl;
}

static ArchiveFileList *regist_archive(char *archive_filename)
{
    int archive_type;

    if((archive_type = get_archive_type(archive_filename)) < 0)
	return NULL;		/* Unknown archive */
    archive_filename = url_expand_home_dir(archive_filename);
    if(find_arc_filelist(archive_filename))
	return NULL;		/* Already registerd */
    return add_arc_filelist(archive_filename, archive_type);
}

static int arc_expand_newfile(StringTable *s, ArchiveFileList *afl,
			      char *pattern)
{
    ArchiveEntryNode *entry;
    char *p;

    for(entry = afl->entry_list; entry; entry = entry->next)
    {
	if(arc_case_wildmat(entry->name, pattern))
	{
	    p = new_segment(&arc_buffer, strlen(afl->archive_name) +
			    strlen(entry->name) + 2);
	    strcpy(p, afl->archive_name);
	    strcat(p, "#");
	    strcat(p, entry->name);
	    if(put_string_table(s, p, strlen(p)) == NULL)
		return -1;
	}
    }
    return 0;
}

char **expand_archive_names(int *nfiles_in_out, char **files)
{
    int i, nfiles, arc_type;
    char *infile_name;
    char *base, *pattern, *p, buff[BUFSIZ];
    char *one_file[1];
    int one;
    ArchiveFileList *afl;

    /* Recusive global */
    static MBlockList *pool;
    static StringTable stab;
    static int error_flag = 0;
    static int depth = 0;

    if(depth == 0)
    {
	error_flag = 0;
	init_string_table(&stab);
	pool = &arc_buffer;
    }

    nfiles = *nfiles_in_out;

    for(i = 0; i < nfiles; i++)
    {
	infile_name = url_expand_home_dir(files[i]);
	if((p = strrchr(infile_name, '#')) == NULL)
	{
	    base = infile_name;
	    pattern = "*";
	}
	else
	{
	    int len = p - infile_name;
	    base = new_segment(pool, len + 1); /* +1 for '\0' */
	    memcpy(base, infile_name, len);
	    base[len] = '\0';
	    pattern = p + 1;
	}

	if((afl = find_arc_filelist(base)) != NULL)
	{
	    if(arc_expand_newfile(&stab, afl, pattern) == -1)
		goto abort_expand;
	    continue;
	}

	arc_type = get_archive_type(base);
	if(arc_type == -1)
	{
	    if(put_string_table(&stab, infile_name, strlen(infile_name))
	       == NULL)
		goto abort_expand;
	    continue;
	}

#ifdef SUPPORT_SOCKET
	if(arc_type == ARCHIVE_NEWSGROUP)
	{
	    URL url;
	    int len1, len2;
	    char *news_prefix;

	    if((url = url_newsgroup_open(base)) == NULL)
	    {
		arc_cant_open(base);
		continue;
	    }

	    strncpy(buff, base, sizeof(buff)-1);
	    p = strchr(buff + 7, '/') + 1; /* news://..../ */
	    *p = '\0';
	    news_prefix = strdup_mblock(pool, buff);
	    len1 = strlen(news_prefix);

	    while(url_gets(url, buff, sizeof(buff)))
	    {
		len2 = strlen(buff);
		p = (char *)new_segment(pool, len1 + len2 + 1);
		strcpy(p, news_prefix);
		strcpy(p + len1, buff);
		one_file[0] = p;
		one = 1;
		depth++;
		expand_archive_names(&one, one_file);
		depth--;
	    }
	    url_close(url);
	    if(error_flag)
		goto abort_expand;
	    continue;
	}
#endif /* SUPPORT_SOCKET */

	if(arc_type == ARCHIVE_DIR)
	{
	    URL url;
	    int len1, len2;

	    if((url = url_dir_open(base)) == NULL)
	    {
		arc_cant_open(base);
		continue;
	    }

	    if(strncmp(base, "dir:", 4) == 0)
		base += 4;
	    len1 = strlen(base);
	    if(IS_PATH_SEP(base[len1 - 1]))
		len1--;
	    while(url_gets(url, buff, sizeof(buff)))
	    {
		if(strcmp(buff, ".") == 0 || strcmp(buff, "..") == 0)
		    continue;

		len2 = strlen(buff);
		p = (char *)new_segment(pool, len1 + len2 + 2);
		strcpy(p, base);
		p[len1] = PATH_SEP;
		strcpy(p + len1 + 1, buff);
		one_file[0] = p;
		one = 1;
		depth++;
		expand_archive_names(&one, one_file);
		depth--;
	    }
	    url_close(url);
	    if(error_flag)
		goto abort_expand;
	    continue;
	}

	if((afl = add_arc_filelist(base, arc_type)) != NULL)
	{
	    if(arc_expand_newfile(&stab, afl, pattern) == -1)
		goto abort_expand;
	}
    }

    if(depth)
	return NULL;
    *nfiles_in_out = stab.nstring;
    reuse_mblock(pool);
    return make_string_array(&stab); /* It is possible NULL */

  abort_expand:
    error_flag = 1;
    if(depth)
	return NULL;
    delete_string_table(&stab);
    free_global_mblock();
    *nfiles_in_out = 0;
    return NULL;
}

ArchiveEntryNode *new_entry_node(char *name, int len)
{
    ArchiveEntryNode *entry;
    entry = (ArchiveEntryNode *)safe_malloc(sizeof(ArchiveEntryNode));
    memset(entry, 0, sizeof(ArchiveEntryNode));
    entry->name = (char *)safe_malloc(len + 1);
    memcpy(entry->name, name, len);
    entry->name[len] = '\0';
    return entry;
}

void free_entry_node(ArchiveEntryNode *entry)
{
    free(entry->name);
    if(entry->cache != NULL)
	free(entry->cache);
    free(entry);
}


static char *compress_buff;
long   compress_buff_len;
static long arc_compress_func(char *buff, long size, void *user_val)
{
    if(compress_buff_len <= 0)
	return 0;
    if(size > compress_buff_len)
	size = compress_buff_len;
    memcpy(buff, compress_buff, size);
    compress_buff += size;
    compress_buff_len -= size;
    return size;
}

void *arc_compress(void *buff, long bufsiz,
		   int compress_level, long *compressed_size)
{
    DeflateHandler compressor;
    long allocated, offset, space, nbytes;
    char *compressed;

    compress_buff = (char *)buff;
    compress_buff_len = bufsiz;
    compressor = open_deflate_handler(arc_compress_func, NULL,
				      compress_level);
    allocated = 1024;
    compressed = (char *)safe_malloc(allocated);
    offset = 0;
    space = allocated;
    while((nbytes = zip_deflate(compressor, compressed + offset, space)) > 0)
    {
	offset += nbytes;
	space -= nbytes;
	if(space == 0)
	{
	    space = allocated;
	    allocated += space;
	    compressed = (char *)safe_realloc(compressed, allocated);
	}
    }
    close_deflate_handler(compressor);
    if(offset == 0)
    {
	free(buff);
	return NULL;
    }
    *compressed_size = offset;
    return compressed;
}

void *arc_decompress(void *buff, long bufsiz, long *decompressed_size)
{
    InflateHandler decompressor;
    long allocated, offset, space, nbytes;
    char *decompressed;

    compress_buff = (char *)buff;
    compress_buff_len = bufsiz;
    decompressor = open_inflate_handler(arc_compress_func, NULL);
    allocated = 1024;
    decompressed = (char *)safe_malloc(allocated);
    offset = 0;
    space = allocated;
    while((nbytes = zip_inflate(decompressor, decompressed + offset, space)) > 0)
    {
	offset += nbytes;
	space -= nbytes;
	if(space == 0)
	{
	    space = allocated;
	    allocated += space;
	    decompressed = (char *)safe_realloc(decompressed, allocated);
	}
    }
    close_inflate_handler(decompressor);
    if(offset == 0)
    {
	free(buff);
	return NULL;
    }
    *decompressed_size = offset;
    return decompressed;
}

void free_archive_files(void)
{
    ArchiveEntryNode *entry, *ecur;
    ArchiveFileList *acur;

    while(arc_filelist)
    {
	acur = arc_filelist;
	arc_filelist = arc_filelist->next;
	entry = acur->entry_list;
	while(entry)
	{
	    ecur = entry;
	    entry = entry->next;
	    free_entry_node(ecur);
	}
	free(acur->archive_name);
	free(acur);
    }
}

/******************************************************************************
* url_arc
*****************************************************************************/
typedef struct _URL_arc
{
    char common[sizeof(struct _URL)];
    URL instream;
    long pos, size;
    int comptype;
    void *decoder;
} URL_arc;

static long url_arc_read(URL url, void *buff, long n);
static long url_arc_tell(URL url);
static void url_arc_close(URL url);

static long archiver_read_func(char *buff, long buff_size, void *v)
{
    URL_arc *url;
    long n;

    url = (URL_arc *)v;

    if(url->size < 0)
	n = buff_size;
    else
    {
	n = url->size - url->pos;
	if(n > buff_size)
	    n = buff_size;
    }

    if(n <= 0)
	return 0;
    n = url_read(url->instream, buff, n);
    if(n <= 0)
	return n;

    return n;
}

URL url_arc_open(char *name)
{
    URL_arc *url;
    char *base, *p;
    int len;
    ArchiveFileList *afl;
    ArchiveEntryNode *entry;
    URL instream;

    if((p = strrchr(name, '#')) == NULL)
	return NULL;
    len = p - name;
    base = new_segment(&arc_buffer, len + 1);
    memcpy(base, name, len);
    base[len] = '\0';
    base = url_expand_home_dir(base);

    if((afl = find_arc_filelist(base)) == NULL)
	afl = regist_archive(base);
    if(afl == NULL)
	return NULL;
    reuse_mblock(&arc_buffer);	/* free `base' */
    name += len + 1;
    while(name[0] == '/') name++;	/* skip '/'s right after # */

    for(entry = afl->entry_list; entry; entry = entry->next)
    {
	if(strcasecmp(entry->name, name) == 0)
	    break;
    }
    if(entry == NULL)
	return NULL;

    if(entry->cache != NULL)
	instream = url_mem_open((char *)entry->cache + entry->start,
				entry->compsize, 0);
    else
    {
	if((instream = url_file_open(base)) == NULL)
	    return NULL;
	url_seek(instream, entry->start, 0);
    }

    url = (URL_arc *)alloc_url(sizeof(URL_arc));
    if(url == NULL)
    {
	url_errno = errno;
	return NULL;
    }

    /* open decoder */
    switch(entry->comptype)
    {
      case ARCHIVEC_STORED:	/* No compression */
      case ARCHIVEC_LZHED_LH0:	/* -lh0- */
      case ARCHIVEC_LZHED_LZ4:	/* -lz4- */
	url->decoder = NULL;

      case ARCHIVEC_DEFLATED:	/* deflate */
	url->decoder =
	    (void *)open_inflate_handler(archiver_read_func, url);
	if(url->decoder == NULL)
	{
	    url_arc_close((URL)url);
	    return NULL;
	}
	break;

      case ARCHIVEC_IMPLODED_LIT8:
      case ARCHIVEC_IMPLODED_LIT4:
      case ARCHIVEC_IMPLODED_NOLIT8:
      case ARCHIVEC_IMPLODED_NOLIT4:
	url->decoder =
	    (void *)open_explode_handler(archiver_read_func,
					 entry->comptype - ARCHIVEC_IMPLODED - 1,
					 entry->compsize, entry->origsize, url);
	if(url->decoder == NULL)
	{
	    url_arc_close((URL)url);
	    return NULL;
	}
	break;

      case ARCHIVEC_LZHED_LH1:	/* -lh1- */
      case ARCHIVEC_LZHED_LH2:	/* -lh2- */
      case ARCHIVEC_LZHED_LH3:	/* -lh3- */
      case ARCHIVEC_LZHED_LH4:	/* -lh4- */
      case ARCHIVEC_LZHED_LH5:	/* -lh5- */
      case ARCHIVEC_LZHED_LZS:	/* -lzs- */
      case ARCHIVEC_LZHED_LZ5:	/* -lz5- */
      case ARCHIVEC_LZHED_LHD:	/* -lhd- */
      case ARCHIVEC_LZHED_LH6:	/* -lh6- */
      case ARCHIVEC_LZHED_LH7:	/* -lh7- */
	url->decoder =
	    (void *)open_unlzh_handler(
				       archiver_read_func,
				       lzh_methods[entry->comptype - ARCHIVEC_LZHED - 1],
				       entry->compsize, entry->origsize, url);
	if(url->decoder == NULL)
	{
	    url_arc_close((URL)url);
	    return NULL;
	}
	break;
      default:
	url_arc_close((URL)url);
	return NULL;
    }


    /* common members */
    URLm(url, type)      = URL_arc_t;
    URLm(url, url_read)  = url_arc_read;
    URLm(url, url_gets)  = NULL;
    URLm(url, url_fgetc) = NULL;
    URLm(url, url_seek)  = NULL;
    URLm(url, url_tell)  = url_arc_tell;
    URLm(url, url_close) = url_arc_close;

    /* private members */
    url->instream = instream;
    url->pos = 0;
    url->size = entry->origsize;
    url->comptype = entry->comptype;
    return (URL)url;
}

static long url_arc_read(URL url, void *vp, long bufsiz)
{
    URL_arc *urlp = (URL_arc *)url;
    long n = 0;
    int comptype;
    void *decoder;
    char *buff = (char *)vp;

    if(urlp->pos == -1)
	return 0;

    comptype = urlp->comptype;
    decoder = urlp->decoder;
    switch(comptype)
    {
      case ARCHIVEC_STORED:
      case ARCHIVEC_LZHED_LH0:	/* -lh0- */
      case ARCHIVEC_LZHED_LZ4:	/* -lz4- */
	n = archiver_read_func(buff, bufsiz, (void *)urlp);
	break;

      case ARCHIVEC_DEFLATED:
	n = zip_inflate((InflateHandler)decoder, buff, bufsiz);
	break;

      case ARCHIVEC_IMPLODED_LIT8:
      case ARCHIVEC_IMPLODED_LIT4:
      case ARCHIVEC_IMPLODED_NOLIT8:
      case ARCHIVEC_IMPLODED_NOLIT4:
	n = explode((ExplodeHandler)decoder, buff, bufsiz);
	break;

      case ARCHIVEC_LZHED_LH1:	/* -lh1- */
      case ARCHIVEC_LZHED_LH2:	/* -lh2- */
      case ARCHIVEC_LZHED_LH3:	/* -lh3- */
      case ARCHIVEC_LZHED_LH4:	/* -lh4- */
      case ARCHIVEC_LZHED_LH5:	/* -lh5- */
      case ARCHIVEC_LZHED_LZS:	/* -lzs- */
      case ARCHIVEC_LZHED_LZ5:	/* -lz5- */
      case ARCHIVEC_LZHED_LHD:	/* -lhd- */
      case ARCHIVEC_LZHED_LH6:	/* -lh6- */
      case ARCHIVEC_LZHED_LH7:	/* -lh7- */
	n = unlzh((UNLZHHandler)decoder, buff, bufsiz);
	break;

      case ARCHIVEC_UU:		/* uu encoded */
      case ARCHIVEC_B64:	/* base64 encoded */
      case ARCHIVEC_QS:		/* quoted string encoded */
      case ARCHIVEC_HQX:	/* HQX encoded */
	n = url_read((URL)decoder, buff, bufsiz);
	break;
    }

    if(n > 0)
	urlp->pos += n;
    return n;
}

static long url_arc_tell(URL url)
{
    return ((URL_arc *)url)->pos;
}

static void url_arc_close(URL url)
{
    URL_arc *urlp = (URL_arc *)url;
    void *decoder;
    int save_errno = errno;

    /* 1. close decoder 
	* 2. close decode_stream
	    * 3. free url
		*/

    decoder = urlp->decoder;
    if(decoder != NULL)
    {
	switch(urlp->comptype)
	{
	  case ARCHIVEC_DEFLATED:
	    close_inflate_handler((InflateHandler)decoder);
	    break;

	  case ARCHIVEC_IMPLODED_LIT8:
	  case ARCHIVEC_IMPLODED_LIT4:
	  case ARCHIVEC_IMPLODED_NOLIT8:
	  case ARCHIVEC_IMPLODED_NOLIT4:
	    close_explode_handler((ExplodeHandler)decoder);
	    break;

	  case ARCHIVEC_LZHED_LH1: /* -lh1- */
	  case ARCHIVEC_LZHED_LH2: /* -lh2- */
	  case ARCHIVEC_LZHED_LH3: /* -lh3- */
	  case ARCHIVEC_LZHED_LH4: /* -lh4- */
	  case ARCHIVEC_LZHED_LH5: /* -lh5- */
	  case ARCHIVEC_LZHED_LZS: /* -lzs- */
	  case ARCHIVEC_LZHED_LZ5: /* -lz5- */
	  case ARCHIVEC_LZHED_LHD: /* -lhd- */
	  case ARCHIVEC_LZHED_LH6: /* -lh6- */
	  case ARCHIVEC_LZHED_LH7: /* -lh7- */
	    close_unlzh_handler((UNLZHHandler)decoder);
	    break;

	  case ARCHIVEC_UU:	/* uu encoded */
	  case ARCHIVEC_B64:	/* base64 encoded */
	  case ARCHIVEC_QS:	/* quoted string encoded */
	  case ARCHIVEC_HQX:	/* HQX encoded */
	    url_close((URL)decoder);
	    break;
	}
    }

    if(urlp->instream != NULL)
	url_close(urlp->instream);
    free(urlp);
    errno = save_errno;
}



/************** wildmat ***************/
/* What character marks an inverted character class? */
#define NEGATE_CLASS		'!'

/* Is "*" a common pattern? */
#define OPTIMIZE_JUST_STAR

/* Do tar(1) matching rules, which ignore a trailing slash? */
#undef MATCH_TAR_PATTERN

/* Define if case is ignored */
#define MATCH_CASE_IGNORE

#include <ctype.h>
#define TEXT_CASE_CHAR(c) (toupper(c))
#define CHAR_CASE_COMP(a, b) (TEXT_CASE_CHAR(a) == TEXT_CASE_CHAR(b))

static char *ParseHex(char *p, int *val)
{
    int i, v;

    *val = 0;
    for(i = 0; i < 2; i++)
    {
	v = *p++;
	if('0' <= v && v <= '9')
	    v = v - '0';
	else if('A' <= v && v <= 'F')
	    v = v - 'A' + 10;
	else if('a' <= v && v <= 'f')
	    v = v - 'a' + 10;
	else
	    return NULL;
	*val = (*val << 4 | v);
    }
    return p;
}

/*
 *  Match text and p, return TRUE, FALSE, or ABORT.
 */
static int DoMatch(char *text, char *p)
{
    register int	last;
    register int	matched;
    register int	reverse;

    for ( ; *p; text++, p++) {
	if (*text == '\0' && *p != '*')
	    return ABORT;
	switch (*p) {
	case '\\':
	    p++;
	    if(*p == 'x')
	    {
		int c;
		if((p = ParseHex(++p, &c)) == NULL)
		    return ABORT;
		if(*text != c)
		    return FALSE;
		continue;
	    }
	    /* Literal match with following character. */

	    /* FALLTHROUGH */
	default:
	    if (*text != *p)
		return FALSE;
	    continue;
	case '?':
	    /* Match anything. */
	    continue;
	case '*':
	    while (*++p == '*')
		/* Consecutive stars act just like one. */
		continue;
	    if (*p == '\0')
		/* Trailing star matches everything. */
		return TRUE;
	    while (*text)
		if ((matched = DoMatch(text++, p)) != FALSE)
		    return matched;
	    return ABORT;
	case '[':
	    reverse = p[1] == NEGATE_CLASS ? TRUE : FALSE;
	    if (reverse)
		/* Inverted character class. */
		p++;
	    matched = FALSE;
	    if (p[1] == ']' || p[1] == '-')
		if (*++p == *text)
		    matched = TRUE;
	    for (last = *p; *++p && *p != ']'; last = *p)
		/* This next line requires a good C compiler. */
		if (*p == '-' && p[1] != ']'
		    ? *text <= *++p && *text >= last : *text == *p)
		    matched = TRUE;
	    if (matched == reverse)
		return FALSE;
	    continue;
	}
    }

#ifdef	MATCH_TAR_PATTERN
    if (*text == '/')
	return TRUE;
#endif	/* MATCH_TAR_ATTERN */
    return *text == '\0';
}

static int DoCaseMatch(char *text, char *p)
{
    register int	last;
    register int	matched;
    register int	reverse;

    for(; *p; text++, p++)
    {
	if(*text == '\0' && *p != '*')
	    return ABORT;
	switch (*p)
	{
	  case '\\':
	    p++;
	    if(*p == 'x')
	    {
		int c;
		if((p = ParseHex(++p, &c)) == NULL)
		    return ABORT;
		if(!CHAR_CASE_COMP(*text, c))
		    return FALSE;
		continue;
	    }
	    /* Literal match with following character. */

	    /* FALLTHROUGH */
	  default:
	    if(!CHAR_CASE_COMP(*text, *p))
		return FALSE;
	    continue;
	  case '?':
	    /* Match anything. */
	    continue;
	  case '*':
	    while(*++p == '*')
		/* Consecutive stars act just like one. */
		continue;
	    if(*p == '\0')
		/* Trailing star matches everything. */
		return TRUE;
	    while(*text)
		if((matched = DoCaseMatch(text++, p)) != FALSE)
		    return matched;
	    return ABORT;
	case '[':
	    reverse = p[1] == NEGATE_CLASS ? TRUE : FALSE;
	    if(reverse)
		/* Inverted character class. */
		p++;
	    matched = FALSE;
	    if(p[1] == ']' || p[1] == '-')
	    {
		if(*++p == *text)
		    matched = TRUE;
	    }
	    for(last = TEXT_CASE_CHAR(*p); *++p && *p != ']';
		last = TEXT_CASE_CHAR(*p))
	    {
		/* This next line requires a good C compiler. */
		if(*p == '-' && p[1] != ']')
		{
		    p++;
		    if(TEXT_CASE_CHAR(*text) <= TEXT_CASE_CHAR(*p) &&
		       TEXT_CASE_CHAR(*text) >= last)
			matched = TRUE;
		}
		else
		{
		    if(CHAR_CASE_COMP(*text, *p))
			matched = TRUE;
		}
	    }
	    if(matched == reverse)
		return FALSE;
	    continue;
	}
    }

#ifdef	MATCH_TAR_PATTERN
    if (*text == '/')
	return TRUE;
#endif	/* MATCH_TAR_ATTERN */
    return *text == '\0';
}

/*
**  User-level routine.  Returns TRUE or FALSE.
*/
int arc_wildmat(char *text, char *p)
{
#ifdef	OPTIMIZE_JUST_STAR
    if (p[0] == '*' && p[1] == '\0')
	return TRUE;
#endif /* OPTIMIZE_JUST_STAR */
    return DoMatch(text, p) == TRUE;
}

int arc_case_wildmat(char *text, char *p)
{
#ifdef	OPTIMIZE_JUST_STAR
    if (p[0] == '*' && p[1] == '\0')
	return TRUE;
#endif /* OPTIMIZE_JUST_STAR */
    return DoCaseMatch(text, p) == TRUE;
}
