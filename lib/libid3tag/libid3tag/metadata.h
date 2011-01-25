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
 * $Id: metadata.h,v 1.00 2005/09/03 09:41:32 bobbin007 Exp $
 */

# ifndef LIBID3TAG_METADATA_H
# define LIBID3TAG_METADATA_H

# include "id3tag.h"

# ifdef __cplusplus
extern "C" {
# endif

struct id3_ucs4_list {
    unsigned int nstrings;
    const id3_ucs4_t** strings;
  };

typedef struct id3_ucs4_list id3_ucs4_list_t;

ID3_EXPORT void id3_ucs4_list_free(id3_ucs4_list_t *list);

enum id3_picture_type
{
  ID3_PICTURE_TYPE_OTHER = 0,
  ID3_PICTURE_TYPE_PNG32ICON = 1,     // 32x32 pixels 'file icon' (PNG only)
  ID3_PICTURE_TYPE_OTHERICON = 2,     // Other file icon
  ID3_PICTURE_TYPE_COVERFRONT = 3,    // Cover (front)
  ID3_PICTURE_TYPE_COVERBACK = 4,     // Cover (back)
  ID3_PICTURE_TYPE_LEAFLETPAGE = 5,   // Leaflet page
  ID3_PICTURE_TYPE_MEDIA = 6,         // Media (e.g. lable side of CD)
  ID3_PICTURE_TYPE_LEADARTIST = 7,    // Lead artist/lead performer/soloist
  ID3_PICTURE_TYPE_ARTIST = 8,        // Artist/performer
  ID3_PICTURE_TYPE_CONDUCTOR = 9,     // Conductor
  ID3_PICTURE_TYPE_BAND = 10,         // Band/Orchestra
  ID3_PICTURE_TYPE_COMPOSER = 11,     // Composer
  ID3_PICTURE_TYPE_LYRICIST = 12,     // Lyricist/text writer
  ID3_PICTURE_TYPE_REC_LOCATION = 13, // Recording Location
  ID3_PICTURE_TYPE_RECORDING = 14,    // During recording
  ID3_PICTURE_TYPE_PERFORMANCE = 15,  // During performance
  ID3_PICTURE_TYPE_VIDEO = 16,        // Movie/video screen capture
  ID3_PICTURE_TYPE_FISH = 17,         // A bright coloured fish
  ID3_PICTURE_TYPE_ILLUSTRATION = 18, // Illustration
  ID3_PICTURE_TYPE_ARTISTLOGO = 19,   // Band/artist logotype
  ID3_PICTURE_TYPE_PUBLISHERLOGO = 20 // Publisher/Studio logotype
};

ID3_EXPORT const id3_ucs4_t* id3_metadata_getartist(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getalbum(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getalbumartist(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_gettitle(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_gettrack(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getpartofset(const struct id3_tag* tag, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getyear(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getgenre(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT id3_ucs4_list_t* id3_metadata_getgenres(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getcomment(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getencodedby(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getcompilation(const struct id3_tag*, enum id3_field_textencoding*);
ID3_EXPORT char id3_metadata_getrating(const struct id3_tag* tag);
ID3_EXPORT int id3_metadata_haspicture(const struct id3_tag*, enum id3_picture_type);
ID3_EXPORT const id3_latin1_t* id3_metadata_getpicturemimetype(const struct id3_tag*, enum id3_picture_type);
ID3_EXPORT id3_byte_t const *id3_metadata_getpicturedata(const struct id3_tag*, enum id3_picture_type, id3_length_t*);
ID3_EXPORT id3_byte_t const* id3_metadata_getuniquefileidentifier(const struct id3_tag*, const char* owner_identifier, id3_length_t*);
ID3_EXPORT const id3_ucs4_t* id3_metadata_getusertext(const struct id3_tag*, const char* description);
ID3_EXPORT int id3_metadata_getfirstnonstandardpictype(const struct id3_tag*, enum id3_picture_type*);

ID3_EXPORT int id3_metadata_setartist(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setalbum(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setalbumartist(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_settitle(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_settrack(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setpartofset(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setyear(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setgenre(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setencodedby(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setrating(struct id3_tag* tag, char value);
ID3_EXPORT int id3_metadata_setcompilation(struct id3_tag* tag, id3_ucs4_t* value);
ID3_EXPORT int id3_metadata_setcomment(struct id3_tag* tag, id3_ucs4_t* value);

# ifdef __cplusplus
}
# endif

# endif
