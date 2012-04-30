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
 * $Id: metadata.c,v 1.00 2005/09/03 09:41:32 bobbin007 Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#include <string.h>
#include <stdlib.h>

# include "metadata.h"
# include "global.h"
# include "ucs4.h"

id3_ucs4_list_t *metadata_getstrings(const struct id3_tag* tag, const char* id, enum id3_field_textencoding* encoding)
{
  int nstrings, j;
  union id3_field const *field;
  struct id3_frame const *frame;
  id3_ucs4_list_t *list;

  frame = id3_tag_findframe(tag, id, 0);
  if (frame == 0)
	  return 0;

  *encoding = id3_field_gettextencoding(id3_frame_field(frame, 0));

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return 0;

  nstrings = id3_field_getnstrings(field);

  list = 0;
  if (nstrings)
  {
    list = (id3_ucs4_list_t*)malloc(sizeof(*list));
    if (list)
      list->strings = (const id3_ucs4_t**)malloc(nstrings * sizeof(*list->strings));
  }
  if (list && list->strings)
  {
    list->nstrings = nstrings;
    for (j = 0; j < list->nstrings; ++j)
      list->strings[j] = id3_field_getstrings(field, j);
  }
  return list;
}

void id3_ucs4_list_free(id3_ucs4_list_t *list)
{
  if (list)
  {
    if (list->strings)
      free(list->strings);
    free(list);
  }
}

const id3_ucs4_t* metadata_getstring(const struct id3_tag* tag, const char* id, enum id3_field_textencoding* encoding)
{
  int nstrings, j;
  char const *name;
  id3_ucs4_t const *ucs4;
  union id3_field const *field;
  struct id3_frame const *frame;

  frame = id3_tag_findframe(tag, id, 0);
  if (frame == 0)
	  return id3_ucs4_empty;

  *encoding = id3_field_gettextencoding(id3_frame_field(frame, 0));

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return id3_ucs4_empty;

  nstrings = id3_field_getnstrings(field);

  ucs4 = id3_ucs4_empty;

  for (j = 0; j < nstrings; ++j)
  {
    ucs4 = id3_field_getstrings(field, j);
    if (ucs4 && *ucs4)
      break;
  }

  return ucs4;
}

int metadata_setstring(struct id3_tag* tag, const char* id, id3_ucs4_t* value)
{
  union id3_field *field;
  struct id3_frame *frame;

  frame = id3_tag_findframe(tag, id, 0);
  if (frame == 0)
  {
	  frame = id3_frame_new(id);
    id3_tag_attachframe(tag, frame);
  }

  id3_field_settextencoding(id3_frame_field(frame, 0), ID3_FIELD_TEXTENCODING_UTF_16);

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return 0;

  return (id3_field_setstrings(field, 1, &value)==0);
}

const id3_ucs4_t* id3_metadata_getartist(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  id3_ucs4_t const *ucs4;

  if ((ucs4=metadata_getstring(tag, ID3_FRAME_ARTIST, encoding)) ||
      (ucs4=metadata_getstring(tag, "TPE2", encoding)) ||
      (ucs4=metadata_getstring(tag, "TPE3", encoding)) ||
      (ucs4=metadata_getstring(tag, "TPE4", encoding)) || 
      (ucs4=metadata_getstring(tag, "TCOM", encoding)))
    return ucs4;

  return id3_ucs4_empty;
}

int id3_metadata_setartist(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_ARTIST, value);
}

const id3_ucs4_t* id3_metadata_getalbum(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, ID3_FRAME_ALBUM, encoding);
}

int id3_metadata_setalbum(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_ALBUM, value);
}

const id3_ucs4_t* id3_metadata_getalbumartist(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  id3_ucs4_t const *ucs4 = metadata_getstring(tag, "TPE2", encoding);
  if (ucs4 && *ucs4) return ucs4;
  ucs4 = id3_metadata_getusertext(tag, "ALBUM ARTIST");
  if (ucs4 && *ucs4) return ucs4;
  ucs4 = id3_metadata_getusertext(tag, "ALBUMARTIST");
  if (ucs4 && *ucs4) return ucs4;
  return id3_ucs4_empty;
}

int id3_metadata_setalbumartist(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, "TPE2", value);
}

char id3_metadata_getrating(const struct id3_tag* tag)
{
  union id3_field const *field;
  struct id3_frame const *frame;
  int value;

  frame = id3_tag_findframe(tag, "POPM", 0);
  if (frame)
  {
    field = id3_frame_field(frame, 1);
    if (field)
    { // based on mediamonkey's values, simplified down a bit
      // http://www.mediamonkey.com/forum/viewtopic.php?f=7&t=40532
      value = id3_field_getint(field);
      if (value == 1) return '1';   // WMP11 madness
      if (value < 9) return '0';
      if (value < 50) return '1';
      if (value < 114) return '2';
      if (value < 168) return '3';
      if (value < 219) return '4';
      return '5';
    }
  }
  else
  {
    const id3_ucs4_t *ucs4 = id3_metadata_getusertext(tag, "RATING");
    if (ucs4 && *ucs4 > '0' && *ucs4 < '6')
      return (char)*ucs4;
  }

  return  '0';
}

int id3_metadata_setrating(struct id3_tag* tag, char value)
{
  union id3_field *field;
  struct id3_frame *frame;
  char popm[] = { 3, 53, 104, 154, 205, 255 };

  if (value < '0' || value > '5')
    return -1;

  frame = id3_tag_findframe(tag, "POPM", 0);
  if (frame == 0)
  {
	  frame = id3_frame_new("POPM");
    id3_tag_attachframe(tag, frame);
  }

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return 0;

  return id3_field_setint(field, popm[value - '0']);
}

const id3_ucs4_t *id3_metadata_getcompilation(const struct id3_tag* tag, enum id3_field_textencoding *encoding)
{
  return metadata_getstring(tag, "TCMP", encoding);
}

int id3_metadata_setcompilation(struct id3_tag* tag, id3_ucs4_t *value)
{
  return metadata_setstring(tag, "TCMP", value);
}

const id3_ucs4_t* id3_metadata_gettitle(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, ID3_FRAME_TITLE, encoding);
}

int id3_metadata_settitle(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_TITLE, value);
}

const id3_ucs4_t* id3_metadata_gettrack(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, ID3_FRAME_TRACK, encoding);
}

int id3_metadata_settrack(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_TRACK, value);
}

const id3_ucs4_t* id3_metadata_getpartofset(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, "TPOS", encoding);
}

int id3_metadata_setpartofset(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, "TPOS", value);
}

const id3_ucs4_t* id3_metadata_getyear(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, ID3_FRAME_YEAR, encoding);
}

int id3_metadata_setyear(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_YEAR, value);
}

const id3_ucs4_t* id3_metadata_getgenre(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, ID3_FRAME_GENRE, encoding);
}

id3_ucs4_list_t* id3_metadata_getgenres(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstrings(tag, ID3_FRAME_GENRE, encoding);
}

int id3_metadata_setgenre(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, ID3_FRAME_GENRE, value);
}

const id3_ucs4_t* id3_metadata_getcomment(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  union id3_field const *field;
  struct id3_frame const *frame;
  int commentNumber = 0;
  const id3_ucs4_t* ucs4 = 0;

  // return the first non-empty comment
  do
  {
    frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, commentNumber++);

    if (frame && frame->nfields == 4)
    {
      //get short description
      field = id3_frame_field(frame, 2);
      if (field == 0)
        continue;
      
      ucs4 = id3_field_getstring(field);

      // Multiple values are allowed per comment field, but storing different comment
      // frames requires a different description for each frame. The first COMM frame
      // encountered without a description will be used as the comment field.
      // Source http://puddletag.sourceforge.net/source/id3.html
      if (ucs4 && *ucs4 == 0)//if short description on this frame is empty - consider this the wanted comment frame
      {
        //fetch encoding of the frame
        field = id3_frame_field(frame, 0);
        
        if(field == 0)
          continue;
          
        *encoding = id3_field_gettextencoding(field);

        //finally fetch the comment
        field = id3_frame_field(frame, 3);
        if (field == 0)
          continue;
    
        return id3_field_getfullstring(field);
      }
    }
  }
  while (frame);
  return ucs4;
}

int id3_metadata_setcomment(struct id3_tag* tag, id3_ucs4_t* value)
{
  union id3_field *field;
  struct id3_frame *frame;

  frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, 0);
  if (frame == 0)
  {
    frame = id3_frame_new(ID3_FRAME_COMMENT);
    id3_tag_attachframe(tag, frame);
  }

  id3_field_settextencoding(id3_frame_field(frame, 0), ID3_FIELD_TEXTENCODING_UTF_16);

  field = id3_frame_field(frame, 3);
  if (field == 0)
    return 0;

  return id3_field_setfullstring(field, value);
}

const id3_ucs4_t* id3_metadata_getencodedby(const struct id3_tag* tag, enum id3_field_textencoding* encoding)
{
  return metadata_getstring(tag, "TENC", encoding);
}

int id3_metadata_setencodedby(struct id3_tag* tag, id3_ucs4_t* value)
{
  return metadata_setstring(tag, "TENC", value);
}

struct id3_frame const* id3_metadata_getpictureframebytype(const struct id3_tag* tag, enum id3_picture_type picture_type)
{
  int i;
  union id3_field const *field;
  struct id3_frame const *frame;

  for (i=0; ; ++i)
  {
    frame = id3_tag_findframe(tag, "APIC", i);
    if (frame == 0)
	    return 0;

    field = id3_frame_field(frame, 2);
    if (field == 0)
      return 0;

    if (id3_field_getint(field)==picture_type)
      break;
  }

  return frame;
}

int id3_metadata_haspicture(const struct id3_tag* tag, enum id3_picture_type picture_type)
{
  return (id3_metadata_getpictureframebytype(tag, picture_type)!=0);
}

const id3_latin1_t* id3_metadata_getpicturemimetype(const struct id3_tag* tag, enum id3_picture_type picture_type)
{
  union id3_field const *field;
	struct id3_frame const *frame;

  frame=id3_metadata_getpictureframebytype(tag, picture_type);
  if (frame==0)
    return 0;

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return 0;

  return id3_field_getlatin1(field);
}

id3_byte_t const* id3_metadata_getpicturedata(const struct id3_tag* tag, enum id3_picture_type picture_type, id3_length_t* length)
{
  union id3_field const *field;
	struct id3_frame const *frame;

  frame=id3_metadata_getpictureframebytype(tag, picture_type);
  if (frame==0)
    return 0;

  field = id3_frame_field(frame, 4);
  if (field == 0)
    return 0;

  return id3_field_getbinarydata(field, length);
}

int id3_metadata_getfirstnonstandardpictype(const struct id3_tag* tag, enum id3_picture_type* picture_type)
{
  int i=0;
  union id3_field const *field;
	struct id3_frame const *frame;

  for (i=0; ; ++i)
  {
    frame = id3_tag_findframe(tag, "APIC", i);
    if (frame == 0)
	    return 0;

    field = id3_frame_field(frame, 2);
    if (field == 0)
      return 0;

    if ((*picture_type=id3_field_getint(field))>ID3_PICTURE_TYPE_PUBLISHERLOGO)
      break;
  }

  return 1;
}

id3_byte_t const* id3_metadata_getuniquefileidentifier(const struct id3_tag* tag, const char* owner_identifier, id3_length_t* length)
{
  int i=0, result;
  id3_ucs4_t const * ucs4;
  id3_latin1_t* latin1;
  union id3_field const *field;
	struct id3_frame const *frame;
  id3_byte_t const* identifier;

  for (i=0; ; ++i)
  {
    frame = id3_tag_findframe(tag, "UFID", i);
    if (frame == 0)
	    return 0;

    field = id3_frame_field(frame, 0);
    if (field == 0)
      return 0;

    if (strcmp(id3_field_getlatin1(field), owner_identifier)==0)
      break;
  }

  field = id3_frame_field(frame, 1);
  if (field == 0)
    return 0;

  return id3_field_getbinarydata(field, length);
}

const id3_ucs4_t* id3_metadata_getusertext(const struct id3_tag* tag, const char* description)
{
  id3_ucs4_t const * ucs4;
  id3_latin1_t* latin1;
  union id3_field const *field;
	struct id3_frame const *frame;
  id3_length_t length;
  int i=0, result;

  for (i=0; ; ++i)
  {
    frame = id3_tag_findframe(tag, "TXXX", i);
    if (frame == 0)
	    return id3_ucs4_empty;

    field = id3_frame_field(frame, 1);
    if (field == 0)
      return id3_ucs4_empty;

    latin1=id3_ucs4_latin1duplicate(id3_field_getstring(field));
    result=strcmp(latin1, description);
    free(latin1);

    if (result==0)
      break;
  }

  field = id3_frame_field(frame, 2);
  if (field == 0)
    return id3_ucs4_empty;


  return id3_field_getstring(field);
}
