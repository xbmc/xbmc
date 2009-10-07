/*
*      Copyright (C) 2005-2009 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#ifndef LIB_VISUALISATION_H
#define LIB_VISUALISATION_H

#ifdef WIN32
#define XBMC_API   __declspec( dllexport )
#else
#define XBMC_API 
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  ////////////// libVisualisation Types ////////////////

  /* An individual preset */
  struct viz_preset;
  typedef struct viz_preset *viz_preset_t;

  /* A list of presets */
  struct viz_preset_list;
  typedef struct viz_preset_list *viz_preset_list_t;

  /* An audio track */
  struct viz_track;
  typedef struct viz_track *viz_track_t;

  ////////////// libRefMem Operations //////////////

  /**
  * Decrement a reference count on the structure pointed
  * to by 'p'
  * \param p preset(s) handle
  * \return NULL
  */
  XBMC_API void viz_release(void* p);

  ////////// Visualisation Preset List Operations //////////

  /**
  * Create a new presetlist structure
  * \return presetlist handle
  */
  XBMC_API viz_preset_list_t viz_preset_list_create(void);

  /*
  * Retrieve the number of presets of the preset list structure
  * in 'list'.
  * \param list presetlist handle
  * \return int number of  presets or <0 indicating error
  */
  XBMC_API int viz_preset_list_get_count(viz_preset_list_t list);

  /**
  * Return a pointer to the preset list structure located at position
  * 'index' of a preset_list list structure 'list'
  * \param list preset_list list handle
  * \param index position in list
  * \return preset handle
  */
  XBMC_API viz_preset_t viz_preset_list_get_item(viz_preset_list_t list, int index);

  /**
  * Add a preset structure to a preset list
  * \param list preset_list list handle
  * \param preset viz_preset handle to add to list
  * \return int error status
  */
  XBMC_API int viz_preset_list_add_item(viz_preset_list_t list, viz_preset_t preset);

  //////////// Visualisation Preset Operations ////////////

  /**
  * Create a new preset structure
  * \return preset handle
  */
  XBMC_API viz_preset_t viz_preset_create(void);

  /**
  * Retrieve the 'name' field of a preset
  * \param preset viz_preset handle
  * \return null-terminated string
  */
  XBMC_API char *viz_preset_name(viz_preset_t preset);

  /**
  * Set the 'name' field of a preset
  * \param preset viz_preset handle
  * \param id char* name
  * \return int error status
  */
  XBMC_API int viz_preset_set_name(viz_preset_t preset, const char* name);

  //////////// Visualisation Track Operations ////////////

  /**
  * Create a new track structure
  * \return track handle
  */
  XBMC_API viz_track_t viz_track_create(void);

  /**
  * Retrieve the 'title' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_title(viz_track_t track);

  /**
  * Set the 'title' field of a track
  * \param track viz_track handle
  * \param id const char* title
  * \return int error status
  */
  XBMC_API int viz_track_set_title(viz_track_t track, const char* title);

  /**
  * Retrieve the 'artist' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_artist(viz_track_t track);

  /**
  * Set the 'artist' field of a track
  * \param track viz_track handle
  * \param id const char* artist
  * \return int error status
  */
  XBMC_API int viz_track_set_artist(viz_track_t track, const char* artist);

  /**
  * Retrieve the 'album' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_album(viz_track_t track);

  /**
  * Set the 'album' field of a track
  * \param track viz_track handle
  * \param id const char* album
  * \return int error status
  */
  XBMC_API int viz_track_set_album(viz_track_t track, const char* album);

  /**
  * Retrieve the 'albumartist' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_albumartist(viz_track_t track);

  /**
  * Set the 'albumartist' field of a track
  * \param track viz_track handle
  * \param id const char* albumartist
  * \return int error status
  */
  XBMC_API int viz_track_set_albumartist(viz_track_t track, const char* albumartist);

  /**
  * Retrieve the 'genre' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_genre(viz_track_t track);

  /**
  * Set the 'genre' field of a track
  * \param track viz_track handle
  * \param id const char* genre
  * \return int error status
  */
  XBMC_API int viz_track_set_genre(viz_track_t track, const char* genre);

  /**
  * Retrieve the 'comment' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_comment(viz_track_t track);

  /**
  * Set the 'comment' field of a track
  * \param track viz_track handle
  * \param id const char* comment
  * \return int error status
  */
  XBMC_API int viz_track_set_comment(viz_track_t track, const char* comment);

  /**
  * Retrieve the 'lyrics' field of a track
  * \param track viz_track handle
  * \return null-terminated string
  */
  XBMC_API char *viz_track_lyrics(viz_track_t track);

  /**
  * Set the 'lyrics' field of a track
  * \param track viz_track handle
  * \param id const char* lyrics
  * \return int error status
  */
  XBMC_API int viz_track_set_lyrics(viz_track_t track, const char* lyrics);

  /**
  * Retrieve the 'tracknum' field of a track
  * \param track viz_track handle
  * \return int track number
  */
  XBMC_API int viz_track_tracknum(viz_track_t track);

  /**
  * Set the 'tracknum' field of a track
  * \param track viz_track handle
  * \param num int track number
  * \return int error status
  */
  XBMC_API int viz_track_set_tracknum(viz_track_t track, int tracknum);

  /**
  * Retrieve the 'discnum' field of a track
  * \param track viz_track handle
  * \return int disc number
  */
  XBMC_API int viz_track_discnum(viz_track_t track);

  /**
  * Set the 'discnumm' field of a track
  * \param track viz_track handle
  * \param num int disc number
  * \return int error status
  */
  XBMC_API int viz_track_set_discnum(viz_track_t track, int discnum);

  /**
  * Retrieve the 'duration' field of a track
  * \param track viz_track handle
  * \return int duration
  */
  XBMC_API int viz_track_duration(viz_track_t track);

  /**
  * Set the 'durationm' field of a track
  * \param track viz_track handle
  * \param num int duration
  * \return int error status
  */
  XBMC_API int viz_track_set_duration(viz_track_t track, int duration);

  /**
  * Retrieve the 'year' field of a track
  * \param track viz_track handle
  * \return int year
  */
  XBMC_API int viz_track_year(viz_track_t track);

  /**
  * Set the 'year' field of a track
  * \param track viz_track handle
  * \param num int year
  * \return int error status
  */
  XBMC_API int viz_track_set_year(viz_track_t track, int year);

  /**
  * Retrieve the 'rating' field of a track
  * \param track viz_track handle
  * \return char rating
  */
  XBMC_API char viz_track_rating(viz_track_t track);

  /**
  * Set the 'rating' field of a track
  * \param track viz_track handle
  * \param rating char track rating
  * \return int error status
  */
  XBMC_API int viz_track_set_rating(viz_track_t track, const char rating);

/*
 * -----------------------------------------------------------------
 * Debug Output Control
 * -----------------------------------------------------------------
 */

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define VIZ_DBG_NONE  -1
#define VIZ_DBG_ERROR  0
#define VIZ_DBG_WARN   1
#define VIZ_DBG_INFO   2
#define VIZ_DBG_DETAIL 3
#define VIZ_DBG_DEBUG  4
#define VIZ_DBG_PROTO  5
#define VIZ_DBG_ALL    6

/**
 * Set the libvisualisation debug level.
 * \param l level
 */
extern void viz_dbg_level(int l);

/**
 * Turn on all libvisualisation debugging.
 */
extern void viz_dbg_all(void);

/**
 * Turn off all libvisualisation debugging.
 */
extern void viz_dbg_none(void);

/**
 * Print a libvisualisation debug message.
 * \param level debug level
 * \param fmt printf style format
 */
extern void viz_dbg(int level, char *fmt, ...);


#ifdef __cplusplus
};
#endif

#endif /* LIB_VISUALISATION_H */

