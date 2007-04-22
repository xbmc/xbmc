/* Content-Encoding support, stolen from ELinks, as usual */

/* Stream reading and decoding (mostly decompression) */
/* $Id: encoding.c,v 1.1.1.1 2003/04/13 20:54:56 karpov Exp $ */

#include "links.h"
#ifdef HAVE_BZLIB_H
#include <bzlib.h> /* Everything needs this after stdio.h */
#endif
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif


/* TODO: When more decoders will join the game, we should probably move them
 * to separate files, maybe even to separate directory. --pasky */


struct decoding_handlers {
	int (*open)(struct stream_encoded *stream, int fd);
	int (*read)(struct stream_encoded *stream, unsigned char *data, int len);
	void (*close)(struct stream_encoded *stream);
	unsigned char **(*listext)();
};


/*************************************************************************
  Dummy encoding (ENCODING_NONE)
*************************************************************************/

struct dummy_enc_data {
	int fd;
};

static int
dummy_open(struct stream_encoded *stream, int fd)
{
	stream->data = mem_alloc(sizeof(struct dummy_enc_data));
	if (!stream->data) return -1;

	((struct dummy_enc_data *) stream->data)->fd = fd;

	return 0;
}

static int
dummy_read(struct stream_encoded *stream, unsigned char *data, int len)
{
	return read(((struct dummy_enc_data *) stream->data)->fd, data, len);
}

static void
dummy_close(struct stream_encoded *stream)
{
	close(((struct dummy_enc_data *) stream->data)->fd);
	mem_free(stream->data);
}

static unsigned char ** dummy_listext()
{
	return NULL;
}

struct decoding_handlers dummy_handlers = {
	dummy_open,
	dummy_read,
	dummy_close,
	dummy_listext,
};


/*************************************************************************
  Gzip encoding (ENCODING_GZIP)
*************************************************************************/

#ifdef HAVE_ZLIB_H

static int
gzip_open(struct stream_encoded *stream, int fd)
{
	stream->data = (void *) gzdopen(fd, "rb");
	if (!stream->data) return -1;

	return 0;
}

static int
gzip_read(struct stream_encoded *stream, unsigned char *data, int len)
{
	return gzread((gzFile *) stream->data, data, len);
}

static void
gzip_close(struct stream_encoded *stream)
{
	gzclose((gzFile *) stream->data);
}

unsigned char **gzip_listext()
{
	static unsigned char *ext[] = { ".gz", ".tgz", NULL};

	return ext;
}


struct decoding_handlers gzip_handlers = {
	gzip_open,
	gzip_read,
	gzip_close,
	gzip_listext,
};

#endif


/*************************************************************************
  Bzip2 encoding (ENCODING_BZIP2)
*************************************************************************/

#ifdef HAVE_BZLIB_H

struct bz2_enc_data {
	FILE *file;
	BZFILE *bzfile;
	int last_read; /* If err after last bzRead() was BZ_STREAM_END.. */
};

/* TODO: When it'll be official, use bzdopen() from Yoshioka Tsuneo. --pasky */

static int
bzip2_open(struct stream_encoded *stream, int fd)
{
	struct bz2_enc_data *data = mem_alloc(sizeof(struct bz2_enc_data));
	int err;

	if (!data) {
		return -1;
	}
	data->last_read = 0;

	data->file = fdopen(fd, "rb");

	data->bzfile = BZ2_bzReadOpen(&err, data->file, 0, 0, NULL, 0);
	if (!data->bzfile) {
		mem_free(data);
		return -1;
	}

	stream->data = data;

	return 0;
}

static int
bzip2_read(struct stream_encoded *stream, unsigned char *buf, int len)
{
	struct bz2_enc_data *data = (struct bz2_enc_data *) stream->data;
	int err = 0;

	if (data->last_read)
		return 0;

	len = BZ2_bzRead(&err, data->bzfile, buf, len);

	if (err == BZ_STREAM_END)
		data->last_read = 1;
	else if (err)
		return -1;

	return len;
}

static void
bzip2_close(struct stream_encoded *stream)
{
	struct bz2_enc_data *data = (struct bz2_enc_data *) stream->data;
	int err;

	BZ2_bzReadClose(&err, data->bzfile);
	fclose(data->file);
	mem_free(data);
}

unsigned char **bzip2_listext()
{
	static unsigned char *ext[] = { ".bz2", NULL};

	return ext;
}

struct decoding_handlers bzip2_handlers = {
	bzip2_open,
	bzip2_read,
	bzip2_close,
	bzip2_listext,
};

#endif


struct decoding_handlers *handlers[] = {
	&dummy_handlers,
#ifdef HAVE_ZLIB_H
	&gzip_handlers,
#else
	&dummy_handlers,
#endif
#ifdef HAVE_BZLIB_H
	&bzip2_handlers,
#else
	&dummy_handlers,
#endif
};

extern unsigned char *encoding_names[] = {
	"none",
	"gzip",
	"bzip2",
};


/*************************************************************************
  Public functions
*************************************************************************/


/* Associates encoded stream with a fd. */
struct stream_encoded *
open_encoded(int fd, enum stream_encoding encoding)
{
	struct stream_encoded *stream;

	stream = mem_alloc(sizeof(struct stream_encoded));
	if (!stream) return NULL;

	stream->encoding = encoding;
	if (handlers[stream->encoding]->open(stream, fd) >= 0)
		return stream;

	mem_free(stream);
	return NULL;
}

/* Read available data from stream and decode them. Note that when data change
 * their size during decoding, 'len' indicates desired size of _returned_ data,
 * not desired size of data read from stream. */
int
read_encoded(struct stream_encoded *stream, unsigned char *data, int len)
{
	return handlers[stream->encoding]->read(stream, data, len);
}

/* Closes encoded stream. Note that fd associated with the stream will be
 * closed here. */
void
close_encoded(struct stream_encoded *stream)
{
	handlers[stream->encoding]->close(stream);
	mem_free(stream);
}

/* Return a list of extensions associated with that encoding. */
unsigned char **listext_encoded(enum stream_encoding encoding)
{
	return handlers[encoding]->listext();
}
