/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef XBMC_ITUNES_VIS_H
#define XBMC_ITUNES_VIS_H

#define IPLUGIN void*

#include <Accelerate/Accelerate.h>
#include <QuickTime/QuickTime.h>

#include "iTunesAPI.h"
#include "iTunesVisualAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

  static OSStatus XBMCITAppProc(void *appCookie, OSType message, struct PlayerMessageInfo *messageInfo);
  typedef OSStatus (*iTunesPluginMainMachOPtr)(OSType message,PluginMessageInfo *messageInfo,void *refCon);

  /* structure to hold imported function pointers */
  typedef struct _Imports
  {
    iTunesPluginMainMachOPtr main;
    VisualPluginProcPtr      visual_handler;
  } Imports;

  typedef enum _ITunesVisType {
    ITunesVisTypeNormal = 1,
    ITunesVisTypeComposition
  } ITunesVisType ;

  typedef enum _ITunesPixelFormat {
    ITunesPixelFormatRGBA32 = 1,
    ITunesPixelFormatRGB24,
    ITunesPixelFormatUnknown = 0,
  } ITunesPixelFormat;

  /* container around relevant iTunes visualiser data */
  typedef struct _ITunesVis
  {
    Imports                  imports;
    PluginMessageInfo        message;
    VisualPluginMessageInfo  visual_message;
    IPLUGIN                  handle;
    ITTrackInfo              track_info;
    ITTrackInfoV1            track_info_v1;
    ITStreamInfo             stream_info;
    ITStreamInfoV1           stream_info_v1;
    void*                    vis_ref;
    void*                    main_ref;
    void*                    screen;
    Rect                     rect;
    char                     filename[512];
    char                     bundle_path[512];
    char                     album_art[512];
    ITunesVisType            vis_type;
    unsigned long            start_time;
    FFTSetup                 fft_setup;
    void*                    gl_context;
    bool                     playing;
    bool                     wants_idle;
  } ITunesVis;

  /* container around track info */
  typedef struct _ITunesTrack
  {
    const char *title;
    const char *artist;
    const char *album;
    const char *album_artist;
    const char *genre;
    const char *comment;
    const char *lyrics;

    int        track_number;
    int        disc_number;
    int        duration;
    int        year;
    char       rating;
  } ITunesTrack;

  /* helper functions for loading and initializing iTunes visualisers */
  int        ivis_get_visualisations( char ***names, char ***paths );
  ITunesVis* ivis_open  ( const char *name );
  bool       ivis_init  ( ITunesVis* plugin, int width, int height );
  void       ivis_start ( ITunesVis* plugin );
  void       ivis_set_track_info ( ITunesVis* plugin, ITunesTrack* track_info );
  void       ivis_set_album_art  ( ITunesVis* plugin, const char* filename );
  void       ivis_close ( ITunesVis* plugin );
  bool       ivis_wants_idle ( ITunesVis* plugin );
  ITunesPixelFormat ivis_render( ITunesVis* plugin, short audio_data[][512],
                                 float freq_data[][512],
                                 void* buffer, long buffer_size, bool idle );
  unsigned long ivis_current_time();

  /* inline helper functions */
  inline bool is_valid_field( const char *field )
  {
    if ( field && ( strlen(field) > 0 ) )
      return true;
    return false;
  }

#ifdef __cplusplus
}
#endif

#ifndef MIN
#define MIN( a, b ) (a)<(b)?(a):(b)
#endif

#ifndef MAX
#define MAX( a, b ) (a)>(b)?(a):(b)
#endif

#endif // XBMC_ITUNES_VIS_H
