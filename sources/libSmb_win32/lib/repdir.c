/* 
   Unix SMB/CIFS implementation.

   Copyright (C) Andrew Tridgell 2005
   Updated for Samba3 64-bit cleanliness (C) Jeremy Allison 2006
   
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
  a replacement for opendir/readdir/telldir/seekdir/closedir for BSD systems

  This is needed because the existing directory handling in FreeBSD
  and OpenBSD (and possibly NetBSD) doesn't correctly handle unlink()
  on files in a directory where telldir() has been used. On a block
  boundary it will occasionally miss a file when seekdir() is used to
  return to a position previously recorded with telldir().

  This also fixes a severe performance and memory usage problem with
  telldir() on BSD systems. Each call to telldir() in BSD adds an
  entry to a linked list, and those entries are cleaned up on
  closedir(). This means with a large directory closedir() can take an
  arbitrary amount of time, causing network timeouts as millions of
  telldir() entries are freed

  Note! This replacement code is not portable. It relies on getdents()
  always leaving the file descriptor at a seek offset that is a
  multiple of DIR_BUF_SIZE. If the code detects that this doesn't
  happen then it will abort(). It also does not handle directories
  with offsets larger than can be stored in a long,

  This code is available under other free software licenses as
  well. Contact the author.
*/

#include <include/includes.h>

 void replace_readdir_dummy(void);
 void replace_readdir_dummy(void) {}

#if defined(REPLACE_READDIR)

#if defined(PARANOID_MALLOC_CHECKER)
#ifdef malloc
#undef malloc
#endif
#endif

#define DIR_BUF_BITS 9
#define DIR_BUF_SIZE (1<<DIR_BUF_BITS)

struct dir_buf {
	int fd;
	int nbytes, ofs;
	SMB_OFF_T seekpos;
	char buf[DIR_BUF_SIZE];
};

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OPENDIR64)
 SMB_STRUCT_DIR *opendir64(const char *dname)
#else
 SMB_STRUCT_DIR *opendir(const char *dname)
#endif
{
	struct dir_buf *d;
	d = malloc(sizeof(*d));
	if (d == NULL) {
		errno = ENOMEM;
		return NULL;
	}
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OPEN64)
	d->fd = open64(dname, O_RDONLY);
#else
	d->fd = open(dname, O_RDONLY);
#endif

	if (d->fd == -1) {
		free(d);
		return NULL;
	}
	d->ofs = 0;
	d->seekpos = 0;
	d->nbytes = 0;
	return (SMB_STRUCT_DIR *)d;
}

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_READDIR64)
 SMB_STRUCT_DIRENT *readdir64(SMB_STRUCT_DIR *dir)
#else
 SMB_STRUCT_DIRENT *readdir(SMB_STRUCT_DIR *dir)
#endif
{
	struct dir_buf *d = (struct dir_buf *)dir;
	SMB_STRUCT_DIRENT *de;

	if (d->ofs >= d->nbytes) {
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_LSEEK64)
		d->seekpos = lseek64(d->fd, 0, SEEK_CUR);
#else
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
#endif

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_GETDENTS64)
		d->nbytes = getdents64(d->fd, d->buf, DIR_BUF_SIZE);
#else
		d->nbytes = getdents(d->fd, d->buf, DIR_BUF_SIZE);
#endif
		d->ofs = 0;
	}
	if (d->ofs >= d->nbytes) {
		return NULL;
	}
	de = (SMB_STRUCT_DIRENT *)&d->buf[d->ofs];
	d->ofs += de->d_reclen;
	return de;
}

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_TELLDIR64)
 long telldir64(SMB_STRUCT_DIR *dir)
#else
 long telldir(SMB_STRUCT_DIR *dir)
#endif
{
	struct dir_buf *d = (struct dir_buf *)dir;
	if (d->ofs >= d->nbytes) {
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_LSEEK64)
		d->seekpos = lseek64(d->fd, 0, SEEK_CUR);
#else
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
#endif
		d->ofs = 0;
		d->nbytes = 0;
	}
	/* this relies on seekpos always being a multiple of
	   DIR_BUF_SIZE. Is that always true on BSD systems? */
	if (d->seekpos & (DIR_BUF_SIZE-1)) {
		abort();
	}
	return d->seekpos + d->ofs;
}

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_SEEKDIR64)
 void seekdir64(SMB_STRUCT_DIR *dir, long ofs)
#else
 void seekdir(SMB_STRUCT_DIR *dir, long ofs)
#endif
{
	struct dir_buf *d = (struct dir_buf *)dir;
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_LSEEK64)
	d->seekpos = lseek64(d->fd, ofs & ~(DIR_BUF_SIZE-1), SEEK_SET);
#else
	d->seekpos = lseek(d->fd, ofs & ~(DIR_BUF_SIZE-1), SEEK_SET);
#endif

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_GETDENTS64)
	d->nbytes = getdents64(d->fd, d->buf, DIR_BUF_SIZE);
#else
	d->nbytes = getdents(d->fd, d->buf, DIR_BUF_SIZE);
#endif

	d->ofs = 0;
	while (d->ofs < (ofs & (DIR_BUF_SIZE-1))) {
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_READDIR64)
		if (readdir64(dir) == NULL) break;
#else
		if (readdir(dir) == NULL) break;
#endif
	}
}

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_REWINDDIR64)
 void rewinddir64(SMB_STRUCT_DIR *dir)
#else
 void rewinddir(SMB_STRUCT_DIR *dir)
#endif
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_SEEKDIR64)
	seekdir64(dir, 0);
#else
	seekdir(dir, 0);
#endif
}

#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_CLOSEDIR64)
 int closedir64(SMB_STRUCT_DIR *dir)
#else
 int closedir(SMB_STRUCT_DIR *dir)
#endif
{
	struct dir_buf *d = (struct dir_buf *)dir;
	int r = close(d->fd);
	if (r != 0) {
		return r;
	}
	free(d);
	return 0;
}

#ifndef dirfd
/* darn, this is a macro on some systems. */
 int dirfd(SMB_STRUCT_DIR *dir)
{
	struct dir_buf *d = (struct dir_buf *)dir;
	return d->fd;
}
#endif
#endif /* REPLACE_READDIR */
