/*
*  Copyright (C) 2004-2006, Eric Lund
*  http://www.mvpmc.org/
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.

*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
* \file track.c
* Functions that operate on individual track
*/
#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include <mvp_refmem.h>
#include <mvp_debug.h>
#include <libvisualisation.h>
#include <viz_local.h>

/*
* viz_track_destroy(viz_track_t p)
*
* Scope: PRIVATE (static)
*
* Description
*
* Destroy the track structure pointed to by 'p' and release
* it's storage. This should only be called by viz_release().
*
* Return Value:
*
* None.
*/
static void
viz_track_destroy(viz_track_t p)
{
  viz_dbg(VIZ_DBG_DEBUG, "%s {\n", __FUNCTION__);
  if (!p) {
    viz_dbg(VIZ_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
    return;
  }
  if (p->title) {
    ref_release(p->title);
  }
  if (p->artist) {
    ref_release(p->artist);
  }
  if (p->album) {
    ref_release(p->album);
  }
  if (p->albumartist) {
    ref_release(p->albumartist);
  }
  if (p->genre) {
    ref_release(p->genre);
  }
  if (p->comment) {
    ref_release(p->comment);
  }
  if (p->lyrics) {
    ref_release(p->lyrics);
  }
  p->tracknum = p->discnum = p->duration = p->year = p->rating = 0;
  viz_dbg(VIZ_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
* viz_track_create(void)
*
* Scope: PUBLIC
*
* Description:
*
* Create a new viz_track structure and return pointer to the structure
*
* Return Value:
*
* Success: A non-NULL viz_track_t
*
* Failure: A NULL viz_track_t
*/
viz_track_t
viz_track_create(void)
{
  viz_track_t ret = ref_alloc(sizeof(*ret));

  viz_dbg(VIZ_DBG_DEBUG, "%s\n", __FUNCTION__);
  if(!ret) {
    return NULL;
  }
  ref_set_destroy(ret, (ref_destroy_t)viz_track_destroy);

  ret->title = NULL;
  ret->artist = NULL;
  ret->album = NULL;
  ret->albumartist = NULL;
  ret->genre = NULL;
  ret->comment = NULL;
  ret->lyrics = NULL;
  ret->tracknum = 0;
  ret->discnum = 0;
  ret->year = 0;
  ret->rating = 0;
  return ret;
}

/*
* viz_track_title(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'title' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'title'
*
* Failure: NULL char*
*/
char *
viz_track_title(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->title;
}

/*
* viz_track_set_title(viz_track_t track, viz_track_type_t title)
*
*
* Scope: PUBLIC
*
* Modifies the 'title' field of a track structure, with a duplicate
* of the string pointed to by 'title'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_title(viz_track_t track, const char* title)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->title) {
    ref_release(track->title);
  }

  track->title = ref_strdup(title);
  return (int) ref_hold(track->title);
}

/*
* viz_track_artist(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'artist' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'artist'
*
* Failure: NULL char*
*/
char *
viz_track_artist(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->artist;
}

/*
* viz_track_set_artist(viz_track_t track, viz_track_type_t artist)
*
*
* Scope: PUBLIC
*
* Modifies the 'artist' field of a track structure, with a duplicate
* of the string pointed to by 'artist'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_artist(viz_track_t track, const char* artist)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->artist) {
    ref_release(track->artist);
  }

  track->artist = ref_strdup(artist);
  return (int) ref_hold(track->artist);
}

/*
* viz_track_album(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'album' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'album'
*
* Failure: NULL char*
*/
char *
viz_track_album(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->album;
}

/*
* viz_track_set_album(viz_track_t track, viz_track_type_t album)
*
*
* Scope: PUBLIC
*
* Modifies the 'album' field of a track structure, with a duplicate
* of the string pointed to by 'album'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_album(viz_track_t track, const char* album)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->album) {
    ref_release(track->album);
  }

  track->album = ref_strdup(album);
  return (int) ref_hold(track->album);
}

/*
* viz_track_albumartist(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'albumartist' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'albumartist'
*
* Failure: NULL char*
*/
char *
viz_track_albumartist(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->albumartist;
}

/*
* viz_track_set_albumartist(viz_track_t track, viz_track_type_t albumartist)
*
*
* Scope: PUBLIC
*
* Modifies the 'albumartist' field of a track structure, with a duplicate
* of the string pointed to by 'albumartist'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_albumartist(viz_track_t track, const char* albumartist)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->albumartist) {
    ref_release(track->albumartist);
  }

  track->albumartist = ref_strdup(albumartist);
  return (int) ref_hold(track->albumartist);
}

/*
* viz_track_genre(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'genre' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'genre'
*
* Failure: NULL char*
*/
char *
viz_track_genre(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->genre;
}

/*
* viz_track_set_genre(viz_track_t track, viz_track_type_t genre)
*
*
* Scope: PUBLIC
*
* Modifies the 'genre' field of a track structure, with a duplicate
* of the string pointed to by 'genre'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_genre(viz_track_t track, const char* genre)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->genre) {
    ref_release(track->genre);
  }

  track->genre = ref_strdup(genre);
  return (int) ref_hold(track->genre);
}

/*
* viz_track_comment(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'comment' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'comment'
*
* Failure: NULL char*
*/
char *
viz_track_comment(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->comment;
}

/*
* viz_track_set_comment(viz_track_t track, viz_track_type_t comment)
*
*
* Scope: PUBLIC
*
* Modifies the 'comment' field of a track structure, with a duplicate
* of the string pointed to by 'comment'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_comment(viz_track_t track, const char* comment)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->comment) {
    ref_release(track->comment);
  }

  track->comment = ref_strdup(comment);
  return (int) ref_hold(track->comment);
}

/*
* viz_track_lyrics(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'lyrics' field of a track structure
*
* Return Value:
*
* Success: A ref counted char* to field 'lyrics'
*
* Failure: NULL char*
*/
char *
viz_track_lyrics(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->lyrics;
}

/*
* viz_track_set_lyrics(viz_track_t track, viz_track_type_t lyrics)
*
*
* Scope: PUBLIC
*
* Modifies the 'lyrics' field of a track structure, with a duplicate
* of the string pointed to by 'lyrics'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_lyrics(viz_track_t track, const char* lyrics)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  if (track->lyrics) {
    ref_release(track->lyrics);
  }

  track->lyrics = ref_strdup(lyrics);
  return (int) ref_hold(track->lyrics);
}

/*
* viz_track_tracknum(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'tracknum' field of a track structure
*
* Return Value:
*
* Integer value of field
*
*/
int
viz_track_tracknum(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->tracknum;
}

/*
* viz_track_set_tracknum(viz_track_t track, int tracknum)
*
*
* Scope: PUBLIC
*
* Modifies the 'tracknum' field of a track structure to contain
* the integer tracknum
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_tracknum(viz_track_t track, int tracknum)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  track->tracknum = tracknum;
  return 1;
}

/*
* viz_track_discnum(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'discnum' field of a track structure
*
* Return Value:
*
* Integer value of field
*
*/
int
viz_track_discnum(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->discnum;
}

/*
* viz_track_set_discnum(viz_track_t track, int discnum)
*
*
* Scope: PUBLIC
*
* Modifies the 'discnum' field of a track structure to contain
* the integer discnum
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_discnum(viz_track_t track, int discnum)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  track->discnum = discnum;
  return 1;
}
/*
* viz_track_duration(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'duration' field of a track structure
*
* Return Value:
*
* Integer value of field
*
*/
int
viz_track_duration(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->duration;
}

/*
* viz_track_set_duration(viz_track_t track, int duration)
*
*
* Scope: PUBLIC
*
* Modifies the 'duration' field of a track structure to contain
* the integer duration
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_duration(viz_track_t track, int duration)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  track->duration = duration;
  return 1;
}

/*
* viz_track_year(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'year' field of a track structure
*
* Return Value:
*
* Integer value of field
*
*/
int
viz_track_year(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->year;
}

/*
* viz_track_set_year(viz_track_t track, int year)
*
*
* Scope: PUBLIC
*
* Modifies the 'year' field of a track structure to contain
* the integer year
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_year(viz_track_t track, int year)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  track->year = year;
  return 1;
}

/*
* viz_track_rating(viz_track_t track)
*
*
* Scope: PUBLIC
*
* Retrieves the 'rating' field of a track structure
*
* Return Value:
*
* char value of field
*
*/
char
viz_track_rating(viz_track_t track)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return NULL;
  }
  return track->rating;
}

/*
* viz_track_set_rating(viz_track_t track, char rating)
*
*
* Scope: PUBLIC
*
* Modifies the 'rating' field of a track structure to contain
* the char rating
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_track_set_rating(viz_track_t track, const char rating)
{
  if (!track) {
    viz_dbg(VIZ_DBG_ERROR, "%s: NULL track structure\n",
      __FUNCTION__);
    return 0;
  }

  track->rating = rating;
  return 1;
}
