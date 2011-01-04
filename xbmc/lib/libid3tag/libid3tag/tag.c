/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: tag.c,v 1.20 2004/02/17 02:04:10 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <string.h>
# include <stdlib.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# include "id3tag.h"
# include "tag.h"
# include "frame.h"
# include "compat.h"
# include "parse.h"
# include "render.h"
# include "latin1.h"
# include "ucs4.h"
# include "genre.h"
# include "crc.h"
# include "field.h"
# include "util.h"

/*
 * NAME:	tag->new()
 * DESCRIPTION:	allocate and return a new, empty tag
 */
struct id3_tag *id3_tag_new(void)
{
  struct id3_tag *tag;

  tag = malloc(sizeof(*tag));
  if (tag) {
    tag->refcount      = 0;
    tag->version       = ID3_TAG_VERSION;
    tag->flags         = 0;
    tag->extendedflags = 0;
    tag->restrictions  = 0;
    tag->options       = /* ID3_TAG_OPTION_UNSYNCHRONISATION | */
                         ID3_TAG_OPTION_COMPRESSION | ID3_TAG_OPTION_CRC;
    tag->nframes       = 0;
    tag->frames        = 0;
    tag->paddedsize    = 0;
  }

  return tag;
}

/*
 * NAME:	tag->delete()
 * DESCRIPTION:	destroy a tag and deallocate all associated memory
 */
void id3_tag_delete(struct id3_tag *tag)
{
  assert(tag);

  if (tag->refcount == 0) {
    id3_tag_clearframes(tag);

    if (tag->frames)
      free(tag->frames);

    free(tag);
  }
}

/*
 * NAME:	tag->addref()
 * DESCRIPTION:	add an external reference to a tag
 */
void id3_tag_addref(struct id3_tag *tag)
{
  assert(tag);

  ++tag->refcount;
}

/*
 * NAME:	tag->delref()
 * DESCRIPTION:	remove an external reference to a tag
 */
void id3_tag_delref(struct id3_tag *tag)
{
  assert(tag && tag->refcount > 0);

  --tag->refcount;
}

/*
 * NAME:	tag->version()
 * DESCRIPTION:	return the tag's original ID3 version number
 */
unsigned int id3_tag_version(struct id3_tag const *tag)
{
  assert(tag);

  return tag->version;
}

/*
 * NAME:	tag->options()
 * DESCRIPTION:	get or set tag options
 */
int id3_tag_options(struct id3_tag *tag, int mask, int values)
{
  assert(tag);

  if (mask)
    tag->options = (tag->options & ~mask) | (values & mask);

  return tag->options;
}

/*
 * NAME:	tag->setlength()
 * DESCRIPTION:	set the minimum rendered tag size
 */
void id3_tag_setlength(struct id3_tag *tag, id3_length_t length)
{
  assert(tag);

  tag->paddedsize = length;
}

/*
 * NAME:	tag->clearframes()
 * DESCRIPTION:	detach and delete all frames associated with a tag
 */
void id3_tag_clearframes(struct id3_tag *tag)
{
  unsigned int i;

  assert(tag);

  for (i = 0; i < tag->nframes; ++i) {
    id3_frame_delref(tag->frames[i]);
    id3_frame_delete(tag->frames[i]);
  }

  tag->nframes = 0;
}

/*
 * NAME:	tag->attachframe()
 * DESCRIPTION:	attach a frame to a tag
 */
int id3_tag_attachframe(struct id3_tag *tag, struct id3_frame *frame)
{
  struct id3_frame **frames;

  assert(tag && frame);

  frames = realloc(tag->frames, (tag->nframes + 1) * sizeof(*frames));
  if (frames == 0)
    return -1;

  tag->frames = frames;
  tag->frames[tag->nframes++] = frame;

  id3_frame_addref(frame);

  return 0;
}

/*
 * NAME:	tag->detachframe()
 * DESCRIPTION:	detach (but don't delete) a frame from a tag
 */
int id3_tag_detachframe(struct id3_tag *tag, struct id3_frame *frame)
{
  unsigned int i;

  assert(tag && frame);

  for (i = 0; i < tag->nframes; ++i) {
    if (tag->frames[i] == frame)
      break;
  }

  if (i == tag->nframes)
    return -1;

  --tag->nframes;
  while (i++ < tag->nframes)
    tag->frames[i - 1] = tag->frames[i];

  id3_frame_delref(frame);

  return 0;
}

/*
 * NAME:	tag->findframe()
 * DESCRIPTION:	find in a tag the nth (0-based) frame with the given frame ID
 */
struct id3_frame *id3_tag_findframe(struct id3_tag const *tag,
				    char const *id, unsigned int index)
{
  unsigned int len, i;

  assert(tag);

  if (id == 0 || *id == 0)
    return (index < tag->nframes) ? tag->frames[index] : 0;

  len = strlen(id);

  if (len == 4) {
    struct id3_compat const *compat;

    compat = id3_compat_lookup(id, len);
    if (compat && compat->equiv && !compat->translate) {
      id  = compat->equiv;
      len = strlen(id);
    }
  }

  for (i = 0; i < tag->nframes; ++i) {
    if (strncmp(tag->frames[i]->id, id, len) == 0 && index-- == 0)
      return tag->frames[i];
  }

  return 0;
}

enum tagtype {
  TAGTYPE_NONE = 0,
  TAGTYPE_ID3V1,
  TAGTYPE_ID3V2,
  TAGTYPE_ID3V2_FOOTER
};

static
enum tagtype tagtype(id3_byte_t const *data, id3_length_t length)
{
  if (length >= 3 &&
      data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
    return TAGTYPE_ID3V1;

  if (length >= 10 &&
      ((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
       (data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
      data[3] < 0xff && data[4] < 0xff &&
      data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
    return data[0] == 'I' ? TAGTYPE_ID3V2 : TAGTYPE_ID3V2_FOOTER;

  return TAGTYPE_NONE;
}

static
void parse_header(id3_byte_t const **ptr,
		  unsigned int *version, int *flags, id3_length_t *size)
{
  *ptr += 3;

  *version = id3_parse_uint(ptr, 2);
  *flags   = id3_parse_uint(ptr, 1);
  *size    = id3_parse_syncsafe(ptr, 4);
}

/*
 * NAME:	tag->query()
 * DESCRIPTION:	if a tag begins at the given location, return its size
 */
signed long id3_tag_query(id3_byte_t const *data, id3_length_t length)
{
  unsigned int version;
  int flags;
  id3_length_t size;

  assert(data);

  switch (tagtype(data, length)) {
  case TAGTYPE_ID3V1:
    return 128;

  case TAGTYPE_ID3V2:
    parse_header(&data, &version, &flags, &size);

    if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
      size += 10;

    return 10 + size;

  case TAGTYPE_ID3V2_FOOTER:
    parse_header(&data, &version, &flags, &size);
    return -size - 10;

  case TAGTYPE_NONE:
    break;
  }

  return 0;
}

static
void trim(char *str)
{
  char *ptr;

  ptr = str + strlen(str);
  while (ptr > str && ptr[-1] == ' ')
    --ptr;

  *ptr = 0;
}

static
int v1_attachstr(struct id3_tag *tag, char const *id,
		 char *text, unsigned long number)
{
  struct id3_frame *frame;
  id3_ucs4_t ucs4[31];

  if (text) {
    trim(text);
    if (*text == 0)
      return 0;
  }

  frame = id3_frame_new(id);
  if (frame == 0)
    return -1;

  if (id3_field_settextencoding(&frame->fields[0],
				ID3_FIELD_TEXTENCODING_ISO_8859_1) == -1)
    goto fail;

  if (text)
    id3_latin1_decode(text, ucs4);
  else
    id3_ucs4_putnumber(ucs4, number);

  if (strcmp(id, ID3_FRAME_COMMENT) == 0) {
    if (id3_field_setlanguage(&frame->fields[1], "XXX") == -1 ||
	id3_field_setstring(&frame->fields[2], id3_ucs4_empty) == -1 ||
	id3_field_setfullstring(&frame->fields[3], ucs4) == -1)
      goto fail;
  }
  else {
    id3_ucs4_t *ptr = ucs4;

    if (id3_field_setstrings(&frame->fields[1], 1, &ptr) == -1)
      goto fail;
  }

  if (id3_tag_attachframe(tag, frame) == -1)
    goto fail;

  return 0;

 fail:
  id3_frame_delete(frame);
  return -1;
}

static
struct id3_tag *v1_parse(id3_byte_t const *data)
{
  struct id3_tag *tag;

  tag = id3_tag_new();
  if (tag) {
    char title[31], artist[31], album[31], year[5], comment[31];
    unsigned int genre, track;

    tag->version = 0x0100;

    tag->options |=  ID3_TAG_OPTION_ID3V1;
    tag->options &= ~ID3_TAG_OPTION_COMPRESSION;

    tag->restrictions =
      ID3_TAG_RESTRICTION_TEXTENCODING_LATIN1_UTF8 |
      ID3_TAG_RESTRICTION_TEXTSIZE_30_CHARS;

    title[30] = artist[30] = album[30] = year[4] = comment[30] = 0;

    memcpy(title,   &data[3],  30);
    memcpy(artist,  &data[33], 30);
    memcpy(album,   &data[63], 30);
    memcpy(year,    &data[93],  4);
    memcpy(comment, &data[97], 30);

    genre = data[127];

    track = 0;
    if (comment[28] == 0 && comment[29] != 0) {
      track = comment[29];
      tag->version = 0x0101;
    }

    /* populate tag frames */

    if (v1_attachstr(tag, ID3_FRAME_TITLE,  title,  0) == -1 ||
	v1_attachstr(tag, ID3_FRAME_ARTIST, artist, 0) == -1 ||
	v1_attachstr(tag, ID3_FRAME_ALBUM,  album,  0) == -1 ||
	v1_attachstr(tag, ID3_FRAME_YEAR,   year,   0) == -1 ||
	(track        && v1_attachstr(tag, ID3_FRAME_TRACK, 0, track) == -1) ||
	(genre < 0xff && v1_attachstr(tag, ID3_FRAME_GENRE, 0, genre) == -1) ||
	v1_attachstr(tag, ID3_FRAME_COMMENT, comment, 0) == -1) {
      id3_tag_delete(tag);
      tag = 0;
    }
  }

  return tag;
}

static
struct id3_tag *v2_parse(id3_byte_t const *ptr)
{
  struct id3_tag *tag;
  id3_byte_t *mem = 0;

  tag = id3_tag_new();
  if (tag) {
    id3_byte_t const *end;
    id3_length_t size;

    parse_header(&ptr, &tag->version, &tag->flags, &size);

    tag->paddedsize = 10 + size;

    if ((tag->flags & ID3_TAG_FLAG_UNSYNCHRONISATION) &&
	ID3_TAG_VERSION_MAJOR(tag->version) < 4) {
      mem = malloc(size);
      if (mem == 0)
	goto fail;

      memcpy(mem, ptr, size);

      size = id3_util_deunsynchronise(mem, size);
      ptr  = mem;
    }

    end = ptr + size;

    if (tag->flags & ID3_TAG_FLAG_EXTENDEDHEADER) {
      switch (ID3_TAG_VERSION_MAJOR(tag->version)) {
      case 2:
	goto fail;

      case 3:
	{
	  id3_byte_t const *ehptr, *ehend;
	  id3_length_t ehsize;

	  enum {
	    EH_FLAG_CRC = 0x8000  /* CRC data present */
	  };

	  if (end - ptr < 4)
	    goto fail;

	  ehsize = id3_parse_uint(&ptr, 4);

	  if (ehsize > end - ptr)
	    goto fail;

	  ehptr = ptr;
	  ehend = ptr + ehsize;

	  ptr = ehend;

	  if (ehend - ehptr >= 6) {
	    int ehflags;
	    id3_length_t padsize;

	    ehflags = id3_parse_uint(&ehptr, 2);
	    padsize = id3_parse_uint(&ehptr, 4);

	    if (padsize > end - ptr)
	      goto fail;

	    end -= padsize;

	    if (ehflags & EH_FLAG_CRC) {
	      unsigned long crc;

	      if (ehend - ehptr < 4)
		goto fail;

	      crc = id3_parse_uint(&ehptr, 4);

	      if (crc != id3_crc_compute(ptr, end - ptr))
		goto fail;

	      tag->extendedflags |= ID3_TAG_EXTENDEDFLAG_CRCDATAPRESENT;
	    }
	  }
	}
	break;

      case 4:
	{
	  id3_byte_t const *ehptr, *ehend;
	  id3_length_t ehsize;
	  unsigned int bytes;

	  if (end - ptr < 4)
	    goto fail;

	  ehptr  = ptr;
	  ehsize = id3_parse_syncsafe(&ptr, 4);

	  if (ehsize < 6 || ehsize > end - ehptr)
	    goto fail;

	  ehend = ehptr + ehsize;

	  bytes = id3_parse_uint(&ptr, 1);

	  if (bytes < 1 || bytes > ehend - ptr)
	    goto fail;

	  ehptr = ptr + bytes;

	  /* verify extended header size */
	  {
	    id3_byte_t const *flagsptr = ptr, *dataptr = ehptr;
	    unsigned int datalen;
	    int ehflags;

	    while (bytes--) {
	      for (ehflags = id3_parse_uint(&flagsptr, 1); ehflags;
		   ehflags = (ehflags << 1) & 0xff) {
		if (ehflags & 0x80) {
		  if (dataptr == ehend)
		    goto fail;
		  datalen = id3_parse_uint(&dataptr, 1);
		  if (datalen > 0x7f || datalen > ehend - dataptr)
		    goto fail;
		  dataptr += datalen;
		}
	      }
	    }
	  }

	  tag->extendedflags = id3_parse_uint(&ptr, 1);

	  ptr = ehend;

	  if (tag->extendedflags & ID3_TAG_EXTENDEDFLAG_TAGISANUPDATE) {
	    bytes  = id3_parse_uint(&ehptr, 1);
	    ehptr += bytes;
	  }

	  if (tag->extendedflags & ID3_TAG_EXTENDEDFLAG_CRCDATAPRESENT) {
	    unsigned long crc;

	    bytes = id3_parse_uint(&ehptr, 1);
	    if (bytes < 5)
	      goto fail;

	    crc = id3_parse_syncsafe(&ehptr, 5);
	    ehptr += bytes - 5;

	    if (crc != id3_crc_compute(ptr, end - ptr))
	      goto fail;
	  }

	  if (tag->extendedflags & ID3_TAG_EXTENDEDFLAG_TAGRESTRICTIONS) {
	    bytes = id3_parse_uint(&ehptr, 1);
	    if (bytes < 1)
	      goto fail;

	    tag->restrictions = id3_parse_uint(&ehptr, 1);
	    ehptr += bytes - 1;
	  }
	}
	break;
      }
    }

    /* frames */

    while (ptr < end) {
      struct id3_frame *frame;

      if (*ptr == 0)
	break;  /* padding */

      frame = id3_frame_parse(&ptr, end - ptr, tag->version);
      if (frame == 0 || id3_tag_attachframe(tag, frame) == -1)
	goto fail;
    }

    if (ID3_TAG_VERSION_MAJOR(tag->version) < 4 &&
	id3_compat_fixup(tag) == -1)
      goto fail;
  }

  if (0) {
  fail:
    id3_tag_delete(tag);
    tag = 0;
  }

  if (mem)
    free(mem);

  return tag;
}

/*
 * NAME:	tag->parse()
 * DESCRIPTION:	parse a complete ID3 tag
 */
struct id3_tag *id3_tag_parse(id3_byte_t const *data, id3_length_t length)
{
  id3_byte_t const *ptr;
  unsigned int version;
  int flags;
  id3_length_t size;

  assert(data);

  switch (tagtype(data, length)) {
  case TAGTYPE_ID3V1:
    return (length < 128) ? 0 : v1_parse(data);

  case TAGTYPE_ID3V2:
    break;

  case TAGTYPE_ID3V2_FOOTER:
  case TAGTYPE_NONE:
    return 0;
  }

  /* ID3v2.x */

  ptr = data;
  parse_header(&ptr, &version, &flags, &size);

  switch (ID3_TAG_VERSION_MAJOR(version)) {
  case 4:
    if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
      size += 10;
  case 2:
  case 3:
    return (length < 10 + size) ? 0 : v2_parse(data);
  }

  return 0;
}

static
void v1_renderstr(struct id3_tag const *tag, char const *frameid,
		  id3_byte_t **buffer, id3_length_t length)
{
  struct id3_frame *frame;
  id3_ucs4_t const *string;

  frame = id3_tag_findframe(tag, frameid, 0);
  if (frame == 0)
    string = id3_ucs4_empty;
  else {
    if (strcmp(frameid, ID3_FRAME_COMMENT) == 0)
      string = id3_field_getfullstring(&frame->fields[3]);
    else
      string = id3_field_getstrings(&frame->fields[1], 0);
  }

  id3_render_paddedstring(buffer, string, length);
}

/*
 * NAME:	v1->render()
 * DESCRIPTION:	render an ID3v1 (or ID3v1.1) tag
 */
static
id3_length_t v1_render(struct id3_tag const *tag, id3_byte_t *buffer)
{
  id3_byte_t data[128], *ptr;
  struct id3_frame *frame;
  unsigned int i;
  int genre = -1;

  ptr = data;

  id3_render_immediate(&ptr, "TAG", 3);

  v1_renderstr(tag, ID3_FRAME_TITLE,   &ptr, 30);
  v1_renderstr(tag, ID3_FRAME_ARTIST,  &ptr, 30);
  v1_renderstr(tag, ID3_FRAME_ALBUM,   &ptr, 30);
  v1_renderstr(tag, ID3_FRAME_YEAR,    &ptr,  4);
  v1_renderstr(tag, ID3_FRAME_COMMENT, &ptr, 30);

  /* ID3v1.1 track number */

  frame = id3_tag_findframe(tag, ID3_FRAME_TRACK, 0);
  if (frame) {
    unsigned int track;

    track = id3_ucs4_getnumber(id3_field_getstrings(&frame->fields[1], 0));
    if (track > 0 && track <= 0xff) {
      ptr[-2] = 0;
      ptr[-1] = track;
    }
  }

  /* ID3v1 genre number */

  frame = id3_tag_findframe(tag, ID3_FRAME_GENRE, 0);
  if (frame) {
    unsigned int nstrings;

    nstrings = id3_field_getnstrings(&frame->fields[1]);

    for (i = 0; i < nstrings; ++i) {
      genre = id3_genre_number(id3_field_getstrings(&frame->fields[1], i));
      if (genre != -1)
	break;
    }

    if (i == nstrings && nstrings > 0)
      genre = ID3_GENRE_OTHER;
  }

  id3_render_int(&ptr, genre, 1);

  /* make sure the tag is not empty */

  if (genre == -1) {
    for (i = 3; i < 127; ++i) {
      if (data[i] != ' ')
	break;
    }

    if (i == 127)
      return 0;
  }

  if (buffer)
    memcpy(buffer, data, 128);

  return 128;
}

/*
 * NAME:	tag->render()
 * DESCRIPTION:	render a complete ID3 tag
 */
id3_length_t id3_tag_render(struct id3_tag const *tag, id3_byte_t *buffer)
{
  id3_length_t size = 0;
  id3_byte_t **ptr,
    *header_ptr = 0, *tagsize_ptr = 0, *crc_ptr = 0, *frames_ptr = 0;
  int flags, extendedflags;
  unsigned int i;

  assert(tag);

  if (tag->options & ID3_TAG_OPTION_ID3V1)
    return v1_render(tag, buffer);

  /* a tag must contain at least one (renderable) frame */

  for (i = 0; i < tag->nframes; ++i) {
    if (id3_frame_render(tag->frames[i], 0, 0) > 0)
      break;
  }

  if (i == tag->nframes)
    return 0;

  ptr = buffer ? &buffer : 0;

  /* get flags */

  flags         = tag->flags         & ID3_TAG_FLAG_KNOWNFLAGS;
  extendedflags = tag->extendedflags & ID3_TAG_EXTENDEDFLAG_KNOWNFLAGS;

  extendedflags &= ~ID3_TAG_EXTENDEDFLAG_CRCDATAPRESENT;
  if (tag->options & ID3_TAG_OPTION_CRC)
    extendedflags |= ID3_TAG_EXTENDEDFLAG_CRCDATAPRESENT;

  extendedflags &= ~ID3_TAG_EXTENDEDFLAG_TAGRESTRICTIONS;
  if (tag->restrictions)
    extendedflags |= ID3_TAG_EXTENDEDFLAG_TAGRESTRICTIONS;

  flags &= ~ID3_TAG_FLAG_UNSYNCHRONISATION;
  if (tag->options & ID3_TAG_OPTION_UNSYNCHRONISATION)
    flags |= ID3_TAG_FLAG_UNSYNCHRONISATION;

  flags &= ~ID3_TAG_FLAG_EXTENDEDHEADER;
  if (extendedflags)
    flags |= ID3_TAG_FLAG_EXTENDEDHEADER;

  flags &= ~ID3_TAG_FLAG_FOOTERPRESENT;
  if (tag->options & ID3_TAG_OPTION_APPENDEDTAG)
    flags |= ID3_TAG_FLAG_FOOTERPRESENT;

  /* header */

  if (ptr)
    header_ptr = *ptr;

  size += id3_render_immediate(ptr, "ID3", 3);
  size += id3_render_int(ptr, ID3_TAG_VERSION, 2);
  size += id3_render_int(ptr, flags, 1);

  if (ptr)
    tagsize_ptr = *ptr;

  size += id3_render_syncsafe(ptr, 0, 4);

  /* extended header */

  if (flags & ID3_TAG_FLAG_EXTENDEDHEADER) {
    id3_length_t ehsize = 0;
    id3_byte_t *ehsize_ptr = 0;

    if (ptr)
      ehsize_ptr = *ptr;

    ehsize += id3_render_syncsafe(ptr, 0, 4);
    ehsize += id3_render_int(ptr, 1, 1);
    ehsize += id3_render_int(ptr, extendedflags, 1);

    if (extendedflags & ID3_TAG_EXTENDEDFLAG_TAGISANUPDATE)
      ehsize += id3_render_int(ptr, 0, 1);

    if (extendedflags & ID3_TAG_EXTENDEDFLAG_CRCDATAPRESENT) {
      ehsize += id3_render_int(ptr, 5, 1);

      if (ptr)
	crc_ptr = *ptr;

      ehsize += id3_render_syncsafe(ptr, 0, 5);
    }

    if (extendedflags & ID3_TAG_EXTENDEDFLAG_TAGRESTRICTIONS) {
      ehsize += id3_render_int(ptr, 1, 1);
      ehsize += id3_render_int(ptr, tag->restrictions, 1);
    }

    if (ehsize_ptr)
      id3_render_syncsafe(&ehsize_ptr, ehsize, 4);

    size += ehsize;
  }

  /* frames */

  if (ptr)
    frames_ptr = *ptr;

  for (i = 0; i < tag->nframes; ++i)
    size += id3_frame_render(tag->frames[i], ptr, tag->options);

  /* padding */

  if (!(flags & ID3_TAG_FLAG_FOOTERPRESENT)) {
    if (size < tag->paddedsize)
      size += id3_render_padding(ptr, 0, tag->paddedsize - size);
    else if (tag->options & ID3_TAG_OPTION_UNSYNCHRONISATION) {
      if (ptr == 0)
	size += 1;
      else {
	if ((*ptr)[-1] == 0xff)
	  size += id3_render_padding(ptr, 0, 1);
      }
    }
  }

  /* patch tag size and CRC */

  if (tagsize_ptr)
    id3_render_syncsafe(&tagsize_ptr, size - 10, 4);

  if (crc_ptr) {
    id3_render_syncsafe(&crc_ptr,
			id3_crc_compute(frames_ptr, *ptr - frames_ptr), 5);
  }

  /* footer */

  if (flags & ID3_TAG_FLAG_FOOTERPRESENT) {
    size += id3_render_immediate(ptr, "3DI", 3);
    size += id3_render_binary(ptr, header_ptr + 3, 7);
  }

  return size;
}
