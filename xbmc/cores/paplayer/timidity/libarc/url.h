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

#ifndef ___URL_H_
#define ___URL_H_

/* This header file from liburl-1.8.3.
 * You can get full source from:
 * http://www.goice.co.jp/member/mo/release/index.html#liburl
 */


#define URL_LIB_VERSION "1.9.5"

/* Define if you want to enable pipe command scheme ("command|") */
#define PIPE_SCHEME_ENABLE

/* Define if you want to appended on a user's home directory if a filename
 * is beginning with '~'
 */
#if !defined(__MACOS__) && !defined(__W32__)
#define TILD_SCHEME_ENABLE
#endif

/* Define if you want to use soft directory cache */
#ifndef URL_DIR_CACHE_DISABLE
#define URL_DIR_CACHE_ENABLE
#endif /* URL_DIR_CACHE_DISABLE */

/* Define if you want to use XOVER command in NNTP */
/* #define URL_NEWS_XOVER_SUPPORT "XOVER", "XOVERVIEW" */

/* M:Must, O:Optional defined */
typedef struct _URL
{
    int   type;								/* M */

    long  (* url_read)(struct _URL *url, void *buff, long n);		/* M */
    char *(* url_gets)(struct _URL *url, char *buff, int n);		/* O */
    int   (* url_fgetc)(struct _URL *url);				/* O */
    long  (* url_seek)(struct _URL *url, long offset, int whence);	/* O */
    long  (* url_tell)(struct _URL *url);				/* O */
    void  (* url_close)(struct _URL *url);				/* M */

    unsigned long nread;	/* Reset in url_seek, url_rewind,
				   url_set_readlimit */
    unsigned long readlimit;
    int		  eof;		/* Used in url_nread and others */
} *URL;
#define URLm(url, m) (((URL)url)->m)

#define url_eof(url) URLm((url), eof)

/* open URL stream */
extern URL url_open(char *url_string);

/* close URL stream */
extern void url_close(URL url);

/* read n bytes */
extern long url_read(URL url, void *buff, long n);
extern long url_safe_read(URL url, void *buff, long n);
extern long url_nread(URL url, void *buff, long n);

/* read a line */
/* Like a fgets */
extern char *url_gets(URL url, char *buff, int n);

/* Allow termination by CR or LF or both. Ignored empty lines.
 * CR or LF is truncated.
 * Success: length of the line.
 * EOF or Error: EOF
 */
extern int url_readline(URL url, char *buff, int n);

/* read a byte */
extern int url_fgetc(URL url);
#define url_getc(url) \
    ((url)->nread >= (url)->readlimit ? ((url)->eof = 1, EOF) : \
     (url)->url_fgetc != NULL ? ((url)->nread++, (url)->url_fgetc(url)) : \
      url_fgetc(url))

/* seek position */
extern long url_seek(URL url, long offset, int whence);

/* get the current position */
extern long url_tell(URL url);

/* skip n bytes */
extern void url_skip(URL url, long n);

/* seek to first position */
extern void url_rewind(URL url);

/* dump */
void *url_dump(URL url, long nbytes, long *real_read);

/* set read limit */
void url_set_readlimit(URL url, long readlimit);

/* url_errno to error message */
extern char *url_strerror(int no);

/* allocate URL structure */
extern URL alloc_url(int size);

/* Check URL type. */
extern int url_check_type(char *url_string);

/* replace `~' to user directory */
extern char *url_expand_home_dir(char *filename);
extern char *url_unexpand_home_dir(char *filename);

extern int url_errno;
enum url_errtypes
{
    URLERR_NONE = 10000,	/* < 10000 represent system call's errno */
    URLERR_NOURL,		/* Unknown URL */
    URLERR_OPERM,		/* Operation not permitted */
    URLERR_CANTOPEN,		/* Can't open a URL */
    URLERR_IURLF,		/* Invalid URL form */
    URLERR_URLTOOLONG,		/* URL too long */
    URLERR_NOMAILADDR,		/* No mail address */
    URLERR_MAXNO
};

struct URL_module
{
    /* url type */
    int type;

    /* URL checker */
    int (* name_check)(char *url_string);

    /* Once call just before url_open(). */
    int (* url_init)(void);

    /* Open specified URL */
    URL (* url_open)(char *url_string);

    /* chain next modules */
    struct URL_module *chain;
};

extern void url_add_module(struct URL_module *m);
extern void url_add_modules(struct URL_module *m, ...);

extern URL url_file_open(char *filename);
extern URL url_dir_open(char *directory_name);
extern URL url_http_open(char *url_string);
extern URL url_ftp_open(char *url_string);
extern URL url_newsgroup_open(char *url_string);
extern URL url_news_open(char *url_string);
extern URL url_pipe_open(char *command);

/* No URL_module */
extern URL url_mem_open(char *memory, long memsiz, int autofree);
extern URL url_inflate_open(URL instream, long compsize, int autoclose);
extern URL url_buff_open(URL url, int autoclose);
extern URL url_cache_open(URL url, int autoclose);
extern void url_cache_detach(URL url);
extern void url_cache_disable(URL url);
extern URL url_uudecode_open(URL reader, int autoclose);
extern URL url_b64decode_open(URL reader, int autoclose);
extern URL url_hqxdecode_open(URL reader, int dataonly, int autoclose);
extern URL url_qsdecode_open(URL reader, int autoclose);
extern URL url_cgi_escape_open(URL reader, int autoclose);
extern URL url_cgi_unescape_open(URL reader, int autoclose);

extern char *url_dir_name(URL url);
extern char *url_newsgroup_name(URL url);
extern int url_news_connection_cache(int flag);

extern char *url_lib_version;
extern char *user_mailaddr;
extern char *url_user_agent;
extern char *url_http_proxy_host;
extern unsigned short url_http_proxy_port;
extern char *url_ftp_proxy_host;
extern unsigned short url_ftp_proxy_port;
extern int url_newline_code;
extern int uudecode_unquote_html;

enum url_types
{
    URL_none_t,			/* Undefined URL */
    URL_file_t,			/* File system */
    URL_dir_t,			/* Directory entry */
    URL_http_t,			/* HTTP */
    URL_ftp_t,			/* FTP */
    URL_news_t,			/* NetNews article */
    URL_newsgroup_t,		/* NetNews group */
    URL_pipe_t,			/* Pipe */
    URL_mem_t,			/* On memory */
    URL_buff_t,			/* Buffered stream */
    URL_cache_t,		/* Cached stream */
    URL_uudecode_t,		/* UU decoder */
    URL_b64decode_t,		/* Base64 decoder */
    URL_qsdecode_t,		/* Quoted-string decoder */
    URL_hqxdecode_t,		/* HQX decoder */
    URL_cgi_escape_t,		/* WWW CGI Escape */
    URL_cgi_unescape_t,		/* WWW CGI Unescape */
    URL_arc_t,			/* arc stream */

    URL_inflate_t = 99,		/* LZ77 decode stream */

    URL_extension_t = 100	/* extentional stream >= 100 */
};

enum url_news_conn_type
{
    URL_NEWS_CONN_NO_CACHE,
    URL_NEWS_CONN_CACHE,
    URL_NEWS_CLOSE_CACHE,
    URL_NEWS_GET_FLAG
};

#define IS_URL_SEEK_SAFE(url) ((url)->url_seek != NULL && \
			       (url)->type != URL_buff_t)

#define URL_MAX_READLIMIT ((~(unsigned long)0) >> 1)
#endif /* ___URL_H_ */
