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
 * $Id: frame.c,v 1.15 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>
# include <string.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# include "id3tag.h"
# include "frame.h"
# include "frametype.h"
# include "compat.h"
# include "field.h"
# include "render.h"
# include "parse.h"
# include "util.h"

static
int valid_idchar(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

/*
 * NAME:	frame->validid()
 * DESCRIPTION:	return true if the parameter string is a legal frame ID
 */
int id3_frame_validid(char const *id)
{
  return id &&
    valid_idchar(id[0]) &&
    valid_idchar(id[1]) &&
    valid_idchar(id[2]) &&
    valid_idchar(id[3]);
}

/*
 * NAME:	frame->new()
 * DESCRIPTION:	allocate and return a new frame
 */
struct id3_frame *id3_frame_new(char const *id)
{
  struct id3_frametype const *frametype;
  struct id3_frame *frame;
  unsigned int i;

  if (!id3_frame_validid(id))
    return 0;

  frametype = id3_frametype_lookup(id, 4);
  if (frametype == 0) {
    switch (id[0]) {
    case 'T':
      frametype = &id3_frametype_text;
      break;

    case 'W':
      frametype = &id3_frametype_url;
      break;

    case 'X':
    case 'Y':
    case 'Z':
      frametype = &id3_frametype_experimental;
      break;

    default:
      frametype = &id3_frametype_unknown;
      if (id3_compat_lookup(id, 4))
	frametype = &id3_frametype_obsolete;
      break;
    }
  }

  frame = malloc(sizeof(*frame) + frametype->nfields * sizeof(*frame->fields));
  if (frame) {
    frame->id[0] = id[0];
    frame->id[1] = id[1];
    frame->id[2] = id[2];
    frame->id[3] = id[3];
    frame->id[4] = 0;

    frame->description       = frametype->description;
    frame->refcount          = 0;
    frame->flags             = frametype->defaultflags;
    frame->group_id          = 0;
    frame->encryption_method = 0;
    frame->encoded           = 0;
    frame->encoded_length    = 0;
    frame->decoded_length    = 0;
    frame->nfields           = frametype->nfields;
    frame->fields            = (union id3_field *) &frame[1];

    for (i = 0; i < frame->nfields; ++i)
      id3_field_init(&frame->fields[i], frametype->fields[i]);
  }

  return frame;
}

void id3_frame_delete(struct id3_frame *frame)
{
  assert(frame);

  if (frame->refcount == 0) {
    unsigned int i;

    for (i = 0; i < frame->nfields; ++i)
      id3_field_finish(&frame->fields[i]);

    if (frame->encoded)
      free(frame->encoded);

    free(frame);
  }
}

/*
 * NAME:	frame->addref()
 * DESCRIPTION:	add an external reference to a frame
 */
void id3_frame_addref(struct id3_frame *frame)
{
  assert(frame);

  ++frame->refcount;
}

/*
 * NAME:	frame->delref()
 * DESCRIPTION:	remove an external reference to a frame
 */
void id3_frame_delref(struct id3_frame *frame)
{
  assert(frame && frame->refcount > 0);

  --frame->refcount;
}

/*
 * NAME:	frame->field()
 * DESCRIPTION:	return a pointer to a field in a frame
 */
union id3_field *id3_frame_field(struct id3_frame const *frame,
				 unsigned int index)
{
  assert(frame);

  return (index < frame->nfields) ? &frame->fields[index] : 0;
}

static
struct id3_frame *obsolete(char const *id, id3_byte_t const *data,
			   id3_length_t length)
{
  struct id3_frame *frame;

  frame = id3_frame_new(ID3_FRAME_OBSOLETE);
  if (frame) {
    if (id3_field_setframeid(&frame->fields[0], id) == -1 ||
	id3_field_setbinarydata(&frame->fields[1], data, length) == -1)
      goto fail;
  }

  if (0) {
  fail:
    if (frame) {
      id3_frame_delete(frame);
      frame = 0;
    }
  }

  return frame;
}

static
struct id3_frame *unparseable(char const *id, id3_byte_t const **ptr,
			      id3_length_t length, int flags,
			      int group_id, int encryption_method,
			      id3_length_t decoded_length)
{
  struct id3_frame *frame = 0;
  id3_byte_t *mem;

  mem = malloc(length ? length : 1);
  if (mem == 0)
    goto fail;

  frame = id3_frame_new(id);
  if (frame == 0)
    free(mem);
  else {
    memcpy(mem, *ptr, length);

    frame->flags             = flags;
    frame->group_id          = group_id;
    frame->encryption_method = encryption_method;
    frame->encoded           = mem;
    frame->encoded_length    = length;
    frame->decoded_length    = decoded_length;
  }

  if (0) {
  fail:
    ;
  }

  *ptr += length;

  return frame;
}

static
int parse_data(struct id3_frame *frame,
	       id3_byte_t const *data, id3_length_t length)
{
  enum id3_field_textencoding encoding;
  id3_byte_t const *end;
  unsigned int i;

  encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

  end = data + length;

  for (i = 0; i < frame->nfields; ++i) {
    if (id3_field_parse(&frame->fields[i], &data, end - data, &encoding) == -1)
      return -1;
  }

  return 0;
}

/*
 * NAME:	frame->parse()
 * DESCRIPTION:	parse raw frame data according to the specified ID3 tag version
 */
#define ITUNES_24_WORKAROUND 1
struct id3_frame *id3_frame_parse(id3_byte_t const **ptr, id3_length_t length,
				  unsigned int version)
{
  struct id3_frame *frame = 0;
  id3_byte_t const *id, *end, *data;
#ifdef ITUNES_24_WORKAROUND
  id3_byte_t const *itunes_workaround_ptr;
  id3_length_t itunes_workaround_size;
  int itunes_workaround_flags;
#endif
  id3_length_t size, decoded_length = 0;
  int flags = 0, group_id = 0, encryption_method = 0;
  struct id3_compat const *compat = 0;
  id3_byte_t *mem = 0;
  char xid[4];

  id  = *ptr;
  end = *ptr + length;

  if (ID3_TAG_VERSION_MAJOR(version) < 4) {
    switch (ID3_TAG_VERSION_MAJOR(version)) {
    case 2:
      if (length < 6)
	goto fail;

      compat = id3_compat_lookup(id, 3);

      *ptr += 3;
      size  = id3_parse_uint(ptr, 3);

      if (size > end - *ptr)
	goto fail;

      end = *ptr + size;

      break;

    case 3:
      if (length < 10)
	goto fail;

      compat = id3_compat_lookup(id, 4);

      /* some braindead apps (iTunes) store completely invalid frames */
      if (id[3] == 0 && !compat)
        compat = id3_compat_lookup(id, 3);

      *ptr += 4;
      size  = id3_parse_uint(ptr, 4);
      flags = id3_parse_uint(ptr, 2);

      if (size > end - *ptr)
	goto fail;

      end = *ptr + size;

      if (flags & (ID3_FRAME_FLAG_FORMATFLAGS & ~0x00e0)) {
	frame = unparseable(id, ptr, end - *ptr, 0, 0, 0, 0);
	goto done;
      }

      flags =
	((flags >> 1) & ID3_FRAME_FLAG_STATUSFLAGS) |
	((flags >> 4) & (ID3_FRAME_FLAG_COMPRESSION |
			 ID3_FRAME_FLAG_ENCRYPTION)) |
	((flags << 1) & ID3_FRAME_FLAG_GROUPINGIDENTITY);

      if (flags & ID3_FRAME_FLAG_COMPRESSION) {
	if (end - *ptr < 4)
	  goto fail;

	decoded_length = id3_parse_uint(ptr, 4);
      }

      if (flags & ID3_FRAME_FLAG_ENCRYPTION) {
	if (end - *ptr < 1)
	  goto fail;

	encryption_method = id3_parse_uint(ptr, 1);
      }

      if (flags & ID3_FRAME_FLAG_GROUPINGIDENTITY) {
	if (end - *ptr < 1)
	  goto fail;

	group_id = id3_parse_uint(ptr, 1);
      }

      break;

    default:
      goto fail;
    }

    /* canonicalize frame ID for ID3v2.4 */

    if (compat && compat->equiv)
      id = compat->equiv;
    else if (ID3_TAG_VERSION_MAJOR(version) == 2) {
      xid[0] = 'Y';
      xid[1] = id[0];
      xid[2] = id[1];
      xid[3] = id[2];

      id = xid;

      flags |=
	ID3_FRAME_FLAG_TAGALTERPRESERVATION |
	ID3_FRAME_FLAG_FILEALTERPRESERVATION;
    }
  }
  else {  /* ID3v2.4 */
    if (length < 10)
      goto fail;

    *ptr += 4;

#ifdef ITUNES_24_WORKAROUND
    itunes_workaround_ptr = *ptr;
    itunes_workaround_size = id3_parse_uint(&itunes_workaround_ptr, 4);
    itunes_workaround_flags = id3_parse_uint(&itunes_workaround_ptr, 2);
#endif
    size = id3_parse_syncsafe(ptr, 4);
    flags = id3_parse_uint(ptr, 2);
#ifdef ITUNES_24_WORKAROUND
    /* Test whether this frame is done sync_safe or not
       iTunes stores 2.4 tags using un-sync-safed sizings (non-spec) */
    if (size != itunes_workaround_size) {
      /* could be a candidate for iTunes craziness - check whether we have a valid frame id after this block */
      if (size < end - *ptr - 4 && !id3_frame_validid(*ptr + size)) {
        /* nope - check if we have one if we use iTunes sizing */
        if (itunes_workaround_size < end - *ptr + 4 && id3_frame_validid(*ptr + itunes_workaround_size))
          size = itunes_workaround_size; /* iTunes sizing fits, so use it */
      }
    }
#endif

    if (size > end - *ptr)
      goto fail;

    end = *ptr + size;

    if (flags & (ID3_FRAME_FLAG_FORMATFLAGS & ~ID3_FRAME_FLAG_KNOWNFLAGS)) {
      frame = unparseable(id, ptr, end - *ptr, flags, 0, 0, 0);
      goto done;
    }

    if (flags & ID3_FRAME_FLAG_GROUPINGIDENTITY) {
      if (end - *ptr < 1)
	goto fail;

      group_id = id3_parse_uint(ptr, 1);
    }

    if ((flags & ID3_FRAME_FLAG_COMPRESSION) &&
	!(flags & ID3_FRAME_FLAG_DATALENGTHINDICATOR))
      goto fail;

    if (flags & ID3_FRAME_FLAG_ENCRYPTION) {
      if (end - *ptr < 1)
	goto fail;

      encryption_method = id3_parse_uint(ptr, 1);
    }

    if (flags & ID3_FRAME_FLAG_DATALENGTHINDICATOR) {
      if (end - *ptr < 4)
	goto fail;

      decoded_length = id3_parse_syncsafe(ptr, 4);
    }
  }

  data = *ptr;
  *ptr = end;

  /* undo frame encodings */

  if ((flags & ID3_FRAME_FLAG_UNSYNCHRONISATION) && end - data > 0) {
    mem = malloc(end - data);
    if (mem == 0)
      goto fail;

    memcpy(mem, data, end - data);

    end  = mem + id3_util_deunsynchronise(mem, end - data);
    data = mem;
  }

  if (flags & ID3_FRAME_FLAG_ENCRYPTION) {
    frame = unparseable(id, &data, end - data, flags,
			group_id, encryption_method, decoded_length);
    goto done;
  }

  if (flags & ID3_FRAME_FLAG_COMPRESSION) {
    id3_byte_t *decomp;

    decomp = id3_util_decompress(data, end - data, decoded_length);
    if (decomp == 0)
      goto fail;

    if (mem)
      free(mem);

    data = mem = decomp;
    end  = data + decoded_length;
  }

  /* check for obsolescence */

  if (compat && !compat->equiv) {
    frame = obsolete(id, data, end - data);
    goto done;
  }

  /* generate the internal frame structure */

  frame = id3_frame_new(id);
  if (frame) {
    frame->flags    = flags;
    frame->group_id = group_id;

    if (compat && compat->translate) {
      if (compat->translate(frame, compat->id, data, end - data) == -1)
	goto fail;
    }
    else {
      if (parse_data(frame, data, end - data) == -1)
	goto fail;
    }
  }

  if (0) {
  fail:
    if (frame) {
      id3_frame_delete(frame);
      frame = 0;
    }
  }

 done:
  if (mem)
    free(mem);

  return frame;
}

static
id3_length_t render_data(id3_byte_t **ptr,
			 union id3_field *fields, unsigned int length)
{
  id3_length_t size = 0;
  enum id3_field_textencoding encoding;
  unsigned int i;

  encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

  for (i = 0; i < length; ++i)
    size += id3_field_render(&fields[i], ptr, &encoding, i < length - 1);

  return size;
}

/*
 * NAME:	frame->render()
 * DESCRIPTION:	render a single, complete frame
 */
id3_length_t id3_frame_render(struct id3_frame const *frame,
			      id3_byte_t **ptr, int options)
{
  id3_length_t size = 0, decoded_length, datalen;
  id3_byte_t *size_ptr = 0, *flags_ptr = 0, *data = 0;
  int flags;

  assert(frame);

  if ((frame->flags & ID3_FRAME_FLAG_TAGALTERPRESERVATION) ||
      ((options & ID3_TAG_OPTION_FILEALTERED) &&
       (frame->flags & ID3_FRAME_FLAG_FILEALTERPRESERVATION)))
    return 0;

  /* a frame must be at least 1 byte big, excluding the header */

  decoded_length = render_data(0, frame->fields, frame->nfields);
  if (decoded_length == 0 && frame->encoded == 0)
    return 0;

  /* header */

  size += id3_render_immediate(ptr, frame->id, 4);

  if (ptr)
    size_ptr = *ptr;

  size += id3_render_syncsafe(ptr, 0, 4);

  if (ptr)
    flags_ptr = *ptr;

  flags = frame->flags;

  size += id3_render_int(ptr, flags, 2);

  if (flags & (ID3_FRAME_FLAG_FORMATFLAGS & ~ID3_FRAME_FLAG_KNOWNFLAGS)) {
    size += id3_render_binary(ptr, frame->encoded, frame->encoded_length);
    if (size_ptr)
      id3_render_syncsafe(&size_ptr, size - 10, 4);

    return size;
  }

  flags &= ID3_FRAME_FLAG_KNOWNFLAGS;

  flags &= ~ID3_FRAME_FLAG_UNSYNCHRONISATION;
  if (options & ID3_TAG_OPTION_UNSYNCHRONISATION)
    flags |= ID3_FRAME_FLAG_UNSYNCHRONISATION;

  if (!(flags & ID3_FRAME_FLAG_ENCRYPTION)) {
    flags &= ~ID3_FRAME_FLAG_COMPRESSION;
    if (options & ID3_TAG_OPTION_COMPRESSION)
      flags |= ID3_FRAME_FLAG_COMPRESSION | ID3_FRAME_FLAG_DATALENGTHINDICATOR;
  }

  if (flags & ID3_FRAME_FLAG_GROUPINGIDENTITY)
    size += id3_render_int(ptr, frame->group_id, 1);
  if (flags & ID3_FRAME_FLAG_ENCRYPTION)
    size += id3_render_int(ptr, frame->encryption_method, 1);
  if (flags & ID3_FRAME_FLAG_DATALENGTHINDICATOR) {
    if (flags & ID3_FRAME_FLAG_ENCRYPTION)
      decoded_length = frame->decoded_length;
    size += id3_render_syncsafe(ptr, decoded_length, 4);
  }

  if (ptr)
    data = *ptr;

  if (flags & ID3_FRAME_FLAG_ENCRYPTION)
    datalen = id3_render_binary(ptr, frame->encoded, frame->encoded_length);
  else {
    if (ptr == 0)
      datalen = decoded_length;
    else {
      datalen = render_data(ptr, frame->fields, frame->nfields);

      if (flags & ID3_FRAME_FLAG_COMPRESSION) {
	id3_byte_t *comp;
	id3_length_t complen;

	comp = id3_util_compress(data, datalen, &complen);
	if (comp == 0)
	  flags &= ~ID3_FRAME_FLAG_COMPRESSION;
	else {
	  *ptr = data;
	  datalen = id3_render_binary(ptr, comp, complen);

	  free(comp);
	}
      }
    }
  }

  /* unsynchronisation */

  if (flags & ID3_FRAME_FLAG_UNSYNCHRONISATION) {
    if (data == 0)
      datalen *= 2;
    else {
      id3_length_t newlen;

      newlen = id3_util_unsynchronise(data, datalen);
      if (newlen == datalen)
	flags &= ~ID3_FRAME_FLAG_UNSYNCHRONISATION;
      else {
	*ptr   += newlen - datalen;
	datalen = newlen;
      }
    }
  }

  size += datalen;

  /* patch size and flags */

  if (size_ptr)
    id3_render_syncsafe(&size_ptr, size - 10, 4);
  if (flags_ptr)
    id3_render_int(&flags_ptr, flags, 2);

  return size;
}
