/* 
   Unix SMB/CIFS implementation.
   stdio replacement
   Copyright (C) Andrew Tridgell 2001
   
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

/*
  stdio is very convenient, but on some systems the file descriptor
  in FILE* is 8 bits, so it fails when more than 255 files are open. 

  XFILE replaces stdio. It is less efficient, but at least it works
  when you have lots of files open

  The main restriction on XFILE is that it doesn't support seeking,
  and doesn't support O_RDWR. That keeps the code simple.
*/

#include "includes.h"

#define XBUFSIZE BUFSIZ

static XFILE _x_stdin =  { 0, NULL, NULL, XBUFSIZE, 0, O_RDONLY, X_IOFBF, 0 };
static XFILE _x_stdout = { 1, NULL, NULL, XBUFSIZE, 0, O_WRONLY, X_IOLBF, 0 };
static XFILE _x_stderr = { 2, NULL, NULL, 0, 0, O_WRONLY, X_IONBF, 0 };

XFILE *x_stdin = &_x_stdin;
XFILE *x_stdout = &_x_stdout;
XFILE *x_stderr = &_x_stderr;

#define X_FLAG_EOF 1
#define X_FLAG_ERROR 2
#define X_FLAG_EINVAL 3

/* simulate setvbuf() */
int x_setvbuf(XFILE *f, char *buf, int mode, size_t size)
{
	x_fflush(f);
	if (f->bufused) return -1;

	/* on files being read full buffering is the only option */
	if ((f->open_flags & O_ACCMODE) == O_RDONLY) {
		mode = X_IOFBF;
	}

	/* destroy any earlier buffer */
	SAFE_FREE(f->buf);
	f->buf = 0;
	f->bufsize = 0;
	f->next = NULL;
	f->bufused = 0;
	f->buftype = mode;

	if (f->buftype == X_IONBF) return 0;

	/* if buffering then we need some size */
	if (size == 0) size = XBUFSIZE;

	f->bufsize = size;
	f->bufused = 0;

	return 0;
}

/* allocate the buffer */
static int x_allocate_buffer(XFILE *f)
{
	if (f->buf) return 1;
	if (f->bufsize == 0) return 0;
	f->buf = SMB_MALLOC(f->bufsize);
	if (!f->buf) return 0;
	f->next = f->buf;
	return 1;
}


/* this looks more like open() than fopen(), but that is quite deliberate.
   I want programmers to *think* about O_EXCL, O_CREAT etc not just
   get them magically added 
*/
XFILE *x_fopen(const char *fname, int flags, mode_t mode)
{
	XFILE *ret;

	ret = SMB_MALLOC_P(XFILE);
	if (!ret) {
		return NULL;
	}

	memset(ret, 0, sizeof(XFILE));

	if ((flags & O_ACCMODE) == O_RDWR) {
		/* we don't support RDWR in XFILE - use file 
		   descriptors instead */
		SAFE_FREE(ret);
		errno = EINVAL;
		return NULL;
	}

	ret->open_flags = flags;

	ret->fd = sys_open(fname, flags, mode);
	if (ret->fd == -1) {
		SAFE_FREE(ret);
		return NULL;
	}

	x_setvbuf(ret, NULL, X_IOFBF, XBUFSIZE);
	
	return ret;
}

XFILE *x_fdup(const XFILE *f)
{
	XFILE *ret;
	int fd;

	fd = dup(x_fileno(f));
	if (fd < 0) {
		return NULL;
	}

	ret = SMB_CALLOC_ARRAY(XFILE, 1);
	if (!ret) {
		close(fd);
		return NULL;
	}

	ret->fd = fd;
	ret->open_flags = f->open_flags;
	x_setvbuf(ret, NULL, X_IOFBF, XBUFSIZE);
	return ret;
}

/* simulate fclose() */
int x_fclose(XFILE *f)
{
	int ret;

	/* make sure we flush any buffered data */
	x_fflush(f);

	ret = close(f->fd);
	f->fd = -1;
	if (f->buf) {
		/* make sure data can't leak into a later malloc */
		memset(f->buf, 0, f->bufsize);
		SAFE_FREE(f->buf);
	}
	/* check the file descriptor given to the function is NOT one of the static
	 * descriptor of this libreary or we will free unallocated memory
	 * --sss */
	if (f != x_stdin && f != x_stdout && f != x_stderr) {
		SAFE_FREE(f);
	}
	return ret;
}

/* simulate fwrite() */
size_t x_fwrite(const void *p, size_t size, size_t nmemb, XFILE *f)
{
	ssize_t ret;
	size_t total=0;

	/* we might be writing unbuffered */
	if (f->buftype == X_IONBF || 
	    (!f->buf && !x_allocate_buffer(f))) {
		ret = write(f->fd, p, size*nmemb);
		if (ret == -1) return -1;
		return ret/size;
	} 


	while (total < size*nmemb) {
		size_t n = f->bufsize - f->bufused;
		n = MIN(n, (size*nmemb)-total);

		if (n == 0) {
			/* it's full, flush it */
			x_fflush(f);
			continue;
		}

		memcpy(f->buf + f->bufused, total+(const char *)p, n);
		f->bufused += n;
		total += n;
	}

	/* when line buffered we need to flush at the last linefeed. This can
	   flush a bit more than necessary, but that is harmless */
	if (f->buftype == X_IOLBF && f->bufused) {
		int i;
		for (i=(size*nmemb)-1; i>=0; i--) {
			if (*(i+(const char *)p) == '\n') {
				x_fflush(f);
				break;
			}
		}
	}

	return total/size;
}

/* thank goodness for asprintf() */
 int x_vfprintf(XFILE *f, const char *format, va_list ap)
{
	char *p;
	int len, ret;
	va_list ap2;

	VA_COPY(ap2, ap);

	len = vasprintf(&p, format, ap2);
	if (len <= 0) return len;
	ret = x_fwrite(p, 1, len, f);
	SAFE_FREE(p);
	return ret;
}

 int x_fprintf(XFILE *f, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = x_vfprintf(f, format, ap);
	va_end(ap);
	return ret;
}

/* at least fileno() is simple! */
int x_fileno(const XFILE *f)
{
	return f->fd;
}

/* simulate fflush() */
int x_fflush(XFILE *f)
{
	int ret;

	if (f->flags & X_FLAG_ERROR) return -1;

	if ((f->open_flags & O_ACCMODE) != O_WRONLY) {
		errno = EINVAL;
		return -1;
	}

	if (f->bufused == 0 || !f->buf) return 0;

	ret = write(f->fd, f->buf, f->bufused);
	if (ret == -1) return -1;
	
	f->bufused -= ret;
	if (f->bufused > 0) {
		f->flags |= X_FLAG_ERROR;
		memmove(f->buf, ret + (char *)f->buf, f->bufused);
		return -1;
	}

	return 0;
}

/* simulate setbuffer() */
void x_setbuffer(XFILE *f, char *buf, size_t size)
{
	x_setvbuf(f, buf, buf?X_IOFBF:X_IONBF, size);
}

/* simulate setbuf() */
void x_setbuf(XFILE *f, char *buf)
{
	x_setvbuf(f, buf, buf?X_IOFBF:X_IONBF, XBUFSIZE);
}

/* simulate setlinebuf() */
void x_setlinebuf(XFILE *f)
{
	x_setvbuf(f, NULL, X_IOLBF, 0);
}


/* simulate feof() */
int x_feof(XFILE *f)
{
	if (f->flags & X_FLAG_EOF) return 1;
	return 0;
}

/* simulate ferror() */
int x_ferror(XFILE *f)
{
	if (f->flags & X_FLAG_ERROR) return 1;
	return 0;
}

/* fill the read buffer */
static void x_fillbuf(XFILE *f)
{
	int n;

	if (f->bufused) return;

	if (!f->buf && !x_allocate_buffer(f)) return;

	n = read(f->fd, f->buf, f->bufsize);
	if (n <= 0) return;
	f->bufused = n;
	f->next = f->buf;
}

/* simulate fgetc() */
int x_fgetc(XFILE *f)
{
	int ret;

	if (f->flags & (X_FLAG_EOF | X_FLAG_ERROR)) return EOF;
	
	if (f->bufused == 0) x_fillbuf(f);

	if (f->bufused == 0) {
		f->flags |= X_FLAG_EOF;
		return EOF;
	}

	ret = *(unsigned char *)(f->next);
	f->next++;
	f->bufused--;
	return ret;
}

/* simulate fread */
size_t x_fread(void *p, size_t size, size_t nmemb, XFILE *f)
{
	size_t total = 0;
	while (total < size*nmemb) {
		int c = x_fgetc(f);
		if (c == EOF) break;
		(total+(char *)p)[0] = (char)c;
		total++;
	}
	return total/size;
}

/* simulate fgets() */
char *x_fgets(char *s, int size, XFILE *stream) 
{
	char *s0 = s;
	int l = size;
	while (l>1) {
		int c = x_fgetc(stream);
		if (c == EOF) break;
		*s++ = (char)c;
		l--;
		if (c == '\n') break;
	}
	if (l==size || x_ferror(stream)) {
		return 0;
	}
	*s = 0;
	return s0;
}

/* trivial seek, works only for SEEK_SET and SEEK_END if SEEK_CUR is
 * set then an error is returned */
off_t x_tseek(XFILE *f, off_t offset, int whence)
{
	if (f->flags & X_FLAG_ERROR)
		return -1;

	/* only SEEK_SET and SEEK_END are supported */
	/* SEEK_CUR needs internal offset counter */
	if (whence != SEEK_SET && whence != SEEK_END) {
		f->flags |= X_FLAG_EINVAL;
		errno = EINVAL;
		return -1;
	}

	/* empty the buffer */
	switch (f->open_flags & O_ACCMODE) {
	case O_RDONLY:
		f->bufused = 0;
		break;
	case O_WRONLY:
		if (x_fflush(f) != 0)
			return -1;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	f->flags &= ~X_FLAG_EOF;
	return (off_t)sys_lseek(f->fd, offset, whence);
}
